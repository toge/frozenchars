#pragma once

#include <cstddef>

#if defined(__has_include) && __has_include(<format>)
#  include <format>
#endif

#include <string_view>

#include "string.hpp"

#ifdef __cpp_lib_format
namespace std {

/**
 * @brief FrozenString を std::format で書式化できるようにする std::formatter 特殊化
 *
 * 書式指定の解釈は std::string_view 用の formatter に委譲する。
 */
template <size_t N>
struct formatter<frozenchars::FrozenString<N>, char> {
  formatter<std::string_view, char> delegate_{};  ///< 処理を委譲する string_view 用 formatter

  /// @brief 書式指定を解析する（委譲）
  constexpr auto parse(format_parse_context& ctx) {
    return delegate_.parse(ctx);
  }

  /// @brief 文字列をフォーマットして出力する（委譲）
  template <typename FormatContext>
  auto format(frozenchars::FrozenString<N> const& value, FormatContext& ctx) const {
    return delegate_.format(value.sv(), ctx);
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
