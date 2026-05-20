#pragma once

#include <cstdint>
#include <initializer_list>
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

} // namespace frozenchars
