#pragma once

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
auto consteval find_substring(FrozenString<N> const& str,
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
auto consteval count_snake_underscores(FrozenString<N> const& str) noexcept -> std::size_t {
  auto count = 0uz;
  for (auto i = 1uz; i < str.length; ++i) {
    if (str.buffer[i] >= 'A' && str.buffer[i] <= 'Z') { ++count; }
  }
  return count;
}

/**
 * @brief 引数の文字列から指定した文字を左端と右端から削除した文字列を生成する
 *
 * @tparam TrimLeft 左端から削除するかどうか
 * @tparam TrimRight 右端から削除するかどうか
 * @tparam TrimChar 削除する文字
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 生成した文字列
 */
template <bool TrimLeft, bool TrimRight, char TrimChar, size_t N>
auto consteval trim_copy(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto start = 0uz;
  auto end = str.length;

  if constexpr (TrimLeft) {
    while (start < str.length && str.buffer[start] == TrimChar) {
      ++start;
    }
  }
  if constexpr (TrimRight) {
    while (end > start && str.buffer[end - 1] == TrimChar) {
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

} // namespace frozenchars::detail
