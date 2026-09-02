#pragma once

#include <string_view>

#include "detail/compress_detail.hpp"
#include "detail/crush_detail.hpp"
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

/**
 * @brief 圧縮済み JSON を元の JSON テキストへ復元する
 *
 * @param input 圧縮済み JSON 文字列（compress の出力形式）
 * @return std::string 復元した JSON テキスト
 * @throw std::runtime_error root が無い、または値参照が範囲外の場合
 */
[[nodiscard]] consteval auto decompress_to_string(std::string_view const input) -> std::string {
  auto const parsed = frozenchars::json::detail::parse_json(input);

  // 値テーブルの各要素を JSON リテラルテキストへ復元する
  auto literal_of = [](frozenchars::json::detail::json_value const& v) -> std::string {
    using frozenchars::json::detail::json_type;
    switch (v.type) {
    case json_type::null: return "null";
    case json_type::boolean: return v.bool_val ? "true" : "false";
    case json_type::number: return std::string(v.str_val);
    case json_type::string: return std::string(v.str_val);
    default: FROZENCHARS_THROW(std::runtime_error("decompress: unexpected value table entry"));
    }
  };

  std::vector<std::string> values;
  frozenchars::json::detail::json_value const* root_val = nullptr;
  if (parsed.type == frozenchars::json::detail::json_type::object) {
    for (size_t i = 0; i < parsed.keys.size(); ++i) {
      auto key = parsed.keys[i];
      if (key.size() >= 2 && key.front() == '"' && key.back() == '"') {
        key = key.substr(1, key.size() - 2);
      }
      if (key == "values") {
        for (auto const& e : parsed.arr[i].arr) values.push_back(literal_of(e));
      } else if (key == "root") {
        root_val = &parsed.arr[i];
      }
    }
  }
  if (root_val == nullptr) FROZENCHARS_THROW(std::runtime_error("decompress: missing root"));

  // root を再帰的に再構築する。文字列は Base62 参照として values へ解決する
  auto rebuild = [&](auto&& self, frozenchars::json::detail::json_value const& v) -> std::string {
    using frozenchars::json::detail::json_type;
    switch (v.type) {
    case json_type::object: {
      std::string r = "{";
      for (size_t i = 0; i < v.keys.size(); ++i) {
        if (i > 0) r += ",";
        r += std::string(v.keys[i]);
        r += ":";
        r += self(self, v.arr[i]);
      }
      r += "}";
      return r;
    }
    case json_type::array: {
      std::string r = "[";
      for (size_t i = 0; i < v.arr.size(); ++i) {
        if (i > 0) r += ",";
        r += self(self, v.arr[i]);
      }
      r += "]";
      return r;
    }
    case json_type::string: {
      auto ref = v.str_val;
      if (ref.size() >= 2 && ref.front() == '"' && ref.back() == '"') {
        ref = ref.substr(1, ref.size() - 2);
      }
      auto const idx = frozenchars::json::detail::from_base62(ref);
      if (idx >= values.size()) FROZENCHARS_THROW(std::runtime_error("decompress: value index out of range"));
      return values[idx];
    }
    default: FROZENCHARS_THROW(std::runtime_error("decompress: unexpected node type"));
    }
  };

  return rebuild(rebuild, *root_val);
}

/**
 * @brief decompress 後の出力に必要なバッファ長をコンパイル時に算出する
 *
 * @tparam Input 入力の FrozenString
 * @return size_t 終端 '\0' を含む必要バッファ長
 */
template <FrozenString Input>
[[nodiscard]] consteval auto decompressed_size() -> size_t {
  return decompress_to_string(Input.sv()).size() + 1;
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
 * @brief 圧縮済み JSON をコンパイル時に復元する
 *
 * @tparam Input 圧縮済み JSON 文字列（compress の出力、FrozenString）
 * @return FrozenString<N> 復元した JSON テキスト（N は自動決定）
 * @details 値テーブルとルート参照を元の JSON 木へ展開する。
 */
template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto decompress() -> FrozenString<detail::decompressed_size<Input>()> {
  constexpr auto N = detail::decompressed_size<Input>();
  auto const result = detail::decompress_to_string(Input.sv());
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
  {
    std::u16string filtered;
    filtered.reserve(u16.size());
    for (auto c : u16) if (c != frozenchars::json::detail::JSON_CRUSH_DELIMITER) filtered.push_back(c);
    u16 = std::move(filtered);
  }
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
