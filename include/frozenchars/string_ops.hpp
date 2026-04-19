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

template <size_t Width, char Fill, size_t N>
auto consteval pad_left(FrozenString<N> const& str) noexcept;

template <size_t Width, char Fill, size_t N>
auto consteval pad_left(char const (&str)[N]) noexcept;

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
  return pad_left<Width, Fill>(str);
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
auto consteval center(FrozenString<N> const& str) noexcept {
  auto constexpr OUT_CAP = N > (Width + 1) ? N : (Width + 1);
  auto res = FrozenString<OUT_CAP>{};

  auto const out_len = std::max(str.length, Width);
  auto const total_pad = out_len - str.length;
  auto const left_pad = total_pad / 2;
  auto const right_pad = total_pad - left_pad;

  for (auto i = 0uz; i < left_pad; ++i) {
    res.buffer[i] = Fill;
  }
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[left_pad + i] = str.buffer[i];
  }
  for (auto i = 0uz; i < right_pad; ++i) {
    res.buffer[left_pad + str.length + i] = Fill;
  }

  res.buffer[out_len] = '\0';
  res.length = out_len;
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

/**
 * @brief 文字列の部分文字列を生成する関数
 *
 * @tparam Pos 開始位置
 * @tparam Len 文字数。負の場合は Pos の左側から abs(Len) 文字
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <size_t Pos, std::ptrdiff_t Len, size_t N>
auto consteval substr(FrozenString<N> const& str) noexcept {
  auto constexpr REQUESTED_LEN = Len >= 0 ? static_cast<size_t>(Len) : static_cast<size_t>(-Len);
  auto res = FrozenString<REQUESTED_LEN + 1>{};
  auto const anchor = std::min(Pos, str.length);
  auto start = anchor;
  auto actual_len = 0uz;

  if constexpr (Len >= 0) {
    actual_len = anchor < str.length ? std::min(REQUESTED_LEN, str.length - anchor) : 0uz;
  } else {
    start = anchor > REQUESTED_LEN ? anchor - REQUESTED_LEN : 0uz;
    actual_len = anchor - start;
  }

  for (auto i = 0uz; i < actual_len; ++i) {
    res.buffer[i] = str.buffer[start + i];
  }
  res.buffer[actual_len] = '\0';
  res.length = actual_len;
  return res;
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
  return substr<Pos, Len>(FrozenString{str});
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
    start = anchor > requested_len ? anchor - requested_len : 0uz;
    actual_len = anchor - start;
  }

  for (auto i = 0uz; i < actual_len; ++i) {
    res.buffer[i] = str.buffer[start + i];
  }
  res.buffer[actual_len] = '\0';
  res.length = actual_len;
  return res;
}

template <char TrimChar = ' ', size_t N>
auto consteval ltrim(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<true, false, TrimChar>(str);
}

template <char TrimChar = ' ', size_t N>
auto consteval rtrim(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<false, true, TrimChar>(str);
}

template <char TrimChar = ' ', size_t N>
auto consteval trim(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<true, true, TrimChar>(str);
}

template <char TrimChar = ' ', size_t N>
auto consteval ltrim(char const (&str)[N]) noexcept {
  return ltrim<TrimChar>(FrozenString{str});
}

template <char TrimChar = ' ', size_t N>
auto consteval rtrim(char const (&str)[N]) noexcept {
  return rtrim<TrimChar>(FrozenString{str});
}

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

template <auto IsDelimiter, size_t N>
auto consteval split_count(FrozenString<N> const& str) noexcept;

template <auto IsDelimiter, size_t N>
auto consteval split(FrozenString<N> const& str) noexcept;

/**
 * @brief 引数なしのjoin関数は空文字を返す
 *
 * @tparam Delim 区切り文字列
 * @return auto 結合結果
 */
template <FrozenString Delim>
auto consteval join() noexcept {
  return FrozenString<1>{};
}

// join - homogeneous std::array of FrozenStrings
/**
 * @brief std::array 内の FrozenString を Delim で結合する
 * std::arrayは要素数が一致している
 *
 * @tparam Delim 区切り文字列
 * @tparam ElemN FrozenStringの要素数 (終端文字'\0'を含む)
 * @tparam Count std::arrayの要素数
 * @param arr 結合する FrozenString の std::array
 * @return auto 結合結果
 */
template <FrozenString Delim, size_t ElemN, size_t Count>
auto consteval join(std::array<FrozenString<ElemN>, Count> const& arr) noexcept {
  if constexpr (Count == 0) {
    return FrozenString<1>{};
  } else {
    auto constexpr DELIM_LEN = Delim.sv().size();
    auto constexpr MAX_ELEM = ElemN > 0 ? ElemN - 1 : 0;
    auto constexpr OUT_CAP = Count * MAX_ELEM + (Count - 1) * DELIM_LEN + 1;
    auto res = FrozenString<OUT_CAP>{};
    auto offset = 0uz;
    for (auto i = 0uz; i < Count; ++i) {
      if (i > 0) {
        for (auto const c : Delim.sv()) {
          res.buffer[offset++] = c;
        }
      }
      for (auto j = 0uz; j < arr[i].length; ++j) {
        res.buffer[offset++] = arr[i].buffer[j];
      }
    }
    res.buffer[offset] = '\0';
    res.length = offset;
    return res;
  }
}

// join - variadic heterogeneous FrozenStrings
/**
 * @brief 複数の FrozenString を Delim で結合する
 *
 * @tparam Delim 区切り文字列
 * @tparam N0 最初の FrozenString の長さ (終端文字'\0'を含む)
 * @tparam Ns 残りの FrozenString の長さ (終端文字'\0'を含む)
 * @param first 最初の FrozenString
 * @param rest 残りの FrozenString（可変引数）
 * @return auto 結合結果
 */
template <FrozenString Delim, size_t N0, size_t... Ns>
auto consteval join(FrozenString<N0> const& first, FrozenString<Ns> const&... rest) noexcept {
  auto constexpr DELIM_LEN = Delim.sv().size();
  auto constexpr COUNT = 1uz + sizeof...(Ns);
  auto constexpr MAX_ELEMS = (N0 > 0 ? N0 - 1 : 0) + (0uz + ... + (Ns > 0 ? Ns - 1 : 0));
  auto constexpr DELIM_TOTAL = COUNT > 1 ? (COUNT - 1) * DELIM_LEN : 0uz;
  auto constexpr OUT_CAP = MAX_ELEMS + DELIM_TOTAL + 1;

  auto res = FrozenString<OUT_CAP>{};
  auto offset = 0uz;

  for (auto i = 0uz; i < first.length; ++i) {
    res.buffer[offset++] = first.buffer[i];
  }
  ([&](auto const& elem) {
    for (auto const c : Delim.sv()) {
      res.buffer[offset++] = c;
    }
    for (auto i = 0uz; i < elem.length; ++i) {
      res.buffer[offset++] = elem.buffer[i];
    }
  }(rest), ...);

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 左側に指定した幅まで Fill 文字を追加してパディングする
 *
 * @tparam Width 出力文字列の幅
 * @tparam Fill パディングに使用する文字
 * @tparam N 入力文字列の長さ (終端文字'\0'を含む)
 * @param str 入力文字列
 * @return auto パディング後の文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval pad_left(FrozenString<N> const& str) noexcept {
  auto constexpr OUT_CAP = N > (Width + 1) ? N : (Width + 1);
  auto res = FrozenString<OUT_CAP>{};

  auto const out_len = std::max(str.length, Width);
  auto const left_pad = out_len - str.length;

  for (auto i = 0uz; i < left_pad; ++i) {
    res.buffer[i] = Fill;
  }
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[left_pad + i] = str.buffer[i];
  }

  res.buffer[out_len] = '\0';
  res.length = out_len;
  return res;
}

template <size_t Width, char Fill = ' ', size_t N>
auto consteval pad_left(char const (&str)[N]) noexcept {
  return pad_left<Width, Fill>(FrozenString{str});
}

/**
 * @brief 右側に指定した幅まで Fill 文字を追加してパディングする
 *
 * @tparam Width 出力文字列の幅
 * @tparam Fill パディングに使用する文字
 * @tparam N 入力文字列の長さ (終端文字'\0'を含む)
 * @param str 入力文字列
 * @return auto パディング後の文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval pad_right(FrozenString<N> const& str) noexcept {
  auto constexpr OUT_CAP = N > (Width + 1) ? N : (Width + 1);
  auto res = FrozenString<OUT_CAP>{};
  auto const out_len = std::max(str.length, Width);
  auto const right_pad = out_len - str.length;
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[i] = str.buffer[i];
  }
  for (auto i = 0uz; i < right_pad; ++i) {
    res.buffer[str.length + i] = Fill;
  }
  res.buffer[out_len] = '\0';
  res.length = out_len;
  return res;
}

template <size_t Width, char Fill = ' ', size_t N>
auto consteval pad_right(char const (&str)[N]) noexcept {
  return pad_right<Width, Fill>(FrozenString{str});
}

/**
 * @brief 文字列中の最初の From を To に置換する
 *
 * @tparam From 置換対象の文字列
 * @tparam To 置換後の文字列
 * @tparam N 入力文字列の長さ (終端文字'\0'を含む)
 * @param str 入力文字列
 * @return auto 置換後の文字列
 */
template <FrozenString From, FrozenString To, size_t N>
auto consteval replace(FrozenString<N> const& str) noexcept {
  static_assert(From.sv().size() > 0, "replace: From must not be empty");
  auto constexpr FROM_LEN = From.sv().size();
  auto constexpr TO_LEN = To.sv().size();
  auto constexpr EXTRA = TO_LEN > FROM_LEN ? TO_LEN - FROM_LEN : 0uz;
  auto constexpr OUT_CAP = N + EXTRA;

  auto res = FrozenString<OUT_CAP>{};
  auto const from_sv = From.sv();
  auto const to_sv = To.sv();
  auto const match_pos = detail::find_substring(str, from_sv);
  if (match_pos == std::string_view::npos) {
    for (auto i = 0uz; i < str.length; ++i) {
      res.buffer[i] = str.buffer[i];
    }
    res.buffer[str.length] = '\0';
    res.length = str.length;
    return res;
  }
  auto offset = 0uz;
  for (auto i = 0uz; i < match_pos; ++i) {
    res.buffer[offset++] = str.buffer[i];
  }
  for (auto const c : to_sv) {
    res.buffer[offset++] = c;
  }
  auto after = match_pos + FROM_LEN;
  while (after < str.length) {
    res.buffer[offset++] = str.buffer[after++];
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
 * @brief 文字列中のすべての From を To に置換する
 *
 * @tparam From 置換対象の文字列
 * @tparam To 置換後の文字列
 * @tparam N 入力文字列の長さ (終端文字'\0'を含む)
 * @param str 入力文字列
 * @return auto 置換後の文字列
 */
template <FrozenString From, FrozenString To, size_t N>
auto consteval replace_all(FrozenString<N> const& str) noexcept {
  static_assert(From.sv().size() > 0, "replace_all: From must not be empty");
  auto constexpr FROM_LEN = From.sv().size();
  auto constexpr TO_LEN = To.sv().size();
  auto constexpr MAX_STR_LEN = N > 0 ? N - 1 : 0;
  auto constexpr MAX_MATCHES = MAX_STR_LEN / FROM_LEN;
  auto constexpr EXTRA = TO_LEN > FROM_LEN ? (TO_LEN - FROM_LEN) * MAX_MATCHES : 0uz;
  auto constexpr OUT_CAP = N + EXTRA;

  auto res = FrozenString<OUT_CAP>{};
  auto const from_sv = From.sv();
  auto const to_sv = To.sv();
  auto offset = 0uz;
  auto pos = 0uz;
  while (pos < str.length) {
    auto const found = detail::find_substring(str, from_sv, pos);
    if (found == std::string_view::npos) {
      while (pos < str.length) {
        res.buffer[offset++] = str.buffer[pos++];
      }
      break;
    }
    while (pos < found) {
      res.buffer[offset++] = str.buffer[pos++];
    }
    for (auto const c : to_sv) {
      res.buffer[offset++] = c;
    }
    pos = found + FROM_LEN;
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

template <FrozenString Delim, typename First, typename... Rest>
  requires (requires(First const& v) { freeze(v); }
            && (... && requires(Rest const& v) { freeze(v); })
            && (!detail::is_frozen_string_v<First> || (... || !detail::is_frozen_string_v<Rest>)))
auto consteval join(First const& first, Rest const&... rest) noexcept {
  return join<Delim>(freeze(first), freeze(rest)...);
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

} // namespace frozenchars
