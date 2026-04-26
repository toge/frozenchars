#pragma once

#include "freeze.hpp"
#include "detail/string_utils.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <string_view>

namespace frozenchars {

/**
 * @brief FrozenString の先頭から最初の終端文字までを含む最小サイズへ縮小する
 *
 * @tparam Str 処理対象の FrozenString 値
 * @return auto 縮小後の FrozenString
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
auto consteval shrink_to_fit() noexcept {
  auto constexpr fit_len = [] {
    for (auto i = 0uz; i < Str.buffer.size(); ++i) {
      if (Str.buffer[i] == '\0') {
        return i;
      }
    }
    return Str.length;
  }();

  auto result = FrozenString<fit_len + 1>{};
  for (auto i = 0uz; i < fit_len; ++i) {
    result.buffer[i] = Str.buffer[i];
  }
  result.buffer[fit_len] = '\0';
  result.length = fit_len;
  return result;
}

/**
 * @brief 文字列を指定幅で左端を埋めた文字列を生成する
 *
 * @tparam Width 埋めた後の幅
 * @tparam Fill 埋める文字
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <size_t Width, char Fill, size_t N>
auto consteval pad_left(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N, Width + 1)> {
  constexpr auto NEW_SIZE = std::max(N, Width + 1);
  auto res = FrozenString<NEW_SIZE>{};

  if (str.length >= Width) {
    for (auto i = 0uz; i < str.length; ++i) {
      res.buffer[i] = str.buffer[i];
    }
    res.buffer[str.length] = '\0';
    res.length = str.length;
    return res;
  }

  auto const fill_count = Width - str.length;
  auto offset = 0uz;
  for (auto i = 0uz; i < fill_count; ++i) {
    res.buffer[offset++] = Fill;
  }
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[offset++] = str.buffer[i];
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t Width, char Fill, size_t N>
auto consteval pad_left(char const (&str)[N]) noexcept {
  return pad_left<Width, Fill>(FrozenString{str});
}

/**
 * @brief 文字列を指定幅で右端を埋めた文字列を生成する
 *
 * @tparam Width 埋めた後の幅
 * @tparam Fill 埋める文字
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <size_t Width, char Fill, size_t N>
auto consteval pad_right(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N, Width + 1)> {
  constexpr auto NEW_SIZE = std::max(N, Width + 1);
  auto res = FrozenString<NEW_SIZE>{};

  if (str.length >= Width) {
    for (auto i = 0uz; i < str.length; ++i) {
      res.buffer[i] = str.buffer[i];
    }
    res.buffer[str.length] = '\0';
    res.length = str.length;
    return res;
  }

  auto offset = 0uz;
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[offset++] = str.buffer[i];
  }
  auto const fill_count = Width - str.length;
  for (auto i = 0uz; i < fill_count; ++i) {
    res.buffer[offset++] = Fill;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t Width, char Fill, size_t N>
auto consteval pad_right(char const (&str)[N]) noexcept {
  return pad_right<Width, Fill>(FrozenString{str});
}

/**
 * @brief 指定回数繰り返した文字列を生成する
 *
 * @tparam Count 繰り返し回数
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 繰り返す文字列
 * @return auto 生成した文字列
 */
template <size_t Count, size_t N>
auto consteval repeat(FrozenString<N> const& str) noexcept {
  auto constexpr UNIT_LEN = N > 0 ? N - 1 : 0;
  auto constexpr NEW_SIZE = UNIT_LEN * Count + 1;

  auto res = FrozenString<NEW_SIZE>{};
  auto offset = 0uz;
  auto const src = str.sv();

  for (auto i = 0uz; i < Count; ++i) {
    for (auto const c : src) {
      res.buffer[offset++] = c;
    }
  }
  if constexpr (NEW_SIZE > 0) {
    res.buffer[NEW_SIZE - 1] = '\0';
  }
  res.length = offset;
  return res;
}

/**
 * @brief 文字列リテラルを繰り返した文字列を生成する
 *
 * @tparam Count 繰り返し回数
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 繰り返す文字列リテラル
 * @return auto 生成した文字列
 */
template <size_t Count, size_t N>
auto consteval repeat(char const (&str)[N]) noexcept {
  return repeat<Count>(FrozenString{str});
}

/**
 * @brief 文字列を指定幅で右寄せした文字列を生成する
 *
 * @tparam Width 右寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval right(FrozenString<N> const& str) noexcept {
  return pad_left<Width, Fill>(str);
}

/**
 * @brief 文字列リテラルを指定幅で右寄せした文字列を生成する
 *
 * @tparam Width 右寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval right(char const (&str)[N]) noexcept {
  return right<Width, Fill>(FrozenString{str});
}

/**
 * @brief 文字列を指定幅で左寄せした文字列を生成する
 *
 * @tparam Width 左寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval left(FrozenString<N> const& str) noexcept {
  return pad_right<Width, Fill>(str);
}

/**
 * @brief 文字列リテラルを指定幅で左寄せした文字列を生成する
 *
 * @tparam Width 左寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval left(char const (&str)[N]) noexcept {
  return left<Width, Fill>(FrozenString{str});
}

/**
 * @brief 文字列を指定幅で中央寄せした文字列を生成する
 *
 * @tparam Width 中央寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval center(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N, Width + 1)> {
  constexpr auto NEW_SIZE = std::max(N, Width + 1);
  if (str.length >= Width) {
    auto res = FrozenString<NEW_SIZE>{};
    for (auto i = 0uz; i < str.length; ++i) {
      res.buffer[i] = str.buffer[i];
    }
    res.buffer[str.length] = '\0';
    res.length = str.length;
    return res;
  }
  auto const left_fill = (Width - str.length) / 2;
  auto const right_fill = Width - str.length - left_fill;

  auto res = FrozenString<NEW_SIZE>{};
  auto offset = 0uz;
  for (auto i = 0uz; i < left_fill; ++i) {
    res.buffer[offset++] = Fill;
  }
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[offset++] = str.buffer[i];
  }
  for (auto i = 0uz; i < right_fill; ++i) {
    res.buffer[offset++] = Fill;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列リテラルを指定幅で中央寄せした文字列を生成する
 *
 * @tparam Width 中央寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval center(char const (&str)[N]) noexcept {
  return center<Width, Fill>(FrozenString{str});
}

/**
 * @brief 文字列をすべて大文字に変換した文字列を生成する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <size_t N>
auto consteval toupper(FrozenString<N> const& str) noexcept {
  auto res = str;
  for (auto i = 0uz; i < res.length; ++i) {
    auto const c = res.buffer[i];
    if (c >= 'a' && c <= 'z') {
      res.buffer[i] = static_cast<char>(c - ('a' - 'A'));
    }
  }
  return res;
}

/**
 * @brief 文字列リテラルをすべて大文字に変換した文字列を生成する
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <size_t N>
auto consteval toupper(char const (&str)[N]) noexcept {
  return toupper(FrozenString{str});
}

/**
 * @brief 文字列をすべて小文字に変換した文字列を生成する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <size_t N>
auto consteval tolower(FrozenString<N> const& str) noexcept {
  auto res = str;
  for (auto i = 0uz; i < res.length; ++i) {
    auto const c = res.buffer[i];
    if (c >= 'A' && c <= 'Z') {
      res.buffer[i] = static_cast<char>(c + ('a' - 'A'));
    }
  }
  return res;
}

/**
 * @brief 文字列リテラルをすべて小文字に変換した文字列を生成する
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <size_t N>
auto consteval tolower(char const (&str)[N]) noexcept {
  return tolower(FrozenString{str});
}

namespace detail {
  template <size_t N, size_t M>
  consteval size_t find_impl(FrozenString<N> const& haystack, FrozenString<M> const& needle, size_t pos = 0) noexcept {
    if (M == 0 || M > N) {
      return std::string_view::npos;
    }
    if (M == 1) { // Special case for char
      for (auto i = pos; i < haystack.length; ++i) {
        if (haystack.buffer[i] == needle.buffer[0]) {
          return i;
        }
      }
      return std::string_view::npos;
    }
    auto const needle_len = needle.length;
    if (needle_len == 0) {
      return pos;
    }
    for (auto i = pos; i <= haystack.length - needle_len; ++i) {
      bool match = true;
      for (auto j = 0uz; j < needle_len; ++j) {
        if (haystack.buffer[i + j] != needle.buffer[j]) {
          match = false;
          break;
        }
      }
      if (match) {
        return i;
      }
    }
    return std::string_view::npos;
  }
}

/**
 * @brief 文字列の指定した範囲を置換した文字列を生成する
 *
 * @tparam From 置換前の文字列
 * @tparam To 置換後の文字列
 * @tparam N 処理対象の文字列の長さ (終端文字'\0'を含む)
 * @param str 処理対象の文字列
 * @return auto 生成した文字列
 */
template <FrozenString From, FrozenString To, size_t N>
consteval auto replace(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N, N + To.size() + 1)> {
  constexpr auto NEW_SIZE = std::max(N, N + To.size() + 1);
  auto res = FrozenString<NEW_SIZE>{};

  auto const pos = detail::find_impl(str, From);
  if (pos == std::string_view::npos) {
    for (auto i = 0uz; i < str.length; ++i) {
      res.buffer[i] = str.buffer[i];
    }
    res.buffer[str.length] = '\0';
    res.length = str.length;
    return res;
  }

  auto offset = 0uz;
  for (auto i = 0uz; i < pos; ++i) {
    res.buffer[offset++] = str.buffer[i];
  }
  for (auto i = 0uz; i < To.length; ++i) {
    res.buffer[offset++] = To.buffer[i];
  }
  for (auto i = pos + From.length; i < str.length; ++i) {
    res.buffer[offset++] = str.buffer[i];
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <FrozenString From, FrozenString To, size_t N>
auto consteval replace(char const (&str)[N]) noexcept {
  return replace<From, To>(FrozenString{str});
}

/**
 * @brief 文字列の部分文字列を生成する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param pos 開始位置
 * @param len 文字数。負の場合は pos の左側から abs(len) 文字
 * @return auto 生成した文字列
 */
template <size_t N>
auto consteval substr(FrozenString<N> const& str, std::size_t pos, std::ptrdiff_t len) noexcept {
  auto res = FrozenString<N>{};
  auto const requested_len = len >= 0
    ? static_cast<size_t>(len)
    : static_cast<size_t>(-len);
  auto const anchor = std::min(pos, str.length);
  auto start = anchor;
  auto actual_len = 0uz;

  if (len >= 0) {
    actual_len = anchor < str.length ? std::min(requested_len, str.length - anchor) : 0uz;
  } else {
    actual_len = std::min(requested_len, anchor);
    start = anchor - actual_len;
  }

  for (auto i = 0uz; i < actual_len; ++i) {
    res.buffer[i] = str.buffer[start + i];
  }
  res.buffer[actual_len] = '\0';
  res.length = actual_len;
  return res;
}

/**
 * @brief 文字列の部分文字列を生成する（NTTP版・正確なサイズ）
 *
 * @tparam Str 対象文字列（FrozenString NTTP）
 * @tparam Pos 開始位置
 * @tparam Len 文字数
 * @return auto 縮小された FrozenString
 */
template <auto Str, size_t Pos, std::ptrdiff_t Len>
  requires detail::is_frozen_string_v<decltype(Str)>
auto consteval substr() noexcept {
  return shrink_to_fit<substr(Str, Pos, Len)>();
}

/**
 * @brief 文字列の部分文字列を生成する（NTTP引数版）
 */
template <size_t Pos, std::ptrdiff_t Len, size_t N>
auto consteval substr(FrozenString<N> const& str) noexcept {
  return substr(str, Pos, Len);
}

/**
 * @brief 文字列リテラルの部分文字列を生成する
 *
 * @tparam Pos 開始位置
 * @tparam Len 文字数。負の場合は Pos の左側から abs(Len) 文字
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <size_t Pos, std::ptrdiff_t Len, size_t N>
auto consteval substr(char const (&str)[N]) noexcept {
  return substr(FrozenString{str}, Pos, Len);
}

template <size_t N>
auto consteval substr(char const (&str)[N], size_t pos, std::ptrdiff_t len) noexcept {
  return substr(FrozenString{str}, pos, len);
}

/**
 * @brief 文字列内のすべての指定した部分文字列を置換した文字列を生成する
 *
 * @tparam From 置換前の文字列
 * @tparam To 置換後の文字列
 * @tparam N 処理対象の文字列の長さ (終端文字'\0'を含む)
 * @param str 処理対象の文字列
 * @return auto 生成した文字列
 */
template <FrozenString From, FrozenString To, size_t N>
consteval auto replace_all(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N * 4, 2048uz)> {
  constexpr auto MAX_REPLACE_SIZE = std::max(N * 4, 2048uz);
  auto res = FrozenString<MAX_REPLACE_SIZE>{};

  auto offset = 0uz;
  auto pos = 0uz;
  while (pos < str.length) {
    auto const found = detail::find_impl(str, From, pos);
    if (found == std::string_view::npos) {
      while (pos < str.length) {
        res.buffer[offset++] = str.buffer[pos++];
      }
      break;
    }
    while (pos < found) {
      res.buffer[offset++] = str.buffer[pos++];
    }
    for (auto i = 0uz; i < To.length; ++i) {
      res.buffer[offset++] = To.buffer[i];
    }
    pos = found + From.length;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <FrozenString From, FrozenString To, size_t N>
auto consteval replace_all(char const (&str)[N]) noexcept {
  return replace_all<From, To>(FrozenString{str});
}

template <size_t Width, char Fill = ' ', typename T>
  requires (!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
auto consteval pad_left(T const& v) noexcept {
  return pad_left<Width, Fill>(freeze(v));
}

template <size_t Width, char Fill = ' ', typename T>
  requires (!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
auto consteval pad_right(T const& v) noexcept {
  return pad_right<Width, Fill>(freeze(v));
}

template <FrozenString Delim, size_t ElemN, size_t Count>
auto consteval join(std::array<FrozenString<ElemN>, Count> const& arr) noexcept {
  constexpr auto NEW_SIZE = (ElemN * Count) + (Delim.size() * Count) + 1;
  auto res = FrozenString<NEW_SIZE>{};
  auto offset = 0uz;
  for (auto i = 0uz; i < Count; ++i) {
    if (i > 0) {
      for (auto const c : Delim.sv()) {
        res.buffer[offset++] = c;
      }
    }
    for (auto const c : arr[i].sv()) {
      res.buffer[offset++] = c;
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <FrozenString Delim, typename... Args>
  requires (sizeof...(Args) > 0)
auto consteval join(Args const&... args) noexcept {
  auto const arr = std::array<FrozenString<2048>, sizeof...(Args)>{freeze(args)...};
  return join<Delim>(arr);
}

template <size_t Width, char Fill = '0', Integral T>
auto consteval pad_left(T const& v) noexcept {
  return pad_left<Width, Fill>(freeze(v));
}

template <size_t Width, char Fill = '0', Integral T>
auto consteval pad_right(T const& v) noexcept {
  return pad_right<Width, Fill>(freeze(v));
}

/**
 * @brief 引数で渡された値を結合する
 *
 * @tparam Args 可変引数の型
 * @param args 結合する引数
 * @return auto 変換後の文字列
 */
template <typename... Args>
auto consteval concat(Args const&... args) noexcept {
  return (freeze(args) + ...);
}

/**
 * @brief 文字列の左端から指定した条件を満たす文字を削除した文字列を生成する
 *
 * @tparam Pred 削除対象を判定する述語
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <auto Pred, size_t N>
auto consteval ltrim_if(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<true, false, Pred>(str);
}

/**
 * @brief 文字列の右端から指定した条件を満たす文字を削除した文字列を生成する
 *
 * @tparam Pred 削除対象を判定する述語
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <auto Pred, size_t N>
auto consteval rtrim_if(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<false, true, Pred>(str);
}

/**
 * @brief 文字列の左端と右端から指定した条件を満たす文字を削除した文字列を生成する
 *
 * @tparam Pred 削除対象を判定する述語
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <auto Pred, size_t N>
auto consteval trim_if(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<true, true, Pred>(str);
}

/**
 * @brief 文字列の左端から特定の文字を削除した文字列を生成する
 *
 * @tparam TrimChar 削除する文字（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <char TrimChar = ' ', size_t N>
auto consteval ltrim(FrozenString<N> const& str) noexcept {
  return ltrim_if<detail::is_char<TrimChar>>(str);
}

/**
 * @brief 文字列の右端から特定の文字を削除した文字列を生成する
 *
 * @tparam TrimChar 削除する文字（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <char TrimChar = ' ', size_t N>
auto consteval rtrim(FrozenString<N> const& str) noexcept {
  return rtrim_if<detail::is_char<TrimChar>>(str);
}

/**
 * @brief 文字列の左端と右端から特定の文字を削除した文字列を生成する
 *
 * @tparam TrimChar 削除する文字（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <char TrimChar = ' ', size_t N>
auto consteval trim(FrozenString<N> const& str) noexcept {
  return trim_if<detail::is_char<TrimChar>>(str);
}

/**
 * @brief 文字列リテラルの左端から特定の文字を削除した文字列を生成する
 *
 * @tparam TrimChar 削除する文字（デフォルト: 半角スペース）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <char TrimChar = ' ', size_t N>
auto consteval ltrim(char const (&str)[N]) noexcept {
  return ltrim<TrimChar>(FrozenString{str});
}

/**
 * @brief 文字列リテラルの右端から特定の文字を削除した文字列を生成する
 *
 * @tparam TrimChar 削除する文字（デフォルト: 半角スペース）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <char TrimChar = ' ', size_t N>
auto consteval rtrim(char const (&str)[N]) noexcept {
  return rtrim<TrimChar>(FrozenString{str});
}

/**
 * @brief 文字列リテラルの左端と右端から特定の文字を削除した文字列を生成する
 *
 * @tparam TrimChar 削除する文字（デフォルト: 半角スペース）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <char TrimChar = ' ', size_t N>
auto consteval trim(char const (&str)[N]) noexcept {
  return trim<TrimChar>(FrozenString{str});
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
auto consteval ltrim(Ptr&& str) noexcept {
  return ltrim<TrimChar>(freeze(str));
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
auto consteval rtrim(Ptr&& str) noexcept {
  return rtrim<TrimChar>(freeze(str));
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
auto consteval trim(Ptr&& str) noexcept {
  return trim<TrimChar>(freeze(str));
}

/**
 * @brief 連続した条件を満たす文字を1つの文字に変換する
 *
 * @tparam Pred 変換対象を判定する述語
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換後の文字列
 */
template <auto Pred, size_t N>
auto consteval collapse_spaces_if(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto in_sequence = false;
  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    if (Pred(c)) {
      if (!in_sequence) {
        res.buffer[offset++] = c;
        in_sequence = true;
      }
    } else {
      res.buffer[offset++] = c;
      in_sequence = false;
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 連続した半角スペースを1つの半角スペースに変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換後の文字列
 */
template <size_t N>
auto consteval collapse_spaces(FrozenString<N> const& str) noexcept {
  return collapse_spaces_if<detail::is_space_char>(str);
}

/**
 * @brief 文字列リテラルの連続した半角スペースを1つの半角スペースに変換する
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 変換後の文字列
 */
template <size_t N>
auto consteval collapse_spaces(char const (&str)[N]) noexcept {
  return collapse_spaces(FrozenString{str});
}

} // namespace frozenchars
