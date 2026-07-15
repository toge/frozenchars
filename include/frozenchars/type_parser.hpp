#pragma once

#include <cstddef>
#include <optional>
#include <tuple>
#include <utility>
#include <variant>

#include "string.hpp"
#include "string_ops.hpp"
#include "detail/type_mapping.hpp"

namespace frozenchars::detail {

/**
 * @brief 2 つの std::variant を結合するユーティリティ
 *
 * @tparam Acc 結果を蓄積する variant（初期値は空 variant）
 * @tparam V2 結合対象の残り variant 型
 *
 * @details V2 の先頭型を Acc に追加し、残りを再帰的に処理する。
 *          detail::variant_cat_t が未対応の環境向けの互換実装。
 */
template <typename Acc, typename V2>
struct variant_cat_impl;

// 基底ケース: V2 が空 variant → Acc が最終結果
template <typename... AccTs>
struct variant_cat_impl<std::variant<AccTs...>, std::variant<>> {
  using type = std::variant<AccTs...>;
};

// 再帰ケース: V2 の先頭を Acc に追加
template <typename... AccTs, typename Head, typename... Tail>
struct variant_cat_impl<std::variant<AccTs...>, std::variant<Head, Tail...>> {
  using type = typename variant_cat_impl<std::variant<AccTs..., Head>, std::variant<Tail...>>::type;
};

/**
 * @brief 2 つの std::variant を結合する
 */
template <typename V1, typename V2>
using variant_cat_t = typename variant_cat_impl<V1, V2>::type;

} // namespace frozenchars::detail

namespace frozenchars {

/**
 * @brief 文字列トークンを対応する型に変換する型トレイト
 *
 * @tparam S 変換対象の FrozenString（型名を含む）
 * @details detail::map_string_to_type をラップし、
 *          ユーザーが直接 `type_mapping<"int"_fs>::type` のように使えるようにする。
 */
template <auto S>
struct type_mapping {
  using type = typename decltype(detail::map_string_to_type<S>())::type;
};

/**
 * @brief 型トークンのサフィックス（*, &, &&）を検出し、対応する型を返す
 *
 * @tparam Token サフィックスを含む型トークン（FrozenString）
 * @return type_identity<パースされた型>
 *
 * @details 末尾のサフィックスを順次除去し、ベース型を解決してから
 *          除去したサフィックスを逆順に適用する。
 *          例: "int*&" → int*& → (int*)& → int*
 */
template <auto Token>
[[nodiscard]] consteval auto parse_type_with_suffix() {
  auto constexpr sv = Token.sv();
  auto constexpr len = sv.size();

  // &&（右辺値参照）
  if constexpr (len >= 2 && sv[len - 2] == '&' && sv[len - 1] == '&') {
    auto constexpr rest = trim(substr(Token, 0, static_cast<std::ptrdiff_t>(len - 2)));
    using Inner = typename decltype(parse_type_with_suffix<rest>())::type;
    return detail::type_identity<Inner&&>{};
  // &（参照）
  } else if constexpr (len >= 1 && sv[len - 1] == '&') {
    auto constexpr rest = trim(substr(Token, 0, static_cast<std::ptrdiff_t>(len - 1)));
    using Inner = typename decltype(parse_type_with_suffix<rest>())::type;
    return detail::type_identity<Inner&>{};
  // *（ポインタ）
  } else if constexpr (len >= 1 && sv[len - 1] == '*') {
    auto constexpr rest = trim(substr(Token, 0, static_cast<std::ptrdiff_t>(len - 1)));
    using Inner = typename decltype(parse_type_with_suffix<rest>())::type;
    return detail::type_identity<Inner*>{};
  // サフィックスなし → ベース型を解決
  } else {
    using T = typename type_mapping<Token>::type;
    static_assert(!std::is_same_v<T, detail::unknown_type>, "Unknown type name");
    return detail::type_identity<T>{};
  }
}

/**
 * @brief 固定文字列をパースして型のリスト（std::tuple）を生成する
 *
 * @tparam EmptyMeansVoid 空文字列を std::tuple<void> として扱うかどうかのフラグ
 * @tparam Str 入力文字列（FrozenString）
 * @return consteval 型のリスト（detail::type_identity<std::tuple<...>>）
 *
 * @details 再帰下降パーサーで文字列を解析する。
 *
 *  構文:
 *    - 型名: "int", "string", "bool" など
 *    - optional: 型名の末尾に `?` を付ける（例: "int?"）
 *    - ネスト tuple: `[型, 型, ...]`（例: "[int, bool]"）
 *    - ネスト optional: `[...]?`（例: "[int, int]?"）
 *    - 空要素（カンマのみ）: void として扱う
 *
 *  パースアルゴリズム:
 *    1. 入力文字列をトリムする
 *    2. 先頭が `[` ならネスト tuple ブランチへ
 *    3. それ以外ならカンマで分割して各トークンを型に変換する
 *    4. 各トークンの末尾が `?` なら std::optional でラップする
 *    5. 残りを再帰的にパースし、std::tuple_cat で結合する
 */
template <bool EmptyMeansVoid, auto Str>
[[nodiscard]] consteval auto parse_to_tuple_impl() noexcept {
  // 前後の空白を除去（タブ・改行含む）
  auto constexpr trimmed = trim_if<detail::is_any_whitespace>(Str);

  // --- 空文字列の処理 ---
  if constexpr (trimmed.length == 0) {
    // EmptyMeansVoid が true なら std::tuple<void>、false なら空 tuple
    if constexpr (EmptyMeansVoid) {
      return detail::type_identity<std::tuple<void>>{};
    } else {
      return detail::type_identity<std::tuple<>>{};
    }
  // --- 先頭が '[' の場合：ネスト tuple ブランチ ---
  } else if constexpr (trimmed.buffer[0] == '[') {
    // 対応する ']' の位置を検索（括弧の深さを考慮）
    auto constexpr closing_pos = detail::find_closing_bracket<trimmed>();
    static_assert(closing_pos != std::string_view::npos, "Missing matching ']'");

    // '[' と ']' の内側を抽出して再帰パース
    auto constexpr inner = substr(trimmed, 1, static_cast<std::ptrdiff_t>(closing_pos - 1));
    using BaseInnerTuple = typename decltype(parse_to_tuple_impl<false, inner>())::type;

    // ']' の直後に空白を飛ばして '?' があるか確認
    // opt_info: { is_opt, search_start } のペア
    auto constexpr opt_info = [](auto const& s, size_t pos) {
      size_t i = pos + 1;
      // ']' の直後の空白をスキップ
      while (i < s.length && detail::is_any_whitespace(s.buffer[i])) {
        ++i;
      }
      // '?' があれば optional として扱う
      bool found = (i < s.length && s.buffer[i] == '?');
      return std::pair{found, found ? i : pos};
    }(trimmed, closing_pos);

    auto constexpr is_opt = opt_info.first;       // optional かどうか
    auto constexpr search_start = opt_info.second; // 次のカンマ検索開始位置

    // optional フラグに応じて型をラップ
    using InnerTuple = std::conditional_t<is_opt, std::optional<BaseInnerTuple>, BaseInnerTuple>;

    // 次のカンマを探す（ネスト括弧内のカンマは無視）
    auto constexpr next_comma = trimmed.sv().find(',', search_start);
    if constexpr (next_comma == std::string_view::npos) {
      // カンマなし → この tuple が最後の要素
      return detail::type_identity<std::tuple<InnerTuple>>{};
    } else {
      // カンマあり → 残りを再帰的にパースして結合
      auto constexpr rest = substr(trimmed, next_comma + 1, static_cast<std::ptrdiff_t>(trimmed.length - next_comma - 1));
      using RestTuple = typename decltype(parse_to_tuple_impl<true, rest>())::type;
      using Combined = decltype(std::tuple_cat(std::declval<std::tuple<InnerTuple>>(), std::declval<RestTuple>()));
      return detail::type_identity<Combined>{};
    }
  // --- 通常の型名ブランチ ---
  } else {
    // トップレベルのカンマを探す（ネスト括弧内のカンマは無視）
    auto constexpr comma_pos = detail::find_top_level_comma<trimmed>();

    if constexpr (comma_pos == std::string_view::npos) {
      // カンマなし → 単一の型トークン
      // 末尾が '?' なら optional
      auto constexpr is_opt = (trimmed.length > 0 && trimmed.buffer[trimmed.length - 1] == '?');
      if constexpr (is_opt) {
        // '?' を除いた型名を取得して optional でラップ（サフィックス対応）
        auto constexpr name = trim_if<detail::is_any_whitespace>(substr(trimmed, 0, static_cast<std::ptrdiff_t>(trimmed.length - 1)));
        using T = typename decltype(parse_type_with_suffix<name>())::type;
        static_assert(!std::is_same_v<T, void>, "'void?' is not supported");
        return detail::type_identity<std::tuple<std::optional<T>>>{};
      } else {
        // サフィックス（*, &, &&）を検出して型に適用
        using Suffixed = typename decltype(parse_type_with_suffix<trimmed>())::type;
        return detail::type_identity<std::tuple<Suffixed>>{};
      }
    } else {
      // カンマで分割 → 先頭トークン + 残りの再帰パース
      auto constexpr token = trim_if<detail::is_any_whitespace>(substr(trimmed, 0, comma_pos));
      // トークン末尾の '?' チェック
      auto constexpr is_opt = (token.length > 0 && token.buffer[token.length - 1] == '?');
      // カンマ以降の残り文字列
      auto constexpr rest_str = substr(trimmed, comma_pos + 1, static_cast<std::ptrdiff_t>(trimmed.length - comma_pos - 1));
      // 残りを再帰的にパース（空要素は void として扱う）
      using RestTuple = typename decltype(parse_to_tuple_impl<true, rest_str>())::type;

      if constexpr (token.length == 0) {
        // 空トークン（カンマが連続）→ void として扱う
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<void>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      } else if constexpr (is_opt) {
        // optional トークン → '?' を除いた名前で型を取得し、サフィックスを適用
        auto constexpr name = trim_if<detail::is_any_whitespace>(substr(token, 0, static_cast<std::ptrdiff_t>(token.length - 1)));
        using Suffixed = typename decltype(parse_type_with_suffix<name>())::type;
        static_assert(!std::is_same_v<Suffixed, void>, "'void?' is not supported");
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<std::optional<Suffixed>>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      } else {
        // 通常のトークン → サフィックスを検出して型に変換して結合
        using Suffixed = typename decltype(parse_type_with_suffix<token>())::type;
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<Suffixed>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      }
    }
  }
}

/**
 * @brief 固定文字列をパースして型のリスト（std::tuple）を生成する
 *
 * @tparam Str 入力文字列（FrozenString）。型名をカンマ区切りで指定する。
 * @return consteval 型のリスト（detail::type_identity<std::tuple<...>>）
 *
 * @details parse_to_tuple_impl のラッパー。EmptyMeansVoid を false で固定する。
 *
 *  使用例:
 *    @code
 *    using T = typename decltype(parse_to_tuple<"int, string?, bool"_fs>())::type;
 *    // T = std::tuple<int, std::optional<std::string>, bool>
 *    @endcode
 *
 *  対応構文:
 *    - 型名: "int", "string", "bool", "double" など
 *    - optional: "int?" → std::optional<int>
 *    - ネスト tuple: "[int, bool]" → std::tuple<int, bool>
 *    - ネスト optional: "[int, int]?" → std::optional<std::tuple<int, int>>
 *    - 空要素: "int,,bool" → std::tuple<int, void, bool>
 *
 *  @note この関数は型計算専用。実行時に std::tuple オブジェクトを返すことはない。
 *        未知の型名や不正な構文はコンパイル時エラーになる。
 */
template <auto Str>
[[nodiscard]] consteval auto parse_to_tuple() {
  return parse_to_tuple_impl<false, Str>();
}

/**
 * @brief 固定文字列をパースして std::variant 型を生成する（内部実装）
 *
 * @tparam EmptyMeansVoid 空文字列を std::monostate として扱うかどうかのフラグ
 * @tparam Str 入力文字列（FrozenString）
 * @return consteval 型のリスト（detail::type_identity<std::variant<...>>）
 *
 * @details parse_to_tuple_impl と同等のロジックで variant を構築する。
 *          void 型は std::monostate にマッピングされる。
 */
template <bool EmptyMeansVoid, auto Str>
[[nodiscard]] consteval auto parse_to_variant_impl() noexcept {
  // 前後の空白を除去（タブ・改行含む）
  auto constexpr trimmed = trim_if<detail::is_any_whitespace>(Str);

  // --- 空文字列の処理 ---
  if constexpr (trimmed.length == 0) {
    // 空 variant は不正。monostate を返す
    return detail::type_identity<std::variant<std::monostate>>{};
  // --- 先頭が '[' の場合：ネスト tuple ブランチ ---
  } else if constexpr (trimmed.buffer[0] == '[') {
    // 対応する ']' の位置を検索（括弧の深さを考慮）
    auto constexpr closing_pos = detail::find_closing_bracket<trimmed>();
    static_assert(closing_pos != std::string_view::npos, "Missing matching ']'");

    // '[' と ']' の内側を抽出して tuple として再帰パース
    auto constexpr inner = substr(trimmed, 1, static_cast<std::ptrdiff_t>(closing_pos - 1));
    using BaseInnerTuple = typename decltype(parse_to_tuple_impl<false, inner>())::type;

    // ']' の直後に空白を飛ばして '?' があるか確認
    auto constexpr opt_info = [](auto const& s, size_t pos) {
      size_t i = pos + 1;
      // ']' の直後の空白をスキップ
      while (i < s.length && detail::is_any_whitespace(s.buffer[i])) {
        ++i;
      }
      // '?' があれば optional として扱う
      bool found = (i < s.length && s.buffer[i] == '?');
      return std::pair{found, found ? i : pos};
    }(trimmed, closing_pos);

    auto constexpr is_opt = opt_info.first;
    auto constexpr search_start = opt_info.second;

    // optional フラグに応じて型をラップ
    using InnerType = std::conditional_t<is_opt, std::optional<BaseInnerTuple>, BaseInnerTuple>;

    // 次のカンマを探す（ネスト括弧内のカンマは無視）
    auto constexpr next_comma = trimmed.sv().find(',', search_start);
    if constexpr (next_comma == std::string_view::npos) {
      return detail::type_identity<std::variant<InnerType>>{};
    } else {
      // カンマあり → 残りを再帰的にパースして結合
      auto constexpr rest = substr(trimmed, next_comma + 1, static_cast<std::ptrdiff_t>(trimmed.length - next_comma - 1));
      using RestVariant = typename decltype(parse_to_variant_impl<true, rest>())::type;
      return detail::type_identity<detail::variant_cat_t<std::variant<InnerType>, RestVariant>>{};
    }
  // --- 通常の型名ブランチ ---
  } else {
    // トップレベルのカンマを探す（ネスト括弧内のカンマは無視）
    auto constexpr comma_pos = detail::find_top_level_comma<trimmed>();

    if constexpr (comma_pos == std::string_view::npos) {
      // カンマなし → 単一の型トークン
      auto constexpr is_opt = (trimmed.length > 0 && trimmed.buffer[trimmed.length - 1] == '?');
      if constexpr (is_opt) {
        // '?' を除いた型名を取得して optional でラップ（サフィックス対応）
        auto constexpr name = trim_if<detail::is_any_whitespace>(substr(trimmed, 0, static_cast<std::ptrdiff_t>(trimmed.length - 1)));
        using T = typename decltype(parse_type_with_suffix<name>())::type;
        static_assert(!std::is_same_v<T, void>, "'void?' is not supported");
        return detail::type_identity<std::variant<std::optional<T>>>{};
      } else {
        // サフィックス（*, &, &&）を検出して型に適用
        using Suffixed = typename decltype(parse_type_with_suffix<trimmed>())::type;
        // void は monostate にマッピング
        using Mapped = std::conditional_t<std::is_same_v<Suffixed, void>, std::monostate, Suffixed>;
        return detail::type_identity<std::variant<Mapped>>{};
      }
    } else {
      // カンマで分割 → 先頭トークン + 残りの再帰パース
      auto constexpr token = trim(substr(trimmed, 0, comma_pos));
      auto constexpr is_opt = (token.length > 0 && token.buffer[token.length - 1] == '?');
      auto constexpr rest_str = substr(trimmed, comma_pos + 1, static_cast<std::ptrdiff_t>(trimmed.length - comma_pos - 1));
      using RestVariant = typename decltype(parse_to_variant_impl<true, rest_str>())::type;

      if constexpr (token.length == 0) {
        // 空トークン（カンマが連続）→ monostate として扱う
        return detail::type_identity<detail::variant_cat_t<std::variant<std::monostate>, RestVariant>>{};
      } else if constexpr (is_opt) {
        // optional トークン → '?' を除いた名前で型を取得し、サフィックスを適用
        auto constexpr name = trim_if<detail::is_any_whitespace>(substr(token, 0, static_cast<std::ptrdiff_t>(token.length - 1)));
        using Suffixed = typename decltype(parse_type_with_suffix<name>())::type;
        static_assert(!std::is_same_v<Suffixed, void>, "'void?' is not supported");
        return detail::type_identity<detail::variant_cat_t<std::variant<std::optional<Suffixed>>, RestVariant>>{};
      } else {
        // 通常のトークン → サフィックスを検出して型に変換して結合
        using Suffixed = typename decltype(parse_type_with_suffix<token>())::type;
        // void は monostate にマッピング
        using Mapped = std::conditional_t<std::is_same_v<Suffixed, void>, std::monostate, Suffixed>;
        return detail::type_identity<detail::variant_cat_t<std::variant<Mapped>, RestVariant>>{};
      }
    }
  }
}

/**
 * @brief 固定文字列をパースして std::variant 型を生成する
 *
 * @tparam Str 入力文字列（FrozenString）。型名をカンマ区切りで指定する。
 * @return consteval 型のリスト（detail::type_identity<std::variant<...>>）
 *
 * @details parse_to_variant_impl のラッパー。EmptyMeansVoid を false で固定する。
 *          void 型は std::monostate にマッピングされる。
 *
 *  使用例:
 *    @code
 *    using V = typename decltype(parse_to_variant<"int, string, bool"_fs>())::type;
 *    // V = std::variant<int, std::string, bool>
 *    @endcode
 */
template <auto Str>
[[nodiscard]] consteval auto parse_to_variant() {
  return parse_to_variant_impl<false, Str>();
}

/**
 * @brief parse_to_tuple の型エイリアス版
 *
 * @tparam Str 入力文字列（FrozenString）
 * @details parse_to_tuple<Str>()::type と同じ型を直接返す。
 *
 *  使用例:
 *    @code
 *    using T = parse_to_tuple_t<"int, string"_fs>;
 *    // T = std::tuple<int, std::string>
 *    @endcode
 */
template <auto Str>
using parse_to_tuple_t = typename decltype(parse_to_tuple<Str>())::type;

/**
 * @brief parse_to_variant の型エイリアス版
 *
 * @tparam Str 入力文字列（FrozenString）
 * @details parse_to_variant<Str>()::type と同じ型を直接返す。
 *
 *  使用例:
 *    @code
 *    using V = parse_to_variant_t<"int, string"_fs>;
 *    // V = std::variant<int, std::string>
 *    @endcode
 */
template <auto Str>
using parse_to_variant_t = typename decltype(parse_to_variant<Str>())::type;

/**
 * @brief type_mapping の型エイリアス版
 *
 * @tparam S 変換対象の FrozenString
 * @details type_mapping<S>::type を直接返す型エイリアス。
 *
 *  使用例:
 *    @code
 *    static_assert(std::is_same_v<type_mapping_v<"int"_fs>, int>);
 *    @endcode
 */
template <auto S>
using type_mapping_v = typename type_mapping<S>::type;

} // namespace frozenchars
