#pragma once

#include "frozen_string.hpp"
#include "detail/char_utils.hpp"
#include <cstddef>
#include <stdexcept>
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
auto consteval remove_leading_spaces(FrozenString<N> const& str, size_t n = 0) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  while (i < str.length) {
    auto spaces = 0uz;
    while (i < str.length && str.buffer[i] == ' ' && (n == 0 || spaces < n)) {
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
auto consteval remove_leading_spaces(char const (&str)[N], size_t n = 0) noexcept {
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
      // コメントあり。コメント開始文字列から行末まで削除
      for (auto j = 0uz; j < comment_pos; ++j) {
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
 * @brief 各行の末尾の連続した半角スペースを削除する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param n 削除する空白の最大数 (0 の場合はすべて削除)
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval remove_trailing_spaces(FrozenString<N> const& str, size_t n = 0) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  while (i < str.length) {
    auto line_start = i;
    while (i < str.length && str.buffer[i] != '\n') {
      ++i;
    }
    auto line_end = i;
    // remove_trailing_spaces は「半角スペース」のみを削除対象にする
    auto spaces = 0uz;
    while (line_end > line_start && str.buffer[line_end - 1] == ' ' && (n == 0 || spaces < n)) {
      --line_end;
      ++spaces;
    }
    for (auto j = line_start; j < line_end; ++j) {
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
auto consteval remove_trailing_spaces(char const (&str)[N], size_t n = 0) noexcept {
  return remove_trailing_spaces(FrozenString{str}, n);
}

/**
 * @brief 指定した開始文字列から終了文字列までの範囲を繰り返し削除する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param start_seq 削除範囲の開始文字列
 * @param end_seq 削除範囲の終了文字列
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval remove_range_comments(FrozenString<N> const& str,
                                     std::string_view const start_seq,
                                     std::string_view const end_seq) noexcept {
  if (start_seq.empty() || end_seq.empty()) {
    throw std::invalid_argument("remove_range_comments: start_seq/end_seq must not be empty");
  }

  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto cursor = 0uz;
  auto const sv = str.sv();

  while (cursor < str.length) {
    auto const start_pos = sv.find(start_seq, cursor);
    if (start_pos == std::string_view::npos) {
      for (auto i = cursor; i < str.length; ++i) {
        res.buffer[offset++] = str.buffer[i];
      }
      break;
    }

    for (auto i = cursor; i < start_pos; ++i) {
      res.buffer[offset++] = str.buffer[i];
    }

    auto const end_pos = sv.find(end_seq, start_pos + start_seq.size());
    if (end_pos == std::string_view::npos) {
      throw std::invalid_argument("remove_range_comments: unmatched end_seq");
    }
    cursor = end_pos + end_seq.size();
  }

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval remove_range_comments(char const (&str)[N],
                                     std::string_view const start_seq,
                                     std::string_view const end_seq) noexcept {
  return remove_range_comments(FrozenString{str}, start_seq, end_seq);
}

/**
 * @brief すべての行を結合する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param sep 行の間に入れる文字列 (デフォルト: "")
 * @return auto 結合結果
 */
template <size_t N>
auto consteval join_lines(FrozenString<N> const& str, std::string_view sep = "") noexcept {
  constexpr auto MAX_SEP_LEN = 32uz;
  constexpr auto OUT_CAP = N + (N * MAX_SEP_LEN);
  auto res = FrozenString<OUT_CAP>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto first_line = true;

  while (i < str.length) {
    if (!first_line) {
      for (auto const c : sep) {
        res.buffer[offset++] = c;
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
auto consteval join_lines(char const (&str)[N], std::string_view sep = "") noexcept {
  return join_lines(FrozenString{str}, sep);
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
