#pragma once

#include "string.hpp"
#include "string_ops.hpp"
#include "detail/char_utils.hpp"

namespace frozenchars::path {

/**
 * @brief パス文字列から親ディレクトリ部分を切り出す
 *
 * 最後の '/' より前を返す。'/' が見つからない場合は "." を返す。
 * 末尾連続 '/' は 1 個に正規化されたものとみなす。
 *
 * @tparam N パス文字列の長さ (終端文字'\0'を含む)
 * @param path_str 対象パス
 * @return FrozenString<(N < 2) ? 2 : N> 親ディレクトリ
 */
template <size_t N>
[[nodiscard]] auto consteval dirname(FrozenString<N> const& path_str) noexcept -> FrozenString<(N < 2) ? 2 : N> {
  constexpr auto M = (N < 2) ? 2 : N;
  auto const slash_pos = detail::find_last_of_impl(path_str, FrozenString<2>{"/"});
  if (slash_pos == std::string_view::npos) {
    auto res = FrozenString<M>{};
    res.buffer[0] = '.';
    if constexpr (M > 1) { res.buffer[1] = '\0'; }
    res.length = 1;
    return res;
  }
  if (slash_pos == 0) {
    auto res = FrozenString<M>{};
    res.buffer[0] = '/';
    if constexpr (M > 1) { res.buffer[1] = '\0'; }
    res.length = 1;
    return res;
  }
  auto res = FrozenString<M>{};
  for (auto i = 0uz; i < slash_pos; ++i) {
    res.buffer[i] = path_str.buffer[i];
  }
  res.buffer[slash_pos] = '\0';
  res.length = slash_pos;
  return res;
}

template <size_t N>
[[nodiscard]] auto consteval dirname(char const (&path_str)[N]) noexcept {
  return dirname(FrozenString{path_str});
}

/**
 * @brief パス文字列からベース名（末尾要素）を切り出す
 *
 * 最後の '/' 以降の部分を返す。'/' がない場合は全体を返す。
 *
 * @tparam N パス文字列の長さ (終端文字'\0'を含む)
 * @param path_str 対象パス
 * @return FrozenString<N> ベース名
 */
template <size_t N>
[[nodiscard]] auto consteval basename(FrozenString<N> const& path_str) noexcept -> FrozenString<N> {
  auto const slash_pos = detail::rfind_substring(path_str, FrozenString<2>{"/"});
  if (slash_pos == std::string_view::npos) {
    auto res = FrozenString<N>{};
    for (auto i = 0uz; i < path_str.length; ++i) {
      res.buffer[i] = path_str.buffer[i];
    }
    res.buffer[path_str.length] = '\0';
    res.length = path_str.length;
    return res;
  }
  auto const start = slash_pos + 1;
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  for (auto i = start; i < path_str.length; ++i) {
    res.buffer[offset++] = path_str.buffer[i];
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
[[nodiscard]] auto consteval basename(char const (&path_str)[N]) noexcept -> FrozenString<N> {
  return basename(FrozenString{path_str});
}

/**
 * @brief パス文字列から拡張子を切り出す
 *
 * basename 中の最後の '.' 以降を返す。ただし先頭の '.' はドットファイルとして扱い無視する。
 * 拡張子がない場合は空文字列を返す。
 *
 * @tparam N パス文字列の長さ (終端文字'\0'を含む)
 * @param path_str 対象パス
 * @return FrozenString<N> 拡張子（'.' を含む）、無ければ空文字列
 */
template <size_t N>
[[nodiscard]] auto consteval extension(FrozenString<N> const& path_str) noexcept -> FrozenString<N> {
  auto const name = basename(path_str);
  if (name.length == 0) return FrozenString<N>{};
  // Find last dot after position 0 (skip leading dot for dotfiles)
  auto dot_pos = std::string_view::npos;
  for (auto i = 1uz; i < name.length; ++i) {
    if (name.buffer[i] == '.') {
      dot_pos = i;
    }
  }
  if (dot_pos == std::string_view::npos) {
    return FrozenString<N>{};
  }
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  for (auto i = dot_pos; i < name.length; ++i) {
    res.buffer[offset++] = name.buffer[i];
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
[[nodiscard]] auto consteval extension(char const (&path_str)[N]) noexcept -> FrozenString<N> {
  return extension(FrozenString{path_str});
}

/**
 * @brief パス文字列から拡張子を除いた主体部を取り出す
 *
 * basename 中の最後の '.' より前を返す。
 * ドットファイル（先頭 '.'）は拡張子の対象としない。
 *
 * @tparam N パス文字列の長さ (終端文字'\0'を含む)
 * @param path_str 対象パス
 * @return FrozenString<N> 主体部（拡張子を含まない basename）
 */
template <size_t N>
[[nodiscard]] auto consteval stem(FrozenString<N> const& path_str) noexcept -> FrozenString<N> {
  auto const name = basename(path_str);
  if (name.length == 0) return FrozenString<N>{};
  auto dot_pos = std::string_view::npos;
  for (auto i = 1uz; i < name.length; ++i) {
    if (name.buffer[i] == '.') {
      dot_pos = i;
    }
  }
  if (dot_pos == std::string_view::npos) {
    auto res = FrozenString<N>{};
    for (auto i = 0uz; i < name.length; ++i) {
      res.buffer[i] = name.buffer[i];
    }
    res.buffer[name.length] = '\0';
    res.length = name.length;
    return res;
  }
  auto res = FrozenString<N>{};
  for (auto i = 0uz; i < dot_pos; ++i) {
    res.buffer[i] = name.buffer[i];
  }
  res.buffer[dot_pos] = '\0';
  res.length = dot_pos;
  return res;
}

template <size_t N>
[[nodiscard]] auto consteval stem(char const (&path_str)[N]) noexcept -> FrozenString<N> {
  return stem(FrozenString{path_str});
}

/**
 * @brief 複数のパス要素を '/' で結合する
 *
 * 要素間の連続 '/' は 1 個に正規化する。可変長引数。
 * 先頭要素が絶対パス（'/' で始まる）の場合、先行要素のディレクトリ部は無視される。
 *
 * @tparam Args パス要素の型
 * @param parts 結合するパス要素
 * @return auto 結合済みパス
 */
template <typename... Args>
[[nodiscard]] auto consteval join(Args const&... parts) noexcept {
  constexpr auto COUNT = sizeof...(Args);
  auto const arr = std::array<FrozenString<2048>, COUNT>{freeze(parts)...};

  // 最大サイズ = 各要素の2048 + 区切り文字 + 終端
  constexpr auto MAX_TOTAL = 2048uz * COUNT + COUNT + 1;
  auto res = FrozenString<MAX_TOTAL>{};
  auto offset = 0uz;
  bool first = true;
  bool prev_ended_with_slash = false;
  for (auto i = 0uz; i < COUNT; ++i) {
    auto sv = arr[i].sv();
    if (sv.empty()) continue;
    if (sv[0] == '/') {
      offset = 0;
      first = true;
      prev_ended_with_slash = false;
    }
    if (!first && !prev_ended_with_slash && sv[0] != '/') {
      res.buffer[offset++] = '/';
    }
    for (auto c : sv) {
      res.buffer[offset++] = c;
    }
    prev_ended_with_slash = (sv.back() == '/');
    first = false;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

}  // namespace frozenchars::path