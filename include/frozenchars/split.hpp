#pragma once

#include "frozen_string.hpp"
#include "detail/split_impl.hpp"
#include <array>
#include <concepts>
#include <cstddef>

namespace frozenchars {

/**
 * @brief 文字列を区切り判定関数で分割したときのトークン数を返す
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto トークン数
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split_count(FrozenString<N> const& str) noexcept {
  return detail::split_count_impl<IsDelimiter>(str);
}

/**
 * @brief 文字列リテラルを区切り判定関数で分割したときのトークン数を返す
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto トークン数
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split_count(char const (&str)[N]) noexcept {
  return split_count<IsDelimiter>(FrozenString{str});
}

/**
 * @brief 文字列を区切り判定関数で分割して std::array に変換する
 * `Count` より多いトークンは切り捨て、足りない要素は空文字列のまま残る
 *
 * @tparam Count 返却する配列の要素数
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 分割結果の配列
 */
template <size_t Count, auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split(FrozenString<N> const& str) noexcept {
  auto res = std::array<FrozenString<N>, Count>{};
  for (auto& token : res) {
    token.length = 0;
  }
  auto const token_count = split_count<IsDelimiter>(str);
  auto const token_limit = std::min(Count, token_count);
  auto src = 0uz;
  auto dst = 0uz;

  while (src < str.length && dst < token_limit) {
    while (src < str.length && IsDelimiter(str.buffer[src])) {
      ++src;
    }
    if (src >= str.length) {
      break;
    }

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
 * @brief 文字列を区切り判定関数で分割し、1回の呼び出しで結果を返す
 * split_count(...) を内部で呼び出して、必要分までトークンを格納する
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 分割結果の配列（未使用要素は空文字）
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split(FrozenString<N> const& str) noexcept {
  auto res = std::array<FrozenString<N>, N>{};
  for (auto& token : res) {
    token.length = 0;
  }

  // 事前にsplit結果のトークン数を数えておく
  auto const token_count = split_count<IsDelimiter>(str);
  auto src = 0uz;
  auto dst = 0uz;

  while (src < str.length && dst < token_count) {
    while (src < str.length && IsDelimiter(str.buffer[src])) {
      ++src;
    }
    if (src >= str.length) {
      break;
    }

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
 * @brief 文字列リテラルを区切り判定関数で分割する
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 分割結果の配列（未使用要素は空文字）
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split(char const (&str)[N]) noexcept {
  return split<IsDelimiter>(FrozenString{str});
}

/**
 * @brief 文字列をNTTPとして受け取り、正確なサイズの配列に分割する
 * トークン数・最大トークン長をコンパイル時に確定し、無駄のない型を返す
 *
 * @tparam Str 分割対象の FrozenString（NTTPとして渡す）
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @return auto std::array<FrozenString<MaxTokenLen+1>, TokenCount>
 */
template <auto Str, auto IsDelimiter = detail::is_whitespace>
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

/**
 * @brief 文字列を区切り判定関数で分割し数値配列へ変換する
 *
 * @tparam Int 解析する数値型（デフォルト: int）
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval 分割・数値変換結果の配列（未使用要素は0）
 */
template <auto IsDelimiter = detail::is_whitespace, Numeric Int = int, size_t N>
  requires (std::predicate<decltype(IsDelimiter), char>
            && !std::same_as<std::remove_cv_t<Int>, bool>)
auto consteval split_numbers(FrozenString<N> const& str) {
  using Result = std::remove_cv_t<Int>;
  auto res = std::array<Result, N>{};
  auto const tokens = split<IsDelimiter>(str);
  auto const token_count = split_count<IsDelimiter>(str);
  for (auto i = 0uz; i < token_count; ++i) {
    if (tokens[i].length == 0) {
      continue;
    }
    res[i] = detail::parse_number_token<Result, detail::integer_base_mode::decimal_only>(tokens[i]);
  }
  return res;
}

/**
 * @brief 文字列を空白区切りで分割し指定数値型配列へ変換する
 *
 * @tparam Int 解析する数値型
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 分割・数値変換結果の配列（未使用要素は0）
 */
template <Numeric Int, size_t N>
  requires (!std::same_as<std::remove_cv_t<Int>, bool>)
auto consteval split_numbers(FrozenString<N> const& str) {
  return split_numbers<detail::is_whitespace, Int>(str);
}

/**
 * @brief 文字列リテラルを区切り判定関数で分割し数値配列へ変換する
 *
 * @tparam Int 解析する数値型（デフォルト: int）
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto consteval 分割・数値変換結果の配列（未使用要素は0）
 */
template <auto IsDelimiter = detail::is_whitespace, Numeric Int = int, size_t N>
  requires (std::predicate<decltype(IsDelimiter), char>
            && !std::same_as<std::remove_cv_t<Int>, bool>)
auto consteval split_numbers(char const (&str)[N]) noexcept {
  return split_numbers<IsDelimiter, Int>(FrozenString{str});
}

/**
 * @brief 文字列リテラルを空白区切りで分割し指定数値型配列へ変換する
 *
 * @tparam Int 解析する数値型
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 分割・数値変換結果の配列（未使用要素は0）
 */
template <Numeric Int, size_t N>
  requires (!std::same_as<std::remove_cv_t<Int>, bool>)
auto consteval split_numbers(char const (&str)[N]) noexcept {
  return split_numbers<detail::is_whitespace, Int>(FrozenString{str});
}

template <ParseNumberTarget Number, size_t N>
constexpr auto parse_number(FrozenString<N> const& str) {
  return detail::parse_number_token<std::remove_cv_t<Number>, detail::integer_base_mode::autodetect>(str);
}

template <ParseNumberTarget Number, size_t N>
consteval auto parse_number(char const (&str)[N]) noexcept {
  return parse_number<std::remove_cv_t<Number>>(FrozenString{str});
}

} // namespace frozenchars
