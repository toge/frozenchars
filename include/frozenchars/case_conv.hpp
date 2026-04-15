#pragma once

#include "frozen_string.hpp"
#include "detail/string_utils.hpp"
#include <cstddef>

namespace frozenchars {

/**
 * @brief 文字列の先頭を大文字に残りを小文字に変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval capitalize(FrozenString<N> const& str) noexcept {
  auto res = tolower(str);
  if (res.length > 0) {
    auto const c = res.buffer[0];
    if (c >= 'a' && c <= 'z') {
      res.buffer[0] = static_cast<char>(c - ('a' - 'A'));
    }
  }
  return res;
}

/**
 * @brief 文字列リテラルの先頭を大文字に残りを小文字に変換する
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval capitalize(char const (&str)[N]) noexcept {
  return capitalize(FrozenString{str});
}

/**
 * @brief camelCase/PascalCase文字列をsnake_caseに変換する
 * 大文字の前にアンダースコアを挿入し、すべての文字を小文字に変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval to_snake_case(FrozenString<N> const& str) noexcept {
  constexpr auto OUT_CAP = 2 * (N > 0 ? N - 1 : 0) + 1;
  auto res = FrozenString<OUT_CAP>{};
  auto offset = 0uz;
  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    if (c >= 'A' && c <= 'Z' && i > 0) {
      res.buffer[offset++] = '_';
    }
    res.buffer[offset++] = (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief camelCase/PascalCase文字列リテラルをsnake_caseに変換する
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval to_snake_case(char const (&str)[N]) noexcept {
  return to_snake_case(FrozenString{str});
}

/**
 * @brief camelCase/PascalCase文字列をsnake_caseに変換する（NTTP版・正確なバッファサイズ）
 * 入力文字列をNTTPとして受け取り、必要なアンダースコア数を事前計算して
 * ぴったりのサイズの FrozenString を返す。
 *
 * @tparam Str 変換対象の FrozenString（NTTPとして渡す）
 * @return auto 変換文字列
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
auto consteval to_snake_case() noexcept {
  constexpr auto EXTRA = detail::count_snake_underscores(Str);
  constexpr auto OUT_CAP = Str.length + EXTRA + 1;
  auto res = FrozenString<OUT_CAP>{};
  auto offset = 0uz;
  for (auto i = 0uz; i < Str.length; ++i) {
    auto const c = Str.buffer[i];
    if (c >= 'A' && c <= 'Z' && i > 0) {
      res.buffer[offset++] = '_';
    }
    res.buffer[offset++] = (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief snake_case文字列をcamelCaseに変換する
 * アンダースコアを除去し、アンダースコアに続く文字を大文字に変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval to_camel_case(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto next_upper = false;
  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    if (c == '_') {
      next_upper = true;
    } else if (next_upper) {
      res.buffer[offset++] = (c >= 'a' && c <= 'z') ? static_cast<char>(c - ('a' - 'A')) : c;
      next_upper = false;
    } else {
      res.buffer[offset++] = c;
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief snake_case文字列リテラルをcamelCaseに変換する
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval to_camel_case(char const (&str)[N]) noexcept {
  return to_camel_case(FrozenString{str});
}

/**
 * @brief snake_case文字列をPascalCaseに変換する
 * アンダースコアを除去し、アンダースコアに続く文字および先頭の文字を大文字に変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval to_pascal_case(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto next_upper = true;
  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    if (c == '_') {
      next_upper = true;
    } else if (next_upper) {
      res.buffer[offset++] = (c >= 'a' && c <= 'z') ? static_cast<char>(c - ('a' - 'A')) : c;
      next_upper = false;
    } else {
      res.buffer[offset++] = c;
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief snake_case文字列リテラルをPascalCaseに変換する
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 変換文字列
 */
template <size_t N>
auto consteval to_pascal_case(char const (&str)[N]) noexcept {
  return to_pascal_case(FrozenString{str});
}

} // namespace frozenchars
