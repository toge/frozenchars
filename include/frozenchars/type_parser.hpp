#pragma once

#include "frozen_string.hpp"
#include "string_ops.hpp"
#include "detail/type_mapping.hpp"
#include <cstddef>
#include <optional>
#include <tuple>
#include <utility>

namespace frozenchars {

/**
 * @brief 文字列トークンを対応する型に変換する
 * @tparam S 判定対象の文字列
 */
template <auto S>
struct type_mapping {
  using type = typename decltype(detail::map_string_to_type<S>())::type;
};

/**
 * @brief 固定文字列をパースして型のリストを生成する
 *
 * @tparam EmptyMeansVoid 空文字列を std::tuple<void> として扱うかどうかのフラグ
 * @tparam Str 入力文字列 (FrozenString)
 * @return consteval 型のリスト（std::tuple）
 */
template <bool EmptyMeansVoid, auto Str>
consteval auto parse_to_tuple_impl() noexcept {
  auto constexpr trimmed = trim(Str);

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
      while (i < s.length && detail::is_any_whitespace(s.buffer[i])) {
        ++i;
      }
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
        auto constexpr name = trim(substr(trimmed, 0, static_cast<std::ptrdiff_t>(trimmed.length - 1)));
        using T = typename type_mapping<name>::type;
        static_assert(!std::is_same_v<T, detail::unknown_type>, "Unknown type name before '?'");
        static_assert(!std::is_same_v<T, void>, "'void?' is not supported");
        return detail::type_identity<std::tuple<std::optional<T>>>{};
      } else {
        using T = typename type_mapping<trimmed>::type;
        static_assert(!std::is_same_v<T, detail::unknown_type>, "Unknown type name");
        return detail::type_identity<std::tuple<T>>{};
      }
    } else {
      auto constexpr token = trim(substr(trimmed, 0, comma_pos));
      auto constexpr is_opt = (token.length > 0 && token.buffer[token.length - 1] == '?');
      auto constexpr rest_str = substr(trimmed, comma_pos + 1, static_cast<std::ptrdiff_t>(trimmed.length - comma_pos - 1));
      using RestTuple = typename decltype(parse_to_tuple_impl<true, rest_str>())::type;

      if constexpr (token.length == 0) {
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<void>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      } else if constexpr (is_opt) {
        auto constexpr name = trim(substr(token, 0, static_cast<std::ptrdiff_t>(token.length - 1)));
        using T = typename type_mapping<name>::type;
        static_assert(!std::is_same_v<T, detail::unknown_type>, "Unknown type name before '?'");
        static_assert(!std::is_same_v<T, void>, "'void?' is not supported");
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<std::optional<T>>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      } else {
        using T = typename type_mapping<token>::type;
        static_assert(!std::is_same_v<T, detail::unknown_type>, "Unknown type name");
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<T>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      }
    }
  }
}

template <auto Str>
consteval auto parse_to_tuple() {
  return parse_to_tuple_impl<false, Str>();
}

} // namespace frozenchars
