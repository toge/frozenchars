#pragma once

#include <charconv>
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
  int64_t num_val = 0;                      ///< number 型のときの整数値
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
  int64_t val = 0;
  std::from_chars(num_str.data(), num_str.data() + num_str.size(), val);
  return json_value{json_type::number, false, val, num_str, {}, {}};
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
  if (p < s.size() && s[p] == ']') { ++p; return {json_type::array, false, 0, {}, std::move(arr), {}}; }
  while (true) {
    skip_ws(s, p);
    arr.push_back(parse_value(s, p));
    skip_ws(s, p);
    if (p < s.size() && s[p] == ',') { ++p; continue; }
    if (p < s.size() && s[p] == ']') { ++p; break; }
    throw std::runtime_error("expected ',' or ']'");
  }
  return {json_type::array, false, 0, {}, std::move(arr), {}};
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
  if (p < s.size() && s[p] == '}') { ++p; return {json_type::object, false, 0, {}, std::move(vals), std::move(keys)}; }
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
  return {json_type::object, false, 0, {}, std::move(vals), std::move(keys)};
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
    return {json_type::string, false, 0, str, {}, {}};
  }
  if (c == 't' && s.substr(p, 4) == "true")  { p += 4; return {json_type::boolean, true, 0, {}, {}, {}}; }
  if (c == 'f' && s.substr(p, 5) == "false") { p += 5; return {json_type::boolean, false, 0, {}, {}, {}}; }
  if (c == 'n' && s.substr(p, 4) == "null")  { p += 4; return {json_type::null, false, 0, {}, {}, {}}; }
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
