#pragma once

#include "char_utils.hpp"
#include <cstddef>
#include <string_view>

namespace frozenchars {

template <size_t N> struct FrozenString;
}

namespace frozenchars::detail {

/**
 * @brief 部分文字列を検索する
 *
 * @tparam N FrozenStringの長さ (終端文字'\0'を含む)
 * @param str 処理対象の文字列
 * @param needle 検索する部分文字列
 * @param pos 検索開始位置
 * @return std::size_t 見つかった位置、見つからなかった場合はstd::string_view::npos
 */
template <size_t N>
[[nodiscard]] auto consteval find_substring(FrozenString<N> const& str,
                              std::string_view needle,
                              std::size_t pos = 0uz) noexcept -> std::size_t {
  if (needle.empty()) {
    return pos <= str.length ? pos : std::string_view::npos;
  }
  if (needle.size() > str.length || pos > str.length - needle.size()) {
    return std::string_view::npos;
  }

  auto const last = str.length - needle.size();
  for (auto i = pos; i <= last; ++i) {
    auto matched = true;
    for (auto j = 0uz; j < needle.size(); ++j) {
      if (str.buffer[i + j] != needle[j]) {
        matched = false;
        break;
      }
    }
    if (matched) {
      return i;
    }
  }
  return std::string_view::npos;
}

/**
 * @brief snake_case変換時に挿入されるアンダースコアの数を計算する
 *
 * @tparam N FrozenStringの長さ (終端文字'\0'を含む)
 * @param str 処理対象の文字列
 * @return std::size_t 挿入されるアンダースコアの数
 */
template <size_t N>
[[nodiscard]] auto consteval count_snake_underscores(FrozenString<N> const& str) noexcept -> std::size_t {
  auto count = 0uz;
  for (auto i = 1uz; i < str.length; ++i) {
    if (str.buffer[i] >= 'A' && str.buffer[i] <= 'Z') {
      ++count;
    }
  }
  return count;
}

/**
 * @brief 引数の文字列から指定した条件を満たす文字を左端と右端から削除した文字列を生成する
 *
 * @tparam TrimLeft 左端から削除するかどうか
 * @tparam TrimRight 右端から削除するかどうか
 * @tparam Pred 削除対象を判定する述語（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <bool TrimLeft, bool TrimRight, auto Pred = is_space_char, size_t N>
[[nodiscard]] auto consteval trim_copy(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto start = 0uz;
  auto end = str.length;

  if constexpr (TrimLeft) {
    while (start < str.length && Pred(str.buffer[start])) {
      ++start;
    }
  }
  if constexpr (TrimRight) {
    while (end > start && Pred(str.buffer[end - 1])) {
      --end;
    }
  }

  auto const out_len = end - start;
  for (auto i = 0uz; i < out_len; ++i) {
    res.buffer[i] = str.buffer[start + i];
  }
  res.buffer[out_len] = '\0';
  res.length = out_len;
  return res;
}

/**
 * @brief 文字列を末尾から前方へ検索し、最後の出現位置を返す
 *
 * @tparam N 対象文字列の長さ (終端 '\0' を含む)
 * @tparam M 検索文字列の長さ (終端 '\0' を含む)
 * @param str 対象文字列
 * @param needle 検索する部分文字列
 * @param pos 検索を開始する末尾側インデックス (指定位置以前の最も後ろを探す)
 * @return std::size_t 見つかった位置、見つからなければ std::string_view::npos
 */
template <size_t N, size_t M>
[[nodiscard]] auto consteval rfind_substring(FrozenString<N> const& str, FrozenString<M> const& needle, std::size_t pos = std::string_view::npos) noexcept -> std::size_t {
  if (needle.length == 0) {
    auto const start = (pos == std::string_view::npos || pos > str.length) ? str.length : pos;
    return start;
  }
  if (needle.length > str.length) {
    return std::string_view::npos;
  }
  auto const upper = (pos == std::string_view::npos || pos > str.length - needle.length) ? str.length - needle.length : pos;
  for (auto i = upper + 1; i-- > 0;) {
    bool match = true;
    for (auto j = 0uz; j < needle.length; ++j) {
      if (str.buffer[i + j] != needle.buffer[j]) {
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
 * @brief 文字集合のいずれかの文字が最初に出現する位置を返す
 *
 * @tparam N 対象文字列の長さ (終端 '\0' を含む)
 * @tparam M 文字集合の長さ (終端 '\0' を含む)
 * @param str 対象文字列
 * @param chars 検索する文字集合
 * @param pos 検索開始位置
 * @return std::size_t 見つかった位置、見つからなければ std::string_view::npos
 */
template <size_t N, size_t M>
[[nodiscard]] auto consteval find_first_of_impl(FrozenString<N> const& str, FrozenString<M> const& chars, std::size_t pos = 0) noexcept -> std::size_t {
  if (chars.length == 0) {
    return std::string_view::npos;
  }
  for (auto i = pos; i < str.length; ++i) {
    for (auto j = 0uz; j < chars.length; ++j) {
      if (str.buffer[i] == chars.buffer[j]) {
        return i;
      }
    }
  }
  return std::string_view::npos;
}

/**
 * @brief 文字集合のいずれかの文字が最後に出現する位置を返す
 *
 * @tparam N 対象文字列の長さ (終端 '\0' を含む)
 * @tparam M 文字集合の長さ (終端 '\0' を含む)
 * @param str 対象文字列
 * @param chars 検索する文字集合
 * @param pos 検索を開始する末尾側インデックス
 * @return std::size_t 見つかった位置、見つからなければ std::string_view::npos
 */
template <size_t N, size_t M>
[[nodiscard]] auto consteval find_last_of_impl(FrozenString<N> const& str, FrozenString<M> const& chars, std::size_t pos = std::string_view::npos) noexcept -> std::size_t {
  if (chars.length == 0) {
    return std::string_view::npos;
  }
  auto const start = (pos == std::string_view::npos || pos >= str.length) ? str.length : pos + 1;
  for (auto i = start; i-- > 0;) {
    for (auto j = 0uz; j < chars.length; ++j) {
      if (str.buffer[i] == chars.buffer[j]) {
        return i;
      }
    }
  }
  return std::string_view::npos;
}

/**
 * @brief 部分文字列の重なり無し出現回数を数える
 *
 * @tparam N 対象文字列の長さ (終端 '\0' を含む)
 * @tparam M 検索文字列の長さ (終端 '\0' を含む)
 * @param str 対象文字列
 * @param needle 検索する部分文字列
 * @return std::size_t 出現回数 (needle が空なら 0)
 */
template <size_t N, size_t M>
[[nodiscard]] auto consteval count_substring_impl(FrozenString<N> const& str, FrozenString<M> const& needle) noexcept -> std::size_t {
  if (needle.length == 0) {
    return 0;
  }
  auto count = 0uz;
  auto pos  = 0uz;
  while (pos <= str.length) {
    auto const found = find_substring(str, needle.sv(), pos);
    if (found == std::string_view::npos) {
      break;
    }
    ++count;
    pos = found + needle.length;
  }
  return count;
}

/**
 * @brief 文字列のバイト列を左右反転した FrozenString を生成する
 *
 * @tparam N 文字列の長さ (終端 '\0' を含む)
 * @param str 対象文字列
 * @return FrozenString<N> 反転された文字列
 */
template <size_t N>
[[nodiscard]] auto consteval reverse_impl(FrozenString<N> const& str) noexcept -> FrozenString<N> {
  auto res = FrozenString<N>{};
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[i] = str.buffer[str.length - 1 - i];
  }
  res.buffer[str.length] = '\0';
  res.length = str.length;
  return res;
}

} // namespace frozenchars::detail
