#pragma once

/**
 * @file example/snippets/type_parser/type_parser.hpp
 * @brief 型列文字列 → std::tuple / std::variant 生成（スニペット）
 *
 * 元: `include/frozenchars/type_parser.hpp` (426行)
 *    + `include/frozenchars/detail/type_mapping.hpp` (94行)
 * 本体から分離し、サンプルとしてここに移動。必要なプロジェクトへコピペして利用する。
 * `frozenchars` 本体 (`string.hpp` / `string_ops.hpp`) には依存するが、
 * それ以外は自己完結する。
 *
 * 使い方:
 *   #include "example/snippets/type_parser/type_parser.hpp"
 *   using T = typename decltype(frozenchars::parse_to_tuple<"int, string?"_fs>())::type;
 */

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

#include "frozenchars/string.hpp"
#include "frozenchars/string_ops.hpp"

namespace frozenchars {
template <size_t N> struct FrozenString;
}

namespace frozenchars::detail {

/**
 * @brief 型情報を保持するための単純な構造体
 */
template <typename T>
struct type_identity {
  using type = T;
};

struct unknown_type {};

/**
 * @brief 文字列トークンを対応する型に変換する
 *
 * @tparam S 判定対象の FrozenString
 */
template <auto S>
[[nodiscard]] consteval auto map_string_to_type() {
  auto constexpr s = S.sv();
  if constexpr (s == "bool") return type_identity<bool>{};
  else if constexpr (s == "char") return type_identity<char>{};
  else if constexpr (s == "int") return type_identity<int>{};
  else if constexpr (s == "uint" || s == "unsigned") return type_identity<unsigned int>{};
  else if constexpr (s == "long") return type_identity<long>{};
  else if constexpr (s == "ulong") return type_identity<unsigned long>{};
  else if constexpr (s == "float") return type_identity<float>{};
  else if constexpr (s == "double") return type_identity<double>{};
  else if constexpr (s == "string" || s == "str") return type_identity<std::string>{};
  else if constexpr (s == "string_view" || s == "sv") return type_identity<std::string_view>{};
  else if constexpr (s == "void") return type_identity<void>{};
  else if constexpr (s == "size_t" || s == "sz") return type_identity<std::size_t>{};
  else if constexpr (s == "int8_t" || s == "int8") return type_identity<std::int8_t>{};
  else if constexpr (s == "int16_t" || s == "int16") return type_identity<std::int16_t>{};
  else if constexpr (s == "int32_t" || s == "int32") return type_identity<std::int32_t>{};
  else if constexpr (s == "int64_t" || s == "int64") return type_identity<std::int64_t>{};
  else if constexpr (s == "uint8_t" || s == "uint8") return type_identity<std::uint8_t>{};
  else if constexpr (s == "uint16_t" || s == "uint16") return type_identity<std::uint16_t>{};
  else if constexpr (s == "uint32_t" || s == "uint32") return type_identity<std::uint32_t>{};
  else if constexpr (s == "uint64_t" || s == "uint64") return type_identity<std::uint64_t>{};
  else if constexpr (s == "long long" || s == "ll") return type_identity<long long>{};
  else if constexpr (s == "unsigned long long" || s == "ull") return type_identity<unsigned long long>{};
  else if constexpr (s == "char16_t") return type_identity<char16_t>{};
  else if constexpr (s == "char32_t") return type_identity<char32_t>{};
  else if constexpr (s == "wchar_t") return type_identity<wchar_t>{};
  else return type_identity<unknown_type>{};
}

/**
 * @brief 閉じ括弧 ']' を探す。括弧の深さを考慮する。
 */
template <auto S>
consteval std::size_t find_closing_bracket() {
  auto constexpr sv = S.sv();
  std::size_t depth = 0;
  for (std::size_t i = 0; i < sv.size(); ++i) {
    if (sv[i] == '[') ++depth;
    else if (sv[i] == ']') {
      if (--depth == 0) return i;
    }
  }
  return std::string_view::npos;
}

/**
 * @brief トップレベルのカンマ ',' を探す。括弧の深さを考慮する。
 */
template <auto S>
consteval std::size_t find_top_level_comma() {
  auto constexpr sv = S.sv();
  std::size_t depth = 0;
  for (std::size_t i = 0; i < sv.size(); ++i) {
    if (sv[i] == '[') ++depth;
    else if (sv[i] == ']') --depth;
    else if (sv[i] == ',' && depth == 0) return i;
  }
  return std::string_view::npos;
}

} // namespace frozenchars::detail

namespace frozenchars::detail {

/**
 * @brief 2 つの std::variant を結合するユーティリティ
 */
template <typename Acc, typename V2>
struct variant_cat_impl;

template <typename... AccTs>
struct variant_cat_impl<std::variant<AccTs...>, std::variant<>> {
  using type = std::variant<AccTs...>;
};

template <typename... AccTs, typename Head, typename... Tail>
struct variant_cat_impl<std::variant<AccTs...>, std::variant<Head, Tail...>> {
  using type = typename variant_cat_impl<std::variant<AccTs..., Head>, std::variant<Tail...>>::type;
};

template <typename V1, typename V2>
using variant_cat_t = typename variant_cat_impl<V1, V2>::type;

} // namespace frozenchars::detail

namespace frozenchars {

/**
 * @brief 文字列トークンを対応する型に変換する型トレイト
 */
template <auto S>
struct type_mapping {
  using type = typename decltype(detail::map_string_to_type<S>())::type;
};

/**
 * @brief 型トークンのサフィックス（*, &, &&）を検出し、対応する型を返す
 */
template <auto Token>
[[nodiscard]] consteval auto parse_type_with_suffix() {
  auto constexpr sv = Token.sv();
  auto constexpr len = sv.size();
  if constexpr (len >= 2 && sv[len - 2] == '&' && sv[len - 1] == '&') {
    auto constexpr rest = trim(substr(Token, 0, static_cast<std::ptrdiff_t>(len - 2)));
    using Inner = typename decltype(parse_type_with_suffix<rest>())::type;
    return detail::type_identity<Inner&&>{};
  } else if constexpr (len >= 1 && sv[len - 1] == '&') {
    auto constexpr rest = trim(substr(Token, 0, static_cast<std::ptrdiff_t>(len - 1)));
    using Inner = typename decltype(parse_type_with_suffix<rest>())::type;
    return detail::type_identity<Inner&>{};
  } else if constexpr (len >= 1 && sv[len - 1] == '*') {
    auto constexpr rest = trim(substr(Token, 0, static_cast<std::ptrdiff_t>(len - 1)));
    using Inner = typename decltype(parse_type_with_suffix<rest>())::type;
    return detail::type_identity<Inner*>{};
  } else {
    using T = typename type_mapping<Token>::type;
    static_assert(!std::is_same_v<T, detail::unknown_type>, "Unknown type name");
    return detail::type_identity<T>{};
  }
}

/**
 * @brief 固定文字列をパースして型のリスト（std::tuple）を生成する
 */
template <bool EmptyMeansVoid, auto Str>
[[nodiscard]] consteval auto parse_to_tuple_impl() noexcept {
  auto constexpr trimmed = trim_if<detail::is_any_whitespace>(Str);
  if constexpr (trimmed.length == 0) {
    if constexpr (EmptyMeansVoid) {
      return detail::type_identity<std::tuple<void>>{};
    } else {
      return detail::type_identity<std::tuple<>>{};
    }
  } else if constexpr (trimmed.buffer[0] == '[') {
    auto constexpr closing_pos = detail::find_closing_bracket<trimmed>();
    static_assert(closing_pos != std::string_view::npos, "Missing matching ']'");
    auto constexpr inner = substr(trimmed, 1, static_cast<std::ptrdiff_t>(closing_pos - 1));
    using BaseInnerTuple = typename decltype(parse_to_tuple_impl<false, inner>())::type;
    auto constexpr opt_info = [](auto const& s, size_t pos) {
      size_t i = pos + 1;
      while (i < s.length && detail::is_any_whitespace(s.buffer[i])) ++i;
      bool found = (i < s.length && s.buffer[i] == '?');
      return std::pair{found, found ? i : pos};
    }(trimmed, closing_pos);
    auto constexpr is_opt = opt_info.first;
    auto constexpr search_start = opt_info.second;
    using InnerTuple = std::conditional_t<is_opt, std::optional<BaseInnerTuple>, BaseInnerTuple>;
    auto constexpr next_comma = trimmed.sv().find(',', search_start);
    if constexpr (next_comma == std::string_view::npos) {
      return detail::type_identity<std::tuple<InnerTuple>>{};
    } else {
      auto constexpr rest = substr(trimmed, next_comma + 1, static_cast<std::ptrdiff_t>(trimmed.length - next_comma - 1));
      using RestTuple = typename decltype(parse_to_tuple_impl<true, rest>())::type;
      using Combined = decltype(std::tuple_cat(std::declval<std::tuple<InnerTuple>>(), std::declval<RestTuple>()));
      return detail::type_identity<Combined>{};
    }
  } else {
    auto constexpr comma_pos = detail::find_top_level_comma<trimmed>();
    if constexpr (comma_pos == std::string_view::npos) {
      auto constexpr is_opt = (trimmed.length > 0 && trimmed.buffer[trimmed.length - 1] == '?');
      if constexpr (is_opt) {
        auto constexpr name = trim_if<detail::is_any_whitespace>(substr(trimmed, 0, static_cast<std::ptrdiff_t>(trimmed.length - 1)));
        using T = typename decltype(parse_type_with_suffix<name>())::type;
        static_assert(!std::is_same_v<T, void>, "'void?' is not supported");
        return detail::type_identity<std::tuple<std::optional<T>>>{};
      } else {
        using Suffixed = typename decltype(parse_type_with_suffix<trimmed>())::type;
        return detail::type_identity<std::tuple<Suffixed>>{};
      }
    } else {
      auto constexpr token = trim_if<detail::is_any_whitespace>(substr(trimmed, 0, comma_pos));
      auto constexpr is_opt = (token.length > 0 && token.buffer[token.length - 1] == '?');
      auto constexpr rest_str = substr(trimmed, comma_pos + 1, static_cast<std::ptrdiff_t>(trimmed.length - comma_pos - 1));
      using RestTuple = typename decltype(parse_to_tuple_impl<true, rest_str>())::type;
      if constexpr (token.length == 0) {
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<void>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      } else if constexpr (is_opt) {
        auto constexpr name = trim_if<detail::is_any_whitespace>(substr(token, 0, static_cast<std::ptrdiff_t>(token.length - 1)));
        using Suffixed = typename decltype(parse_type_with_suffix<name>())::type;
        static_assert(!std::is_same_v<Suffixed, void>, "'void?' is not supported");
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<std::optional<Suffixed>>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      } else {
        using Suffixed = typename decltype(parse_type_with_suffix<token>())::type;
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<Suffixed>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      }
    }
  }
}

/**
 * @brief 固定文字列をパースして型のリスト（std::tuple）を生成する
 */
template <auto Str>
[[nodiscard]] consteval auto parse_to_tuple() {
  return parse_to_tuple_impl<false, Str>();
}

/**
 * @brief 固定文字列をパースして std::variant 型を生成する（内部実装）
 */
template <bool EmptyMeansVoid, auto Str>
[[nodiscard]] consteval auto parse_to_variant_impl() noexcept {
  auto constexpr trimmed = trim_if<detail::is_any_whitespace>(Str);
  if constexpr (trimmed.length == 0) {
    return detail::type_identity<std::variant<std::monostate>>{};
  } else if constexpr (trimmed.buffer[0] == '[') {
    auto constexpr closing_pos = detail::find_closing_bracket<trimmed>();
    static_assert(closing_pos != std::string_view::npos, "Missing matching ']'");
    auto constexpr inner = substr(trimmed, 1, static_cast<std::ptrdiff_t>(closing_pos - 1));
    using BaseInnerTuple = typename decltype(parse_to_tuple_impl<false, inner>())::type;
    auto constexpr opt_info = [](auto const& s, size_t pos) {
      size_t i = pos + 1;
      while (i < s.length && detail::is_any_whitespace(s.buffer[i])) ++i;
      bool found = (i < s.length && s.buffer[i] == '?');
      return std::pair{found, found ? i : pos};
    }(trimmed, closing_pos);
    auto constexpr is_opt = opt_info.first;
    auto constexpr search_start = opt_info.second;
    using InnerType = std::conditional_t<is_opt, std::optional<BaseInnerTuple>, BaseInnerTuple>;
    auto constexpr next_comma = trimmed.sv().find(',', search_start);
    if constexpr (next_comma == std::string_view::npos) {
      return detail::type_identity<std::variant<InnerType>>{};
    } else {
      auto constexpr rest = substr(trimmed, next_comma + 1, static_cast<std::ptrdiff_t>(trimmed.length - next_comma - 1));
      using RestVariant = typename decltype(parse_to_variant_impl<true, rest>())::type;
      return detail::type_identity<detail::variant_cat_t<std::variant<InnerType>, RestVariant>>{};
    }
  } else {
    auto constexpr comma_pos = detail::find_top_level_comma<trimmed>();
    if constexpr (comma_pos == std::string_view::npos) {
      auto constexpr is_opt = (trimmed.length > 0 && trimmed.buffer[trimmed.length - 1] == '?');
      if constexpr (is_opt) {
        auto constexpr name = trim_if<detail::is_any_whitespace>(substr(trimmed, 0, static_cast<std::ptrdiff_t>(trimmed.length - 1)));
        using T = typename decltype(parse_type_with_suffix<name>())::type;
        static_assert(!std::is_same_v<T, void>, "'void?' is not supported");
        return detail::type_identity<std::variant<std::optional<T>>>{};
      } else {
        using Suffixed = typename decltype(parse_type_with_suffix<trimmed>())::type;
        using Mapped = std::conditional_t<std::is_same_v<Suffixed, void>, std::monostate, Suffixed>;
        return detail::type_identity<std::variant<Mapped>>{};
      }
    } else {
      auto constexpr token = trim(substr(trimmed, 0, comma_pos));
      auto constexpr is_opt = (token.length > 0 && token.buffer[token.length - 1] == '?');
      auto constexpr rest_str = substr(trimmed, comma_pos + 1, static_cast<std::ptrdiff_t>(trimmed.length - comma_pos - 1));
      using RestVariant = typename decltype(parse_to_variant_impl<true, rest_str>())::type;
      if constexpr (token.length == 0) {
        return detail::type_identity<detail::variant_cat_t<std::variant<std::monostate>, RestVariant>>{};
      } else if constexpr (is_opt) {
        auto constexpr name = trim_if<detail::is_any_whitespace>(substr(token, 0, static_cast<std::ptrdiff_t>(token.length - 1)));
        using Suffixed = typename decltype(parse_type_with_suffix<name>())::type;
        static_assert(!std::is_same_v<Suffixed, void>, "'void?' is not supported");
        return detail::type_identity<detail::variant_cat_t<std::variant<std::optional<Suffixed>>, RestVariant>>{};
      } else {
        using Suffixed = typename decltype(parse_type_with_suffix<token>())::type;
        using Mapped = std::conditional_t<std::is_same_v<Suffixed, void>, std::monostate, Suffixed>;
        return detail::type_identity<detail::variant_cat_t<std::variant<Mapped>, RestVariant>>{};
      }
    }
  }
}

/**
 * @brief 固定文字列をパースして std::variant 型を生成する
 */
template <auto Str>
[[nodiscard]] consteval auto parse_to_variant() {
  return parse_to_variant_impl<false, Str>();
}

template <auto Str>
using parse_to_tuple_t = typename decltype(parse_to_tuple<Str>())::type;

template <auto Str>
using parse_to_variant_t = typename decltype(parse_to_variant<Str>())::type;

template <auto S>
using type_mapping_v = typename type_mapping<S>::type;

} // namespace frozenchars
