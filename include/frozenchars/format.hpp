#pragma once

#include <cstddef>

#if defined(__has_include) && __has_include(<format>)
#  include <format>
#endif

#include <string_view>

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

namespace frozenchars {

/**
 * @brief NTTP として渡された FrozenString を std::string_view として取り出す
 *
 * @details
 *   std::format / std::format_to / std::formatted_size 等の第一引数
 *   (std::format_string<Args...>) に FrozenString を直接渡すために利用する。
 *
 *   戻り値は NTTP のバッファを指す std::string_view であり、consteval
 *   文脈で評価されるため定数式となる。これにより std::format_string の
 *   consteval コンストラクタにそのまま渡せる。
 *
 *   使用例:
 *     std::format(frozenchars::to_sv<"hello {}"_fs>(), 42);
 *     std::format_to(out, frozenchars::to_sv<"x={}"_fs>(), 1);
 *     std::formatted_size(frozenchars::to_sv<"{} {}"_fs>(), "a", 1);
 */
template <frozenchars::FrozenString Str>
consteval auto to_sv() noexcept -> std::string_view {
  return Str.sv();
}

} // namespace frozenchars

#endif
