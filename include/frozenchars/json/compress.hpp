#pragma once

#include <string_view>

#include "detail/compress_detail.hpp"
#include "frozenchars/json/crush.hpp"
#include "frozenchars/string.hpp"

namespace frozenchars::json {

/**
 * @brief JSON 文字列が構文的に妥当かを判定する
 *
 * @param input 検証する文字列
 * @return bool 妥当なら true
 */
[[nodiscard]] constexpr auto validate_json(std::string_view input) noexcept -> bool {
  return detail::validate_json(input);
}

namespace detail {

/**
 * @brief compress 後の出力に必要なバッファ長をコンパイル時に算出する
 *
 * @tparam Input 入力の FrozenString
 * @return size_t 終端 '\0' を含む必要バッファ長
 * @note compress() の戻り値型 FrozenString<N> の N を決定するために使う。
 */
template <FrozenString Input>
[[nodiscard]] consteval auto compressed_size() -> size_t {
  auto const parsed = frozenchars::json::detail::parse_json(Input.sv());
  auto const result = frozenchars::json::detail::compress_to_string(parsed);
  return result.size() + 1;
}

} // namespace detail

/**
 * @brief JSON 文字列をコンパイル時に構造圧縮する
 *
 * @tparam Input 圧縮対象の JSON 文字列（FrozenString、非空）
 * @return FrozenString<N> 値テーブルとルート参照を持つ圧縮 JSON（N は自動決定）
 * @details JSON をパースし、重複値をテーブル参照へ置き換えた JSON を生成する。
 */
template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto compress() -> FrozenString<detail::compressed_size<Input>()> {
  constexpr auto N = detail::compressed_size<Input>();
  auto const parsed = frozenchars::json::detail::parse_json(Input.sv());
  auto const result = frozenchars::json::detail::compress_to_string(parsed);

  auto res = FrozenString<N>{};
  for (size_t i = 0; i < result.size() && i < N - 1; ++i) {
    res.buffer[i] = result[i];
  }
  res.buffer[result.size()] = '\0';
  res.length = result.size();
  return res;
}

/**
 * @brief JSON をコンパイル時に構造圧縮したうえで JSON-Crush 圧縮する
 *
 * @tparam Input 圧縮対象の JSON 文字列（FrozenString、非空）
 * @return FrozenString<N> compress → crush を連続適用した結果（N は入力長から推定）
 * @details まず compress で値テーブル化し、その結果を UTF-16 変換・swap・js_crush で
 * さらに圧縮する。戻り値バッファ長は入力長に基づく上限見積り（length*64+32）を用いる。
 */
template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto crush_compress() {
  // まず構造圧縮（重複値をテーブル参照へ）
  auto const parsed = frozenchars::json::detail::parse_json(Input.sv());
  auto const compressed_str = frozenchars::json::detail::compress_to_string(parsed);

  // 続けて JSON-Crush 圧縮：UTF-16変換→デリミタ除去→swap→js_crush
  auto u16 = frozenchars::json::detail::utf8_to_utf16(compressed_str);
  u16.erase(std::remove(u16.begin(), u16.end(), frozenchars::json::detail::JSON_CRUSH_DELIMITER), u16.end());
  auto const swapped = frozenchars::json::detail::json_crush_swap<char16_t>(u16, true);
  auto const crush_result = frozenchars::json::detail::js_crush_utf16<char16_t>(swapped);
  // 圧縮本体に分割文字列をデリミタ区切りで連結し、末尾に終端記号 '_' を付加
  auto output_u16 = crush_result.crushed;
  if (!crush_result.split.empty()) {
    output_u16.push_back(frozenchars::json::detail::JSON_CRUSH_DELIMITER);
    output_u16.append(crush_result.split);
  }
  output_u16.push_back(u'_');
  auto const output_u8 = frozenchars::json::detail::utf16_to_utf8(output_u16);

  // 入力長から必要バッファ長を上限見積りし、FrozenString へコピー
  constexpr auto N = Input.length * 64 + 32;
  auto res = FrozenString<N>{};
  for (size_t i = 0; i < output_u8.size() && i < N - 1; ++i) {
    res.buffer[i] = output_u8[i];
  }
  res.buffer[output_u8.size()] = '\0';
  res.length = output_u8.size();
  return res;
}

} // namespace frozenchars::json
