#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>
#include <vector>

#include "inja_value.hpp"

namespace frozenchars::inja {

// ============================================================
// fixed_string - NTTP 対応固定長文字列型
// ============================================================

/**
 * @brief テンプレート引数として関数名を渡すための NTTP 対応固定長文字列型。
 *
 * structured type の要件（全メンバ public、参照なし）を満たす。
 * `fn<"upper", impl>` のように文字列リテラルを NTTP として利用するために使用する。
 */
template <std::size_t N>
struct fixed_string {
  char data[N]{};

  constexpr fixed_string(char const (&s)[N]) noexcept {
    for (auto i = 0uz; i < N; ++i) {
      data[i] = s[i];
    }
  }

  [[nodiscard]] constexpr auto sv() const noexcept -> std::string_view {
    return {data, N - 1};
  }

  [[nodiscard]] constexpr auto operator==(std::string_view s) const noexcept -> bool {
    return sv() == s;
  }

  template <std::size_t M>
  [[nodiscard]] constexpr auto operator==(fixed_string<M> const& other) const noexcept -> bool {
    return sv() == other.sv();
  }

  auto operator<=>(fixed_string const&) const = default;
};

template <std::size_t N>
fixed_string(char const (&)[N]) -> fixed_string<N>;

// ============================================================
// fn - function_list エントリ型
// ============================================================

/**
 * @brief function_list に登録する関数エントリ。
 *
 * @tparam Name  固定文字列 NTTP（関数名）
 * @tparam Fn    関数実装（`inja_value(*)(std::vector<inja_value> const&)` 互換の callable）
 *
 * 使用例:
 * @code
 * fn<"upper", my_upper_impl>
 * @endcode
 */
template <fixed_string Name, auto Fn>
struct fn {
  static constexpr auto name = Name;

  [[nodiscard]] static auto invoke(std::vector<inja_value> const& args) -> inja_value {
    return Fn(args);
  }
};

// ============================================================
// function_list - compile-time 関数登録テーブル
// ============================================================

/**
 * @brief compile-time 関数登録リスト。
 *
 * `fn<Name, Impl>` エントリのパックを保持し、次の操作を提供する:
 * - `contains(name)` : 関数名が登録済みか constexpr で判定する
 * - `dispatch(name, args)` : fold expression による short-circuit dispatch
 *   （hash / type erasure を使わないため optimizer がインライン展開しやすい）
 *
 * @tparam Entries  fn<Name, Fn> 型のパック
 *
 * 使用例:
 * @code
 * using my_fns = function_list<
 *   fn<"upper", upper_impl>,
 *   fn<"lower", lower_impl>
 * >;
 * @endcode
 */
template <typename... Entries>
struct function_list {
  /// is_function_list concept 識別用タグ
  struct function_list_tag {};

  /**
   * @brief 関数名が登録済みか compile-time に判定する。
   */
  [[nodiscard]] static constexpr auto contains(std::string_view name) noexcept -> bool {
    return (... || (Entries::name == name));
  }

  /**
   * @brief 関数名に対応するエントリを呼び出し、結果を返す。
   *
   * fold expression による短絡 OR で最初にマッチしたエントリを呼び出す。
   * マッチしなければ std::nullopt を返す。
   */
  [[nodiscard]] static auto dispatch(std::string_view name, std::vector<inja_value> const& args)
    -> std::optional<inja_value> {
    auto result = std::optional<inja_value>{};
    std::ignore = (false || ... || (Entries::name == name
      ? (result.emplace(Entries::invoke(args)), true)
      : false));
    return result;
  }

  /// 登録件数
  static constexpr auto size = sizeof...(Entries);
};

/// 空の function_list（builtin 関数を一切使わないテンプレート用）
using empty_function_list = function_list<>;

// ============================================================
// is_function_list concept
// ============================================================

/**
 * @brief function_list 型か判定する concept。
 *
 * function_list_tag 入れ子型の有無で判定する。
 */
template <typename T>
concept is_function_list = requires { typename T::function_list_tag; };

// ============================================================
// function_call_set - consteval 用関数名収集コンテナ
// ============================================================

/**
 * @brief テンプレートから抽出した関数呼び出し名を保持する constexpr コンテナ。
 *
 * @tparam MaxCalls   格納可能な最大関数数
 * @tparam MaxNameLen 関数名の最大長（終端 NUL を含む）
 */
template <std::size_t MaxCalls = 256, std::size_t MaxNameLen = 64>
struct function_call_set {
  std::array<std::array<char, MaxNameLen>, MaxCalls> names{};
  std::array<std::size_t, MaxCalls> name_lens{};
  std::size_t count{};

  /// 重複なしで関数名を追加する（容量超過・名前超過は無視）
  constexpr auto insert(std::string_view name) noexcept -> void {
    if (name.size() >= MaxNameLen || count >= MaxCalls) {
      return;
    }
    for (auto i = 0uz; i < count; ++i) {
      if (std::string_view{names[i].data(), name_lens[i]} == name) {
        return;
      }
    }
    for (auto i = 0uz; i < name.size(); ++i) {
      names[count][i] = name[i];
    }
    names[count][name.size()] = '\0';
    name_lens[count] = name.size();
    ++count;
  }

  /// 指定した名前が含まれているか判定する
  [[nodiscard]] constexpr auto contains(std::string_view name) const noexcept -> bool {
    for (auto i = 0uz; i < count; ++i) {
      if (std::string_view{names[i].data(), name_lens[i]} == name) {
        return true;
      }
    }
    return false;
  }

  /// i 番目の名前を取得する
  [[nodiscard]] constexpr auto get(std::size_t i) const noexcept -> std::string_view {
    return {names[i].data(), name_lens[i]};
  }
};

// ============================================================
// 式テキストからの関数呼び出し名抽出
// ============================================================

namespace detail {

constexpr auto is_ident_start(char c) noexcept -> bool {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

constexpr auto is_ident_cont(char c) noexcept -> bool {
  return is_ident_start(c) || (c >= '0' && c <= '9');
}

constexpr auto is_inja_keyword(std::string_view name) noexcept -> bool {
  return name == "true" || name == "false" || name == "null"
      || name == "and"  || name == "or"    || name == "not"
      || name == "in";
}

/**
 * @brief 式テキスト内の関数呼び出し名を function_call_set へ追加する。
 *
 * 文字列リテラル内の識別子はスキップする。
 * 識別子の直後（空白を除く）に `(` が続く場合を関数呼び出しとみなす。
 */
template <std::size_t MaxCalls, std::size_t MaxNameLen>
constexpr auto extract_calls_from_expr(
  std::string_view text,
  function_call_set<MaxCalls, MaxNameLen>& out) noexcept -> void {
  auto pos = std::size_t{0};
  while (pos < text.size()) {
    auto const c = text[pos];
    // 文字列リテラルをスキップする
    if (c == '"' || c == '\'') {
      auto const q = c;
      ++pos;
      while (pos < text.size() && text[pos] != q) {
        if (text[pos] == '\\' && pos + 1 < text.size()) {
          ++pos;
        }
        ++pos;
      }
      if (pos < text.size()) {
        ++pos;
      }
      continue;
    }
    // 識別子開始
    if (is_ident_start(c)) {
      auto const start = pos;
      while (pos < text.size() && is_ident_cont(text[pos])) {
        ++pos;
      }
      auto const name = text.substr(start, pos - start);
      // 空白スキップ後に '(' が続けば関数呼び出し
      auto tmp = pos;
      while (tmp < text.size() && (text[tmp] == ' ' || text[tmp] == '\t')) {
        ++tmp;
      }
      if (tmp < text.size() && text[tmp] == '(' && !is_inja_keyword(name)) {
        out.insert(name);
      }
      continue;
    }
    ++pos;
  }
}

} // namespace detail

} // namespace frozenchars::inja
