#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#ifndef FROZENCHARS_OBJECT_MAP_HEADER
#include <unordered_map>
#define FROZENCHARS_OBJECT_MAP_HEADER
#define FROZENCHARS_OBJECT_MAP std::unordered_map
#else
#include <ankerl/unordered_dense.h>
#endif

namespace frozenchars {

/**
 * @brief テンプレート値における null を表すタグ型。
 */
struct template_null final {};

/**
 * @brief テンプレート評価時に使う動的値型。
 */
struct template_value;

/**
 * @brief テンプレート配列値の実体型。
 */
using template_array = std::vector<template_value>;

/**
 * @brief テンプレートオブジェクト値の実体型。
 *
 * FROZENCHARS_OBJECT_MAP マクロで選択可能（デフォルト: std::unordered_map）
 * コンパイル時に -DFROZENCHARS_OBJECT_MAP_HEADER や define で指定してカスタマイズ可能。
 * 例: -DFROZENCHARS_OBJECT_MAP=ankerl::unordered_dense::map
 */
using template_object = FROZENCHARS_OBJECT_MAP<std::string, template_value>;

/**
 * @brief テンプレート言語で扱う値の共用体ラッパ。
 *
 * `null / bool / 整数 / 浮動小数 / 文字列 / 配列 / オブジェクト`
 * を単一の型で扱うためのコンテナ。
 */
struct template_value {
  /**
   * @brief 内部ストレージ型。
   */
  using storage_type = std::variant<template_null, bool, std::int64_t, double, std::string, template_array, template_object>;

  /**
   * @brief 値の実体。
   */
  storage_type storage{};

  /**
   * @brief null 値で初期化する。
   */
  template_value() : storage(template_null{}) {}

  /**
   * @brief nullptr を null 値として受け取る。
   */
  template_value(std::nullptr_t) : storage(template_null{}) {}

  /**
   * @brief 真偽値で初期化する。
   */
  template_value(bool v) : storage(v) {}

  /**
   * @brief C 文字列を文字列値として初期化する。
   */
  template_value(char const* v) : storage(std::string{v}) {}

  /**
   * @brief std::string で初期化する。
   */
  template_value(std::string v) : storage(std::move(v)) {}

  /**
   * @brief 配列値で初期化する。
   */
  template_value(template_array v) : storage(std::move(v)) {}

  /**
   * @brief オブジェクト値で初期化する。
   */
  template_value(template_object v) : storage(std::move(v)) {}

  /**
   * @brief 整数型を int64 値として保持する。
   * @tparam Int bool 以外の整数型
   * @param v 入力値
   */
  template <typename Int>
  requires(std::is_integral_v<std::remove_cvref_t<Int>> && !std::is_same_v<std::remove_cvref_t<Int>, bool>)
  template_value(Int v) : storage(static_cast<std::int64_t>(v)) {}

  /**
   * @brief 浮動小数型を double 値として保持する。
   * @tparam Float 浮動小数型
   * @param v 入力値
   */
  template <typename Float>
  requires(std::is_floating_point_v<std::remove_cvref_t<Float>>)
  template_value(Float v) : storage(static_cast<double>(v)) {}
};

/**
 * @brief テンプレート評価時の実行時エラー。
 */
class template_render_error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

/**
 * @brief 値が null か判定する。
 * @param v 対象値
 * @return null なら true
 */
[[nodiscard]] inline auto is_null(template_value const& v) -> bool {
  return std::holds_alternative<template_null>(v.storage);
}

/**
 * @brief 値を bool として取り出す。
 * @param v 対象値
 * @return bool 値
 * @throws template_render_error 型が不一致の場合
 */
[[nodiscard]] inline auto as_bool(template_value const& v) -> bool {
  if (auto const* p = std::get_if<bool>(&v.storage)) {
    return *p;
  }
  throw template_render_error{"value is not bool"};
}

/**
 * @brief 値を int64 として取り出す。
 * @param v 対象値
 * @return int64 値
 * @throws template_render_error 型が不一致の場合
 */
[[nodiscard]] inline auto as_int(template_value const& v) -> std::int64_t {
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return *p;
  }
  throw template_render_error{"value is not int"};
}

/**
 * @brief 値を double として取り出す。
 *
 * int64 は double に昇格して返す。
 *
 * @param v 対象値
 * @return 数値
 * @throws template_render_error 数値型でない場合
 */
[[nodiscard]] inline auto as_double(template_value const& v) -> double {
  if (auto const* p = std::get_if<double>(&v.storage)) {
    return *p;
  }
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return static_cast<double>(*p);
  }
  throw template_render_error{"value is not number"};
}

/**
 * @brief 値を文字列参照として取り出す。
 * @param v 対象値
 * @return 文字列参照
 * @throws template_render_error 型が不一致の場合
 */
[[nodiscard]] inline auto as_string(template_value const& v) -> std::string const& {
  if (auto const* p = std::get_if<std::string>(&v.storage)) {
    return *p;
  }
  throw template_render_error{"value is not string"};
}

/**
 * @brief 値を配列参照として取り出す。
 * @param v 対象値
 * @return 配列参照
 * @throws template_render_error 型が不一致の場合
 */
[[nodiscard]] inline auto as_array(template_value const& v) -> template_array const& {
  if (auto const* p = std::get_if<template_array>(&v.storage)) {
    return *p;
  }
  throw template_render_error{"value is not array"};
}

/**
 * @brief 値をオブジェクト参照として取り出す。
 * @param v 対象値
 * @return オブジェクト参照
 * @throws template_render_error 型が不一致の場合
 */
[[nodiscard]] inline auto as_object(template_value const& v) -> template_object const& {
  if (auto const* p = std::get_if<template_object>(&v.storage)) {
    return *p;
  }
  throw template_render_error{"value is not object"};
}

/**
 * @brief 初期化リストから配列値を生成する。
 * @param items 要素列
 * @return template_value(配列)
 */
[[nodiscard]] inline auto make_template_array(std::initializer_list<template_value> items) -> template_value {
  return template_value{template_array{items}};
}

/**
 * @brief 初期化リストからオブジェクト値を生成する。
 *
 * メモ: 事前に容量確保し、ハッシュテーブル再配置を最小化して高速化。
 *
 * @param items キー・値列
 * @return template_value(オブジェクト)
 */
[[nodiscard]] inline auto make_template_object(std::initializer_list<std::pair<std::string, template_value>> items) -> template_value {
  auto out = template_object{};
  out.reserve(items.size());
  for (auto const& [k, v] : items) {
    out.insert({k, v});
  }
  return template_value{std::move(out)};
}

/**
* @brief inja 互換寄りの真偽値変換を行う。
*
* false 扱い: null / false / 0 / 0.0 / 空文字 / 空配列 / 空オブジェクト
* true 扱い: 上記以外
*
* @param v 対象値
* @return 真偽値
 */
[[nodiscard]] inline auto template_truthy(template_value const& v) -> bool {
  if (std::holds_alternative<template_null>(v.storage)) {
    return false;
  }
  if (auto const* p = std::get_if<bool>(&v.storage)) {
    return *p;
  }
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return *p != 0;
  }
  if (auto const* p = std::get_if<double>(&v.storage)) {
    return *p != 0.0;
  }
  if (auto const* p = std::get_if<std::string>(&v.storage)) {
    return !p->empty();
  }
  if (auto const* p = std::get_if<template_array>(&v.storage)) {
    return !p->empty();
  }
  return !std::get<template_object>(v.storage).empty();
}

} // namespace frozenchars

namespace frozenchars::inja {

/**
* @brief 初期化リストから配列値を生成する。
* @param items 要素列
* @return template_value(配列)
*/
[[nodiscard]] inline auto array(std::initializer_list<template_value> items) -> template_value {
  return make_template_array(items);
}

/**
* @brief 初期化リストからオブジェクト値を生成する。
*
* メモ: 事前に容量確保し、ハッシュテーブル再配置を最小化して高速化。
*
* @param items キー・値列
* @return template_value(オブジェクト)
*/
[[nodiscard]] inline auto object(std::initializer_list<std::pair<std::string, template_value>> items) -> template_value {
  return make_template_object(items);
}

} // namespace frozenchars::inja

namespace frozenchars {

/**
* @brief 値をテンプレート出力用文字列に変換する。
*
* 配列とオブジェクトは現状の簡易表現（`[array]`, `{object}`）を返す。
*
* @param v 対象値
* @return 出力文字列
*/
[[nodiscard]] inline auto template_to_string(template_value const& v) -> std::string {
  if (std::holds_alternative<template_null>(v.storage)) {
    return "null";
  }
  if (auto const* p = std::get_if<bool>(&v.storage)) {
    return *p ? "true" : "false";
  }
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return std::to_string(*p);
  }
  if (auto const* p = std::get_if<double>(&v.storage)) {
    auto s = std::to_string(*p);
    while (s.size() > 2 && s.back() == '0' && s[s.size() - 2] != '.') {
      s.pop_back();
    }
    if (!s.empty() && s.back() == '.') {
      s.pop_back();
    }
    return s;
  }
  if (auto const* p = std::get_if<std::string>(&v.storage)) {
    return *p;
  }
  if (std::holds_alternative<template_array>(v.storage)) {
    return "[array]";
  }
  return "{object}";
}

/// @brief Convert string to uppercase
/// @param str Input string view
/// @return Uppercase version of the string
[[nodiscard]] inline auto fn_upper(std::string_view str) -> std::string {
  auto result = std::string{str};
  for (auto& c : result) {
    c = std::toupper(static_cast<unsigned char>(c));
  }
  return result;
}

/// @brief Convert string to lowercase
/// @param str Input string view
/// @return Lowercase version of the string
[[nodiscard]] inline auto fn_lower(std::string_view str) -> std::string {
  auto result = std::string{str};
  for (auto& c : result) {
    c = std::tolower(static_cast<unsigned char>(c));
  }
  return result;
}

/// @brief Capitalize first character of string
/// @param str Input string view
/// @return String with first character capitalized
[[nodiscard]] inline auto fn_capitalize(std::string_view str) -> std::string {
  if (str.empty()) {
    return std::string{str};
  }
  auto result = std::string{str};
  result[0] = std::toupper(static_cast<unsigned char>(result[0]));
  return result;
}

/// @brief Replace first occurrence of substring
/// @param str Input string view
/// @param old_str Substring to find
/// @param new_str Replacement substring
/// @return String with first occurrence replaced
[[nodiscard]] inline auto fn_replace(std::string_view str, std::string_view old_str, std::string_view new_str) -> std::string {
  auto result = std::string{str};
  auto pos = size_t{0};
  while ((pos = result.find(old_str, pos)) != std::string::npos) {
    result.replace(pos, old_str.length(), new_str);
    pos += new_str.length();
  }
  return result;
}

/// @brief Get length of array
/// @param arr Input array
/// @return Number of elements
[[nodiscard]] inline auto fn_length(template_array const& arr) -> std::int64_t {
  return static_cast<std::int64_t>(arr.size());
}

/// @brief Get first element of array
/// @param arr Input array
/// @return First element (throws if empty)
[[nodiscard]] inline auto fn_first(template_array const& arr) -> template_value {
  if (arr.empty()) {
    throw template_render_error{"first() called on empty array"};
  }
  return arr[0];
}

/// @brief Get last element of array
/// @param arr Input array
/// @return Last element (throws if empty)
[[nodiscard]] inline auto fn_last(template_array const& arr) -> template_value {
  if (arr.empty()) {
    throw template_render_error{"last() called on empty array"};
  }
  return arr[arr.size() - 1];
}

/// @brief Join array elements into string with separator
/// @param arr Input array
/// @param sep Separator string
/// @return Joined string
[[nodiscard]] inline auto fn_join(template_array const& arr, std::string_view sep) -> std::string {
  if (arr.empty()) {
    return "";
  }

  auto result = std::string{};
  for (auto i = std::size_t{0}; i < arr.size(); ++i) {
    if (i > 0) {
      result += sep;
    }

    auto const& elem = arr[i];
    if (std::holds_alternative<std::string>(elem.storage)) {
      result += std::get<std::string>(elem.storage);
    } else if (std::holds_alternative<std::int64_t>(elem.storage)) {
      result += std::to_string(std::get<std::int64_t>(elem.storage));
    } else if (std::holds_alternative<double>(elem.storage)) {
      result += std::to_string(std::get<double>(elem.storage));
    } else if (std::holds_alternative<bool>(elem.storage)) {
      result += std::get<bool>(elem.storage) ? "true" : "false";
    } else {
      result += "null";
    }
  }
  return result;
}

/// @brief Sort array (creates new sorted copy)
/// @param arr Input array
/// @return New sorted array
[[nodiscard]] inline auto fn_sort(template_array const& arr) -> template_array {
  auto result = arr;
  std::sort(result.begin(), result.end(), [](template_value const& a, template_value const& b) {
    // Helper lambda to convert to double if possible
    auto to_double = [](template_value const& v) -> std::optional<double> {
      if (std::holds_alternative<double>(v.storage)) {
        return std::get<double>(v.storage);
      }
      if (std::holds_alternative<std::int64_t>(v.storage)) {
        return static_cast<double>(std::get<std::int64_t>(v.storage));
      }
      return std::nullopt;
    };

    auto a_num = to_double(a);
    auto b_num = to_double(b);

    if (a_num && b_num) {
      return *a_num < *b_num;
    }

    // String comparison
    if (std::holds_alternative<std::string>(a.storage) && std::holds_alternative<std::string>(b.storage)) {
      return std::get<std::string>(a.storage) < std::get<std::string>(b.storage);
    }

    // If types differ, numbers come before strings
    if (a_num && std::holds_alternative<std::string>(b.storage)) {
      return true;
    }
    if (std::holds_alternative<std::string>(a.storage) && b_num) {
      return false;
    }

    return false;
  });
  return result;
}

/// @brief Generate range of integers
/// @param end Upper bound (exclusive)
/// @return Array [0, 1, 2, ..., end-1]
[[nodiscard]] inline auto fn_range(std::int64_t end) -> template_array {
  auto result = template_array{};
  for (auto i = std::int64_t{0}; i < end; ++i) {
    result.push_back(template_value{i});
  }
  return result;
}

/// @brief Generate range of integers with start and end
/// @param start Start value (inclusive)
/// @param end End value (exclusive)
/// @return Array [start, start+1, ..., end-1]
[[nodiscard]] inline auto fn_range(std::int64_t start, std::int64_t end) -> template_array {
  auto result = template_array{};
  for (auto i = start; i < end; ++i) {
    result.push_back(template_value{i});
  }
  return result;
}

/// @brief Generate range of integers with start, end, and step
/// @param start Start value (inclusive)
/// @param end End value (exclusive)
/// @param step Step value
/// @return Array with values spaced by step
[[nodiscard]] inline auto fn_range(std::int64_t start, std::int64_t end, std::int64_t step) -> template_array {
  auto result = template_array{};
  if (step > 0) {
    for (auto i = start; i < end; i += step) {
      result.push_back(template_value{i});
    }
  } else if (step < 0) {
    for (auto i = start; i > end; i += step) {
      result.push_back(template_value{i});
    }
  }
  return result;
}

/// @brief Get absolute value of a number
/// @param num Input number (int64_t or double)
/// @return Absolute value with same type as input
[[nodiscard]] inline auto fn_abs(template_value const& num) -> template_value {
  if (std::holds_alternative<std::int64_t>(num.storage)) {
    auto const val = std::get<std::int64_t>(num.storage);
    return template_value{std::abs(val)};
  }
  if (std::holds_alternative<double>(num.storage)) {
    auto const val = std::get<double>(num.storage);
    return template_value{std::abs(val)};
  }
  throw template_render_error{"abs() expects numeric argument"};
}

/// @brief Round a number to specified decimal places
/// @param num Input number (int64_t or double)
/// @param digits Number of decimal places (default 0)
/// @return Rounded value as double
[[nodiscard]] inline auto fn_round(template_value const& num, template_value const& digits) -> template_value {
  double val = 0.0;
  if (std::holds_alternative<std::int64_t>(num.storage)) {
    val = static_cast<double>(std::get<std::int64_t>(num.storage));
  } else if (std::holds_alternative<double>(num.storage)) {
    val = std::get<double>(num.storage);
  } else {
    throw template_render_error{"round() expects numeric first argument"};
  }

  if (!std::holds_alternative<std::int64_t>(digits.storage)) {
    throw template_render_error{"round() expects integer second argument"};
  }

  auto const places = std::get<std::int64_t>(digits.storage);
  auto const factor = std::pow(10.0, static_cast<double>(places));
  return template_value{std::round(val * factor) / factor};
}

/// @brief Round a number to 0 decimal places (overload for single argument)
/// @param num Input number (int64_t or double)
/// @return Rounded value as double
[[nodiscard]] inline auto fn_round(template_value const& num) -> template_value {
  double val = 0.0;
  if (std::holds_alternative<std::int64_t>(num.storage)) {
    val = static_cast<double>(std::get<std::int64_t>(num.storage));
  } else if (std::holds_alternative<double>(num.storage)) {
    val = std::get<double>(num.storage);
  } else {
    throw template_render_error{"round() expects numeric argument"};
  }
  return template_value{std::round(val)};
}

/// @brief Get maximum value from array of numbers
/// @param arr Input array
/// @return Maximum value (int64_t if all integers, double otherwise)
[[nodiscard]] inline auto fn_max(template_array const& arr) -> template_value {
  if (arr.empty()) {
    throw template_render_error{"max() called on empty array"};
  }

  // First pass: determine if we have any doubles
  auto has_double = false;
  for (auto const& elem : arr) {
    if (std::holds_alternative<double>(elem.storage)) {
      has_double = true;
      break;
    }
  }

  if (has_double) {
    auto max_val = -std::numeric_limits<double>::infinity();
    for (auto const& elem : arr) {
      if (std::holds_alternative<std::int64_t>(elem.storage)) {
        max_val = std::max(max_val, static_cast<double>(std::get<std::int64_t>(elem.storage)));
      } else if (std::holds_alternative<double>(elem.storage)) {
        max_val = std::max(max_val, std::get<double>(elem.storage));
      } else {
        throw template_render_error{"max() array contains non-numeric value"};
      }
    }
    return template_value{max_val};
  } else {
    auto max_int = std::numeric_limits<std::int64_t>::min();
    for (auto const& elem : arr) {
      if (std::holds_alternative<std::int64_t>(elem.storage)) {
        max_int = std::max(max_int, std::get<std::int64_t>(elem.storage));
      } else {
        throw template_render_error{"max() array contains non-numeric value"};
      }
    }
    return template_value{max_int};
  }
}

/// @brief Get minimum value from array of numbers
/// @param arr Input array
/// @return Minimum value (int64_t if all integers, double otherwise)
[[nodiscard]] inline auto fn_min(template_array const& arr) -> template_value {
  if (arr.empty()) {
    throw template_render_error{"min() called on empty array"};
  }

  // First pass: determine if we have any doubles
  auto has_double = false;
  for (auto const& elem : arr) {
    if (std::holds_alternative<double>(elem.storage)) {
      has_double = true;
      break;
    }
  }

  if (has_double) {
    auto min_val = std::numeric_limits<double>::infinity();
    for (auto const& elem : arr) {
      if (std::holds_alternative<std::int64_t>(elem.storage)) {
        min_val = std::min(min_val, static_cast<double>(std::get<std::int64_t>(elem.storage)));
      } else if (std::holds_alternative<double>(elem.storage)) {
        min_val = std::min(min_val, std::get<double>(elem.storage));
      } else {
        throw template_render_error{"min() array contains non-numeric value"};
      }
    }
    return template_value{min_val};
  } else {
    auto min_int = std::numeric_limits<std::int64_t>::max();
    for (auto const& elem : arr) {
      if (std::holds_alternative<std::int64_t>(elem.storage)) {
        min_int = std::min(min_int, std::get<std::int64_t>(elem.storage));
      } else {
        throw template_render_error{"min() array contains non-numeric value"};
      }
    }
    return template_value{min_int};
  }
}

/// @brief Check if a number is even
/// @param num Input integer
/// @return true if even, false if odd
[[nodiscard]] inline auto fn_even(template_value const& num) -> template_value {
  if (!std::holds_alternative<std::int64_t>(num.storage)) {
    throw template_render_error{"even() expects integer argument"};
  }
  auto const val = std::get<std::int64_t>(num.storage);
  return template_value{val % 2 == 0};
}

/// @brief Check if a number is odd
/// @param num Input integer
/// @return true if odd, false if even
[[nodiscard]] inline auto fn_odd(template_value const& num) -> template_value {
  if (!std::holds_alternative<std::int64_t>(num.storage)) {
    throw template_render_error{"odd() expects integer argument"};
  }
  auto const val = std::get<std::int64_t>(num.storage);
  return template_value{val % 2 != 0};
}

/// @brief Check if a number is divisible by another
/// @param num Dividend (integer)
/// @param divisor Divisor (integer, non-zero)
/// @return true if divisible, false otherwise
[[nodiscard]] inline auto fn_divisibleBy(template_value const& num, template_value const& divisor) -> template_value {
  if (!std::holds_alternative<std::int64_t>(num.storage)) {
    throw template_render_error{"divisibleBy() expects integer first argument"};
  }
  if (!std::holds_alternative<std::int64_t>(divisor.storage)) {
    throw template_render_error{"divisibleBy() expects integer second argument"};
  }

  auto const val = std::get<std::int64_t>(num.storage);
  auto const div = std::get<std::int64_t>(divisor.storage);

  if (div == 0) {
    throw template_render_error{"divisibleBy() divisor cannot be zero"};
  }

  return template_value{val % div == 0};
}

/// @brief Convert value to integer
/// @param val Input value (any type)
/// @return Integer conversion
/// @throws template_render_error if conversion is invalid (object type or non-convertible string)
[[nodiscard]] inline auto fn_int(template_value const& val) noexcept(false) -> template_value {
  if (std::holds_alternative<std::int64_t>(val.storage)) {
    return val;
  }
  if (std::holds_alternative<double>(val.storage)) {
    return template_value{static_cast<std::int64_t>(std::get<double>(val.storage))};
  }
  if (std::holds_alternative<bool>(val.storage)) {
    return template_value{std::get<bool>(val.storage) ? 1 : 0};
  }
  if (std::holds_alternative<std::string>(val.storage)) {
    auto const& str = std::get<std::string>(val.storage);
    try {
      auto const result = std::stoll(str, nullptr, 10);
      return template_value{result};
    } catch (...) {
      throw template_render_error{"int() cannot convert string to integer"};
    }
  }
  if (std::holds_alternative<template_array>(val.storage)) {
    auto const& arr = std::get<template_array>(val.storage);
    return template_value{static_cast<std::int64_t>(arr.size())};
  }
  if (std::holds_alternative<template_null>(val.storage)) {
    return template_value{0};
  }
  throw template_render_error{"int() cannot convert object type"};
}

/// @brief Convert value to float
/// @param val Input value (any type)
/// @return Float conversion
/// @throws template_render_error if conversion is invalid (object type or non-convertible string)
[[nodiscard]] inline auto fn_float(template_value const& val) noexcept(false) -> template_value {
  if (std::holds_alternative<double>(val.storage)) {
    return val;
  }
  if (std::holds_alternative<std::int64_t>(val.storage)) {
    return template_value{static_cast<double>(std::get<std::int64_t>(val.storage))};
  }
  if (std::holds_alternative<bool>(val.storage)) {
    return template_value{std::get<bool>(val.storage) ? 1.0 : 0.0};
  }
  if (std::holds_alternative<std::string>(val.storage)) {
    auto const& str = std::get<std::string>(val.storage);
    try {
      auto const result = std::stod(str);
      return template_value{result};
    } catch (...) {
      throw template_render_error{"float() cannot convert string to float"};
    }
  }
  if (std::holds_alternative<template_array>(val.storage)) {
    auto const& arr = std::get<template_array>(val.storage);
    return template_value{static_cast<double>(arr.size())};
  }
  if (std::holds_alternative<template_null>(val.storage)) {
    return template_value{0.0};
  }
  throw template_render_error{"float() cannot convert object type"};
}

/// @brief Check if value is a string
/// @param val Input value
/// @return true if value is string type, false otherwise
[[nodiscard]] inline auto fn_isString(template_value const& val) -> template_value {
  return template_value{std::holds_alternative<std::string>(val.storage)};
}

/// @brief Check if value is an array
/// @param val Input value
/// @return true if value is array type, false otherwise
[[nodiscard]] inline auto fn_isArray(template_value const& val) -> template_value {
  return template_value{std::holds_alternative<template_array>(val.storage)};
}

/// @brief Check if value is numeric (int64_t or double)
/// @param val Input value
/// @return true if value is numeric type, false otherwise
[[nodiscard]] inline auto fn_isNumber(template_value const& val) -> template_value {
  return template_value{
    std::holds_alternative<std::int64_t>(val.storage) || 
    std::holds_alternative<double>(val.storage)
  };
}

/// @brief Check if value is an object
/// @param val Input value
/// @return true if value is object type, false otherwise
[[nodiscard]] inline auto fn_isObject(template_value const& val) -> template_value {
  return template_value{std::holds_alternative<template_object>(val.storage)};
}

/// @brief Check if value is a boolean
/// @param val Input value
/// @return true if value is bool type, false otherwise
[[nodiscard]] inline auto fn_isBoolean(template_value const& val) -> template_value {
  return template_value{std::holds_alternative<bool>(val.storage)};
}

/// @brief Check if value is a float (double)
/// @param val Input value
/// @return true if value is double type, false otherwise
[[nodiscard]] inline auto fn_isFloat(template_value const& val) -> template_value {
  return template_value{std::holds_alternative<double>(val.storage)};
}

/// @brief Check if value is an integer (int64_t)
/// @param val Input value
/// @return true if value is int64_t type, false otherwise
[[nodiscard]] inline auto fn_isInteger(template_value const& val) -> template_value {
  return template_value{std::holds_alternative<std::int64_t>(val.storage)};
}

/// @brief Check if value is null
/// @param val Input value
/// @return true if value is null, false otherwise
[[nodiscard]] inline auto fn_isNone(template_value const& val) -> template_value {
  return template_value{std::holds_alternative<template_null>(val.storage)};
}

/// @brief Check if value is empty
/// @param val Input value
/// @return true if value is empty (empty string, empty array, empty object, or null)
[[nodiscard]] inline auto fn_isEmpty(template_value const& val) -> template_value {
  if (std::holds_alternative<template_null>(val.storage)) {
    return template_value{true};
  }
  if (auto const* p = std::get_if<std::string>(&val.storage)) {
    return template_value{p->empty()};
  }
  if (auto const* p = std::get_if<template_array>(&val.storage)) {
    return template_value{p->empty()};
  }
  if (auto const* p = std::get_if<template_object>(&val.storage)) {
    return template_value{p->empty()};
  }
  return template_value{false};
}

// ============ Utility Functions ============

/// @brief Helper to check if a value is considered "empty" for the default function
/// @param val Input value
/// @return true if value is null or empty container/string
[[nodiscard]] inline auto is_empty_value(template_value const& val) -> bool {
  if (std::holds_alternative<template_null>(val.storage)) {
    return true;
  }
  if (auto const* p = std::get_if<std::string>(&val.storage)) {
    return p->empty();
  }
  if (auto const* p = std::get_if<template_array>(&val.storage)) {
    return p->empty();
  }
  if (auto const* p = std::get_if<template_object>(&val.storage)) {
    return p->empty();
  }
  return false;
}

/// @brief Helper to check if two template_values are equal
/// @param lhs Left hand side value
/// @param rhs Right hand side value
/// @return true if values are equal
[[nodiscard]] inline auto values_equal(template_value const& lhs, template_value const& rhs) -> bool {
  if (std::holds_alternative<template_null>(lhs.storage) && std::holds_alternative<template_null>(rhs.storage)) {
    return true;
  }
  if (auto const* p = std::get_if<bool>(&lhs.storage)) {
    if (auto const* q = std::get_if<bool>(&rhs.storage)) {
      return *p == *q;
    }
  }
  if (auto const* p = std::get_if<std::int64_t>(&lhs.storage)) {
    if (auto const* q = std::get_if<std::int64_t>(&rhs.storage)) {
      return *p == *q;
    }
    if (auto const* q = std::get_if<double>(&rhs.storage)) {
      return static_cast<double>(*p) == *q;
    }
  }
  if (auto const* p = std::get_if<double>(&lhs.storage)) {
    if (auto const* q = std::get_if<double>(&rhs.storage)) {
      return *p == *q;
    }
    if (auto const* q = std::get_if<std::int64_t>(&rhs.storage)) {
      return *p == static_cast<double>(*q);
    }
  }
  if (auto const* p = std::get_if<std::string>(&lhs.storage)) {
    if (auto const* q = std::get_if<std::string>(&rhs.storage)) {
      return *p == *q;
    }
  }
  return false;
}

/// @brief Returns value if not empty, otherwise returns fallback
/// @param val Input value to test
/// @param fallback Fallback value if val is empty
/// @return val if non-empty, fallback otherwise
[[nodiscard]] inline auto fn_default(template_value const& val, template_value const& fallback) noexcept(false) -> template_value {
  return is_empty_value(val) ? fallback : val;
}

/// @brief Gets element from array at index (0-based)
/// @param arr Array to access
/// @param index Index to retrieve (supports negative indices from end)
/// @return Element at index
/// @throws template_render_error on invalid input or out of bounds
[[nodiscard]] inline auto fn_at(template_value const& arr, template_value const& index) noexcept(false) -> template_value {
  if (!std::holds_alternative<template_array>(arr.storage)) {
    throw template_render_error{"at() expects array as first argument"};
  }
  
  if (!std::holds_alternative<std::int64_t>(index.storage)) {
    throw template_render_error{"at() expects integer as index"};
  }
  
  auto const& array = std::get<template_array>(arr.storage);
  auto idx = std::get<std::int64_t>(index.storage);
  auto const size = static_cast<std::int64_t>(array.size());
  
  if (idx < 0) {
    idx = size + idx;
  }
  
  if (idx < 0 || idx >= size) {
    throw template_render_error{"at() index out of bounds"};
  }
  
  return array[static_cast<std::size_t>(idx)];
}

/// @brief Checks if array or object is not empty
/// @param val Array or object to check
/// @return true if val is non-empty array or object, false otherwise
[[nodiscard]] inline auto fn_exists(template_value const& val) noexcept(false) -> template_value {
  if (auto const* p = std::get_if<template_array>(&val.storage)) {
    return template_value{!p->empty()};
  }
  if (auto const* p = std::get_if<template_object>(&val.storage)) {
    return template_value{!p->empty()};
  }
  return template_value{false};
}

/// @brief Checks if value exists in array or matches value in object
/// @param val Value to search for
/// @param container Array or object to search in
/// @return true if value found in array or matches any object value
/// @throws template_render_error if container is not array/object
[[nodiscard]] inline auto fn_existsIn(template_value const& val, template_value const& container) noexcept(false) -> template_value {
  if (auto const* p = std::get_if<template_array>(&container.storage)) {
    for (auto const& elem : *p) {
      if (values_equal(val, elem)) {
        return template_value{true};
      }
    }
    return template_value{false};
  }
  
  if (auto const* p = std::get_if<template_object>(&container.storage)) {
    for (auto const& [k, v] : *p) {
      if (values_equal(val, v)) {
        return template_value{true};
      }
    }
    return template_value{false};
  }
  
  throw template_render_error{"existsIn() expects array or object as second argument"};
}

} // namespace frozenchars

namespace frozenchars::inja {

} // namespace frozenchars::inja
