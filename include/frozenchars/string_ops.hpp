#pragma once

#include "detail/string_utils.hpp"
#include "freeze.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <string_view>
#include <utility>

namespace frozenchars {

/**
 * @brief 行区切り文字の種類
 */
enum class LineBreak {
  Br,       ///< <br> HTML タグ（全バリエーション）
  EscN,     ///< \n リテラル（バックスラッシュ+n）
  Nl,       ///< 実改行（LF, 0x0A）
};

/**
 * @brief <br> タグのバリエーションにマッチするか判定し、一致長を返す
 *
 * マッチ対象: <br>, <BR>, <br/>, <br />, <Br/> 等（大文字小文字不問、空白・スラッシュ任意）
 */
template <size_t N>
[[nodiscard]] consteval auto match_br_tag(FrozenString<N> const& str, size_t pos) noexcept -> size_t {
  if (pos >= str.length || str.buffer[pos] != '<') return 0;
  auto i = pos + 1;
  if (i >= str.length) return 0;
  auto c = str.buffer[i];
  if (c != 'b' && c != 'B') return 0;
  ++i;
  if (i >= str.length) return 0;
  c = str.buffer[i];
  if (c != 'r' && c != 'R') return 0;
  ++i;
  while (i < str.length && (str.buffer[i] == ' ' || str.buffer[i] == '\t')) ++i;
  if (i < str.length && str.buffer[i] == '/') ++i;
  if (i >= str.length || str.buffer[i] != '>') return 0;
  ++i;
  return i - pos;
}

/**
 * @brief <br> バリエーションを特定の文字列に置換する
 */
template <FrozenString To, size_t N>
[[nodiscard]] consteval auto br_to_target(FrozenString<N> const& str) noexcept {
  constexpr auto MAX_SIZE = std::max(N * 4, 2048uz);
  auto res = FrozenString<MAX_SIZE>{};
  auto offset = 0uz;
  auto pos = 0uz;
  while (pos < str.length) {
    auto const len = match_br_tag(str, pos);
    if (len == 0) {
      res.buffer[offset++] = str.buffer[pos++];
    } else {
      for (auto i = 0uz; i < To.length; ++i) res.buffer[offset++] = To.buffer[i];
      pos += len;
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 行区切り表現を相互変換する
 */
template <LineBreak From, LineBreak To, size_t N>
[[nodiscard]] consteval auto convert_linebreak(FrozenString<N> const& str) noexcept {
  if constexpr (From == To) return str;

  if constexpr (From == LineBreak::Br) {
    if constexpr (To == LineBreak::Nl) return br_to_target<"\n">(str);
    if constexpr (To == LineBreak::EscN) return br_to_target<"\\n">(str);
  } else if constexpr (From == LineBreak::Nl) {
    if constexpr (To == LineBreak::Br) return replace_all<"\n", "<br>">(str);
    if constexpr (To == LineBreak::EscN) return replace_all<"\n", "\\n">(str);
  } else if constexpr (From == LineBreak::EscN) {
    if constexpr (To == LineBreak::Br) return replace_all<"\\n", "<br>">(str);
    if constexpr (To == LineBreak::Nl) return replace_all<"\\n", "\n">(str);
  }
}

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
  constexpr auto NEW_SIZE = (ElemN * Count) + (Delim.size() * (Count > 0 ? Count - 1 : 0)) + 1;
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
  auto constexpr sql_find_type_shortening(char const* word, size_t len) noexcept -> sql_type_mapping const* {
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
