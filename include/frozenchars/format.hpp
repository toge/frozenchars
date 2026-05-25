#pragma once

#include <cstddef>
#if defined(__has_include) && __has_include(<format>)
#  include <format>
#endif
#include <string_view>
#include <type_traits>
#include <utility>

#include "string.hpp"

#ifdef __cpp_lib_format
namespace std {

template <size_t N>
struct formatter<frozenchars::FrozenString<N>, char> {
  formatter<std::string_view, char> delegate_{};

  constexpr auto parse(format_parse_context& ctx) {
    return delegate_.parse(ctx);
  }

  template <typename FormatContext>
  auto format(frozenchars::FrozenString<N> const& value, FormatContext& ctx) const {
    auto const sv = value.sv();
    auto const pos = sv.find('\0');
    if (pos != std::string_view::npos) {
      return delegate_.format(sv.substr(0, pos), ctx);
    }
    return delegate_.format(sv, ctx);
  }
};

} // namespace std

#if defined(__has_include) && __has_include(<format>)
namespace frozenchars {

/**
 * @brief NTTP（非型テンプレート引数）を介して安全にformat_stringを生成するヘルパー
 */
template <frozenchars::FrozenString Str, typename... Args>
consteval auto to_format_string() noexcept {
  return std::format_string<Args...>{Str.sv()};
}

namespace detail {

/**
 * @brief std::format へ渡す引数型を正規化するヘルパー
 * @details 配列型（文字列リテラル等）をポインタへ崩壊させ、
 *          フォーマット文字列の型リストとの不一致を避ける。
 */
template <typename T>
constexpr auto normalize_format_arg(T&& value) noexcept -> std::decay_t<T> {
  return static_cast<std::decay_t<T>>(std::forward<T>(value));
}

}

/**
 * @brief 型リストを明示せずに FrozenString から安全な format を行うヘルパー
 * @details 呼び出し時の実引数から型を自動推論し、内部で format_string を生成する。
 */
template <frozenchars::FrozenString Str, typename... Args>
auto frozen_format(Args&&... args) {
  constexpr auto fmt = to_format_string<Str, std::decay_t<Args>...>();
  return std::format(fmt, detail::normalize_format_arg(std::forward<Args>(args))...);
}

} // namespace frozenchars

#endif

#endif
