#pragma once

#include "../frozen_string.hpp"
#include <algorithm>
#include <concepts>
#include <cstddef>

namespace frozenchars::detail {

/**
 * @brief 文字列を区切り判定関数で分割したときのトークン数を数える
 *
 * @tparam IsDelimiter 区切り文字判定関数
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return size_t トークン数
 */
template <auto IsDelimiter, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split_count_impl(FrozenString<N> const& str) noexcept -> size_t {
  auto count = 0uz;
  auto in_token = false;
  for (auto i = 0uz; i < str.length; ++i) {
    if (!IsDelimiter(str.buffer[i])) {
      if (!in_token) {
        ++count;
        in_token = true;
      }
    } else {
      in_token = false;
    }
  }
  return count;
}

/**
 * @brief 文字列を区切り判定関数で分割したときのトークン数を数える（実行時引数版）
 */
template <size_t N, typename Pred>
auto consteval split_count_impl(FrozenString<N> const& str, Pred is_delimiter) noexcept -> size_t {
  auto count = 0uz;
  auto in_token = false;
  for (auto i = 0uz; i < str.length; ++i) {
    if (!is_delimiter(str.buffer[i])) {
      if (!in_token) {
        ++count;
        in_token = true;
      }
    } else {
      in_token = false;
    }
  }
  return count;
}

/**
 * @brief 分割後の最大トークン長を計算する
 *
 * @tparam IsDelimiter 区切り文字判定関数
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return size_t 最大トークン長
 */
template <auto IsDelimiter, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval max_token_len_impl(FrozenString<N> const& str) noexcept -> size_t {
  auto max_len = 0uz;
  auto cur_len = 0uz;
  for (auto i = 0uz; i < str.length; ++i) {
    if (!IsDelimiter(str.buffer[i])) {
      ++cur_len;
      max_len = std::max(max_len, cur_len);
    } else {
      cur_len = 0;
    }
  }
  return max_len;
}

} // namespace frozenchars::detail
