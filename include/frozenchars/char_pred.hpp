#pragma once

namespace frozenchars {

/**
 * @brief 英大文字か判定する
 *
 * @param c 判定する文字
 * @return true 英大文字（'A'-'Z'）の場合
 */
constexpr bool is_upper(char c) noexcept {
  return c >= 'A' && c <= 'Z';
}

/**
 * @brief 英小文字か判定する
 *
 * @param c 判定する文字
 * @return true 英小文字（'a'-'z'）の場合
 */
constexpr bool is_lower(char c) noexcept {
  return c >= 'a' && c <= 'z';
}

/**
 * @brief 英字（大文字・小文字）か判定する
 *
 * @param c 判定する文字
 * @return true 英字の場合
 */
constexpr bool is_alpha(char c) noexcept {
  return is_upper(c) || is_lower(c);
}

/**
 * @brief 10進数字か判定する
 *
 * @param c 判定する文字
 * @return true 10進数字（'0'-'9'）の場合
 */
constexpr bool is_digit(char c) noexcept {
  return c >= '0' && c <= '9';
}

/**
 * @brief 英数字か判定する
 *
 * @param c 判定する文字
 * @return true 英数字の場合
 */
constexpr bool is_alnum(char c) noexcept {
  return is_alpha(c) || is_digit(c);
}

/**
 * @brief 16進数字か判定する
 *
 * @param c 判定する文字
 * @return true 16進数字（0-9, a-f, A-F）の場合
 */
constexpr bool is_xdigit(char c) noexcept {
  return is_digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/**
 * @brief 制御文字か判定する
 *
 * @param c 判定する文字
 * @return true 制御文字（0x00-0x1F, 0x7F）の場合
 */
constexpr bool is_cntrl(char c) noexcept {
  return (c >= 0 && c <= '\x1F') || c == '\x7F';
}

/**
 * @brief 表示可能な文字（スペース以外）か判定する
 *
 * @param c 判定する文字
 * @return true 表示可能文字（0x21-0x7E）の場合
 */
constexpr bool is_graph(char c) noexcept {
  return c >= '\x21' && c <= '\x7E';
}

/**
 * @brief 表示可能な文字（スペース含む）か判定する
 *
 * @param c 判定する文字
 * @return true 表示可能文字またはスペース（0x20-0x7E）の場合
 */
constexpr bool is_print(char c) noexcept {
  return c >= '\x20' && c <= '\x7E';
}

/**
 * @brief 句読点か判定する
 *
 * 以下のASCII範囲の文字が該当します:
 * 0x21-0x2F, 0x3A-0x40, 0x5B-0x60, 0x7B-0x7E
 * (!"#$%&'()*+,-./:;<=>?@[\]^_`{|}~)
 *
 * @param c 判定する文字
 * @return true 句読点の場合
 */
constexpr bool is_punct(char c) noexcept {
  return (c >= '\x21' && c <= '\x2F')
      || (c >= '\x3A' && c <= '\x40')
      || (c >= '\x5B' && c <= '\x60')
      || (c >= '\x7B' && c <= '\x7E');
}

/**
 * @brief 空白文字（スペースまたはタブ）か判定する
 *
 * @param c 判定する文字
 * @return true スペースまたはタブの場合
 */
constexpr bool is_blank(char c) noexcept {
  return c == ' ' || c == '\t';
}

/**
 * @brief ASCII 空白文字か判定する
 *
 * @param c 判定する文字
 * @return true 空白文字（スペース、タブ、改行、復帰、\f、\v）の場合
 */
constexpr bool is_space(char c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

} // namespace frozenchars
