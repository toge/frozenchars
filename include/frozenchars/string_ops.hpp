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
[[nodiscard]] auto consteval shrink_to_fit() noexcept {
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
[[nodiscard]] auto consteval pad_left(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N, Width + 1)> {
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
[[nodiscard]] auto consteval pad_left(char const (&str)[N]) noexcept {
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
[[nodiscard]] auto consteval pad_right(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N, Width + 1)> {
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
[[nodiscard]] auto consteval pad_right(char const (&str)[N]) noexcept {
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
[[nodiscard]] auto consteval repeat(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval repeat(char const (&str)[N]) noexcept {
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
[[nodiscard]] auto consteval right(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval right(char const (&str)[N]) noexcept {
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
[[nodiscard]] auto consteval left(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval left(char const (&str)[N]) noexcept {
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
[[nodiscard]] auto consteval center(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N, Width + 1)> {
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
[[nodiscard]] auto consteval center(char const (&str)[N]) noexcept {
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
[[nodiscard]] auto consteval toupper(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval toupper(char const (&str)[N]) noexcept {
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
[[nodiscard]] auto consteval tolower(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval tolower(char const (&str)[N]) noexcept {
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

  template <auto Str, auto From>
    requires (is_frozen_string_v<decltype(Str)> && is_frozen_string_v<decltype(From)>)
  [[nodiscard]] consteval auto count_occurrences() noexcept -> std::size_t {
    auto count = 0uz;
    auto pos = 0uz;
    while (pos < Str.length) {
      auto const found = find_impl(Str, From, pos);
      if (found == std::string_view::npos) break;
      ++count;
      pos = found + From.length;
    }
    return count;
  }

  template <auto Str, auto From, auto To>
    requires (is_frozen_string_v<decltype(Str)> && is_frozen_string_v<decltype(From)> && is_frozen_string_v<decltype(To)>)
  [[nodiscard]] consteval auto replace_all_exact_size() noexcept -> std::size_t {
    constexpr auto occurrences = count_occurrences<Str, From>();
    if constexpr (occurrences == 0) {
      return Str.length + 1;
    } else {
      constexpr auto removed = occurrences * From.length;
      constexpr auto added = occurrences * To.length;
      return Str.length - removed + added + 1;
    }
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
[[nodiscard]] consteval auto replace(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N, N + To.size() + 1)> {
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
[[nodiscard]] auto consteval replace(char const (&str)[N]) noexcept {
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
[[nodiscard]] auto consteval substr(FrozenString<N> const& str, std::size_t pos, std::ptrdiff_t len) noexcept {
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
[[nodiscard]] auto consteval substr() noexcept {
  return shrink_to_fit<substr(Str, Pos, Len)>();
}

/**
 * @brief 文字列の部分文字列を生成する（NTTP引数版）
 */
template <size_t Pos, std::ptrdiff_t Len, size_t N>
[[nodiscard]] auto consteval substr(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval substr(char const (&str)[N]) noexcept {
  return substr(FrozenString{str}, Pos, Len);
}

template <size_t N>
[[nodiscard]] auto consteval substr(char const (&str)[N], size_t pos, std::ptrdiff_t len) noexcept {
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
[[nodiscard]] consteval auto replace_all(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N * 4, 2048uz)> {
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
[[nodiscard]] auto consteval replace_all(char const (&str)[N]) noexcept {
  return replace_all<From, To>(FrozenString{str});
}

/**
 * @brief 文字列内のすべての指定した部分文字列を置換した文字列を生成する（NTTP版・正確なサイズ）
 *
 * @tparam Str 処理対象の文字列（FrozenString NTTP）
 * @tparam From 置換前の文字列
 * @tparam To 置換後の文字列
 * @return auto 生成した文字列
 */
template <auto Str, auto From, auto To>
  requires (detail::is_frozen_string_v<decltype(Str)>
            && detail::is_frozen_string_v<decltype(From)>
            && detail::is_frozen_string_v<decltype(To)>)
[[nodiscard]] consteval auto replace_all() noexcept -> FrozenString<detail::replace_all_exact_size<Str, From, To>()> {
  constexpr auto NEW_SIZE = detail::replace_all_exact_size<Str, From, To>();
  auto res = FrozenString<NEW_SIZE>{};
  auto offset = 0uz;
  auto pos = 0uz;
  while (pos < Str.length) {
    auto const found = detail::find_impl(Str, From, pos);
    if (found == std::string_view::npos) {
      while (pos < Str.length) {
        res.buffer[offset++] = Str.buffer[pos++];
      }
      break;
    }
    while (pos < found) {
      res.buffer[offset++] = Str.buffer[pos++];
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

template <size_t Width, char Fill = ' ', typename T>
  requires (!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
[[nodiscard]] auto consteval pad_left(T const& v) noexcept {
  return pad_left<Width, Fill>(freeze(v));
}

template <size_t Width, char Fill = ' ', typename T>
  requires (!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
[[nodiscard]] auto consteval pad_right(T const& v) noexcept {
  return pad_right<Width, Fill>(freeze(v));
}

template <FrozenString Delim, size_t ElemN, size_t Count>
[[nodiscard]] auto consteval join(std::array<FrozenString<ElemN>, Count> const& arr) noexcept {
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
[[nodiscard]] auto consteval join(Args const&... args) noexcept {
  auto const arr = std::array<FrozenString<2048>, sizeof...(Args)>{freeze(args)...};
  return join<Delim>(arr);
}

template <size_t Width, char Fill = '0', Integral T>
[[nodiscard]] auto consteval pad_left(T const& v) noexcept {
  return pad_left<Width, Fill>(freeze(v));
}

template <size_t Width, char Fill = '0', Integral T>
[[nodiscard]] auto consteval pad_right(T const& v) noexcept {
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
[[nodiscard]] auto consteval concat(Args const&... args) noexcept {
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
[[nodiscard]] auto consteval ltrim_if(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval rtrim_if(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval trim_if(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval ltrim(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval rtrim(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval trim(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval ltrim(char const (&str)[N]) noexcept {
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
[[nodiscard]] auto consteval rtrim(char const (&str)[N]) noexcept {
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
[[nodiscard]] auto consteval trim(char const (&str)[N]) noexcept {
  return trim<TrimChar>(FrozenString{str});
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval ltrim(Ptr&& str) noexcept {
  return ltrim<TrimChar>(freeze(str));
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval rtrim(Ptr&& str) noexcept {
  return rtrim<TrimChar>(freeze(str));
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval trim(Ptr&& str) noexcept {
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
[[nodiscard]] auto consteval collapse_spaces_if(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval collapse_spaces(FrozenString<N> const& str) noexcept {
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
[[nodiscard]] auto consteval collapse_spaces(char const (&str)[N]) noexcept {
  return collapse_spaces(FrozenString{str});
}

namespace detail {

/**
 * @brief HTML/XML 向けの空白文字判定を行う
 *
 * @param c 判定対象文字
 * @return auto 空白文字なら true
 */
auto constexpr is_markup_space(char c) noexcept {
  return is_any_whitespace(c);
}

/**
 * @brief SQL で前後空白の削除対象にできる記号か判定する
 *
 * @param c 判定対象文字
 * @return auto 記号なら true
 */
auto constexpr is_sql_punct(char c) noexcept {
  return c == ',' || c == ';' || c == '(' || c == ')' || c == '='
    || c == '+' || c == '-' || c == '*' || c == '/' || c == '<' || c == '>';
}

/**
 * @brief HTML/XML 本文を最小限の空白へ圧縮する内部実装
 *
 * 文字列リテラル内の文字は保持し、タグ周辺の不要空白と
 * コメント（`<!-- ... -->`）を除去します。
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 入力文字列
 * @return auto 圧縮後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_markup(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto in_tag = false;
  auto in_quote = '\0';
  auto pending_space = false;

  while (i < str.length) {
    // HTML/XML コメントブロックをスキップする
    if (in_quote == '\0' && i + 3 < str.length
        && str.buffer[i] == '<'
        && str.buffer[i + 1] == '!'
        && str.buffer[i + 2] == '-'
        && str.buffer[i + 3] == '-') {
      i += 4;
      while (i + 2 < str.length
             && !(str.buffer[i] == '-'
                  && str.buffer[i + 1] == '-'
                  && str.buffer[i + 2] == '>')) {
        ++i;
      }
      if (i + 2 < str.length) {
        i += 3;
      }
      pending_space = true;
      continue;
    }

    auto const c = str.buffer[i];

    // クォート内は内容をそのまま保持する
    if (in_quote != '\0') {
      res.buffer[offset++] = c;
      if (c == in_quote) {
        in_quote = '\0';
      }
      ++i;
      continue;
    }

    if (c == '"' || c == '\'') {
      if (pending_space) {
        auto const prev = offset == 0 ? '\0' : res.buffer[offset - 1];
        if (prev != '\0' && prev != '<' && prev != '>' && prev != '=' && prev != '/') {
          res.buffer[offset++] = ' ';
        }
        pending_space = false;
      }
      in_quote = c;
      res.buffer[offset++] = c;
      ++i;
      continue;
    }

    // タグ境界の前後空白は削除し、タグ内では単一空白に正規化する
    if (c == '<') {
      if (offset > 0 && res.buffer[offset - 1] == ' ') {
        --offset;
      }
      in_tag = true;
      pending_space = false;
      res.buffer[offset++] = c;
      ++i;
      continue;
    }

    if (c == '>') {
      if (offset > 0 && res.buffer[offset - 1] == ' ') {
        --offset;
      }
      in_tag = false;
      pending_space = false;
      res.buffer[offset++] = c;
      ++i;
      continue;
    }

    if (is_markup_space(c)) {
      pending_space = true;
      ++i;
      continue;
    }

    // 連続空白は1つに集約し、記号の前後には不要空白を入れない
    if (pending_space) {
      auto const prev = offset == 0 ? '\0' : res.buffer[offset - 1];
      auto const next = c;
      auto const should_emit_space =
        prev != '\0'
        && prev != '<'
        && prev != '>'
        && prev != '='
        && prev != '/'
        && next != '>'
        && next != '='
        && next != '/';
      if (should_emit_space) {
        res.buffer[offset++] = ' ';
      }
      pending_space = false;
    }
    res.buffer[offset++] = c;
    ++i;
  }

  if (offset > 0 && res.buffer[offset - 1] == ' ') {
    --offset;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

} // namespace detail

/**
 * @brief HTML 文字列を minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_html(FrozenString<N> const& str) noexcept {
  return detail::minify_markup(str);
}

/**
 * @brief HTML 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_html(char const (&str)[N]) noexcept {
  return minify_html(FrozenString{str});
}

/**
 * @brief XML 文字列を minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_xml(FrozenString<N> const& str) noexcept {
  return detail::minify_markup(str);
}

/**
 * @brief XML 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_xml(char const (&str)[N]) noexcept {
  return minify_xml(FrozenString{str});
}

/**
 * @brief JSON 文字列を minify する
 *
 * 文字列リテラル内は保持し、リテラル外の空白とコメント
 * （行コメント `//` およびブロックコメント）を除去します。
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_json(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto in_string = false;
  auto escaped = false;

  while (i < str.length) {
    auto const c = str.buffer[i];
    // JSON 文字列内ではエスケープ状態を維持しつつそのままコピーする
    if (in_string) {
      res.buffer[offset++] = c;
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      ++i;
      continue;
    }

    if (c == '"') {
      in_string = true;
      res.buffer[offset++] = c;
      ++i;
      continue;
    }

    // コメントと空白は文字列外でのみ削除する
    if (c == '/' && i + 1 < str.length && str.buffer[i + 1] == '/') {
      i += 2;
      while (i < str.length && str.buffer[i] != '\n') {
        ++i;
      }
      continue;
    }

    if (c == '/' && i + 1 < str.length && str.buffer[i + 1] == '*') {
      i += 2;
      while (i + 1 < str.length && !(str.buffer[i] == '*' && str.buffer[i + 1] == '/')) {
        ++i;
      }
      if (i + 1 < str.length) {
        i += 2;
      }
      continue;
    }

    if (detail::is_any_whitespace(c)) {
      ++i;
      continue;
    }

    res.buffer[offset++] = c;
    ++i;
  }

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief JSON 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_json(char const (&str)[N]) noexcept {
  return minify_json(FrozenString{str});
}

/**
 * @brief YAML 文字列を minify する
 *
 * インデント構造を壊さない範囲で、行末空白とコメントを削除します。
 * （引用符内の `#` は保持）
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_yaml(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto line_start = 0uz;
  auto last_non_space = std::string_view::npos;
  auto in_single = false;
  auto in_double = false;
  auto escaped = false;

  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    // 非クォート領域の '#' 以降をコメントとして除去する
    if (!in_single && !in_double && c == '#') {
      while (i < str.length && str.buffer[i] != '\n') {
        ++i;
      }
      if (i >= str.length) {
        break;
      }
    }

    // 行末空白を落としてから改行を出力する（空行は圧縮）
    if (i < str.length && str.buffer[i] == '\n') {
      if (last_non_space == std::string_view::npos) {
        offset = line_start;
      } else {
        offset = last_non_space + 1;
        res.buffer[offset++] = '\n';
      }
      line_start = offset;
      last_non_space = std::string_view::npos;
      in_single = false;
      in_double = false;
      escaped = false;
      continue;
    }

    if (i >= str.length) {
      break;
    }

    auto const current = str.buffer[i];
    res.buffer[offset++] = current;

    // クォート状態を更新し、引用符内の # はコメント扱いしない
    if (in_double) {
      if (escaped) {
        escaped = false;
      } else if (current == '\\') {
        escaped = true;
      } else if (current == '"') {
        in_double = false;
      }
    } else if (in_single) {
      if (current == '\'') {
        in_single = false;
      }
    } else {
      if (current == '"') {
        in_double = true;
      } else if (current == '\'') {
        in_single = true;
      }
    }

    if (current != ' ' && current != '\t' && current != '\r') {
      last_non_space = offset - 1;
    }
  }

  if (last_non_space == std::string_view::npos) {
    offset = line_start;
  } else {
    offset = last_non_space + 1;
  }

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief YAML 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_yaml(char const (&str)[N]) noexcept {
  return minify_yaml(FrozenString{str});
}

/**
 * @brief SQL 文字列を minify する
 *
 * 文字列リテラル・識別子引用を保持しつつ、コメントと不要空白を削除します。
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_sql(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto in_single = false;
  auto in_double = false;
  auto in_backtick = false;
  auto in_bracket = false;
  auto pending_space = false;

  while (i < str.length) {
    auto const c = str.buffer[i];

    // SQL 文字列・識別子引用の内部はそのまま保持する
    if (in_single) {
      res.buffer[offset++] = c;
      if (c == '\'') {
        if (i + 1 < str.length && str.buffer[i + 1] == '\'') {
          res.buffer[offset++] = '\'';
          i += 2;
          continue;
        }
        in_single = false;
      }
      ++i;
      continue;
    }

    if (in_double) {
      res.buffer[offset++] = c;
      if (c == '"') {
        if (i + 1 < str.length && str.buffer[i + 1] == '"') {
          res.buffer[offset++] = '"';
          i += 2;
          continue;
        }
        in_double = false;
      }
      ++i;
      continue;
    }

    if (in_backtick) {
      res.buffer[offset++] = c;
      if (c == '`') {
        in_backtick = false;
      }
      ++i;
      continue;
    }

    if (in_bracket) {
      res.buffer[offset++] = c;
      if (c == ']') {
        in_bracket = false;
      }
      ++i;
      continue;
    }

    // ラインコメント/ブロックコメントを削除する
    if (c == '-' && i + 1 < str.length && str.buffer[i + 1] == '-') {
      i += 2;
      while (i < str.length && str.buffer[i] != '\n') {
        ++i;
      }
      pending_space = true;
      continue;
    }

    if (c == '/' && i + 1 < str.length && str.buffer[i + 1] == '*') {
      i += 2;
      while (i + 1 < str.length && !(str.buffer[i] == '*' && str.buffer[i + 1] == '/')) {
        ++i;
      }
      if (i + 1 < str.length) {
        i += 2;
      }
      pending_space = true;
      continue;
    }

    // 空白は遅延出力し、前後が記号でない場合のみ1文字出力する
    if (detail::is_any_whitespace(c)) {
      pending_space = true;
      ++i;
      continue;
    }

    if (pending_space) {
      auto const prev = offset == 0 ? '\0' : res.buffer[offset - 1];
      auto const prev_is_punct = detail::is_sql_punct(prev);
      auto const next_is_punct = detail::is_sql_punct(c);
      if (prev != '\0' && !prev_is_punct && !next_is_punct) {
        res.buffer[offset++] = ' ';
      }
      pending_space = false;
    }

    if (c == '\'') {
      in_single = true;
    } else if (c == '"') {
      in_double = true;
    } else if (c == '`') {
      in_backtick = true;
    } else if (c == '[') {
      in_bracket = true;
    }

    res.buffer[offset++] = c;
    ++i;
  }

  if (offset > 0 && res.buffer[offset - 1] == ' ') {
    --offset;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief SQL 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_sql(char const (&str)[N]) noexcept {
  return minify_sql(FrozenString{str});
}

} // namespace frozenchars
