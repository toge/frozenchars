#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string_view>

namespace frozenchars {
template <size_t N> struct FrozenString;
}

namespace frozenchars::detail {

/**
 * @brief 半角スペースか判定する
 *
 * @param c 判定する文字
 * @return auto 半角スペースなら true
 */
auto constexpr is_space_char(char c) noexcept {
  return c == ' ';
}

/**
 * @brief ASCII 空白文字か判定する
 *
 * @param c 判定する文字
 * @return auto 空白なら true
 */
auto constexpr is_any_whitespace(char c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

/**
 * @brief 指定した文字かどうかを判定する述語を生成する
 *
 * @tparam Target 対象の文字
 */
template <char Target>
inline constexpr auto is_char = [](char c) noexcept { return c == Target; };

/**
 * @brief ASCII の16進数字かどうかを判定する
 *
 * @param c 判定する文字
 * @return auto 16進数字文字なら true
 */
auto consteval is_hex_digit(char const c) noexcept {
  return (c >= '0' && c <= '9')
    || (c >= 'a' && c <= 'f')
    || (c >= 'A' && c <= 'F');
}

/**
 * @brief 16進数字1文字を 0..15 の数値に変換する
 *
 * @param c 変換する16進数字
 * @return auto 変換結果の数字
 */
auto consteval hex_digit_to_value(char const c) {
  if (c >= '0' && c <= '9') {
    return static_cast<std::uint8_t>(c - '0');
  }
  if (c >= 'a' && c <= 'f') {
    return static_cast<std::uint8_t>(10 + (c - 'a'));
  }
  if (c >= 'A' && c <= 'F') {
    return static_cast<std::uint8_t>(10 + (c - 'A'));
  }
  throw std::invalid_argument("parse_hex_color: invalid hex digit");
}

/**
 * @brief 16進数字2文字を 1byte に変換する
 *
 * @param hi 上位4bitを表す16進数字
 * @param lo 下位4bitを表す16進数字
 * @return auto 変換結果
 */
auto consteval parse_hex_byte(char const hi, char const lo) {
  if (!is_hex_digit(hi) || !is_hex_digit(lo)) {
    throw std::invalid_argument("parse_hex_color: invalid hex digit");
  }
  return static_cast<std::uint8_t>((hex_digit_to_value(hi) << 4u) | hex_digit_to_value(lo));
}

/**
 * @brief 16進数字1文字を nibble 複製して 1byte に変換する
 *
 * @param c 変換する16進数字
 * @return auto 変換結果
 */
auto consteval parse_hex_shorthand_byte(char const c) {
  auto const value = hex_digit_to_value(c);
  return static_cast<std::uint8_t>((value << 4u) | value);
}

/**
 * @brief URLエンコードでエンコード不要な文字（unreserved）か判定する
 * RFC 3986: ALPHA / DIGIT / "-" / "." / "_" / "~"
 *
 * @param c 判定する文字
 * @return auto エンコード不要なら true
 */
auto constexpr is_unreserved(char const c) noexcept {
  return (c >= 'A' && c <= 'Z')
    || (c >= 'a' && c <= 'z')
    || (c >= '0' && c <= '9')
    || c == '-' || c == '.' || c == '_' || c == '~';
}

/**
 * @brief URLエンコード後の文字数を計算する
 *
 * @tparam N FrozenStringの長さ (終端文字'\0'を含む)
 * @param str 処理対象の文字列
 * @return std::size_t エンコード後の文字数
 */
template <size_t N>
auto consteval count_url_encoded_size(FrozenString<N> const& str) noexcept -> std::size_t {
  auto count = 0uz;
  for (auto const c : str.sv()) {
    if (is_unreserved(c)) {
      ++count;
    } else {
      count += 3;
    }
  }
  return count;
}

/**
 * @brief URLデコード後の文字数を計算する
 *
 * @tparam N FrozenStringの長さ (終端文字'\0'を含む)
 * @param str 処理対象の文字列
 * @return std::size_t デコード後の文字数
 */
template <size_t N>
auto consteval count_url_decoded_size(FrozenString<N> const& str) noexcept -> std::size_t {
  auto count = 0uz;
  auto const s = str.sv();
  for (auto i = 0uz; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size() && is_hex_digit(s[i + 1]) && is_hex_digit(s[i + 2])) {
      ++count;
      i += 2;
    } else if (s[i] == '+') {
      ++count;
    } else {
      ++count;
    }
  }
  return count;
}

/**
 * @brief 0..15 の数値を16進数字に変換する
 *
 * @param value 変換する数値
 * @param uppercase 大文字にするかどうか
 * @return auto 変換結果の文字
 */
auto constexpr value_to_hex_digit(std::uint8_t const value, bool const uppercase = true) noexcept {
  if (value < 10) {
    return static_cast<char>('0' + value);
  }
  if (uppercase) {
    return static_cast<char>('A' + (value - 10));
  }
  return static_cast<char>('a' + (value - 10));
}

/**
 * @brief Base64の文字テーブル
 */
inline constexpr char base64_chars[] =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789+/";

/**
 * @brief Base64文字から数値(0..63)への変換
 *
 * @param c 変換する文字
 * @return auto 変換後の数値。不正な文字の場合は 255
 */
auto constexpr base64_char_to_value(char const c) noexcept -> std::uint8_t {
  if (c >= 'A' && c <= 'Z') {
    return static_cast<std::uint8_t>(c - 'A');
  }
  if (c >= 'a' && c <= 'z') {
    return static_cast<std::uint8_t>(c - 'a' + 26);
  }
  if (c >= '0' && c <= '9') {
    return static_cast<std::uint8_t>(c - '0' + 52);
  }
  if (c == '+') {
    return 62;
  }
  if (c == '/') {
    return 63;
  }
  return 255;
}

/**
 * @brief Base64エンコード後の文字数を計算する
 *
 * @param n 入力バイト数
 * @return std::size_t エンコード後の文字数（パディング込み）
 */
auto constexpr count_base64_encoded_size(size_t n) noexcept -> std::size_t {
  return ((n + 2) / 3) * 4;
}

/**
 * @brief Base64デコード後の最大バイト数を計算する
 *
 * @tparam N FrozenStringの長さ (終端文字'\0'を含む)
 * @param str 処理対象の文字列
 * @return std::size_t デコード後のバイト数（パディングを考慮）
 */
template <size_t N>
auto consteval count_base64_decoded_size(FrozenString<N> const& str) noexcept -> std::size_t {
  auto const s = str.sv();
  if (s.empty()) {
    return 0;
  }
  auto count = (s.size() / 4) * 3;
  if (s.ends_with("==")) {
    count -= 2;
  }
  else if (s.ends_with("=")) {
    count -= 1;
  }
  return count;
}

} // namespace frozenchars::detail
