#pragma once

#include "detail/string_utils.hpp"
#include "freeze.hpp"
#include "literals.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <optional>
#include <string_view>
#include <vector>

namespace frozenchars {

/**
 * @brief 行区切り文字の種類
 */
enum class LineBreak {
  Br,    ///< <br> HTML タグ（全バリエーション）
  EscN,  ///< \n リテラル（バックスラッシュ+n）
  Nl,    ///< 実改行（LF, 0x0A）
};

/**
 * @brief <br> タグのバリエーションにマッチするか判定し、一致長を返す
 *
 * マッチ対象: <br>, <BR>, <br/>, <br />, <Br/> 等（大文字小文字不問、空白・スラッシュ任意）
 */
template <size_t N>
[[nodiscard]] consteval auto match_br_tag(FrozenString<N> const& str, size_t pos) noexcept -> size_t {
  if (pos >= str.length || str.buffer[pos] != '<')
    return 0;
  auto i = pos + 1;
  if (i >= str.length)
    return 0;
  auto c = str.buffer[i];
  if (c != 'b' && c != 'B')
    return 0;
  ++i;
  if (i >= str.length)
    return 0;
  c = str.buffer[i];
  if (c != 'r' && c != 'R')
    return 0;
  ++i;
  while (i < str.length && (str.buffer[i] == ' ' || str.buffer[i] == '\t'))
    ++i;
  if (i < str.length && str.buffer[i] == '/')
    ++i;
  if (i >= str.length || str.buffer[i] != '>')
    return 0;
  ++i;
  return i - pos;
}

/**
 * @brief <br> バリエーションを特定の文字列に置換する
 */
template <FrozenString To, size_t N>
[[nodiscard]] consteval auto br_to_target(FrozenString<N> const& str) noexcept {
  constexpr auto MAX_SIZE = std::max(N * 4, 2048uz);
  auto           res      = FrozenString<MAX_SIZE>{};
  auto           offset   = 0uz;
  auto           pos      = 0uz;
  while (pos < str.length) {
    auto const len = match_br_tag(str, pos);
    if (len == 0) {
      res.buffer[offset++] = str.buffer[pos++];
    } else {
      for (auto i = 0uz; i < To.length; ++i)
        res.buffer[offset++] = To.buffer[i];
      pos += len;
    }
  }
  res.buffer[offset] = '\0';
  res.length         = offset;
  return res;
}

/**
 * @brief 行区切り表現を相互変換する
 */
template <LineBreak From, LineBreak To, size_t N>
[[nodiscard]] consteval auto convert_linebreak(FrozenString<N> const& str) noexcept {
  if constexpr (From == To)
    return str;

  if constexpr (From == LineBreak::Br) {
    if constexpr (To == LineBreak::Nl)
      return br_to_target<"\n">(str);
    if constexpr (To == LineBreak::EscN)
      return br_to_target<"\\n">(str);
  } else if constexpr (From == LineBreak::Nl) {
    if constexpr (To == LineBreak::Br)
      return replace_all<"\n", "<br>">(str);
    if constexpr (To == LineBreak::EscN)
      return replace_all<"\n", "\\n">(str);
  } else if constexpr (From == LineBreak::EscN) {
    if constexpr (To == LineBreak::Br)
      return replace_all<"\\n", "<br>">(str);
    if constexpr (To == LineBreak::Nl)
      return replace_all<"\\n", "\n">(str);
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

/**
 * @brief 各行を指定幅に左パディングする（文字列リテラル版）
 */
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

/**
 * @brief 各行を指定幅に右パディングする（文字列リテラル版）
 */
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

/**
 * @brief 指定文字を指定回数繰り返した文字列を生成する（NTTP版）
 *
 * @tparam Count 繰り返し回数
 * @tparam Ch 繰り返す文字
 * @return FrozenString<Count + 1> 生成した文字列
 */
template <size_t Count, char Ch>
[[nodiscard]] auto consteval repeat_char() noexcept -> FrozenString<Count + 1> {
  auto res = FrozenString<Count + 1>{};
  for (auto i = 0uz; i < Count; ++i) {
    res.buffer[i] = Ch;
  }
  res.buffer[Count] = '\0';
  res.length = Count;
  return res;
}

/**
 * @brief 文字列を指定最大長に省略する（接尾辞は既定 "..."）
 */
template <size_t MaxLen, size_t N>
[[nodiscard]] auto consteval abbreviate(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N, MaxLen + 1)> {
  return abbreviate<MaxLen, FrozenString<4>{"..."}>(str);
}

/**
 * @brief 文字列を指定最大長に省略する（NTTP で接尾辞を指定）
 *
 * 仕様: Suffix が指定されている場合は MaxLen の値に関わらず必ず付与する。
 * MaxLen が Suffix.length 以下の場合は Suffix 単体を切り詰めて返す。
 */
template <size_t MaxLen, auto Suffix, size_t N>
  requires detail::is_frozen_string_v<decltype(Suffix)>
[[nodiscard]] auto consteval abbreviate(FrozenString<N> const& str) noexcept -> FrozenString<std::max(N, MaxLen + 1)> {
  if (str.length <= MaxLen) {
    return str;
  }

  auto res = FrozenString<MaxLen + 1>{};
  if constexpr (Suffix.length > 0) {
    // Suffix を必ず末尾に付与する。MaxLen < Suffix.length の場合は Suffix の先頭 MaxLen 文字で置き換える。
    if constexpr (MaxLen <= Suffix.length) {
      for (auto i = 0uz; i < MaxLen; ++i) {
        res.buffer[i] = Suffix.buffer[i];
      }
      res.buffer[MaxLen] = '\0';
      res.length = MaxLen;
      return res;
    } else {
      auto const prefix_len = MaxLen - Suffix.length;
      for (auto i = 0uz; i < prefix_len; ++i) {
        res.buffer[i] = str.buffer[i];
      }
      for (auto i = 0uz; i < Suffix.length; ++i) {
        res.buffer[prefix_len + i] = Suffix.buffer[i];
      }
      res.buffer[MaxLen] = '\0';
      res.length = MaxLen;
      return res;
    }
  }

  // Suffix なし: 単純に先頭 MaxLen 文字
  for (auto i = 0uz; i < MaxLen; ++i) {
    res.buffer[i] = str.buffer[i];
  }
  res.buffer[MaxLen] = '\0';
  res.length = MaxLen;
  return res;
}

/**
 * @brief 文字列を指定最大長に省略する（文字列リテラル版、接尾辞は既定 "..."）
 */
template <size_t MaxLen, size_t N>
[[nodiscard]] auto consteval abbreviate(char const (&str)[N]) noexcept -> FrozenString<std::max(N, MaxLen + 1)> {
  return abbreviate<MaxLen>(FrozenString{str});
}

/**
 * @brief 文字列を指定最大長に省略する（文字列リテラル版、NTTP で接尾辞を指定）
 */
template <size_t MaxLen, auto Suffix, size_t N>
[[nodiscard]] auto consteval abbreviate(char const (&str)[N]) noexcept -> FrozenString<std::max(N, MaxLen + 1)> {
  return abbreviate<MaxLen, Suffix>(FrozenString{str});
}

/**
 * @brief 連続する空白を 1 文字の半角スペースに正規化する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return FrozenString<N> 正規化後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval normalize_whitespace(FrozenString<N> const& str) noexcept -> FrozenString<N> {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto seen_ws = false;
  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    if (detail::is_any_whitespace(c)) {
      if (!seen_ws && offset > 0) {
        res.buffer[offset++] = ' ';
        seen_ws = true;
      }
      continue;
    }
    res.buffer[offset++] = c;
    seen_ws = false;
  }
  while (offset > 0 && res.buffer[offset - 1] == ' ') {
    --offset;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 連続する空白を 1 文字の半角スペースに正規化する（文字列リテラル版）
 */
template <size_t N>
[[nodiscard]] auto consteval normalize_whitespace(char const (&str)[N]) noexcept -> FrozenString<N> {
  return normalize_whitespace(FrozenString{str});
}

/**
 * @brief normalize_whitespace の別名（Python の str.split()+join 相当）
 */
template <size_t N>
[[nodiscard]] auto consteval squeeze(FrozenString<N> const& str) noexcept -> FrozenString<N> {
  return normalize_whitespace(str);
}

/**
 * @brief 連続する空白を 1 文字の半角スペースに正規化する（文字列リテラル版）
 */
template <size_t N>
[[nodiscard]] auto consteval squeeze(char const (&str)[N]) noexcept -> FrozenString<N> {
  return squeeze(FrozenString{str});
}

namespace detail {

/**
 * @brief 文字列 haystack 内で部分文字列 needle の最初の出現位置を検索する
 * @tparam N haystack のバッファ長
 * @tparam M needle のバッファ長
 * @param haystack 検索対象の文字列
 * @param needle 検索する部分文字列
 * @param pos 検索開始位置（デフォルト 0）
 * @return 見つかった位置、見つからなければ npos
 */
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

/**
 * @brief 文字列 Str 内で From が重複なしで出現する回数を NTTP で計算する
 * @tparam Str 検索対象の文字列
 * @tparam From 検索する部分文字列
 * @return 出現回数
 */
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

/**
 * @brief Str 内の From をすべて To に置換した結果のバッファサイズ（終端 \0 含む）を NTTP で計算する
 * @tparam Str 元の文字列
 * @tparam From 置換前の部分文字列
 * @tparam To 置換後の文字列
 * @return 必要なバッファサイズ（置換後長 + 1）
 */
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

/**
 * @brief 文字列置換を行う（文字列リテラル版、最初の 1 箇所のみ）
 */
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
  auto requested_len = size_t{};
  if (len >= 0) {
    requested_len = static_cast<size_t>(len);
  } else if (len == std::numeric_limits<std::ptrdiff_t>::min()) {
    requested_len = static_cast<size_t>(std::numeric_limits<std::ptrdiff_t>::max()) + 1uz;
  } else {
    requested_len = static_cast<size_t>(-len);
  }
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

/**
 * @brief 実行時引数で位置・長さを指定して部分文字列を切り出す（文字列リテラル版）
 */
template <size_t N>
[[nodiscard]] auto consteval substr(char const (&str)[N], size_t pos, std::ptrdiff_t len) noexcept {
  return substr(FrozenString{str}, pos, len);
}

/**
 * @brief 文字列内のすべての指定した部分文字列を置換した文字列を生成する
 *
 * @tparam From 置換前の文字列（空文字列は不可）
 * @tparam To 置換後の文字列
 * @tparam N 処理対象の文字列の長さ (終端文字'\0'を含む)
 * @param str 処理対象の文字列
 * @return auto 生成した文字列
 */
template <FrozenString From, FrozenString To, size_t N>
[[nodiscard]] consteval auto replace_all(FrozenString<N> const& str) noexcept {
  static_assert(From.length > 0, "replace_all: From cannot be an empty string");
  // 入力長 S = N-1。マッチ k 個の出力 = S + k*(To.length - From.length)。
  // To.length > From.length: k 最大時に出力最大。
  // To.length < From.length: k = 0（マッチなし）時に出力最大 = S。
  // 両ケースを max で統一する。
  constexpr auto S                 = (N > 1uz) ? N - 1uz : 0uz;
  constexpr auto MAX_MATCHES       = S / From.length;
  constexpr auto MAX_OUT_ALL_MATCH = MAX_MATCHES * To.length + S % From.length;
  constexpr auto MAX_OUT           = std::max(S, MAX_OUT_ALL_MATCH);
  constexpr auto MAX_REPLACE_SIZE  = std::max(MAX_OUT + 1uz, 1uz);
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

/**
 * @brief 文字列置換を行う（文字列リテラル版、全箇所）
 */
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
  return shrink_to_fit<replace_all<From, To>(Str)>();
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
  }
  if (str.length < Prefix.length)
    return false;
  for (auto i = 0uz; i < Prefix.length; ++i) {
    if (str.buffer[i] != Prefix.buffer[i])
      return false;
  }
  return true;
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

/**
 * @brief freeze可能な非整数値を左詰めする（freeze 後に pad_left に委譲）
 * @tparam Width 目標幅
 * @tparam Fill 埋め文字（デフォルト空白）
 * @param v 埋める値
 */
template <size_t Width, char Fill = ' ', typename T>
  requires(!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
[[nodiscard]] auto consteval pad_left(T const& v) noexcept {
  return pad_left<Width, Fill>(freeze(v));
}

/**
 * @brief freeze可能な非整数値を右詰めする（freeze 後に pad_right に委譲）
 * @tparam Width 目標幅
 * @tparam Fill 埋め文字（デフォルト空白）
 * @param v 埋める値
 */
template <size_t Width, char Fill = ' ', typename T>
  requires(!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
[[nodiscard]] auto consteval pad_right(T const& v) noexcept {
  return pad_right<Width, Fill>(freeze(v));
}

/**
 * @brief FrozenString の配列を NTTP 区切り文字で結合する
 * @tparam Delim 区切り文字列
 * @tparam ElemN 各要素のバッファ長
 * @tparam Count 要素数
 * @param arr 結合する配列
 */
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

/**
 * @brief 可変長引数を freeze して NTTP 区切り文字で結合する
 * @tparam Delim 区切り文字列
 * @tparam Args 引数の型パック
 * @param args 結合する値
 */
template <FrozenString Delim, typename... Args>
  requires(sizeof...(Args) > 0)
[[nodiscard]] auto consteval join(Args const&... args) noexcept {
  auto const arr = std::array<FrozenString<2048>, sizeof...(Args)>{freeze(args)...};
  return join<Delim>(arr);
}

/**
 * @brief 整数値を文字列化して左詰めする（デフォルト埋め文字 '0'）
 * @tparam Width 目標幅
 * @tparam Fill 埋め文字（デフォルト '0'）
 * @param v 数値
 */
template <size_t Width, char Fill = '0', Integral T>
[[nodiscard]] auto consteval pad_left(T const& v) noexcept {
  return pad_left<Width, Fill>(freeze(v));
}

/**
 * @brief 整数値を文字列化して右詰めする（デフォルト埋め文字 '0'）
 * @tparam Width 目標幅
 * @tparam Fill 埋め文字（デフォルト '0'）
 * @param v 数値
 */
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

/**
 * @brief 先頭から指定文字を削除する（char ポインタ版）
 */
template <char TrimChar = ' ', typename Ptr>
  requires(std::same_as<std::remove_cvref_t<Ptr>, char const*> || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval ltrim(Ptr&& str) noexcept {
  return ltrim<TrimChar>(freeze(str));
}

/**
 * @brief 末尾から指定文字を削除する（char ポインタ版）
 */
template <char TrimChar = ' ', typename Ptr>
  requires(std::same_as<std::remove_cvref_t<Ptr>, char const*> || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval rtrim(Ptr&& str) noexcept {
  return rtrim<TrimChar>(freeze(str));
}

/**
 * @brief 先頭・末尾から指定文字を削除する（char ポインタ版）
 */
template <char TrimChar = ' ', typename Ptr>
  requires(std::same_as<std::remove_cvref_t<Ptr>, char const*> || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval trim(Ptr&& str) noexcept {
  return trim<TrimChar>(freeze(str));
}

namespace detail {

/**
 * @brief strip 系関数のデフォルト削除文字集合（Python の ASCII 空白と同一）
 */
inline constexpr auto default_strip_chars = FrozenString{" \t\n\r\f\v"};

/**
 * @brief 指定した文字集合のいずれかの文字か判定する述語を生成する
 *
 * @tparam Chars 文字集合（NTTP）
 */
template <FrozenString Chars>
inline constexpr auto is_in_chars = [](char c) noexcept {
  for (auto const x : Chars.sv()) {
    if (x == c) {
      return true;
    }
  }
  return false;
};

} // namespace detail

/**
 * @brief 文字列の左端から文字集合に含まれる文字を削除した文字列を生成する
 * Python の str.lstrip() 相当。デフォルトでは ASCII 空白を除去する
 *
 * @tparam Chars 削除する文字集合（デフォルト: ASCII 空白）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <FrozenString Chars = detail::default_strip_chars, size_t N>
[[nodiscard]] auto consteval lstrip(FrozenString<N> const& str) noexcept {
  return ltrim_if<detail::is_in_chars<Chars>>(str);
}

/**
 * @brief 文字列の右端から文字集合に含まれる文字を削除した文字列を生成する
 * Python の str.rstrip() 相当。デフォルトでは ASCII 空白を除去する
 *
 * @tparam Chars 削除する文字集合（デフォルト: ASCII 空白）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <FrozenString Chars = detail::default_strip_chars, size_t N>
[[nodiscard]] auto consteval rstrip(FrozenString<N> const& str) noexcept {
  return rtrim_if<detail::is_in_chars<Chars>>(str);
}

/**
 * @brief 文字列の両端から文字集合に含まれる文字を削除した文字列を生成する
 * Python の str.strip() 相当。デフォルトでは ASCII 空白を除去する
 *
 * @tparam Chars 削除する文字集合（デフォルト: ASCII 空白）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <FrozenString Chars = detail::default_strip_chars, size_t N>
[[nodiscard]] auto consteval strip(FrozenString<N> const& str) noexcept {
  return trim_if<detail::is_in_chars<Chars>>(str);
}

/**
 * @brief 文字列リテラルの左端から文字集合に含まれる文字を削除した文字列を生成する
 *
 * @tparam Chars 削除する文字集合（デフォルト: ASCII 空白）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <FrozenString Chars = detail::default_strip_chars, size_t N>
[[nodiscard]] auto consteval lstrip(char const (&str)[N]) noexcept {
  return lstrip<Chars>(FrozenString{str});
}

/**
 * @brief 文字列リテラルの右端から文字集合に含まれる文字を削除した文字列を生成する
 *
 * @tparam Chars 削除する文字集合（デフォルト: ASCII 空白）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <FrozenString Chars = detail::default_strip_chars, size_t N>
[[nodiscard]] auto consteval rstrip(char const (&str)[N]) noexcept {
  return rstrip<Chars>(FrozenString{str});
}

/**
 * @brief 文字列リテラルの両端から文字集合に含まれる文字を削除した文字列を生成する
 *
 * @tparam Chars 削除する文字集合（デフォルト: ASCII 空白）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 生成した文字列
 */
template <FrozenString Chars = detail::default_strip_chars, size_t N>
[[nodiscard]] auto consteval strip(char const (&str)[N]) noexcept {
  return strip<Chars>(FrozenString{str});
}

/**
 * @brief 先頭から指定文字集合に含まれる文字を削除する（char ポインタ版）
 */
template <FrozenString Chars = detail::default_strip_chars, typename Ptr>
  requires(std::same_as<std::remove_cvref_t<Ptr>, char const*> || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval lstrip(Ptr&& str) noexcept {
  return lstrip<Chars>(freeze(str));
}

/**
 * @brief 末尾から指定文字集合に含まれる文字を削除する（char ポインタ版）
 */
template <FrozenString Chars = detail::default_strip_chars, typename Ptr>
  requires(std::same_as<std::remove_cvref_t<Ptr>, char const*> || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval rstrip(Ptr&& str) noexcept {
  return rstrip<Chars>(freeze(str));
}

/**
 * @brief 先頭・末尾から指定文字集合に含まれる文字を削除する（char ポインタ版）
 */
template <FrozenString Chars = detail::default_strip_chars, typename Ptr>
  requires(std::same_as<std::remove_cvref_t<Ptr>, char const*> || std::same_as<std::remove_cvref_t<Ptr>, char*>)
[[nodiscard]] auto consteval strip(Ptr&& str) noexcept {
  return strip<Chars>(freeze(str));
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
    {"BOOLEAN", 7, "BOOL", 4},
    {"CHARACTER", 9, "CHAR", 4},
    {"DECIMAL", 7, "DEC", 3},
    {"INTEGER", 7, "INT", 3},
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
  constexpr auto OUT_CAP = (N > 0 ? N * 2 : 1);  // 最悪ケース: 全文字の間に改行が挿入され 2 倍になる
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

/*===============================================================================*\
 * 検索系: find / rfind / find_first_of / find_last_of / count_substring / reverse
\*===============================================================================*/

/**
 * @brief 文字列内で部分文字列が最初に出現する位置を返す
 *
 * @tparam Sub 検索する部分文字列 (FrozenString NTTP)
 * @tparam N 対象文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return std::size_t 見つかった位置、見つからなければ std::string_view::npos
 */
template <FrozenString Sub, size_t N>
[[nodiscard]] auto consteval find(FrozenString<N> const& str) noexcept -> std::size_t {
  return detail::find_impl(str, Sub);
}

/**
 * @brief 部分文字列を検索する（文字列リテラル版）
 */
template <FrozenString Sub, size_t N>
[[nodiscard]] auto consteval find(char const (&str)[N]) noexcept -> std::size_t {
  return find<Sub>(FrozenString{str});
}

/**
 * @brief 部分文字列を検索する（freeze 可能な任意の型を受け付ける汎用版）
 */
template <auto Sub>
  requires detail::is_frozen_string_v<decltype(Sub)>
[[nodiscard]] auto consteval find(auto const& str) noexcept -> std::size_t {
  return find<Sub>(freeze(str));
}

/**
 * @brief 文字列内で部分文字列が最後に出現する位置を返す
 *
 * @tparam Sub 検索する部分文字列 (FrozenString NTTP)
 * @tparam N 対象文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return std::size_t 見つかった位置、見つからなければ std::string_view::npos
 */
template <FrozenString Sub, size_t N>
[[nodiscard]] auto consteval rfind(FrozenString<N> const& str) noexcept -> std::size_t {
  return detail::rfind_substring(str, Sub);
}

/**
 * @brief 部分文字列を後方から検索する（文字列リテラル版）
 */
template <FrozenString Sub, size_t N>
[[nodiscard]] auto consteval rfind(char const (&str)[N]) noexcept -> std::size_t {
  return rfind<Sub>(FrozenString{str});
}

/**
 * @brief 部分文字列を後方から検索する（freeze 可能な任意の型を受け付ける汎用版）
 */
template <auto Sub>
  requires detail::is_frozen_string_v<decltype(Sub)>
[[nodiscard]] auto consteval rfind(auto const& str) noexcept -> std::size_t {
  return rfind<Sub>(freeze(str));
}

/**
 * @brief 文字集合のいずれかの文字が最初に出現する位置を返す
 *
 * @tparam Chars 検索する文字集合 (FrozenString NTTP)
 * @tparam N 対象文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return std::size_t 見つかった位置、見つからなければ std::string_view::npos
 */
template <FrozenString Chars, size_t N>
[[nodiscard]] auto consteval find_first_of(FrozenString<N> const& str) noexcept -> std::size_t {
  return detail::find_first_of_impl(str, Chars);
}

/**
 * @brief 指定文字集合のいずれかを前方から検索する（文字列リテラル版）
 */
template <FrozenString Chars, size_t N>
[[nodiscard]] auto consteval find_first_of(char const (&str)[N]) noexcept -> std::size_t {
  return find_first_of<Chars>(FrozenString{str});
}

/**
 * @brief 指定文字集合のいずれかを前方から検索する（freeze 可能な任意の型を受け付ける汎用版）
 */
template <auto Chars>
  requires detail::is_frozen_string_v<decltype(Chars)>
[[nodiscard]] auto consteval find_first_of(auto const& str) noexcept -> std::size_t {
  return find_first_of<Chars>(freeze(str));
}

/**
 * @brief 文字集合のいずれかの文字が最後に出現する位置を返す
 *
 * @tparam Chars 検索する文字集合 (FrozenString NTTP)
 * @tparam N 対象文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return std::size_t 見つかった位置、見つからなければ std::string_view::npos
 */
template <FrozenString Chars, size_t N>
[[nodiscard]] auto consteval find_last_of(FrozenString<N> const& str) noexcept -> std::size_t {
  return detail::find_last_of_impl(str, Chars);
}

/**
 * @brief 指定文字集合のいずれかを後方から検索する（文字列リテラル版）
 */
template <FrozenString Chars, size_t N>
[[nodiscard]] auto consteval find_last_of(char const (&str)[N]) noexcept -> std::size_t {
  return find_last_of<Chars>(FrozenString{str});
}

/**
 * @brief 指定文字集合のいずれかを後方から検索する（freeze 可能な任意の型を受け付ける汎用版）
 */
template <auto Chars>
  requires detail::is_frozen_string_v<decltype(Chars)>
[[nodiscard]] auto consteval find_last_of(auto const& str) noexcept -> std::size_t {
  return find_last_of<Chars>(freeze(str));
}

/**
 * @brief 部分文字列の重なり無し出現回数を返す
 *
 * @tparam Sub 検索する部分文字列 (FrozenString NTTP)
 * @tparam N 対象文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return std::size_t 出現回数 (Sub が空なら 0)
 */
template <FrozenString Sub, size_t N>
[[nodiscard]] auto consteval count_substring(FrozenString<N> const& str) noexcept -> std::size_t {
  return detail::count_substring_impl(str, Sub);
}

/**
 * @brief 部分文字列の出現回数を数える（文字列リテラル版）
 */
template <FrozenString Sub, size_t N>
[[nodiscard]] auto consteval count_substring(char const (&str)[N]) noexcept -> std::size_t {
  return count_substring<Sub>(FrozenString{str});
}

/**
 * @brief 部分文字列の出現回数を数える（freeze 可能な任意の型を受け付ける汎用版）
 */
template <auto Sub>
  requires detail::is_frozen_string_v<decltype(Sub)>
[[nodiscard]] auto consteval count_substring(auto const& str) noexcept -> std::size_t {
  return count_substring<Sub>(freeze(str));
}

/**
 * @brief 文字列を左右反転した FrozenString を生成する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return FrozenString<N> 反転した文字列
 */
template <size_t N>
[[nodiscard]] auto consteval reverse(FrozenString<N> const& str) noexcept -> FrozenString<N> {
  return detail::reverse_impl(str);
}

/**
 * @brief 文字列を逆順にする（文字列リテラル版）
 */
template <size_t N>
[[nodiscard]] auto consteval reverse(char const (&str)[N]) noexcept -> FrozenString<N> {
  return reverse(FrozenString{str});
}

/**
 * @brief 文字列を逆順にする（freeze 可能な任意の型を受け付ける汎用版）
 */
template <typename T>
  requires(!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
[[nodiscard]] auto consteval reverse(T const& v) noexcept {
  return reverse(freeze(v));
}

/*===============================================================================*\
 * 行単位整形: indent / dedent
\*===============================================================================*/

/**
 * @brief 各行の先頭に指定文字を指定個数付与する
 *
 * 空行（'\n' のみから成る行、または末尾空行）にはインデントを付与しない。
 *
 * @tparam IndentWidth 1行あたりのインデント文字数
 * @tparam IndentChar インデントに使う文字 (デフォルト: '\t')
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto インデント済み文字列
 */
template <size_t IndentWidth, char IndentChar = '\t', size_t N>
[[nodiscard]] auto consteval indent(FrozenString<N> const& str) noexcept {
  constexpr auto MAX_SIZE = std::max(N * (IndentWidth + 1) + 1, 2048uz);
  auto           res      = FrozenString<MAX_SIZE>{};
  auto           offset   = 0uz;
  auto           i        = 0uz;
  while (i < str.length) {
    auto line_start = i;
    while (i < str.length && str.buffer[i] != '\n')
      ++i;
    auto line_end = i;
    auto is_empty = (line_end == line_start);
    if (!is_empty) {
      for (auto k = 0uz; k < IndentWidth; ++k) {
        res.buffer[offset++] = IndentChar;
      }
    }
    for (auto j = line_start; j < line_end; ++j) {
      res.buffer[offset++] = str.buffer[j];
    }
    if (i < str.length && str.buffer[i] == '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
  }
  res.buffer[offset] = '\0';
  res.length         = offset;
  return res;
}

/**
 * @brief 各行にインデントを追加する（文字列リテラル版）
 */
template <size_t IndentWidth, char IndentChar = '\t', size_t N>
[[nodiscard]] auto consteval indent(char const (&str)[N]) noexcept {
  return indent<IndentWidth, IndentChar>(FrozenString{str});
}

/**
 * @brief 各行にインデントを追加する（freeze 可能な任意の型を受け付ける汎用版）
 */
template <size_t IndentWidth, char IndentChar = '\t', typename T>
  requires(!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
[[nodiscard]] auto consteval indent(T const& v) noexcept {
  return indent<IndentWidth, IndentChar>(freeze(v));
}

/**
 * @brief 各行の先頭に共通する空白文字を最大限削除する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return FrozenString<N> 共通先頭空白を削除した文字列
 */
template <size_t N>
[[nodiscard]] auto consteval dedent(FrozenString<N> const& str) noexcept -> FrozenString<N> {
  // pass 1: 全行の先頭空白最小値を求める
  auto common = str.length;
  auto i      = 0uz;
  while (i < str.length) {
    auto ws = 0uz;
    while (i < str.length && str.buffer[i] != '\n') {
      auto const c = str.buffer[i];
      if (c == ' ' || c == '\t') {
        ++ws;
        ++i;
      } else
        break;
    }
    if (i < str.length && str.buffer[i] != '\n') {
      if (ws < common)
        common = ws;
    }
    while (i < str.length && str.buffer[i] != '\n')
      ++i;
    if (i < str.length && str.buffer[i] == '\n')
      ++i;
  }
  if (common > str.length)
    common = 0;

  // pass 2: common 個の先頭空白を削除して出力
  auto res    = FrozenString<N>{};
  auto offset = 0uz;
  i           = 0uz;
  while (i < str.length) {
    auto       ws       = 0uz;
    auto const line_pos = i;
    while (i < str.length && str.buffer[i] != '\n') {
      auto const c = str.buffer[i];
      if (c == ' ' || c == '\t') {
        ++ws;
        ++i;
      } else
        break;
    }
    auto const skip = (ws < common) ? ws : common;
    i               = line_pos + skip;
    while (i < str.length && str.buffer[i] != '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
    if (i < str.length && str.buffer[i] == '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
  }
  res.buffer[offset] = '\0';
  res.length         = offset;
  return res;
}

/**
 * @brief 各行の共通インデントを削除する（文字列リテラル版）
 */
template <size_t N>
[[nodiscard]] auto consteval dedent(char const (&str)[N]) noexcept -> FrozenString<N> {
  return dedent(FrozenString{str});
}

/**
 * @brief 各行の共通インデントを削除する（freeze 可能な任意の型を受け付ける汎用版）
 */
template <typename T>
  requires(!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
[[nodiscard]] auto consteval dedent(T const& v) noexcept {
  return dedent(freeze(v));
}

namespace detail {

/**
 * @brief レーベンシュタイン距離の行バッファをスタック配列で持つ上限（要素数）
 *
 * 1 要素は std::size_t（8 バイト）。これを超えるとスタックオーバーフローの
 * リスクがあるため、混合版はヒープ（std::vector）へフォールバックする。
 */
inline constexpr std::size_t k_levenshtein_stack_cap = 2048uz;

/**
 * @brief 2 つの文字列の編集距離（レーベンシュタイン距離）を DP で計算する
 *
 * @tparam Cap 1 行分のバッファ容量（max(長さa, 長さb) + 1 以上）
 * @param a 一方の文字列
 * @param b もう一方の文字列
 * @return std::size_t 編集距離
 */
template <std::size_t Cap>
[[nodiscard]] constexpr auto levenshtein_distance_impl(std::string_view const a, std::string_view const b) noexcept -> std::size_t {
  auto prev = std::array<std::size_t, Cap>{};
  auto curr = std::array<std::size_t, Cap>{};
  for (auto j = 0uz; j <= b.size(); ++j) {
    prev[j] = j;
  }
  for (auto i = 1uz; i <= a.size(); ++i) {
    curr[0] = i;
    for (auto j = 1uz; j <= b.size(); ++j) {
      auto const cost = (a[i - 1] == b[j - 1]) ? 0uz : 1uz;
      curr[j] = std::min(curr[j - 1] + 1, std::min(prev[j] + 1, prev[j - 1] + cost));
    }
    std::swap(prev, curr);
  }
  return prev[b.size()];
}

/**
 * @brief 2 つの文字列の編集距離を上限付きで計算する（早期打ち切り版）
 *
 * 行の最小値が cutoff を超えた時点で cutoff+1 を返す。呼び出し側は
 * 戻り値 > cutoff で枝刈り判定できる。
 *
 * @tparam Cap 1 行分のバッファ容量（max(長さa, 長さb) + 1 以上）
 * @param a 一方の文字列
 * @param b もう一方の文字列
 * @param cutoff 許容する最大距離
 * @return std::size_t 編集距離。cutoff 超過時は cutoff+1（cutoff が SIZE_MAX のときは SIZE_MAX）
 */
template <std::size_t Cap>
[[nodiscard]] constexpr auto levenshtein_distance_impl_cutoff(
    std::string_view const a, std::string_view const b, std::size_t const cutoff) noexcept -> std::size_t {
  if (cutoff == std::numeric_limits<std::size_t>::max()) {
    return levenshtein_distance_impl<Cap>(a, b);
  }
  auto const len_diff = a.size() > b.size() ? a.size() - b.size() : b.size() - a.size();
  if (len_diff > cutoff) {
    return cutoff + 1;
  }
  auto prev = std::array<std::size_t, Cap>{};
  auto curr = std::array<std::size_t, Cap>{};
  for (auto j = 0uz; j <= b.size(); ++j) {
    prev[j] = j;
  }
  for (auto i = 1uz; i <= a.size(); ++i) {
    curr[0] = i;
    auto row_min = curr[0];
    for (auto j = 1uz; j <= b.size(); ++j) {
      auto const cost = (a[i - 1] == b[j - 1]) ? 0uz : 1uz;
      curr[j] = std::min(curr[j - 1] + 1, std::min(prev[j] + 1, prev[j - 1] + cost));
      row_min = std::min(row_min, curr[j]);
    }
    if (row_min > cutoff) {
      return cutoff + 1;
    }
    std::swap(prev, curr);
  }
  auto const result = prev[b.size()];
  return result > cutoff ? cutoff + 1 : result;
}

/**
 * @brief 複数の FrozenString に共通する最長共通接頭辞の長さを求める
 *
 * @tparam Strs 対象文字列（FrozenString NTTP）
 * @return std::size_t 最長共通接頭辞の長さ（空パックは 0）
 */
template <FrozenString... Strs>
[[nodiscard]] consteval auto lcp_length() noexcept -> std::size_t {
  if constexpr (sizeof...(Strs) == 0) {
    return 0;
  } else {
    constexpr std::array views{ Strs.sv()... };
    auto const min_len = std::min({Strs.sv().size()...});
    for (auto i = 0uz; i < min_len; ++i) {
      auto const c = views[0][i];
      if (!((Strs.sv()[i] == c) && ...)) {
        return i;
      }
    }
    return min_len;
  }
}

} // namespace detail

/**
 * @brief 2 つの FrozenString NTTP 間の編集距離（レーベンシュタイン距離）を計算する
 *
 * コマンドラインの誤字検出（"git comit" → "git commit" のサジェストなど）に利用できる。
 *
 * @tparam A 一方の文字列（FrozenString NTTP）
 * @tparam B もう一方の文字列（FrozenString NTTP）
 * @return std::size_t 編集距離
 */
template <FrozenString A, FrozenString B>
[[nodiscard]] consteval auto levenshtein_distance() noexcept -> std::size_t {
  return detail::levenshtein_distance_impl<std::max(A.size(), B.size()) + 1>(A.sv(), B.sv());
}

/**
 * @brief 2 つの FrozenString 間の編集距離（レーベンシュタイン距離）を計算する（引数版）
 *
 * @tparam N 一方のバッファ長（終端含む）
 * @tparam M もう一方のバッファ長（終端含む）
 * @param a 一方の文字列
 * @param b もう一方の文字列
 * @return std::size_t 編集距離
 */
template <std::size_t N, std::size_t M>
[[nodiscard]] auto consteval levenshtein_distance(FrozenString<N> const& a, FrozenString<M> const& b) noexcept -> std::size_t {
  return detail::levenshtein_distance_impl<std::max(N, M) + 1>(a.sv(), b.sv());
}

/**
 * @brief 2 つの文字列の編集距離（レーベンシュタイン距離）を計算する（実行時 string_view 版）
 *
 * コンパイル時引数が必要な consteval 版とは異なり、実行時の std::string_view を直接受け取れる。
 * C++23 の constexpr std::vector により、コンパイル時にも実行時にも動作する。
 *
 * @param a 一方の文字列
 * @param b もう一方の文字列
 * @return std::size_t 編集距離
 */
[[nodiscard]] constexpr auto levenshtein_distance(std::string_view const a, std::string_view const b) -> std::size_t {
  std::vector<std::size_t> prev(b.size() + 1);
  std::vector<std::size_t> curr(b.size() + 1);
  for (auto j = 0uz; j <= b.size(); ++j) {
    prev[j] = j;
  }
  for (auto i = 1uz; i <= a.size(); ++i) {
    curr[0] = i;
    for (auto j = 1uz; j <= b.size(); ++j) {
      auto const cost = (a[i - 1] == b[j - 1]) ? 0uz : 1uz;
      curr[j] = std::min(curr[j - 1] + 1, std::min(prev[j] + 1, prev[j - 1] + cost));
    }
    std::swap(prev, curr);
  }
  return prev[b.size()];
}

/**
 * @brief 2 つの文字列の編集距離を上限付きで計算する（実行時 string_view 版・早期打ち切り）
 *
 * cutoff を超えたことが確定した時点で cutoff+1 を返す。
 *
 * @param a 一方の文字列
 * @param b もう一方の文字列
 * @param cutoff 許容する最大距離
 * @return std::size_t 編集距離。cutoff 超過時は cutoff+1
 */
[[nodiscard]] constexpr auto levenshtein_distance(
    std::string_view const a, std::string_view const b, std::size_t const cutoff) -> std::size_t {
  if (cutoff == std::numeric_limits<std::size_t>::max()) {
    return levenshtein_distance(a, b);
  }
  auto const len_diff = a.size() > b.size() ? a.size() - b.size() : b.size() - a.size();
  if (len_diff > cutoff) {
    return cutoff + 1;
  }
  std::vector<std::size_t> prev(b.size() + 1);
  std::vector<std::size_t> curr(b.size() + 1);
  for (auto j = 0uz; j <= b.size(); ++j) {
    prev[j] = j;
  }
  for (auto i = 1uz; i <= a.size(); ++i) {
    curr[0] = i;
    auto row_min = curr[0];
    for (auto j = 1uz; j <= b.size(); ++j) {
      auto const cost = (a[i - 1] == b[j - 1]) ? 0uz : 1uz;
      curr[j] = std::min(curr[j - 1] + 1, std::min(prev[j] + 1, prev[j - 1] + cost));
      row_min = std::min(row_min, curr[j]);
    }
    if (row_min > cutoff) {
      return cutoff + 1;
    }
    std::swap(prev, curr);
  }
  auto const result = prev[b.size()];
  return result > cutoff ? cutoff + 1 : result;
}

/**
 * @brief FrozenString NTTP と実行時文字列の編集距離（レーベンシュタイン距離）を計算する（混合版）
 *
 * 片方がコンパイル時既知なので、DP の行バッファを静的な std::array で持てる（ヒープ確保なし）。
 * レーベンシュタイン距離は対称なので、実行時文字列を外側・NTTP を内側に回して行サイズを固定する。
 *
 * NTTP 文字列が StackCap を超える場合は、スタック配列ではオーバーフローしうるため
 * 実行時版（std::vector、ヒープ確保）へ自動フォールバックする。その場合のみ noexcept が外れる。
 *
 * @tparam A 一方の文字列（FrozenString NTTP）
 * @tparam StackCap 行バッファをスタック配列で持つ上限（要素数）。デフォルトは
 *   detail::k_levenshtein_stack_cap。小さい NTTP でもヒープ版を強制できる。
 * @param b もう一方の文字列（実行時）
 * @return std::size_t 編集距離
 */
template <FrozenString A, std::size_t StackCap = detail::k_levenshtein_stack_cap>
[[nodiscard]] constexpr auto levenshtein_distance(std::string_view const b) noexcept(A.size() + 1 <= StackCap)
    -> std::size_t {
  if constexpr (A.size() + 1 <= StackCap) {
    return detail::levenshtein_distance_impl<A.size() + 1>(b, A.sv());
  } else {
    return levenshtein_distance(A.sv(), b);
  }
}

/**
 * @brief FrozenString NTTP と実行時文字列の編集距離を上限付きで計算する（混合版・早期打ち切り）
 *
 * cutoff を超えたことが確定した時点で cutoff+1 を返す。StackCap 以内なら
 * スタック配列版、超過時はヒープ版へフォールバックする。
 *
 * @tparam A 一方の文字列（FrozenString NTTP）
 * @tparam StackCap 行バッファをスタック配列で持つ上限
 * @param b もう一方の文字列（実行時）
 * @param cutoff 許容する最大距離
 * @return std::size_t 編集距離。cutoff 超過時は cutoff+1
 */
template <FrozenString A, std::size_t StackCap = detail::k_levenshtein_stack_cap>
[[nodiscard]] constexpr auto levenshtein_distance(
    std::string_view const b, std::size_t const cutoff) noexcept(A.size() + 1 <= StackCap) -> std::size_t {
  if constexpr (A.size() + 1 <= StackCap) {
    return detail::levenshtein_distance_impl_cutoff<A.size() + 1>(b, A.sv(), cutoff);
  } else {
    return levenshtein_distance(A.sv(), b, cutoff);
  }
}

/**
 * @brief 実行時入力に最も近い候補文字列を返す（CLI 誤字サジェスト用）
 *
 * 各候補との編集距離を計算し、最小距離が max_distance 以下ならその候補を返す。
 * 同距離の候補が複数ある場合は宣言順で最初のものを返す。
 * 候補が空、または距離が閾値を超える場合は std::nullopt を返す。
 *
 * 候補は NTTP なので長さがコンパイル時既知 → 距離の下限（|長さ差|）による枝刈りが効く。
 * 内部で呼ぶ levenshtein_distance はデフォルトの StackCap を使用する（候補はコマンド名程度の
 * 短い文字列が前提で、ヒープフォールバックは実質発生しない）。
 *
 * @tparam Candidates 候補文字列（FrozenString NTTP）
 * @param input 実行時の入力文字列（CLI 引数など）
 * @param max_distance 許容する最大編集距離
 * @return std::optional<std::string_view> 最も近い候補、該当なしは std::nullopt
 */
template <FrozenString... Candidates>
[[nodiscard]] constexpr auto suggest(std::string_view const input, std::size_t const max_distance) noexcept(
    ((Candidates.size() + 1 <= detail::k_levenshtein_stack_cap) && ...))
    -> std::optional<std::string_view> {
  std::optional<std::string_view> best;
  std::optional<std::size_t> best_dist;
  auto consider = [&]<FrozenString Candidate>() {
    if (best_dist && *best_dist == 0) {
      return;  // 既に完全一致が見つかっている
    }
    auto const candidate = std::string_view{Candidate.buffer.data(), Candidate.length};
    auto const cutoff = best_dist ? std::min(max_distance, *best_dist - 1) : max_distance;
    auto const len_diff = input.size() > candidate.size() ? input.size() - candidate.size()
                                                          : candidate.size() - input.size();
    if (len_diff > cutoff) {
      return;  // 距離は |長さ差| 以上なので、この候補は絶対に閾値/現bestを超える
    }
    auto const dist = levenshtein_distance<Candidate>(input, cutoff);
    if (dist > cutoff) {
      return;  // 早期打ち切り: cutoff 超過
    }
    if (!best_dist || dist < *best_dist) {
      best_dist = dist;
      best = candidate;
    }
  };
  (void)consider;
  (consider.template operator()<Candidates>(), ...);
  return best;
}

/**
 * @brief 複数の FrozenString に共通する最長共通接頭辞（Longest Common Prefix）を抽出する
 *
 * 空パックは空文字列、単一要素はその文字列自身を返す。
 *
 * @tparam Strs 対象文字列（FrozenString NTTP）
 * @return FrozenString<L + 1> 最長共通接頭辞
 */
template <FrozenString... Strs>
[[nodiscard]] consteval auto lcp() noexcept -> FrozenString<detail::lcp_length<Strs...>() + 1> {
  constexpr auto LEN = detail::lcp_length<Strs...>();
  auto res = FrozenString<LEN + 1>{};
  if constexpr (sizeof...(Strs) > 0) {
    constexpr std::array views{ Strs.sv()... };
    for (auto i = 0uz; i < LEN; ++i) {
      res.buffer[i] = views[0][i];
    }
  }
  res.buffer[LEN] = '\0';
  res.length = LEN;
  return res;
}

}  // namespace frozenchars
