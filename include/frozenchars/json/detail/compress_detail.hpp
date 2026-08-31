#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace frozenchars::json::detail {

/**
 * @brief JSON 値の種別
 */
enum class json_type : uint8_t { null, boolean, number, string, array, object };

/**
 * @brief パース済み JSON 値を表すノード
 *
 * @details 配列・オブジェクトの子要素は @ref arr に格納する。オブジェクトの場合は
 * @ref keys と @ref arr が同じインデックスで対応する（並列配列）。文字列は元入力への
 * ビューを保持するため、入力文字列の寿命に依存する。
 */
struct json_value {
  json_type type = json_type::null;         ///< この値の種別
  bool bool_val = false;                    ///< boolean 型のときの真偽値
  std::string_view str_val{};               ///< string/number 型のときの元文字列ビュー
  std::vector<json_value> arr{};            ///< array/object 型のときの子要素
  std::vector<std::string_view> keys{};     ///< object 型のときのキー（arr と同一インデックスで対応）
};

/**
 * @brief 空白文字（スペース・タブ・改行・復帰）を読み飛ばす
 *
 * @param s 対象文字列
 * @param p 現在位置。空白でない文字まで進める
 */
constexpr auto skip_ws(std::string_view const s, size_t& p) -> void {
  while (p < s.size() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\n' || s[p] == '\r')) ++p;
}

/**
 * @brief ダブルクォートで囲まれた JSON 文字列をパースする
 *
 * @param s 対象文字列
 * @param p 開始位置（'"' を指す）。終端クォートの次まで進める
 * @return std::string_view 前後のクォートを含む文字列ビュー
 * @throw std::runtime_error 先頭が '"' でない、または終端クォートが無い場合
 */
[[nodiscard]] constexpr auto parse_string(std::string_view const s, size_t& p) -> std::string_view {
  if (p >= s.size() || s[p] != '"') throw std::runtime_error("expected '\"'");
  auto const start = p;
  ++p;
  while (p < s.size() && s[p] != '"') {
    if (s[p] == '\\') ++p;
    ++p;
  }
  if (p >= s.size()) throw std::runtime_error("unterminated string");
  ++p;
  return std::string_view(s.data() + start, p - start);
}

/**
 * @brief 任意の JSON 値をパースする（前方宣言）
 */
[[nodiscard]] constexpr auto parse_value(std::string_view const s, size_t& p) -> json_value;

/**
 * @brief JSON 数値をパースする
 *
 * @param s 対象文字列
 * @param p 開始位置。数値の終端まで進める
 * @return json_value number 型の値。整数部を int64_t として保持し、元文字列も str_val に残す
 * @note 符号・小数部・指数部を読み飛ばすが、値は from_chars による整数変換のみ行う
 */
[[nodiscard]] constexpr auto parse_number(std::string_view const s, size_t& p) -> json_value {
  auto const start = p;
  if (p < s.size() && s[p] == '-') ++p;
  while (p < s.size() && s[p] >= '0' && s[p] <= '9') ++p;
  if (p < s.size() && s[p] == '.') {
    ++p;
    while (p < s.size() && s[p] >= '0' && s[p] <= '9') ++p;
  }
  if (p < s.size() && (s[p] == 'e' || s[p] == 'E')) {
    ++p;
    if (p < s.size() && (s[p] == '+' || s[p] == '-')) ++p;
    while (p < s.size() && s[p] >= '0' && s[p] <= '9') ++p;
  }
  auto const num_str = std::string_view(s.data() + start, p - start);
  return json_value{json_type::number, false, num_str, {}, {}};
}

/**
 * @brief JSON 配列をパースする
 *
 * @param s 対象文字列
 * @param p 開始位置（'[' を指す）。閉じ ']' の次まで進める
 * @return json_value array 型の値
 * @throw std::runtime_error 要素の区切り ',' または閉じ ']' が現れない場合
 */
[[nodiscard]] constexpr auto parse_array(std::string_view const s, size_t& p) -> json_value {
  ++p;
  skip_ws(s, p);
  std::vector<json_value> arr;
  if (p < s.size() && s[p] == ']') { ++p; return {json_type::array, false, {}, std::move(arr), {}}; }
  while (true) {
    skip_ws(s, p);
    arr.push_back(parse_value(s, p));
    skip_ws(s, p);
    if (p < s.size() && s[p] == ',') { ++p; continue; }
    if (p < s.size() && s[p] == ']') { ++p; break; }
    throw std::runtime_error("expected ',' or ']'");
  }
  return {json_type::array, false, {}, std::move(arr), {}};
}

/**
 * @brief JSON オブジェクトをパースする
 *
 * @param s 対象文字列
 * @param p 開始位置（'{' を指す）。閉じ '}' の次まで進める
 * @return json_value object 型の値。キーは keys、値は arr に同一インデックスで格納する
 * @throw std::runtime_error ':' 区切りが無い、または ',' / '}' が現れない場合
 */
[[nodiscard]] constexpr auto parse_object(std::string_view const s, size_t& p) -> json_value {
  ++p;
  skip_ws(s, p);
  std::vector<std::string_view> keys;
  std::vector<json_value> vals;
  if (p < s.size() && s[p] == '}') { ++p; return {json_type::object, false, {}, std::move(vals), std::move(keys)}; }
  while (true) {
    skip_ws(s, p);
    auto const key = parse_string(s, p);
    skip_ws(s, p);
    if (p >= s.size() || s[p] != ':') throw std::runtime_error("expected ':'");
    ++p;
    skip_ws(s, p);
    keys.push_back(key);
    vals.push_back(parse_value(s, p));
    skip_ws(s, p);
    if (p < s.size() && s[p] == ',') { ++p; continue; }
    if (p < s.size() && s[p] == '}') { ++p; break; }
    throw std::runtime_error("expected ',' or '}'");
  }
  return {json_type::object, false, {}, std::move(vals), std::move(keys)};
}

/**
 * @brief 先頭文字から種別を判定して任意の JSON 値をパースする
 *
 * @param s 対象文字列
 * @param p 開始位置。値の終端まで進める
 * @return json_value パースした値
 * @throw std::runtime_error 入力終端に達した、または未対応の文字が現れた場合
 */
[[nodiscard]] constexpr auto parse_value(std::string_view const s, size_t& p) -> json_value {
  skip_ws(s, p);
  if (p >= s.size()) throw std::runtime_error("unexpected EOF");
  auto const c = s[p];
  if (c == '{') return parse_object(s, p);
  if (c == '[') return parse_array(s, p);
  if (c == '"') {
    auto const str = parse_string(s, p);
    return {json_type::string, false, str, {}, {}};
  }
  auto starts_with = [&](size_t pos, std::string_view pat) -> bool {
    if (pat.size() > s.size() - pos) return false;
    for (size_t i = 0; i < pat.size(); ++i) if (s[pos + i] != pat[i]) return false;
    return true;
  };
  if (c == 't' && starts_with(p, "true"))  { p += 4; return {json_type::boolean, true, {}, {}, {}}; }
  if (c == 'f' && starts_with(p, "false")) { p += 5; return {json_type::boolean, false, {}, {}, {}}; }
  if (c == 'n' && starts_with(p, "null"))  { p += 4; return {json_type::null, false, {}, {}, {}}; }
  if (c == '-' || (c >= '0' && c <= '9')) return parse_number(s, p);
  throw std::runtime_error("unexpected character");
}

/**
 * @brief JSON 文字列全体をパースする
 *
 * @param s JSON 文字列
 * @return json_value ルート値
 * @throw std::runtime_error パース失敗、または末尾に余分な内容がある場合
 */
[[nodiscard]] constexpr auto parse_json(std::string_view const s) -> json_value {
  size_t p = 0;
  auto val = parse_value(s, p);
  skip_ws(s, p);
  if (p != s.size()) throw std::runtime_error("trailing content");
  return val;
}

/**
 * @brief JSON 文字列が構文的に妥当かを判定する
 *
 * @param s 検証する文字列
 * @return bool 全体を過不足なくパースできれば true、例外や余分な内容があれば false
 */
[[nodiscard]] constexpr auto validate_json(std::string_view const s) noexcept -> bool {
  try {
    size_t p = 0;
    (void)parse_value(s, p);
    skip_ws(s, p);
    return p == s.size();
  } catch (...) {
    return false;
  }
}

} // namespace frozenchars::json::detail


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
