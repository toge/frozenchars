#pragma once

#include "frozen_string.hpp"
#include "string_ops.hpp"

namespace frozenchars {

/**
 * @brief 拡張正規表現のコメントと空白を削除してstd::regexで使える正規表現に変換する
 *
 * @tparam N
 * @param pattern 拡張正規表現で記載されたパターン
 * @return auto 拡張ではない正規表現
 */
template <size_t N>
auto constexpr remove_regex_comment(FrozenString<N> const& pattern) noexcept {
  auto constexpr is_extended_mode_whitespace = [](char const c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f';
  };

  auto const has_extended_mode_prefix =
    pattern.length >= 4
    && pattern.buffer[0] == '('
    && pattern.buffer[1] == '?'
    && pattern.buffer[2] == 'x'
    && pattern.buffer[3] == ')';

  auto result = FrozenString<N>{};
  auto offset = 0uz;
  auto escaped = false;
  auto in_char_class = false;
  auto in_comment = false;

  for (auto i = has_extended_mode_prefix ? 4uz : 0uz; i < pattern.length; ++i) {
    auto const c = pattern.buffer[i];

    if (in_comment) {
      if (c == '\n' || c == '\r') {
        in_comment = false;
      }
      continue;
    }

    if (escaped) {
      result.buffer[offset++] = c;
      escaped = false;
      continue;
    }

    if (c == '\\') {
      result.buffer[offset++] = c;
      escaped = true;
      continue;
    }

    if (!in_char_class && c == '#') {
      in_comment = true;
      continue;
    }

    if (!in_char_class && is_extended_mode_whitespace(c)) {
      continue;
    }

    if (c == '[') {
      in_char_class = true;
    } else if (in_char_class && c == ']') {
      in_char_class = false;
    }

    result.buffer[offset++] = c;
  }

  result.buffer[offset] = '\0';
  result.length = offset;
  return result;
}

template <size_t N>
auto constexpr remove_regex_comment(char const (&pattern)[N]) noexcept {
  return remove_regex_comment(FrozenString{pattern});
}

template <auto Pattern>
  requires detail::is_frozen_string_v<decltype(Pattern)>
auto consteval remove_regex_comment() noexcept {
  return shrink_to_fit<remove_regex_comment(Pattern)>();
}

} // namespace frozenchars
