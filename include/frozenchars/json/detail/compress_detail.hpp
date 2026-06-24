#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "json_parser.hpp"

namespace frozenchars::json::detail {

constexpr auto BASE62_CHARS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

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

[[nodiscard]] constexpr auto int64_to_string(int64_t v) -> std::string {
  if (v == 0) return "0";
  std::string r;
  bool neg = v < 0;
  if (neg) v = -v;
  while (v > 0) { r.push_back(static_cast<char>('0' + v % 10)); v /= 10; }
  if (neg) r.push_back('-');
  std::reverse(r.begin(), r.end());
  return r;
}

[[nodiscard]] constexpr auto val_equal(json_value const& a, json_value const& b) -> bool {
  if (a.type != b.type) return false;
  switch (a.type) {
    case json_type::null: return true;
    case json_type::boolean: return a.bool_val == b.bool_val;
    case json_type::number: return a.num_val == b.num_val;
    case json_type::string: return a.str_val == b.str_val;
    case json_type::array:
      if (a.arr.size() != b.arr.size()) return false;
      for (size_t i = 0; i < a.arr.size(); ++i)
        if (!val_equal(a.arr[i], b.arr[i])) return false;
      return true;
    case json_type::object:
      if (a.keys.size() != b.keys.size()) return false;
      for (size_t i = 0; i < a.keys.size(); ++i) {
        if (a.keys[i] != b.keys[i]) return false;
        if (!val_equal(a.arr[i], b.arr[i])) return false;
      }
      return true;
  }
  return false;
}

[[nodiscard]] constexpr auto value_to_string(json_value const& val) -> std::string {
  switch (val.type) {
    case json_type::null: return "_";
    case json_type::boolean: return val.bool_val ? "true" : "false";
    case json_type::number: return int64_to_string(val.num_val);
    case json_type::string: {
      auto sv = val.str_val;
      if (sv.size() >= 2 && sv.front() == '"' && sv.back() == '"')
        sv = sv.substr(1, sv.size() - 2);
      return std::string(sv);
    }
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

struct CompressMemory {
  std::vector<std::string> values;
  std::vector<std::string> value_cache;
};

[[nodiscard]] constexpr auto get_or_add_value(CompressMemory& mem, [[maybe_unused]] json_value const& val, std::string const& serialized) -> std::string {
  for (size_t i = 0; i < mem.values.size(); ++i) {
    if (mem.value_cache[i] == serialized) {
      return to_base62(static_cast<uint64_t>(i));
    }
  }
  mem.values.push_back(serialized);
  mem.value_cache.push_back(serialized);
  return to_base62(static_cast<uint64_t>(mem.values.size() - 1));
}

[[nodiscard]] constexpr auto compress_value(CompressMemory& mem, json_value const& val) -> std::string;

[[nodiscard]] constexpr auto compress_object(CompressMemory& mem, json_value const& val) -> std::string {
  std::string result = "{";
  for (size_t i = 0; i < val.keys.size(); ++i) {
    if (i > 0) result += ",";
    auto key_str = std::string(val.keys[i]);
    if (key_str.size() >= 2 && key_str.front() == '"' && key_str.back() == '"')
      key_str = key_str.substr(1, key_str.size() - 2);
    result += key_str + ":";
    result += compress_value(mem, val.arr[i]);
  }
  result += "}";
  return result;
}

[[nodiscard]] constexpr auto compress_array(CompressMemory& mem, json_value const& val) -> std::string {
  std::string result = "[";
  for (size_t i = 0; i < val.arr.size(); ++i) {
    if (i > 0) result += ",";
    result += compress_value(mem, val.arr[i]);
  }
  result += "]";
  return result;
}

[[nodiscard]] constexpr auto compress_value(CompressMemory& mem, json_value const& val) -> std::string {
  switch (val.type) {
    case json_type::null:
    case json_type::boolean:
    case json_type::number:
    case json_type::string: {
      auto const s = value_to_string(val);
      return get_or_add_value(mem, val, s);
    }
    case json_type::array: return compress_array(mem, val);
    case json_type::object: return compress_object(mem, val);
  }
  return {};
}

[[nodiscard]] constexpr auto compress_to_string(json_value const& root) -> std::string {
  CompressMemory mem;
  auto const encoded_root = compress_value(mem, root);

  std::string result = R"({"values":[)";
  for (size_t i = 0; i < mem.values.size(); ++i) {
    if (i > 0) result += ",";
    result += "\"" + mem.values[i] + "\"";
  }
  result += "],\"root\":";
  result += encoded_root;
  result += "}";
  return result;
}

} // namespace frozenchars::json::detail
