#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "json_parser.hpp"

namespace frozenchars::json::detail {

/// Base62 エンコードで使用する文字集合（0-9, A-Z, a-z の順）
constexpr auto BASE62_CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

/**
 * @brief 符号なし整数を Base62 文字列に変換する
 *
 * @param value 変換する値
 * @return std::string Base62 表現。0 のときは "0"
 */
[[nodiscard]] constexpr auto to_base62(uint64_t value) -> std::string {
  if (value == 0) return "0";
  std::string result;
  while (value > 0) {
    result.push_back(BASE62_CHARS[value % 62]);
    value /= 62;
  }
  std::reverse(result.begin(), result.end());
  return result;
}

/**
 * @brief Base62 文字列を符号なし整数に変換する
 *
 * @param s 変換する Base62 文字列
 * @return uint64_t 変換結果
 * @throw std::runtime_error 不正な文字を含む場合
 */
[[nodiscard]] constexpr auto from_base62(std::string_view const s) -> uint64_t {
  uint64_t value = 0;
  for (auto const c : s) {
    value *= 62;
    if (c >= '0' && c <= '9') {
      value += static_cast<uint64_t>(c - '0');
    } else if (c >= 'A' && c <= 'Z') {
      value += static_cast<uint64_t>(c - 'A' + 10);
    } else if (c >= 'a' && c <= 'z') {
      value += static_cast<uint64_t>(c - 'a' + 36);
    } else {
      throw std::runtime_error("from_base62: invalid character");
    }
  }
  return value;
}

/**
 * @brief JSON 値を圧縮用の中間文字列表現に変換する
 *
 * @param val 変換する値
 * @return std::string 各スカラー値の有効な JSON リテラル原文。
 * string / number は元入力のクォート・エスケープ・小数を保持する。
 * 配列・オブジェクトは再帰的に展開する
 */
[[nodiscard]] constexpr auto value_to_string(json_value const& val) -> std::string {
  switch (val.type) {
  case json_type::null: return "null";
  case json_type::boolean: return val.bool_val ? "true" : "false";
  case json_type::number: return std::string(val.str_val);
  case json_type::string: return std::string(val.str_val);
  case json_type::array: {
    std::string r = "[";
    for (size_t i = 0; i < val.arr.size(); ++i) {
      if (i > 0) r += ",";
      r += value_to_string(val.arr[i]);
    }
    r += "]";
    return r;
  }
  case json_type::object: {
    std::string r = "{";
    for (size_t i = 0; i < val.keys.size(); ++i) {
      if (i > 0) r += ",";
      r += std::string(val.keys[i]);
      r += ":";
      r += value_to_string(val.arr[i]);
    }
    r += "}";
    return r;
  }
  }
  return {};
}

/**
 * @brief 圧縮中に登場した値の一覧を保持する
 */
struct CompressMemory {
  std::vector<std::string> values;       ///< 登録順に格納した値テーブル（出力に使用）
};

/**
 * @brief 値をテーブルに登録し、その参照インデックスを Base62 で返す
 *
 * @param mem 値テーブル
 * @param serialized 登録する直列化済み文字列
 * @return std::string 既存なら既存インデックス、新規なら追加後のインデックスの Base62 表現
 */
[[nodiscard]] constexpr auto get_or_add_value(CompressMemory& mem, std::string const& serialized) -> std::string {
  for (size_t i = 0; i < mem.values.size(); ++i) {
    if (mem.values[i] == serialized) {
      return to_base62(static_cast<uint64_t>(i));
    }
  }
  mem.values.push_back(serialized);
  return to_base62(static_cast<uint64_t>(mem.values.size() - 1));
}

/**
 * @brief JSON 値を圧縮表現に変換する（前方宣言）
 */
[[nodiscard]] constexpr auto compress_value(CompressMemory& mem, json_value const& val) -> std::string;

/**
 * @brief オブジェクトを圧縮表現に変換する
 *
 * @param mem 値テーブル
 * @param val object 型の JSON 値
 * @return std::string キーはそのまま、値はテーブル参照に置換した "{k:v,...}" 形式
 */
[[nodiscard]] constexpr auto compress_object(CompressMemory& mem, json_value const& val) -> std::string {
  std::string result = "{";
  for (size_t i = 0; i < val.keys.size(); ++i) {
    if (i > 0) result += ",";
    result += std::string(val.keys[i]);
    result += ":";
    result += compress_value(mem, val.arr[i]);
  }
  result += "}";
  return result;
}

/**
 * @brief 配列を圧縮表現に変換する
 *
 * @param mem 値テーブル
 * @param val array 型の JSON 値
 * @return std::string 各要素をテーブル参照に置換した "[v,...]" 形式
 */
[[nodiscard]] constexpr auto compress_array(CompressMemory& mem, json_value const& val) -> std::string {
  std::string result = "[";
  for (size_t i = 0; i < val.arr.size(); ++i) {
    if (i > 0) result += ",";
    result += compress_value(mem, val.arr[i]);
  }
  result += "]";
  return result;
}

/**
 * @brief JSON 値を圧縮表現に変換する
 *
 * @param mem 値テーブル
 * @param val 対象の JSON 値
 * @return std::string スカラー値はテーブル参照、配列・オブジェクトは構造を保った圧縮表現
 */
[[nodiscard]] constexpr auto compress_value(CompressMemory& mem, json_value const& val) -> std::string {
  switch (val.type) {
  case json_type::null:
  case json_type::boolean:
  case json_type::number:
  case json_type::string: {
    auto const s = value_to_string(val);
    auto const ref = get_or_add_value(mem, s);
    return "\"" + ref + "\"";
  }
  case json_type::array: return compress_array(mem, val);
  case json_type::object: return compress_object(mem, val);
  }
  return {};
}

/**
 * @brief JSON ツリー全体を圧縮した JSON 文字列に変換する
 *
 * @param root ルート値
 * @return std::string 値テーブル "values" と圧縮済みルート参照 "root" を持つ JSON 文字列
 */
[[nodiscard]] constexpr auto compress_to_string(json_value const& root) -> std::string {
  CompressMemory mem;
  auto const encoded_root = compress_value(mem, root);

  std::string result = R"({"values":[)";
  for (size_t i = 0; i < mem.values.size(); ++i) {
    if (i > 0) result += ",";
    result += mem.values[i];
  }
  result += "],\"root\":";
  result += encoded_root;
  result += "}";
  return result;
}

} // namespace frozenchars::json::detail
