#pragma once

#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace frozenchars::json::detail {

enum class json_type : uint8_t { null, boolean, number, string, array, object };

struct json_value {
  json_type type = json_type::null;
  bool bool_val = false;
  int64_t num_val = 0;
  std::string_view str_val{};
  std::vector<json_value> arr{};
  std::vector<std::string_view> keys{};
};

constexpr auto skip_ws(std::string_view const s, size_t& p) -> void {
  while (p < s.size() && (s[p] == ' ' || s[p] == '\t' || s[p] == '\n' || s[p] == '\r')) ++p;
}

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

[[nodiscard]] constexpr auto parse_value(std::string_view const s, size_t& p) -> json_value;

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

[[nodiscard]] constexpr auto parse_json(std::string_view const s) -> json_value {
  size_t p = 0;
  auto val = parse_value(s, p);
  skip_ws(s, p);
  if (p != s.size()) throw std::runtime_error("trailing content");
  return val;
}

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
