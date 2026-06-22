#pragma once

#include "detail/string_utils.hpp"
#include "freeze.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
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
  result.length          = fit_len;
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
  auto           res      = FrozenString<NEW_SIZE>{};

  if (str.length >= Width) {
    for (auto i = 0uz; i < str.length; ++i) {
      res.buffer[i] = str.buffer[i];
    }
    res.buffer[str.length] = '\0';
    res.length             = str.length;
    return res;
  }

  auto const fill_count = Width - str.length;
  auto       offset     = 0uz;
  for (auto i = 0uz; i < fill_count; ++i) {
    res.buffer[offset++] = Fill;
  }
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[offset++] = str.buffer[i];
  }
  res.buffer[offset] = '\0';
  res.length         = offset;
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
  auto           res      = FrozenString<NEW_SIZE>{};

  if (str.length >= Width) {
    for (auto i = 0uz; i < str.length; ++i) {
      res.buffer[i] = str.buffer[i];
    }
    res.buffer[str.length] = '\0';
    res.length             = str.length;
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
  res.length         = offset;
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

  auto       res    = FrozenString<NEW_SIZE>{};
  auto       offset = 0uz;
  auto const src    = str.sv();

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
    res.length             = str.length;
    return res;
  }
  auto const left_fill  = (Width - str.length) / 2;
  auto const right_fill = Width - str.length - left_fill;

  auto res    = FrozenString<NEW_SIZE>{};
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
  res.length         = offset;
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
    if (needle.length == 0) {
      return pos;
    }
    if (needle.length > haystack.length || pos > haystack.length - needle.length) {
      return std::string_view::npos;
    }
    if (needle.length == 1) {  // Special case for char
      for (auto i = pos; i < haystack.length; ++i) {
        if (haystack.buffer[i] == needle.buffer[0]) {
          return i;
        }
      }
      return std::string_view::npos;
    }
    auto const needle_len = needle.length;
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
    requires(is_frozen_string_v<decltype(Str)> && is_frozen_string_v<decltype(From)>)
  [[nodiscard]] consteval auto count_occurrences() noexcept -> std::size_t {
    auto count = 0uz;
    auto pos   = 0uz;
    while (pos < Str.length) {
      auto const found = find_impl(Str, From, pos);
      if (found == std::string_view::npos)
        break;
      ++count;
      pos = found + From.length;
    }
    return count;
  }

  template <auto Str, auto From, auto To>
    requires(is_frozen_string_v<decltype(Str)> && is_frozen_string_v<decltype(From)> && is_frozen_string_v<decltype(To)>)
  [[nodiscard]] consteval auto replace_all_exact_size() noexcept -> std::size_t {
    constexpr auto occurrences = count_occurrences<Str, From>();
    if constexpr (occurrences == 0) {
      return Str.length + 1;
    } else {
      constexpr auto removed = occurrences * From.length;
      constexpr auto added   = occurrences * To.length;
      return Str.length - removed + added + 1;
    }
  }
}  // namespace detail

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
[[nodiscard]] consteval auto replace(FrozenString<N> const& str) noexcept -> FrozenString<N + To.size() + 1> {
  constexpr auto NEW_SIZE = N + To.size() + 1;
  auto           res      = FrozenString<NEW_SIZE>{};

  auto const pos = detail::find_impl(str, From);
  if (pos == std::string_view::npos) {
    for (auto i = 0uz; i < str.length; ++i) {
      res.buffer[i] = str.buffer[i];
    }
    res.buffer[str.length] = '\0';
    res.length             = str.length;
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
  res.length         = offset;
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
  auto       res = FrozenString<N>{};
  auto const requested_len =
      len >= 0 ? static_cast<size_t>(len) : (len == std::numeric_limits<std::ptrdiff_t>::min() ? static_cast<size_t>(std::numeric_limits<std::ptrdiff_t>::max()) + 1uz : static_cast<size_t>(-len));
  auto const anchor     = std::min(pos, str.length);
  auto       start      = anchor;
  auto       actual_len = 0uz;

  if (len >= 0) {
    actual_len = anchor < str.length ? std::min(requested_len, str.length - anchor) : 0uz;
  } else {
    actual_len = std::min(requested_len, anchor);
    start      = anchor - actual_len;
  }

  for (auto i = 0uz; i < actual_len; ++i) {
    res.buffer[i] = str.buffer[start + i];
  }
  res.buffer[actual_len] = '\0';
  res.length             = actual_len;
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
  auto           res              = FrozenString<MAX_REPLACE_SIZE>{};

  auto offset = 0uz;
  auto pos    = 0uz;
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
  res.length         = offset;
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
  requires(detail::is_frozen_string_v<decltype(Str)> && detail::is_frozen_string_v<decltype(From)> && detail::is_frozen_string_v<decltype(To)>)
[[nodiscard]] consteval auto replace_all() noexcept -> FrozenString<detail::replace_all_exact_size<Str, From, To>()> {
  constexpr auto NEW_SIZE = detail::replace_all_exact_size<Str, From, To>();
  auto           res      = FrozenString<NEW_SIZE>{};
  auto           offset   = 0uz;
  auto           pos      = 0uz;
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
  res.length         = offset;
  return res;
}

/**
 * @brief 文字列が部分文字列を含むかを判定する
 *
 * @tparam Substr 検索する部分文字列
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return bool 部分文字列を含むなら true
 */
template <FrozenString Substr, size_t N>
[[nodiscard]] auto consteval contains(FrozenString<N> const& str) noexcept -> bool {
  return detail::find_impl(str, Substr) != std::string_view::npos;
}

/**
 * @brief 文字列リテラルが部分文字列を含むかを判定する
 *
 * @tparam Substr 検索する部分文字列
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return bool 部分文字列を含むなら true
 */
template <FrozenString Substr, size_t N>
[[nodiscard]] auto consteval contains(char const (&str)[N]) noexcept -> bool {
  return contains<Substr>(FrozenString{str});
}

/**
 * @brief freeze可能な文字列が部分文字列を含むかを判定する
 *
 * @tparam Substr 検索する部分文字列 (FrozenString NTTP)
 * @param str 対象文字列
 * @return bool 部分文字列を含むなら true
 */
template <auto Substr>
  requires detail::is_frozen_string_v<decltype(Substr)>
[[nodiscard]] auto consteval contains(auto const& str) noexcept -> bool {
  return contains<Substr>(freeze(str));
}

/**
 * @brief 文字列が指定した接頭辞で始まるかを判定する
 *
 * @tparam Prefix 検索する接頭辞
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return bool 指定した接頭辞で始まるなら true
 */
template <FrozenString Prefix, size_t N>
[[nodiscard]] auto consteval starts_with(FrozenString<N> const& str) noexcept -> bool {
  if constexpr (Prefix.length == 0) {
    return true;
  } else {
    if (str.length < Prefix.length)
      return false;
    for (auto i = 0uz; i < Prefix.length; ++i) {
      if (str.buffer[i] != Prefix.buffer[i])
        return false;
    }
    return true;
  }
}

/**
 * @brief 文字列リテラルが指定した接頭辞で始まるかを判定する
 *
 * @tparam Prefix 検索する接頭辞
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return bool 指定した接頭辞で始まるなら true
 */
template <FrozenString Prefix, size_t N>
[[nodiscard]] auto consteval starts_with(char const (&str)[N]) noexcept -> bool {
  return starts_with<Prefix>(FrozenString{str});
}

/**
 * @brief freeze可能な文字列が指定した接頭辞で始まるかを判定する
 *
 * @tparam Prefix 検索する接頭辞 (FrozenString NTTP)
 * @param str 対象文字列
 * @return bool 指定した接頭辞で始まるなら true
 */
template <auto Prefix>
  requires detail::is_frozen_string_v<decltype(Prefix)>
[[nodiscard]] auto consteval starts_with(auto const& str) noexcept -> bool {
  return starts_with<Prefix>(freeze(str));
}

/**
 * @brief 文字列が指定した接尾辞で終わるかを判定する
 *
 * @tparam Suffix 検索する接尾辞
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return bool 指定した接尾辞で終わるなら true
 */
template <FrozenString Suffix, size_t N>
[[nodiscard]] auto consteval ends_with(FrozenString<N> const& str) noexcept -> bool {
  if constexpr (Suffix.length == 0) {
    return true;
  } else {
    if (str.length < Suffix.length)
      return false;
    auto const start = str.length - Suffix.length;
    for (auto i = 0uz; i < Suffix.length; ++i) {
      if (str.buffer[start + i] != Suffix.buffer[i])
        return false;
    }
    return true;
  }
}

/**
 * @brief 文字列リテラルが指定した接尾辞で終わるかを判定する
 *
 * @tparam Suffix 検索する接尾辞
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return bool 指定した接尾辞で終わるなら true
 */
template <FrozenString Suffix, size_t N>
[[nodiscard]] auto consteval ends_with(char const (&str)[N]) noexcept -> bool {
  return ends_with<Suffix>(FrozenString{str});
}

/**
 * @brief freeze可能な文字列が指定した接尾辞で終わるかを判定する
 *
 * @tparam Suffix 検索する接尾辞 (FrozenString NTTP)
 * @param str 対象文字列
 * @return bool 指定した接尾辞で終わるなら true
 */
template <auto Suffix>
  requires detail::is_frozen_string_v<decltype(Suffix)>
[[nodiscard]] auto consteval ends_with(auto const& str) noexcept -> bool {
  return ends_with<Suffix>(freeze(str));
}

/**
 * @brief 文字列を区切り文字で3分割する
 *
 * @tparam Delim 区切り文字列
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto std::tuple (分割前, 区切り文字, 分割後)
 */
template <FrozenString Delim, size_t N>
[[nodiscard]] auto consteval partition(FrozenString<N> const& str) noexcept {
  auto const pos = detail::find_impl(str, Delim);
  if (pos == std::string_view::npos) {
    // 戻り値型を match パスと一致させるため、同サイズの空の FrozenString を返す
    return std::tuple{str, decltype(Delim){}, FrozenString<N>{}};
  }

  // before_len/after_len を NTTP として使わないよう入力バッファサイズ N を利用する。
  // FrozenString<N> は十分な容量を持ち、length フィールドが実際の長さを管理する。
  auto const before_len  = pos;
  auto const after_start = pos + Delim.length;
  auto const after_len   = str.length - after_start;

  auto before = FrozenString<N>{};
  for (auto i = 0uz; i < before_len; ++i) {
    before.buffer[i] = str.buffer[i];
  }
  before.buffer[before_len] = '\0';
  before.length             = before_len;

  auto after = FrozenString<N>{};
  for (auto i = 0uz; i < after_len; ++i) {
    after.buffer[i] = str.buffer[after_start + i];
  }
  after.buffer[after_len] = '\0';
  after.length            = after_len;

  return std::tuple{before, Delim, after};
}

/**
 * @brief 文字列リテラルを区切り文字で3分割する
 *
 * @tparam Delim 区切り文字列
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto std::tuple (分割前, 区切り文字, 分割後)
 */
template <FrozenString Delim, size_t N>
[[nodiscard]] auto consteval partition(char const (&str)[N]) noexcept {
  return partition<Delim>(FrozenString{str});
}

/**
 * @brief freeze可能な文字列を区切り文字で3分割する
 *
 * @tparam Delim 区切り文字列 (FrozenString NTTP)
 * @param str 対象文字列
 * @return auto std::tuple (分割前, 区切り文字, 分割後)
 */
template <auto Delim>
  requires detail::is_frozen_string_v<decltype(Delim)>
[[nodiscard]] auto consteval partition(auto const& str) noexcept {
  return partition<Delim>(freeze(str));
}

template <size_t Width, char Fill = ' ', typename T>
  requires(!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
[[nodiscard]] auto consteval pad_left(T const& v) noexcept {
  return pad_left<Width, Fill>(freeze(v));
}

template <size_t Width, char Fill = ' ', typename T>
  requires(!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
[[nodiscard]] auto consteval pad_right(T const& v) noexcept {
  return pad_right<Width, Fill>(freeze(v));
}

template <FrozenString Delim, size_t ElemN, size_t Count>
[[nodiscard]] auto consteval join(std::array<FrozenString<ElemN>, Count> const& arr) noexcept {
  constexpr auto NEW_SIZE = (ElemN * Count) + (Delim.size() * Count) + 1;
  auto           res      = FrozenString<NEW_SIZE>{};
  auto           offset   = 0uz;
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
  res.length         = offset;
  return res;
}

template <FrozenString Delim, typename... Args>
  requires(sizeof...(Args) > 0)
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
  requires(std::same_as<std::remove_cvref_t<Ptr>, char const*> || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval ltrim(Ptr&& str) noexcept {
  return ltrim<TrimChar>(freeze(str));
}

template <char TrimChar = ' ', typename Ptr>
  requires(std::same_as<std::remove_cvref_t<Ptr>, char const*> || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval rtrim(Ptr&& str) noexcept {
  return rtrim<TrimChar>(freeze(str));
}

template <char TrimChar = ' ', typename Ptr>
  requires(std::same_as<std::remove_cvref_t<Ptr>, char const*> || std::same_as<std::remove_cvref_t<Ptr>, char*>)
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
  auto res         = FrozenString<N>{};
  auto offset      = 0uz;
  auto in_sequence = false;
  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    if (Pred(c)) {
      if (!in_sequence) {
        res.buffer[offset++] = c;
        in_sequence          = true;
      }
    } else {
      res.buffer[offset++] = c;
      in_sequence          = false;
    }
  }
  res.buffer[offset] = '\0';
  res.length         = offset;
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
    return c == ',' || c == ';' || c == '(' || c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '<' || c == '>' || c == ':' || c == '|' || c == '@' || c == '#';
  }

  /**
   * @brief 布爾属性名か判定する
   *
   * @param name 属性名の先頭ポインタ
   * @param len 属性名の長さ
   * @return auto 布爾属性なら true
   */
  auto constexpr is_boolean_attribute(char const* name, size_t len) noexcept {
    // oxfmt-ignore
    return (len == 8 && name[0] == 'd' && name[1] == 'i' && name[2] == 's' && name[3] == 'a' && name[4] == 'b' && name[5] == 'l' && name[6] == 'e' && name[7] == 'd')
        || (len == 7 && name[0] == 'c' && name[1] == 'h' && name[2] == 'e' && name[3] == 'c' && name[4] == 'k' && name[5] == 'e' && name[6] == 'd')
        || (len == 8 && name[0] == 'r' && name[1] == 'e' && name[2] == 'a' && name[3] == 'd' && name[4] == 'o' && name[5] == 'n' && name[6] == 'l' && name[7] == 'y')
        || (len == 8 && name[0] == 'r' && name[1] == 'e' && name[2] == 'q' && name[3] == 'u' && name[4] == 'i' && name[5] == 'r' && name[6] == 'e' && name[7] == 'd')
        || (len == 9 && name[0] == 'a' && name[1] == 'u' && name[2] == 't' && name[3] == 'o' && name[4] == 'f' && name[5] == 'o' && name[6] == 'c' && name[7] == 'u' && name[8] == 's')
        || (len == 8 && name[0] == 'a' && name[1] == 'u' && name[2] == 't' && name[3] == 'o' && name[4] == 'p' && name[5] == 'l' && name[6] == 'a' && name[7] == 'y')
        || (len == 8 && name[0] == 'c' && name[1] == 'o' && name[2] == 'n' && name[3] == 't' && name[4] == 'r' && name[5] == 'o' && name[6] == 'l' && name[7] == 's')
        || (len == 4 && name[0] == 'l' && name[1] == 'o' && name[2] == 'o' && name[3] == 'p')
        || (len == 5 && name[0] == 'm' && name[1] == 'u' && name[2] == 't' && name[3] == 'e' && name[4] == 'd')
        || (len == 6 && name[0] == 'h' && name[1] == 'i' && name[2] == 'd' && name[3] == 'd' && name[4] == 'e' && name[5] == 'n')
        || (len == 5 && name[0] == 'd' && name[1] == 'e' && name[2] == 'f' && name[3] == 'e' && name[4] == 'r')
        || (len == 5 && name[0] == 'a' && name[1] == 's' && name[2] == 'y' && name[3] == 'n' && name[4] == 'c')
        || (len == 9 && name[0] == 'n' && name[1] == 'o' && name[2] == 'v' && name[3] == 'a' && name[4] == 'l' && name[5] == 'i' && name[6] == 'd' && name[7] == 'a' && name[8] == 't')
        || (len == 13 && name[0] == 'f' && name[1] == 'o' && name[2] == 'r' && name[3] == 'm' && name[4] == 'n' && name[5] == 'o' && name[6] == 'v' && name[7] == 'a' && name[8] == 'l' && name[9] == 'i' && name[10] == 'd' && name[11] == 'a' && name[12] == 't')
        || (len == 8 && name[0] == 'r' && name[1] == 'e' && name[2] == 'v' && name[3] == 'e' && name[4] == 'r' && name[5] == 's' && name[6] == 'e' && name[7] == 'd')
        || (len == 6 && name[0] == 'o' && name[1] == 'p' && name[2] == 'e' && name[3] == 'n' && name[4] == 'e' && name[5] == 'd')
        || (len == 8 && name[0] == 's' && name[1] == 'e' && name[2] == 'l' && name[3] == 'e' && name[4] == 'c' && name[5] == 't' && name[6] == 'e' && name[7] == 'd')
        || (len == 6 && name[0] == 'a' && name[1] == 'u' && name[2] == 't' && name[3] == 'o' && name[4] == 'p' && name[5] == 'l')
        || (len == 6 && name[0] == 'n' && name[1] == 'o' && name[2] == 'w' && name[3] == 'r' && name[4] == 'a' && name[5] == 'p')
        || (len == 6 && name[0] == 's' && name[1] == 'c' && name[2] == 'o' && name[3] == 'p' && name[4] == 'e' && name[5] == 'd')
        || (len == 8 && name[0] == 's' && name[1] == 'e' && name[2] == 'a' && name[3] == 'm' && name[4] == 'l' && name[5] == 'e' && name[6] == 's' && name[7] == 's')
        || (len == 5 && name[0] == 'i' && name[1] == 's' && name[2] == 'm' && name[3] == 'a' && name[4] == 'p')
        || (len == 9 && name[0] == 'i' && name[1] == 't' && name[2] == 'e' && name[3] == 'm' && name[4] == 's' && name[5] == 'c' && name[6] == 'o' && name[7] == 'p' && name[8] == 'e');
  }

  /**
   * @brief 冗長な属性か判定する（デフォルト値と一致する場合）
   *
   * @param tag タグ名の先頭ポインタ
   * @param tag_len タグ名の長さ
   * @param attr_name 属性名の先頭ポインタ
   * @param attr_len 属性名の長さ
   * @param attr_val 属性値の先頭ポインタ
   * @param val_len 属性値の長さ
   * @return auto 冗長なら true
   */
  auto constexpr is_redundant_attribute(char const* tag, size_t tag_len, char const* attr_name, size_t attr_len, char const* attr_val, size_t val_len) noexcept {
    // <script type="text/javascript"> → type 属性は冗長
    if (tag_len == 6 && tag[0] == 's' && tag[1] == 'c' && tag[2] == 'r' && tag[3] == 'i' && tag[4] == 'p' && tag[5] == 't' && attr_len == 4 && attr_name[0] == 't' && attr_name[1] == 'y' && attr_name[2] == 'p' && attr_name[3] == 'e') {
      // text/javascript, application/javascript, module 等
      if (val_len == 15 && attr_val[0] == 't' && attr_val[1] == 'e' && attr_val[2] == 'x' && attr_val[3] == 't' && attr_val[4] == '/' && attr_val[5] == 'j' && attr_val[6] == 'a' && attr_val[7] == 'v' && attr_val[8] == 'a' && attr_val[9] == 's' && attr_val[10] == 'c' && attr_val[11] == 'r' && attr_val[12] == 'i' && attr_val[13] == 'p' && attr_val[14] == 't') {
        return true;
      }
      if (val_len == 19 && attr_val[0] == 'a' && attr_val[1] == 'p' && attr_val[2] == 'p' && attr_val[3] == 'l' && attr_val[4] == 'i' && attr_val[5] == 'c' && attr_val[6] == 'a' && attr_val[7] == 't' && attr_val[8] == 'i' && attr_val[9] == 'o' && attr_val[10] == 'n' && attr_val[11] == '/' && attr_val[12] == 'j' && attr_val[13] == 'a' && attr_val[14] == 'v' && attr_val[15] == 'a' && attr_val[16] == 's' && attr_val[17] == 'c' && attr_val[18] == 'r') {
        return true;
      }
      if (val_len == 6 && attr_val[0] == 'm' && attr_val[1] == 'o' && attr_val[2] == 'd' && attr_val[3] == 'u' && attr_val[4] == 'l' && attr_val[5] == 'e') {
        return true;
      }
    }
    // <style type="text/css"> → type 属性は冗長
    if (tag_len == 5 && tag[0] == 's' && tag[1] == 't' && tag[2] == 'y' && tag[3] == 'l' && tag[4] == 'e' && attr_len == 4 && attr_name[0] == 't' && attr_name[1] == 'y' && attr_name[2] == 'p' && attr_name[3] == 'e') {
      if (val_len == 8 && attr_val[0] == 't' && attr_val[1] == 'e' && attr_val[2] == 'x' && attr_val[3] == 't' && attr_val[4] == '/' && attr_val[5] == 'c' && attr_val[6] == 's' && attr_val[7] == 's') {
        return true;
      }
    }
    // <input type="text"> → type 属性は冗長（デフォルトが text）
    if (tag_len == 5 && tag[0] == 'i' && tag[1] == 'n' && tag[2] == 'p' && tag[3] == 'u' && tag[4] == 't' && attr_len == 4 && attr_name[0] == 't' && attr_name[1] == 'y' && attr_name[2] == 'p' && attr_name[3] == 'e') {
      if (val_len == 4 && attr_val[0] == 't' && attr_val[1] == 'e' && attr_val[2] == 'x' && attr_val[3] == 't') {
        return true;
      }
    }
    // <form method="get"> → method 属性は冗長（デフォルトが get）
    if (tag_len == 4 && tag[0] == 'f' && tag[1] == 'o' && tag[2] == 'r' && tag[3] == 'm' && attr_len == 6 && attr_name[0] == 'm' && attr_name[1] == 'e' && attr_name[2] == 't' && attr_name[3] == 'h' && attr_name[4] == 'o' && attr_name[5] == 'd') {
      if (val_len == 3 && attr_val[0] == 'g' && attr_val[1] == 'e' && attr_val[2] == 't') {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief 省略可能な終了タグか判定する
   *
   * @param tag タグ名の先頭ポインタ
   * @param tag_len タグ名の長さ
   * @return auto 省略可能なら true
   */
  auto constexpr is_optional_end_tag(char const* tag, size_t tag_len) noexcept {
    // oxfmt-ignore
    return (tag_len == 2 && tag[0] == 'l' && tag[1] == 'i')
        || (tag_len == 2 && tag[0] == 'd' && tag[1] == 't')
        || (tag_len == 2 && tag[0] == 'd' && tag[1] == 'd')
        || (tag_len == 1 && tag[0] == 'p')
        || (tag_len == 2 && tag[0] == 't' && tag[1] == 'r')
        || (tag_len == 2 && tag[0] == 't' && tag[1] == 'd')
        || (tag_len == 2 && tag[0] == 't' && tag[1] == 'h')
        || (tag_len == 5 && tag[0] == 't' && tag[1] == 'h' && tag[2] == 'e' && tag[3] == 'a' && tag[4] == 'd')
        || (tag_len == 5 && tag[0] == 't' && tag[1] == 'b' && tag[2] == 'o' && tag[3] == 'd' && tag[4] == 'y')
        || (tag_len == 5 && tag[0] == 't' && tag[1] == 'f' && tag[2] == 'o' && tag[3] == 'o' && tag[4] == 't')
        || (tag_len == 6 && tag[0] == 'o' && tag[1] == 'p' && tag[2] == 't' && tag[3] == 'i' && tag[4] == 'o' && tag[5] == 'n')
        || (tag_len == 7 && tag[0] == 'o' && tag[1] == 'p' && tag[2] == 't' && tag[3] == 'g' && tag[4] == 'r' && tag[5] == 'o' && tag[6] == 'u')
        || (tag_len == 8 && tag[0] == 'c' && tag[1] == 'o' && tag[2] == 'l' && tag[3] == 'g' && tag[4] == 'r' && tag[5] == 'o' && tag[6] == 'u' && tag[7] == 'p')
        || (tag_len == 7 && tag[0] == 'c' && tag[1] == 'a' && tag[2] == 'p' && tag[3] == 't' && tag[4] == 'i' && tag[5] == 'o' && tag[6] == 'n');
  }

  /**
   * @brief 属性値がクォート不要か判定する
   *
   * @param val 属性値の先頭ポインタ
   * @param len 属性値の長さ
   * @return auto クォート不要なら true
   */
  auto constexpr can_remove_attribute_quotes(char const* val, size_t len) noexcept {
    if (len == 0) {
      return false;
    }
    for (size_t j = 0; j < len; ++j) {
      auto const ch = val[j];
      if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\f' || ch == '\r' || ch == '"' || ch == '\'' || ch == '`' || ch == '=' || ch == '<' || ch == '>') {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief タグ名が一致するか判定する（大文字小文字不区別）
   *
   * @param tag タグ名の先頭ポインタ
   * @param tag_len タグ名の長さ
   * @param ref 比較対象文字列
   * @param ref_len 比較対象の長さ
   * @return auto 一致なら true
   */
  auto constexpr tag_equal_ci(char const* tag, size_t tag_len, char const* ref, size_t ref_len) noexcept {
    if (tag_len != ref_len) {
      return false;
    }
    for (size_t j = 0; j < tag_len; ++j) {
      auto a = tag[j];
      auto b = ref[j];
      if (a >= 'A' && a <= 'Z') {
        a = static_cast<char>(a + 32);
      }
      if (b >= 'A' && b <= 'Z') {
        b = static_cast<char>(b + 32);
      }
      if (a != b) {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief HTML/XML 本文を最小限の空白へ圧縮する内部実装
   *
   * 文字列リテラル内の文字は保持し、タグ周辺の不要空白と
   * コメント（`<!-- ... -->`）を除去します。
   * さらに、冗長な属性の削除、布爾属性の圧縮、省略可能な終了タグの除去、
   * 属性値のクォート除去を行います。
   *
   * @tparam N 文字列長（終端文字を含む）
   * @param str 入力文字列
   * @return auto 圧縮後の文字列
   */
  template <size_t N>
  [[nodiscard]] auto consteval minify_markup(FrozenString<N> const& str) noexcept {
    auto res           = FrozenString<N>{};
    auto offset        = 0uz;
    auto i             = 0uz;
    auto in_quote      = '\0';
    auto pending_space = false;

    while (i < str.length) {
      // HTML/XML コメントブロックをスキップする
      if (in_quote == '\0' && i + 3 < str.length && str.buffer[i] == '<' && str.buffer[i + 1] == '!' && str.buffer[i + 2] == '-' && str.buffer[i + 3] == '-') {
        i += 4;
        while (i + 2 < str.length && !(str.buffer[i] == '-' && str.buffer[i + 1] == '-' && str.buffer[i + 2] == '>')) {
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
        in_quote             = c;
        res.buffer[offset++] = c;
        ++i;
        continue;
      }

      // タグ境界の前後空白は削除し、タグ内では単一空白に正規化する
      if (c == '<') {
        if (offset > 0 && res.buffer[offset - 1] == ' ') {
          --offset;
        }
        pending_space = false;

        // 閉じタグの省略: </tag> のうち省略可能なタグをスキップ
        if (i + 1 < str.length && str.buffer[i + 1] == '/') {
          // タグ名を読み取る
          auto tag_start = i + 2;
          auto tag_end   = tag_start;
          while (tag_end < str.length && str.buffer[tag_end] != '>' && !is_markup_space(str.buffer[tag_end])) {
            ++tag_end;
          }
          // '>' まで進めてタグ全体をスキップ
          auto scan = tag_end;
          while (scan < str.length && str.buffer[scan] != '>') {
            ++scan;
          }
          if (scan < str.length) {
            ++scan;
          }
          if (is_optional_end_tag(str.buffer.data() + tag_start, tag_end - tag_start)) {
            i = scan;
            continue;
          }
          // 省略不可の閉じタグはそのまま出力
          for (auto k = i; k < scan; ++k) {
            res.buffer[offset++] = str.buffer[k];
          }
          i = scan;
          continue;
        }

        // 開いたタグ: 属性の最適化を行う
        if (i + 1 < str.length && str.buffer[i + 1] != '/' && str.buffer[i + 1] != '!' && str.buffer[i + 1] != '?') {
          // タグ名を読み取る
          auto tag_start = i + 1;
          auto tag_end   = tag_start;
          while (tag_end < str.length && str.buffer[tag_end] != '>' && !is_markup_space(str.buffer[tag_end])) {
            ++tag_end;
          }
          auto const tag_len = tag_end - tag_start;

          // タグの閉じ '>' までスキャンして属性を処理する
          auto pos = tag_end;
          // '<' とタグ名を出力
          res.buffer[offset++] = '<';
          for (auto k = tag_start; k < tag_end; ++k) {
            res.buffer[offset++] = str.buffer[k];
          }

          // 属性を解析して出力する
          while (pos < str.length && str.buffer[pos] != '>') {
            // 空白をスキップして単一空白を出力
            if (is_markup_space(str.buffer[pos])) {
              // 次に有効な属性があるか先読み
              auto peek = pos;
              while (peek < str.length && is_markup_space(str.buffer[peek])) {
                ++peek;
              }
              // 次が属性名（アルファベット or '_' or ':'）の場合のみ空白を出力
              if (peek < str.length && str.buffer[peek] != '>' && str.buffer[peek] != '/') {
                // 連続空白を1つに集約（既に1つ目を出力済みならスキップ）
                if (res.buffer[offset - 1] != ' ') {
                  res.buffer[offset++] = ' ';
                }
              }
              pos = peek;
              continue;
            }

            // '/' (自己閉じ) ならそのまま出力
            if (str.buffer[pos] == '/') {
              if (offset > 0 && res.buffer[offset - 1] != ' ' && res.buffer[offset - 1] != '<') {
                res.buffer[offset++] = ' ';
              }
              res.buffer[offset++] = '/';
              ++pos;
              continue;
            }

            // 属性名の先頭: アルファベット, '_', ':'
            if ((str.buffer[pos] >= 'a' && str.buffer[pos] <= 'z') || (str.buffer[pos] >= 'A' && str.buffer[pos] <= 'Z') || str.buffer[pos] == '_' || str.buffer[pos] == ':') {
              // 属性名を読み取る
              auto attr_start = pos;
              while (pos < str.length && str.buffer[pos] != '=' && str.buffer[pos] != '>' && !is_markup_space(str.buffer[pos])) {
                ++pos;
              }
              auto const attr_len = pos - attr_start;

              // '=' を探す（空白を飛ばして）
              auto eq_pos = pos;
              while (eq_pos < str.length && is_markup_space(str.buffer[eq_pos])) {
                ++eq_pos;
              }

              if (eq_pos < str.length && str.buffer[eq_pos] == '=') {
                // 属性値を読み取る
                auto val_start = eq_pos + 1;
                while (val_start < str.length && is_markup_space(str.buffer[val_start])) {
                  ++val_start;
                }
                auto val_quote = '\0';
                auto val_end   = val_start;
                if (val_start < str.length && (str.buffer[val_start] == '"' || str.buffer[val_start] == '\'')) {
                  val_quote = str.buffer[val_start];
                  ++val_end;
                  while (val_end < str.length && str.buffer[val_end] != val_quote) {
                    ++val_end;
                  }
                  if (val_end < str.length) {
                    ++val_end;
                  }
                } else {
                  while (val_end < str.length && !is_markup_space(str.buffer[val_end]) && str.buffer[val_end] != '>') {
                    ++val_end;
                  }
                }
                auto const val_content_start = val_quote != '\0' ? val_start + 1 : val_start;
                auto const val_content_end   = val_quote != '\0' ? val_end - 1 : val_end;
                auto const val_content_len   = val_content_end - val_content_start;

                // 冗長属性チェック
                if (is_redundant_attribute(str.buffer.data() + tag_start, tag_len, str.buffer.data() + attr_start, attr_len, str.buffer.data() + val_content_start, val_content_len)) {
                  pos = val_end;
                  continue;
                }

                // 布爾属性チェック: value が属性名と同一なら value を省略
                if (val_content_len == attr_len) {
                  auto is_same = true;
                  for (size_t k = 0; k < attr_len; ++k) {
                    if (str.buffer[attr_start + k] != str.buffer[val_content_start + k]) {
                      is_same = false;
                      break;
                    }
                  }
                  if (is_same && is_boolean_attribute(str.buffer.data() + attr_start, attr_len)) {
                    // 属性名のみ出力
                    for (auto k = attr_start; k < attr_start + attr_len; ++k) {
                      res.buffer[offset++] = str.buffer[k];
                    }
                    pos = val_end;
                    continue;
                  }
                }

                // 通常の属性: 値のクォート除去を試みる
                // 属性値が安全ならクォートなしで出力
                auto can_unquote = val_quote != '\0' && can_remove_attribute_quotes(str.buffer.data() + val_content_start, val_content_len);
                if (can_unquote) {
                  // 属性名
                  for (auto k = attr_start; k < attr_start + attr_len; ++k) {
                    res.buffer[offset++] = str.buffer[k];
                  }
                  res.buffer[offset++] = '=';
                  // クォートなしの値
                  for (auto k = val_content_start; k < val_content_end; ++k) {
                    res.buffer[offset++] = str.buffer[k];
                  }
                } else {
                  // 属性名
                  for (auto k = attr_start; k < attr_start + attr_len; ++k) {
                    res.buffer[offset++] = str.buffer[k];
                  }
                  res.buffer[offset++] = '=';
                  // クォート付きの値
                  for (auto k = val_start; k < val_end; ++k) {
                    res.buffer[offset++] = str.buffer[k];
                  }
                }
                pos = val_end;
              } else {
                // 布爾属性（= なし）: そのまま出力
                for (auto k = attr_start; k < pos; ++k) {
                  res.buffer[offset++] = str.buffer[k];
                }
              }
              continue;
            }

            // その他の文字（タグ内）はそのまま出力
            res.buffer[offset++] = str.buffer[pos];
            ++pos;
          }

          // '>' を出力（不要な末尾空白を削除）
          while (offset > 0 && res.buffer[offset - 1] == ' ') {
            --offset;
          }
          if (pos < str.length && str.buffer[pos] == '>') {
            res.buffer[offset++] = '>';
            ++pos;
          }
          i = pos;
          continue;
        }

        res.buffer[offset++] = c;
        ++i;
        continue;
      }

      if (c == '>') {
        if (offset > 0 && res.buffer[offset - 1] == ' ') {
          --offset;
        }
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
        auto const prev              = offset == 0 ? '\0' : res.buffer[offset - 1];
        auto const should_emit_space = prev != '\0' && prev != '<' && prev != '>' && prev != '=' && prev != '/' && c != '>' && c != '=' && c != '/';
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
    res.length         = offset;
    return res;
  }

}  // namespace detail

// ===== SQL keyword uppercase =====

namespace detail {

  /**
   * @brief SQL 識別子の先頭文字か判定する
   *
   * @param c 判定対象文字
   * @return auto 識別子先頭なら true
   */
  auto constexpr is_sql_id_start(char c) noexcept {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
  }

  /**
   * @brief SQL 識別子の構成文字か判定する
   *
   * @param c 判定対象文字
   * @return auto 識別子構成文字なら true
   */
  auto constexpr is_sql_id_char(char c) noexcept {
    return is_sql_id_start(c) || (c >= '0' && c <= '9');
  }

  /**
   * @brief SQL 予約語リスト（大文字、昇順ソート済み）
   */
  inline constexpr char const* sql_reserved_words[] = {
      "ABS",
      "ALL",
      "ALLOCATE",
      "ALTER",
      "AND",
      "ANY",
      "ARE",
      "ARRAY",
      "AS",
      "ASENSITIVE",
      "ASYMMETRIC",
      "AT",
      "AUTHORIZATION",
      "BEGIN",
      "BETWEEN",
      "BIGINT",
      "BINARY",
      "BLOB",
      "BOOLEAN",
      "BOTH",
      "BY",
      "CALL",
      "CASCADE",
      "CASCADED",
      "CASE",
      "CAST",
      "CHAR",
      "CHARACTER",
      "CHECK",
      "CLOB",
      "CLOSE",
      "COALESCE",
      "COLLATE",
      "COLUMN",
      "COMMIT",
      "CONNECT",
      "CONSTRAINT",
      "CONTAINS",
      "CONTINUE",
      "CORRESPONDING",
      "CREATE",
      "CROSS",
      "CURRENT",
      "CURRENT_DATE",
      "CURRENT_DEFAULT_TRANSFORM_GROUP",
      "CURRENT_PATH",
      "CURRENT_ROLE",
      "CURRENT_TIME",
      "CURRENT_TIMESTAMP",
      "CURRENT_TRANSFORM_GROUP_FOR_TYPE",
      "CURRENT_USER",
      "CURSOR",
      "DATE",
      "DATETIME",
      "DEALLOCATE",
      "DEC",
      "DECIMAL",
      "DECLARE",
      "DEFAULT",
      "DELETE",
      "DEREF",
      "DESC",
      "DETERMINISTIC",
      "DISCONNECT",
      "DISTINCT",
      "DOUBLE",
      "DROP",
      "DYNAMIC",
      "EACH",
      "ELSE",
      "ELSEIF",
      "END",
      "ESCAPE",
      "EXCEPT",
      "EXCEPTION",
      "EXEC",
      "EXECUTE",
      "EXISTS",
      "EXTERNAL",
      "EXTRACT",
      "FALSE",
      "FETCH",
      "FLOAT",
      "FOR",
      "FOREIGN",
      "FREE",
      "FROM",
      "FULL",
      "FUNCTION",
      "GET",
      "GLOBAL",
      "GRANT",
      "GROUP",
      "GROUPING",
      "HANDLER",
      "HAVING",
      "HOLD",
      "IDENTITY",
      "IF",
      "IMMEDIATE",
      "IN",
      "INDICATOR",
      "INNER",
      "INOUT",
      "INPUT",
      "INSENSITIVE",
      "INSERT",
      "INT",
      "INTEGER",
      "INTERSECT",
      "INTO",
      "IS",
      "ITERATE",
      "JOIN",
      "KEY",
      "LANGUAGE",
      "LARGE",
      "LATERAL",
      "LEADING",
      "LEAVE",
      "LEFT",
      "LIKE",
      "LIMIT",
      "LOCAL",
      "LOCALTIME",
      "LOCALTIMESTAMP",
      "LOOP",
      "MATCH",
      "MEMBER",
      "MERGE",
      "METHOD",
      "MINUS",
      "MOD",
      "MODIFIES",
      "MODULE",
      "MULTISET",
      "NATIONAL",
      "NATURAL",
      "NCHAR",
      "NCLOB",
      "NEW",
      "NO",
      "NONE",
      "NOT",
      "NULL",
      "NUMERIC",
      "OF",
      "OLD",
      "ON",
      "ONLY",
      "OPEN",
      "OR",
      "ORDER",
      "OUT",
      "OUTER",
      "OUTPUT",
      "OVERLAPS",
      "PARAMETER",
      "PARTITION",
      "PRECEDING",
      "PRIMARY",
      "PROCEDURE",
      "RANGE",
      "READS",
      "REAL",
      "RECURSIVE",
      "REF",
      "REFERENCES",
      "REFERENCING",
      "RELEASE",
      "RESULT",
      "RETURN",
      "RETURNS",
      "REVOKE",
      "RIGHT",
      "ROLLBACK",
      "ROLLUP",
      "ROW",
      "ROWS",
      "SAVEPOINT",
      "SCROLL",
      "SEARCH",
      "SECOND",
      "SELECT",
      "SENSITIVE",
      "SESSION_USER",
      "SET",
      "SHOW",
      "SIMILAR",
      "SMALLINT",
      "SOME",
      "SPECIFIC",
      "SPECIFICTYPE",
      "SQL",
      "SQLCODE",
      "SQLEXCEPTION",
      "SQLSTATE",
      "SQLWARNING",
      "START",
      "STATIC",
      "SUBMULTISET",
      "SUBSTRING",
      "SYMMETRIC",
      "TABLE",
      "TEMPORARY",
      "THEN",
      "TIME",
      "TIMESTAMP",
      "TIMEZONE_HOUR",
      "TIMEZONE_MINUTE",
      "TO",
      "TRAILING",
      "TRANSACTION",
      "TREAT",
      "TRIGGER",
      "TRIM",
      "TRUE",
      "UNDO",
      "UNION",
      "UNIQUE",
      "UNKNOWN",
      "UNNEST",
      "UPDATE",
      "UPPER",
      "USER",
      "USING",
      "VALUE",
      "VALUES",
      "VARCHAR",
      "VARYING",
      "VIEW",
      "WHEN",
      "WHENEVER",
      "WHERE",
      "WHILE",
      "WINDOW",
      "WITH",
      "WITHIN",
      "WITHOUT",
      "YEAR",
  };

  /**
   * @brief SQL 予約語かどうかを二分探索で判定する
   *
   * @param word 判定対象の識別子（大文字）
   * @param len 文字列長
   * @return auto 予約語なら true
   */
  auto consteval sql_is_reserved(char const* word, size_t len) noexcept {
    auto constexpr count = sizeof(sql_reserved_words) / sizeof(sql_reserved_words[0]);
    auto lo              = 0uz;
    auto hi              = count;
    while (lo < hi) {
      auto const mid = lo + (hi - lo) / 2;
      auto const r   = sql_reserved_words[mid];
      auto       j   = 0uz;
      while (j < len && r[j] != '\0' && word[j] == r[j]) {
        ++j;
      }
      auto const cmp = (j < len && r[j] != '\0') ? (static_cast<unsigned char>(word[j]) - static_cast<unsigned char>(r[j])) : (j < len ? 1 : (r[j] != '\0' ? -1 : 0));
      if (cmp == 0) {
        return true;
      } else if (cmp < 0) {
        hi = mid;
      } else {
        lo = mid + 1;
      }
    }
    return false;
  }

  /**
   * @brief SQL 型キーワード短縮マッピング
   */
  struct sql_type_mapping {
    char const* long_form;
    size_t      long_len;
    char const* short_form;
    size_t      short_len;
  };

  inline constexpr sql_type_mapping sql_type_shortenings[] = {
      {"BOOLEAN",   7, "BOOL",  4},
      {"CHARACTER", 9, "CHAR",  4},
      {"INTEGER",   7, "INT",   3},
  };

  /**
   * @brief SQL 型キーワードの短縮マッピングを検索する
   *
   * @param word 大文字変換済みの識別子
   * @param len 文字列長
   * @return auto マッピングがあればポインタ、なければ nullptr
   */
  auto consteval sql_find_type_shortening(char const* word, size_t len) noexcept -> sql_type_mapping const* {
    for (auto const& m : sql_type_shortenings) {
      if (len != m.long_len) {
        continue;
      }
      auto match = true;
      for (auto j = 0uz; j < len; ++j) {
        if (word[j] != m.long_form[j]) {
          match = false;
          break;
        }
      }
      if (match) {
        return &m;
      }
    }
    return nullptr;
  }

}  // namespace detail

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
  auto res       = FrozenString<N>{};
  auto offset    = 0uz;
  auto i         = 0uz;
  auto in_string = false;
  auto escaped   = false;

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
      in_string            = true;
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
  res.length         = offset;
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
  auto res            = FrozenString<N>{};
  auto offset         = 0uz;
  auto line_start     = 0uz;
  auto last_non_space = std::string_view::npos;
  auto in_single      = false;
  auto in_double      = false;
  auto escaped        = false;

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
        offset               = last_non_space + 1;
        res.buffer[offset++] = '\n';
      }
      line_start     = offset;
      last_non_space = std::string_view::npos;
      in_single      = false;
      in_double      = false;
      escaped        = false;
      continue;
    }

    if (i >= str.length) {
      break;
    }

    auto const current   = str.buffer[i];
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
  res.length         = offset;
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
 * shorten_types が true の場合、型キーワードも短縮します（INTEGER→INT 等）。
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @param shorten_types 型キーワード短縮を有効にするか（既定 true）
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_sql(FrozenString<N> const& str, bool shorten_types = true) noexcept {
  auto res           = FrozenString<N>{};
  auto offset        = 0uz;
  auto i             = 0uz;
  auto in_single     = false;
  auto in_double     = false;
  auto in_backtick   = false;
  auto in_bracket    = false;
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
      auto const prev          = offset == 0 ? '\0' : res.buffer[offset - 1];
      auto const prev_is_punct = detail::is_sql_punct(prev);
      auto const next_is_punct = detail::is_sql_punct(c);
      auto const next_is_close = c == ')';
      if (prev != '\0' && !prev_is_punct && !next_is_punct && !next_is_close) {
        res.buffer[offset++] = ' ';
      }
      pending_space = false;
    }

    // 型キーワードの短縮
    if (shorten_types && detail::is_sql_id_start(c)) {
      auto const word_start = i;
      while (i < str.length && detail::is_sql_id_char(str.buffer[i])) {
        ++i;
      }
      auto const word_len = i - word_start;

      auto upper_buf = std::array<char, 256>{};
      if (word_len <= upper_buf.size()) {
        for (auto j = 0uz; j < word_len; ++j) {
          auto const ch = str.buffer[word_start + j];
          upper_buf[j]  = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - ('a' - 'A')) : ch;
        }

        auto const* mapping = detail::sql_find_type_shortening(upper_buf.data(), word_len);
        if (mapping != nullptr) {
          if (word_len == 9 && upper_buf[0] == 'C') {
            auto peek = i;
            while (peek < str.length && detail::is_any_whitespace(str.buffer[peek])) {
              ++peek;
            }
            if (peek + 7 <= str.length) {
              auto varying_match = true;
              for (auto j = 0uz; j < 7; ++j) {
                auto const ch        = str.buffer[peek + j];
                auto const upper_ch  = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - ('a' - 'A')) : ch;
                if (upper_ch != "VARYING"[j]) {
                  varying_match = false;
                  break;
                }
              }
              if (varying_match && peek + 7 < str.length && !detail::is_sql_id_char(str.buffer[peek + 7])) {
                constexpr char VARCHAR[] = "VARCHAR";
                for (auto j = 0uz; j < 7; ++j) {
                  res.buffer[offset++] = VARCHAR[j];
                }
                i = peek + 7;
                pending_space = true;
                continue;
              }
            }
          }
          for (auto j = 0uz; j < mapping->short_len; ++j) {
            res.buffer[offset++] = mapping->short_form[j];
          }
          continue;
        }
      }

      for (auto j = 0uz; j < word_len; ++j) {
        res.buffer[offset++] = str.buffer[word_start + j];
      }
      continue;
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
  res.length         = offset;
  return res;
}

/**
 * @brief SQL 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @param shorten_types 型キーワード短縮を有効にするか（既定 true）
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_sql(char const (&str)[N], bool shorten_types = true) noexcept {
  return minify_sql(FrozenString{str}, shorten_types);
}

/**
 * @brief SQL 予約語を大文字に変換する
 *
 * 文字列リテラル・識別子引用の内部は保持し、予約語だけを大文字化します。
 * 識別子の判定は英数字とアンダースコアのみのトークンとし、
 * ドット区切りのカラム参照（`t.col`）やキャスト（`INT::text`）にも対応します。
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @return auto 変換後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval sql_uppercase_keywords(FrozenString<N> const& str) noexcept {
  auto res         = FrozenString<N>{};
  auto offset      = 0uz;
  auto i           = 0uz;
  auto in_single   = false;
  auto in_double   = false;
  auto in_backtick = false;
  auto in_bracket  = false;

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

    // ラインコメント/ブロックコメントはそのまま保持する
    if (c == '-' && i + 1 < str.length && str.buffer[i + 1] == '-') {
      while (i < str.length && str.buffer[i] != '\n') {
        res.buffer[offset++] = str.buffer[i++];
      }
      continue;
    }

    if (c == '/' && i + 1 < str.length && str.buffer[i + 1] == '*') {
      while (i < str.length) {
        res.buffer[offset++] = str.buffer[i];
        if (str.buffer[i] == '*' && i + 1 < str.length && str.buffer[i + 1] == '/') {
          res.buffer[offset++] = str.buffer[i + 1];
          i += 2;
          break;
        }
        ++i;
      }
      continue;
    }

    // 引用符の開始
    if (c == '\'') {
      in_single            = true;
      res.buffer[offset++] = c;
      ++i;
      continue;
    }
    if (c == '"') {
      in_double            = true;
      res.buffer[offset++] = c;
      ++i;
      continue;
    }
    if (c == '`') {
      in_backtick          = true;
      res.buffer[offset++] = c;
      ++i;
      continue;
    }
    if (c == '[') {
      in_bracket           = true;
      res.buffer[offset++] = c;
      ++i;
      continue;
    }

    // 識別子トークンを抽出し、予約語なら大文字化する
    if (detail::is_sql_id_start(c)) {
      auto const word_start = i;
      while (i < str.length && detail::is_sql_id_char(str.buffer[i])) {
        ++i;
      }
      auto const word_len = i - word_start;

      // トークンを大文字に変換して予約語照合する
      auto upper_buf = std::array<char, 256>{};
      if (word_len <= upper_buf.size()) {
        for (auto j = 0uz; j < word_len; ++j) {
          auto const ch = str.buffer[word_start + j];
          upper_buf[j]  = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - ('a' - 'A')) : ch;
        }
        if (detail::sql_is_reserved(upper_buf.data(), word_len)) {
          for (auto j = 0uz; j < word_len; ++j) {
            res.buffer[offset++] = upper_buf[j];
          }
        } else {
          for (auto j = 0uz; j < word_len; ++j) {
            res.buffer[offset++] = str.buffer[word_start + j];
          }
        }
      } else {
        // 長すぎる識別子はそのまま保持
        for (auto j = 0uz; j < word_len; ++j) {
          res.buffer[offset++] = str.buffer[word_start + j];
        }
      }
      continue;
    }

    // その他の文字はそのまま出力する
    res.buffer[offset++] = c;
    ++i;
  }

  res.buffer[offset] = '\0';
  res.length         = offset;
  return res;
}

/**
 * @brief SQL 予約語リテラルを大文字に変換する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @return auto 変換後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval sql_uppercase_keywords(char const (&str)[N]) noexcept {
  return sql_uppercase_keywords(FrozenString{str});
}

/**
 * @brief 文字列を指定幅で折り返す（ワードラップ）
 *
 * スペース区切りで単語を認識し、指定された幅を超える前に改行を挿入します。
 * 既存の改行は保持されます。各行の先頭の余分なスペースは削除されます。
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param width 1行の最大幅（文字数）
 * @return auto 折り返し後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval word_wrap(FrozenString<N> const& str, size_t width) noexcept {
  if (width == 0) {
    width = 1;
  }
  constexpr auto OUT_CAP = (N > 0 ? N * 2 : 1);
  auto           res     = FrozenString<OUT_CAP>{};
  auto           offset  = 0uz;
  auto           col     = 0uz;

  for (auto i = 0uz; i < str.length;) {
    auto const c = str.buffer[i];

    // 改行はそのまま保持
    if (c == '\n') {
      if (offset > 0 && col == 0 && res.buffer[offset - 1] == ' ') {
        --offset;
      }
      res.buffer[offset++] = '\n';
      col                  = 0;
      ++i;
      continue;
    }

    // 空白文字を検出
    if (detail::is_any_whitespace(c)) {
      if (col == 0) {
        ++i;
        continue;
      }
      // 次の単語を探す
      auto next_word = i + 1;
      while (next_word < str.length && detail::is_any_whitespace(str.buffer[next_word]) && str.buffer[next_word] != '\n') {
        ++next_word;
      }
      if (next_word >= str.length) {
        break;
      }
      auto word_end = next_word;
      while (word_end < str.length && !detail::is_any_whitespace(str.buffer[word_end]) && str.buffer[word_end] != '\n') {
        ++word_end;
      }
      auto const word_len = word_end - next_word;

      // 幅を超える場合は改行を挿入
      if (col + 1 + word_len > width) {
        if (offset > 0 && res.buffer[offset - 1] == ' ') {
          --offset;
          --col;
        }
        res.buffer[offset++] = '\n';
        col                  = 0;
        ++i;
        continue;
      }

      res.buffer[offset++] = ' ';
      ++col;
      ++i;
      continue;
    }

    // 通常の文字
    res.buffer[offset++] = c;
    ++col;
    ++i;
  }

  while (offset > 0 && res.buffer[offset - 1] == ' ') {
    --offset;
  }

  res.buffer[offset] = '\0';
  res.length         = offset;
  return res;
}

/**
 * @brief 文字列リテラルを指定幅で折り返す（ワードラップ）
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @param width 1行の最大幅（文字数）
 * @return auto 折り返し後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval word_wrap(char const (&str)[N], size_t width) noexcept {
  return word_wrap(FrozenString{str}, width);
}

}  // namespace frozenchars
