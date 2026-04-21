#pragma once

#include "frozen_string.hpp"
#include "detail/split_impl.hpp"
#include <array>
#include <concepts>
#include <cstddef>
#include <stdexcept>

namespace frozenchars {

/**
 * @brief 文字列を区切り判定関数で分割したときのトークン数を返す
 */
template <auto IsDelimiter = detail::is_any_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split_count(FrozenString<N> const& str) noexcept {
  return detail::split_count_impl<IsDelimiter>(str);
}

/**
 * @brief 文字列を区切り判定関数で分割したときのトークン数を返す（実行時引数版）
 */
template <size_t N, typename Pred>
auto consteval split_count(FrozenString<N> const& str, Pred is_delimiter) noexcept {
  return detail::split_count_impl(str, is_delimiter);
}

/**
 * @brief 文字列リテラルを区切り判定関数で分割したときのトークン数を返す
 */
template <auto IsDelimiter = detail::is_any_whitespace, size_t N>
auto consteval split_count(char const (&str)[N]) noexcept {
  return split_count<IsDelimiter>(FrozenString{str});
}

/**
 * @brief 文字列リテラルを区切り判定関数で分割したときのトークン数を返す（実行時引数版）
 */
template <size_t N, typename Pred>
auto consteval split_count(char const (&str)[N], Pred is_delimiter) noexcept {
  return split_count(FrozenString{str}, is_delimiter);
}

/**
 * @brief 文字列を区切り判定関数で分割して std::array に変換する
 */
template <size_t Count, auto IsDelimiter = detail::is_any_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split(FrozenString<N> const& str) noexcept {
  auto res = std::array<FrozenString<N>, Count>{};
  for (auto& token : res) token.length = 0;

  auto const token_limit = std::min(Count, split_count<IsDelimiter>(str));
  auto src = 0uz;
  auto dst = 0uz;

  while (src < str.length && dst < token_limit) {
    while (src < str.length && IsDelimiter(str.buffer[src])) ++src;
    if (src >= str.length) break;

    auto token_len = 0uz;
    while (src < str.length && !IsDelimiter(str.buffer[src])) {
      res[dst].buffer[token_len++] = str.buffer[src++];
    }
    res[dst].buffer[token_len] = '\0';
    res[dst].length = token_len;
    ++dst;
  }
  return res;
}

/**
 * @brief 文字列を区切り判定関数で分割して std::array に変換する（実行時引数版）
 */
template <size_t Count, size_t N, typename Pred>
auto consteval split(FrozenString<N> const& str, Pred is_delimiter) noexcept {
  auto res = std::array<FrozenString<N>, Count>{};
  for (auto& token : res) token.length = 0;

  auto const token_limit = std::min(Count, split_count(str, is_delimiter));
  auto src = 0uz;
  auto dst = 0uz;

  while (src < str.length && dst < token_limit) {
    while (src < str.length && is_delimiter(str.buffer[src])) ++src;
    if (src >= str.length) break;

    auto token_len = 0uz;
    while (src < str.length && !is_delimiter(str.buffer[src])) {
      res[dst].buffer[token_len++] = str.buffer[src++];
    }
    res[dst].buffer[token_len] = '\0';
    res[dst].length = token_len;
    ++dst;
  }
  return res;
}

/**
 * @brief 文字列リテラルを区切り判定関数で分割する
 */
template <size_t Count, auto IsDelimiter = detail::is_any_whitespace, size_t N>
auto consteval split(char const (&str)[N]) noexcept {
  return split<Count, IsDelimiter>(FrozenString{str});
}

/**
 * @brief 文字列をNTTPとして受け取り、正確なサイズの配列に分割する
 */
template <auto Str, auto IsDelimiter = detail::is_any_whitespace>
  requires (detail::is_frozen_string_v<decltype(Str)>
            && std::predicate<decltype(IsDelimiter), char>)
auto consteval split() noexcept {
  constexpr auto token_count = detail::split_count_impl<IsDelimiter>(Str);
  constexpr auto max_len     = detail::max_token_len_impl<IsDelimiter>(Str);
  auto res = std::array<FrozenString<max_len + 1>, token_count>{};
  auto src = 0uz;
  auto dst = 0uz;
  while (src < Str.length && dst < token_count) {
    while (src < Str.length && IsDelimiter(Str.buffer[src])) ++src;
    if (src >= Str.length) break;
    auto token_len = 0uz;
    while (src < Str.length && !IsDelimiter(Str.buffer[src])) {
      res[dst].buffer[token_len++] = Str.buffer[src++];
    }
    res[dst].buffer[token_len] = '\0';
    res[dst].length = token_len;
    ++dst;
  }
  return res;
}

// Simple integer parser to satisfy split_numbers
namespace detail {
  template <typename T, size_t N>
  consteval T parse_int_simple(FrozenString<N> const& str) {
    T res = 0;
    bool neg = false;
    size_t i = 0;
    if (str.length > 0 && str.buffer[0] == '-') {
      neg = true;
      i = 1;
    } else if (str.length > 0 && str.buffer[0] == '+') {
      i = 1;
    }
    for (; i < str.length; ++i) {
      if (str.buffer[i] < '0' || str.buffer[i] > '9') break;
      res = res * 10 + (str.buffer[i] - '0');
    }
    return neg ? -res : res;
  }
}

/**
 * @brief 文字列を区切り判定関数で分割し数値配列へ変換する
 */
template <auto IsDelimiter = detail::is_any_whitespace, Numeric Int = int, size_t N>
auto consteval split_numbers(FrozenString<N> const& str) {
  using Result = std::remove_cv_t<Int>;
  auto res = std::array<Result, N>{};
  auto const token_count = split_count<IsDelimiter>(str);
  auto const tokens = split<N, IsDelimiter>(str);
  for (auto i = 0uz; i < token_count; ++i) {
    res[i] = detail::parse_int_simple<Result>(tokens[i]);
  }
  return res;
}

/**
 * @brief 文字列を区切り判定関数で分割し数値配列へ変換する（実行時引数版）
 */
template <typename Pred, Numeric Int = int, size_t N>
auto consteval split_numbers(FrozenString<N> const& str, Pred is_delimiter) {
  using Result = std::remove_cv_t<Int>;
  auto res = std::array<Result, N>{};
  auto const token_count = split_count(str, is_delimiter);
  auto const tokens = split<N>(str, is_delimiter);
  for (auto i = 0uz; i < token_count; ++i) {
    res[i] = detail::parse_int_simple<Result>(tokens[i]);
  }
  return res;
}

/**
 * @brief 文字列を空白区切りで分割し指定数値型配列へ変換する
 */
template <Numeric Int, size_t N>
auto consteval split_numbers(FrozenString<N> const& str) {
  return split_numbers<detail::is_any_whitespace, Int>(str);
}

/**
 * @brief 文字列リテラルを区切り判定関数で分割し数値配列へ変換する
 */
template <auto IsDelimiter = detail::is_any_whitespace, Numeric Int = int, size_t N>
auto consteval split_numbers(char const (&str)[N]) noexcept {
  return split_numbers<IsDelimiter, Int>(FrozenString{str});
}

/**
 * @brief 文字列リテラルを空白区切りで分割し指定数値型配列へ変換する
 */
template <Numeric Int, size_t N>
auto consteval split_numbers(char const (&str)[N]) noexcept {
  return split_numbers<detail::is_any_whitespace, Int>(FrozenString{str});
}

} // namespace frozenchars
