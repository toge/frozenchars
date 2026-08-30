#pragma once

/**
 * @file example/snippets/color/color.hpp
 * @brief hex カラー文字列 → RGB/RGBA タプル変換（スニペット）
 *
 * 元: `include/frozenchars/color.hpp` (135行)
 * 本体から分離し、サンプルとしてここに移動。必要なプロジェクトへコピペして利用する。
 * `frozenchars` 本体への依存はなく、標準ライブラリのみで自己完結する。
 */

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <tuple>

namespace frozenchars {

// ---- 内部ヘルパ（元 detail/char_utils.hpp 由来、スニペット内で自己完結） ----

namespace detail {

[[nodiscard]] inline auto consteval is_hex_digit(char const c) noexcept -> bool {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

[[nodiscard]] inline auto consteval hex_digit_to_value(char const c) -> std::uint8_t {
  if (c >= '0' && c <= '9') return static_cast<std::uint8_t>(c - '0');
  if (c >= 'a' && c <= 'f') return static_cast<std::uint8_t>(10 + (c - 'a'));
  if (c >= 'A' && c <= 'F') return static_cast<std::uint8_t>(10 + (c - 'A'));
  throw std::invalid_argument("parse_hex_color: invalid hex digit");
}

[[nodiscard]] inline auto consteval parse_hex_byte(char const hi, char const lo) -> std::uint8_t {
  if (!is_hex_digit(hi) || !is_hex_digit(lo)) {
    throw std::invalid_argument("parse_hex_color: invalid hex digit");
  }
  return static_cast<std::uint8_t>((hex_digit_to_value(hi) << 4u) | hex_digit_to_value(lo));
}

[[nodiscard]] inline auto consteval parse_hex_shorthand_byte(char const c) -> std::uint8_t {
  auto const value = hex_digit_to_value(c);
  return static_cast<std::uint8_t>((value << 4u) | value);
}

} // namespace detail

/**
 * @brief `#RGB` / `#RRGGBB` 形式の色文字列を RGB タプルへ変換する
 *
 * @param str 対象文字列
 * @return auto consteval `(r, g, b)` の順に並んだタプル
 */
[[nodiscard]] inline auto consteval parse_hex_rgb(std::string_view str) {
  if (str.empty() || str[0] != '#' || (str.size() != 4 && str.size() != 7)) {
    throw std::invalid_argument("parse_hex_rgb: expected #RGB or #RRGGBB");
  }
  if (str.size() == 4) {
    return std::tuple{
      detail::parse_hex_shorthand_byte(str[1]),
      detail::parse_hex_shorthand_byte(str[2]),
      detail::parse_hex_shorthand_byte(str[3])
    };
  }
  return std::tuple{
    detail::parse_hex_byte(str[1], str[2]),
    detail::parse_hex_byte(str[3], str[4]),
    detail::parse_hex_byte(str[5], str[6])
  };
}

/**
 * @brief `#RGB` / `#RRGGBB` 形式の色文字列を RGB タプルへ変換する
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto `(r, g, b)` の順に並んだタプル
 */
template <size_t N>
[[nodiscard]] inline auto consteval parse_hex_rgb(char const (&str)[N]) {
  return parse_hex_rgb(std::string_view{str, N - 1});
}

/**
 * @brief `#RGBA` / `#RRGGBBAA` 形式の色文字列を RGBA タプルへ変換する関数
 *
 * @param str 対象文字列
 * @return auto  `(r, g, b, a)` の順に並んだタプル
 */
[[nodiscard]] inline auto consteval parse_hex_rgba(std::string_view str) {
  if (str.empty() || str[0] != '#' || (str.size() != 5 && str.size() != 9)) {
    throw std::invalid_argument("parse_hex_rgba: expected #RGBA or #RRGGBBAA");
  }
  if (str.size() == 5) {
    return std::tuple{
      detail::parse_hex_shorthand_byte(str[1]),
      detail::parse_hex_shorthand_byte(str[2]),
      detail::parse_hex_shorthand_byte(str[3]),
      detail::parse_hex_shorthand_byte(str[4])
    };
  }
  return std::tuple{
    detail::parse_hex_byte(str[1], str[2]),
    detail::parse_hex_byte(str[3], str[4]),
    detail::parse_hex_byte(str[5], str[6]),
    detail::parse_hex_byte(str[7], str[8])
  };
}

/**
 * @brief 文字列リテラル版の `#RGBA` / `#RRGGBBAA` パーサ
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto `(r, g, b, a)` の順に並んだタプル
 */
template <size_t N>
[[nodiscard]] inline auto consteval parse_hex_rgba(char const (&str)[N]) {
  return parse_hex_rgba(std::string_view{str, N - 1});
}

/**
 * @brief RGB タプルを BGR タプルへ並び替える
 */
template <typename R, typename G, typename B>
[[nodiscard]] inline auto consteval to_bgr(std::tuple<R, G, B> const& rgb) {
  return std::tuple<B, G, R>{std::get<2>(rgb), std::get<1>(rgb), std::get<0>(rgb)};
}

/**
 * @brief RGBA タプルを BGRA タプルへ並び替える
 */
template <typename R, typename G, typename B, typename A>
[[nodiscard]] inline auto consteval to_bgra(std::tuple<R, G, B, A> const& rgba) {
  return std::tuple<B, G, R, A>{std::get<2>(rgba), std::get<1>(rgba), std::get<0>(rgba), std::get<3>(rgba)};
}

/**
 * @brief RGBA タプルを ABGR タプルへ並び替える
 */
template <typename R, typename G, typename B, typename A>
[[nodiscard]] inline auto consteval to_abgr(std::tuple<R, G, B, A> const& rgba) {
  return std::tuple<A, B, G, R>{std::get<3>(rgba), std::get<2>(rgba), std::get<1>(rgba), std::get<0>(rgba)};
}

} // namespace frozenchars
