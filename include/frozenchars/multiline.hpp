#pragma once

#include "frozen_string.hpp"
#include "detail/char_utils.hpp"
#include <cstddef>
#include <string_view>

namespace frozenchars {

/**
 * @brief 各行の先頭の空白を指定個数分削除する
 * 空白がなくなった行についてはそれ以上削除しない
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param n 削除する空白の数
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval remove_leading_spaces(FrozenString<N> const& str, size_t n) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  while (i < str.length) {
    auto spaces = 0uz;
    while (i < str.length && str.buffer[i] == ' ' && spaces < n) {
      ++i;
      ++spaces;
    }
    while (i < str.length && str.buffer[i] != '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
    if (i < str.length && str.buffer[i] == '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval remove_leading_spaces(char const (&str)[N], size_t n) noexcept {
  return remove_leading_spaces(FrozenString{str}, n);
}

/**
 * @brief 指定されたコメント開始文字列で始まる行を削除する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param comment_seq コメント開始文字列 (デフォルト: "#")
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval remove_comment_lines(FrozenString<N> const& str, std::string_view comment_seq = "#") noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto const sv = str.sv();

  while (i < str.length) {
    auto const line_start = i;
    while (i < str.length && str.buffer[i] != '\n') {
      ++i;
    }
    auto const line_end = i;
    auto const line = sv.substr(line_start, line_end - line_start);

    if (!line.starts_with(comment_seq)) {
      for (auto j = line_start; j < line_end; ++j) {
        res.buffer[offset++] = str.buffer[j];
      }
      if (i < str.length && str.buffer[i] == '\n') {
        res.buffer[offset++] = str.buffer[i];
      }
    }
    if (i < str.length && str.buffer[i] == '\n') {
      ++i;
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval remove_comment_lines(char const (&str)[N], std::string_view comment_seq = "#") noexcept {
  return remove_comment_lines(FrozenString{str}, comment_seq);
}

/**
 * @brief 指定された文字列以降行末までを削除する
 * 指定された文字列直前に空白文字が連続している場合はそれも削除します。
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param comment_seq コメント開始文字列 (デフォルト: "#")
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval remove_comments(FrozenString<N> const& str, std::string_view comment_seq = "#") noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto const sv = str.sv();

  while (i < str.length) {
    auto const line_start = i;
    while (i < str.length && str.buffer[i] != '\n') {
      ++i;
    }
    auto const line_end = i;
    auto const line = sv.substr(line_start, line_end - line_start);

    auto const comment_pos = line.find(comment_seq);
    if (comment_pos == std::string_view::npos) {
      // コメントなし
      for (auto j = line_start; j < line_end; ++j) {
        res.buffer[offset++] = str.buffer[j];
      }
    } else {
      // コメントあり。直前の空白も削除
      auto last_non_space = comment_pos;
      while (last_non_space > 0 && detail::is_whitespace(line[last_non_space - 1])) {
        --last_non_space;
      }
      for (auto j = 0uz; j < last_non_space; ++j) {
        res.buffer[offset++] = line[j];
      }
    }

    if (i < str.length && str.buffer[i] == '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval remove_comments(char const (&str)[N], std::string_view comment_seq = "#") noexcept {
  return remove_comments(FrozenString{str}, comment_seq);
}

/**
 * @brief すべての行を結合する
 * 行末と行頭どちらにもスペースがない場合にはスペースを入れる
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 結合結果
 */
template <size_t N>
auto consteval join_lines(FrozenString<N> const& str) noexcept {
  constexpr auto OUT_CAP = (N > 0 ? N - 1 : 0) * 2 + 1;
  auto res = FrozenString<OUT_CAP>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto first_line = true;

  while (i < str.length) {
    if (!first_line) {
      auto needs_space = true;
      if (offset > 0 && (res.buffer[offset - 1] == ' ' || res.buffer[offset - 1] == '\t')) {
        needs_space = false;
      }
      if (i < str.length && (str.buffer[i] == ' ' || str.buffer[i] == '\t')) {
        needs_space = false;
      }
      if (needs_space) {
        res.buffer[offset++] = ' ';
      }
    }

    while (i < str.length && str.buffer[i] != '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
    if (i < str.length && str.buffer[i] == '\n') {
      ++i;
    }
    first_line = false;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval join_lines(char const (&str)[N]) noexcept {
  return join_lines(FrozenString{str});
}

/**
 * @brief すべての行末の空白を削除する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval trim_trailing_spaces(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  while (i < str.length) {
    auto line_start = i;
    while (i < str.length && str.buffer[i] != '\n') {
      ++i;
    }
    auto line_end = i;
    auto last_non_space = line_end;
    while (last_non_space > line_start && detail::is_whitespace(str.buffer[last_non_space - 1])) {
      --last_non_space;
    }
    for (auto j = line_start; j < last_non_space; ++j) {
      res.buffer[offset++] = str.buffer[j];
    }
    if (i < str.length && str.buffer[i] == '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval trim_trailing_spaces(char const (&str)[N]) noexcept {
  return trim_trailing_spaces(FrozenString{str});
}

/**
 * @brief すべての空行を削除する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval remove_empty_lines(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  while (i < str.length) {
    auto line_start = i;
    while (i < str.length && str.buffer[i] != '\n') {
      ++i;
    }
    if (i > line_start) {
      for (auto j = line_start; j < i; ++j) {
        res.buffer[offset++] = str.buffer[j];
      }
      if (i < str.length && str.buffer[i] == '\n') {
        res.buffer[offset++] = str.buffer[i++];
      }
    } else {
      if (i < str.length && str.buffer[i] == '\n') {
        ++i;
      }
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval remove_empty_lines(char const (&str)[N]) noexcept {
  return remove_empty_lines(FrozenString{str});
}

} // namespace frozenchars
