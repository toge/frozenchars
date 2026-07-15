#pragma once

#include <algorithm>

#include "detail/crush_detail.hpp"
#include "frozenchars/string.hpp"

namespace frozenchars::json {

namespace detail {

/**
 * @brief crush 後の出力に必要なバッファ長をコンパイル時に算出する
 *
 * @tparam Input 入力の FrozenString
 * @return size_t 終端 '\0' を含む必要バッファ長
 * @note 実際の crush と同じ手順（UTF-16変換→swap→js_crush→デリミタ付加）を実行して
 * 結果長を求める。crush() の戻り値型 FrozenString<N> の N を決定するために使う。
 */
template <FrozenString Input>
[[nodiscard]] consteval auto crushed_size() -> size_t {
  auto const input_sv = Input.sv();
  auto u16 = frozenchars::json::detail::utf8_to_utf16(input_sv);
  u16.erase(std::remove(u16.begin(), u16.end(), frozenchars::json::detail::JSON_CRUSH_DELIMITER), u16.end());

  auto const swapped = frozenchars::json::detail::json_crush_swap<char16_t>(u16, true);
  auto const result = frozenchars::json::detail::js_crush_utf16<char16_t>(swapped);

  auto output_u16 = result.crushed;
  if (!result.split.empty()) {
    output_u16.push_back(frozenchars::json::detail::JSON_CRUSH_DELIMITER);
    output_u16.append(result.split);
  }
  output_u16.push_back(u'_');

  auto const output_u8 = frozenchars::json::detail::utf16_to_utf8(output_u16);
  return output_u8.size() + 1;
}

} // namespace detail

/**
 * @brief JSON 文字列をコンパイル時に JSON-Crush 圧縮する
 *
 * @tparam Input 圧縮対象の JSON 文字列（FrozenString、非空）
 * @return FrozenString<N> 圧縮結果（N は crushed_size で自動決定）
 * @details UTF-16 へ変換し、区切り用デリミタを除去したうえで構造文字の swap と
 * js_crush を適用する。分割文字列があればデリミタで連結し、末尾に '_' を付けて
 * UTF-8 に戻す。
 */
template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto crush() -> FrozenString<detail::crushed_size<Input>()> {
  constexpr auto N = detail::crushed_size<Input>();
  auto const input_sv = Input.sv();

  // UTF-16 に変換し、内部で使うデリミタが入力に含まれていれば除去
  auto u16 = frozenchars::json::detail::utf8_to_utf16(input_sv);
  u16.erase(std::remove(u16.begin(), u16.end(), frozenchars::json::detail::JSON_CRUSH_DELIMITER), u16.end());

  // JSON 構造文字を短い記号へ swap
  auto const swapped = frozenchars::json::detail::json_crush_swap<char16_t>(u16, true);

  // 繰り返し部分文字列を置換文字へ圧縮
  auto const result = frozenchars::json::detail::js_crush_utf16<char16_t>(swapped);

  // 圧縮本体に分割文字列（辞書）をデリミタ区切りで連結し、末尾に終端記号 '_' を付加
  auto output_u16 = result.crushed;
  if (!result.split.empty()) {
    output_u16.push_back(frozenchars::json::detail::JSON_CRUSH_DELIMITER);
    output_u16.append(result.split);
  }
  output_u16.push_back(u'_');

  auto const output_u8 = frozenchars::json::detail::utf16_to_utf8(output_u16);

  // FrozenString バッファへコピーして終端を付ける
  auto res = FrozenString<N>{};
  for (size_t i = 0; i < output_u8.size() && i < N - 1; ++i) {
    res.buffer[i] = output_u8[i];
  }
  res.buffer[output_u8.size()] = '\0';
  res.length = output_u8.size();
  return res;
}

} // namespace frozenchars::json
