#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace frozenchars::detail {

/**
 * @brief RFC 3629 に従う UTF-8 シーケンスの長さを検証して返す
 */
[[nodiscard]] constexpr auto utf8_sequence_length(std::string_view str, std::size_t pos) noexcept -> std::size_t {
  if (pos >= str.size()) {
    return 0;
  }

  auto const first = static_cast<unsigned char>(str[pos]);
  if (first < 0x80) {
    return 1;
  }
  auto const continuation = [&](std::size_t at) noexcept { return at < str.size() && (static_cast<unsigned char>(str[at]) & 0xC0) == 0x80; };

  if (first >= 0xC2 && first <= 0xDF) {
    return continuation(pos + 1) ? 2 : 0;
  }
  if (first >= 0xE0 && first <= 0xEF) {
    if (!continuation(pos + 1)) {
      return 0;
    }
    auto const second = static_cast<unsigned char>(str[pos + 1]);
    if ((first == 0xE0 && second < 0xA0) || (first == 0xED && second > 0x9F)) {
      return 0;
    }
    return continuation(pos + 2) ? 3 : 0;
  }
  if (first >= 0xF0 && first <= 0xF4) {
    if (!continuation(pos + 1)) {
      return 0;
    }
    auto const second = static_cast<unsigned char>(str[pos + 1]);
    if ((first == 0xF0 && second < 0x90) || (first == 0xF4 && second > 0x8F)) {
      return 0;
    }
    return continuation(pos + 2) && continuation(pos + 3) ? 4 : 0;
  }
  return 0;
}

/**
 * @brief 文字列全体が RFC 3629 UTF-8 として妥当か判定する
 */
[[nodiscard]] constexpr auto is_valid_utf8_string(std::string_view str) noexcept -> bool {
  for (std::size_t pos = 0; pos < str.size();) {
    auto const length = utf8_sequence_length(str, pos);
    if (length == 0) {
      return false;
    }
    pos += length;
  }
  return true;
}

} // namespace frozenchars::detail
