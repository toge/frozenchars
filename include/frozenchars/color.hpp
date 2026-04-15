#pragma once

#include "frozen_string.hpp"
#include "detail/char_utils.hpp"
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <tuple>

namespace frozenchars {

/**
 * @brief `#RGB` / `#RRGGBB` 形式の色文字列を RGB タプルへ変換する
 *
 * @param str 対象文字列
 * @return auto consteval `(r, g, b)` の順に並んだタプル
 */
auto consteval parse_hex_rgb(std::string_view str) {
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
auto consteval parse_hex_rgb(char const (&str)[N]) {
  return parse_hex_rgb(std::string_view{str, N - 1});
}

/**
 * @brief `#RGBA` / `#RRGGBBAA` 形式の色文字列を RGBA タプルへ変換する関数
 *
 * @param str 対象文字列
 * @return auto  `(r, g, b, a)` の順に並んだタプル
 */
auto consteval parse_hex_rgba(std::string_view str) {
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
auto consteval parse_hex_rgba(char const (&str)[N]) {
  return parse_hex_rgba(std::string_view{str, N - 1});
}

/**
 * @brief RGB タプルを BGR タプルへ並び替える
 *
 * @tparam R 赤チャネル型
 * @tparam G 緑チャネル型
 * @tparam B 青チャネル型
 * @param rgb `(r, g, b)` の順のタプル
 * @return auto `(b, g, r)` の順のタプル
 */
template <typename R, typename G, typename B>
auto consteval to_bgr(std::tuple<R, G, B> const& rgb) {
  return std::tuple<B, G, R>{std::get<2>(rgb), std::get<1>(rgb), std::get<0>(rgb)};
}

/**
 * @brief RGBA タプルを BGRA タプルへ並び替える
 *
 * @tparam R 赤チャネル型
 * @tparam G 緑チャネル型
 * @tparam B 青チャネル型
 * @tparam A アルファチャネル型
 * @param rgba `(r, g, b, a)` の順のタプル
 * @return auto `(b, g, r, a)` の順のタプル
 */
template <typename R, typename G, typename B, typename A>
auto consteval to_bgra(std::tuple<R, G, B, A> const& rgba) {
  return std::tuple<B, G, R, A>{std::get<2>(rgba), std::get<1>(rgba), std::get<0>(rgba), std::get<3>(rgba)};
}

/**
 * @brief RGBA タプルを ABGR タプルへ並び替える
 *
 * @tparam R 赤チャネル型
 * @tparam G 緑チャネル型
 * @tparam B 青チャネル型
 * @tparam A アルファチャネル型
 * @param rgba `(r, g, b, a)` の順のタプル
 * @return auto `(a, b, g, r)` の順のタプル
 */
template <typename R, typename G, typename B, typename A>
auto consteval to_abgr(std::tuple<R, G, B, A> const& rgba) {
  return std::tuple<A, B, G, R>{std::get<3>(rgba), std::get<2>(rgba), std::get<1>(rgba), std::get<0>(rgba)};
}

} // namespace frozenchars
