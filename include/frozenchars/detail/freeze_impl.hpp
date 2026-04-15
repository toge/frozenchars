#pragma once

#include "char_utils.hpp"
#include <cstddef>
#include <algorithm>
#include <span>
#include <string_view>

namespace frozenchars::detail {

/**
 * @brief 1要素を 0..255 の値として扱う
 * - std::byte は std::to_integer<unsigned char> を使用
 * - それ以外は unsigned char へキャスト
 *
 * @tparam T 変換する要素の型
 * @param v 変換する要素
 * @return auto 変換された 0..255 の値
 */
template <typename T>
auto consteval to_u8(T const v) noexcept {
  if constexpr (std::same_as<std::remove_cv_t<T>, std::byte>) {
    return std::to_integer<unsigned char>(v);
  } else {
    return static_cast<unsigned char>(v);
  }
}

/**
 * @brief ヌル終端ポインタを FrozenString<257> に変換する
 * - nullptr は空文字とする
 * - '\0' もしくは 256 文字で打ち切り
 *
 * @tparam Elem 変換する要素の型
 * @param arg 変換するヌル終端ポインタ
 * @return auto 変換文字列
 */
template <typename Elem>
auto consteval freeze_from_ptr(Elem const* arg) noexcept {
  auto res = FrozenString<257>{};
  if (arg == nullptr) {
    res.buffer[0] = '\0';
    res.length = 0;
    return res;
  }

  auto len = 0uz;
  for (; len < res.buffer.size() - 1; ++len) {
    auto const byte = to_u8(arg[len]);
    if (byte == 0u) {
      break;
    }
    res.buffer[len] = static_cast<char>(byte);
  }
  res.buffer[len] = '\0';
  res.length = len;
  return res;
}

/**
 * @brief span を FrozenString<257> に変換する
 * - 先頭から 0 値までを文字列として扱う
 * - 0 がなくても最大 256 文字までコピー
 *
 * @tparam Elem 変換する要素の型
 * @tparam Extent span の長さ
 * @param arg 変換する span
 * @return auto 変換文字列
 */
template <typename Elem, size_t Extent>
auto consteval freeze_from_span(std::span<Elem const, Extent> arg) noexcept {
  auto res = FrozenString<257>{};
  auto const max_len = std::min(arg.size(), res.buffer.size() - 1);
  auto len = 0uz;
  for (; len < max_len; ++len) {
    auto const byte = to_u8(arg[len]);
    if (byte == 0u) {
      break;
    }
    res.buffer[len] = static_cast<char>(byte);
  }
  res.buffer[len] = '\0';
  res.length = len;
  return res;
}

/**
 * @brief string_view を FrozenString<257> に変換する
 * - 終端は長さベース
 * - 最大 256 文字までコピー
 *
 * @param s 変換する string_view
 * @return auto 変換文字列
 */
auto consteval freeze_from_sv(std::string_view s) noexcept {
  auto res = FrozenString<257>{};
  auto const len = std::min(s.size(), res.buffer.size() - 1);
  for (auto i = 0uz; i < len; ++i) {
    res.buffer[i] = s[i];
  }
  res.buffer[len] = '\0';
  res.length = len;
  return res;
}

} // namespace frozenchars::detail
