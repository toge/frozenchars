#pragma once

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
  {
    std::u16string filtered;
    filtered.reserve(u16.size());
    for (auto c : u16) if (c != frozenchars::json::detail::JSON_CRUSH_DELIMITER) filtered.push_back(c);
    u16 = std::move(filtered);
  }

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

/**
 * @brief crush 済み文字列を元の UTF-8 JSON テキストへ復元する
 *
 * @param input crush 出力（末尾に '_' を含む）
 * @return std::string 復元した UTF-8 JSON テキスト
 * @throw std::runtime_error 末尾 '_' が無い、または置換文字が見つからない場合
 */
[[nodiscard]] consteval auto uncrush_to_string(std::string_view const input) -> std::string {
  if (input.empty() || input.back() != '_') {
    throw std::runtime_error("uncrush: missing trailing '_'");
  }
  auto const body_utf8 = input.substr(0, input.size() - 1);
  auto u16 = frozenchars::json::detail::utf8_to_utf16(body_utf8);

  // デリミタ U+0001 で body / split に分割
  std::u16string_view body = u16;
  std::u16string_view split;
  if (auto const delim = u16.rfind(frozenchars::json::detail::JSON_CRUSH_DELIMITER); delim != std::u16string::npos) {
    body = std::u16string_view(u16.data(), delim);
    split = std::u16string_view(u16.data() + delim + 1, u16.size() - delim - 1);
  }

  // split を正順（先頭 = 最後に使われた置換文字）に処理する。
  // 各置換文字は自分の辞書エントリ「置換文字＋元部分文字列」を末尾に持ち、
  // 置換文字は「当時の文字列全体に出現しない」文字から選ばれるため、
  // rfind は常にその辞書エントリを一意に指す。
  auto data = std::u16string(body);
  for (auto const rc : split) {
    auto const p = data.rfind(rc);
    if (p == std::u16string::npos) {
      throw std::runtime_error("uncrush: replacement char not found");
    }
    auto const original = data.substr(p + 1);
    data.resize(p);
    size_t pos = 0;
    while ((pos = data.find(rc, pos)) != std::u16string::npos) {
      data.replace(pos, 1, original);
      pos += original.size();
    }
  }

  // JSON 構造文字を復元（swap の逆方向）
  auto const swapped_back = frozenchars::json::detail::json_crush_swap<char16_t>(data, false);
  return frozenchars::json::detail::utf16_to_utf8(swapped_back);
}

/**
 * @brief uncrush 後の出力に必要なバッファ長をコンパイル時に算出する
 *
 * @tparam Input 入力の FrozenString
 * @return size_t 終端 '\0' を含む必要バッファ長
 */
template <FrozenString Input>
[[nodiscard]] consteval auto uncrushed_size() -> size_t {
  return uncrush_to_string(Input.sv()).size() + 1;
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
  {
    std::u16string filtered;
    filtered.reserve(u16.size());
    for (auto c : u16) if (c != frozenchars::json::detail::JSON_CRUSH_DELIMITER) filtered.push_back(c);
    u16 = std::move(filtered);
  }

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

/**
 * @brief crush 済み JSON をコンパイル時に復元する
 *
 * @tparam Input crush 出力（FrozenString、末尾 '_' を含む）
 * @return FrozenString<N> 復元した UTF-8 JSON テキスト（N は自動決定）
 * @details 辞書を逆展開して置換文字を元の部分文字列へ戻し、構造文字を復元する。
 */
template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto uncrush() -> FrozenString<detail::uncrushed_size<Input>()> {
  constexpr auto N = detail::uncrushed_size<Input>();
  auto const result = detail::uncrush_to_string(Input.sv());
  auto res = FrozenString<N>{};
  for (size_t i = 0; i < result.size() && i < N - 1; ++i) {
    res.buffer[i] = result[i];
  }
  res.buffer[result.size()] = '\0';
  res.length = result.size();
  return res;
}

} // namespace frozenchars::json
