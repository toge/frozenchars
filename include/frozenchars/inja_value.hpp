#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "inja_types.hpp"

#ifndef FROZENCHARS_OBJECT_MAP_HEADER
#include <unordered_map>
#define FROZENCHARS_OBJECT_MAP_HEADER
#endif

namespace frozenchars::inja {

/**
 * @brief テンプレート値における null を表すタグ型
 */
struct inja_null final {
  bool operator==(inja_null const&) const noexcept {
    return true;
  }
};

/**
 * @brief テンプレート評価時に使う動的値型
 */
struct inja_value;

/**
 * @brief テンプレート配列値の実体型
 */
using inja_array = std::variant<
  std::vector<inja_value>,   // 混合型（従来どおり）
  std::vector<std::int64_t>, // 整数特化
  std::vector<double>,       // 浮動小数特化
  std::vector<std::string>   // 文字列特化
>;

/**
 * @brief テンプレートオブジェクト値の実体型
 *
 * FROZENCHARS_OBJECT_MAP マクロで選択可能。
 * 未指定時は透過検索対応の `std::unordered_map` を利用する。
 * コンパイル時に -DFROZENCHARS_OBJECT_MAP_HEADER や define で指定してカスタマイズ可能。
 * 例: -DFROZENCHARS_OBJECT_MAP=ankerl::unordered_dense::map
 */
#ifndef FROZENCHARS_OBJECT_MAP
struct transparent_string_hash {
  using is_transparent = void;

  [[nodiscard]] auto operator()(std::string_view s) const noexcept -> std::size_t {
    return std::hash<std::string_view>{}(s);
  }

  [[nodiscard]] auto operator()(std::string const& s) const noexcept -> std::size_t {
    return std::hash<std::string_view>{}(s);
  }

  [[nodiscard]] auto operator()(char const* s) const noexcept -> std::size_t {
    return std::hash<std::string_view>{}(s);
  }
};

using inja_object = std::unordered_map<std::string, inja_value, transparent_string_hash, std::equal_to<>>;
#else
using inja_object = FROZENCHARS_OBJECT_MAP<std::string, inja_value>;
#endif

/**
 * @brief テンプレート言語で扱う値の共用体ラッパ
 *
 * `null / bool / 整数 / 浮動小数 / 文字列 / 配列 / オブジェクト`
 * を単一の型で扱うためのコンテナ。
 */
struct inja_value {
  /**
   * @brief 内部ストレージ型。
   */
  using storage_type = std::variant<inja_null, bool, std::int64_t, double, std::string, inja_array, inja_object>;

  /**
   * @brief 値の実体。
   */
  storage_type storage{};

  /**
   * @brief null 値で初期化する。
   */
  inja_value() : storage(inja_null{}) {}

  /**
   * @brief nullptr を null 値として受け取る。
   */
  inja_value(std::nullptr_t) : storage(inja_null{}) {}

  /**
   * @brief 真偽値で初期化する。
   */
  inja_value(bool v) : storage(v) {}

  /**
   * @brief C 文字列を文字列値として初期化する。
   */
  inja_value(char const* v) : storage(std::string{v}) {}

  /**
   * @brief std::string で初期化する。
   */
  inja_value(std::string v) : storage(std::move(v)) {}

  /**
   * @brief 配列値で初期化する。
   */
  inja_value(inja_array v) : storage(std::move(v)) {}

  /**
   * @brief オブジェクト値で初期化する。
   */
  inja_value(inja_object v) : storage(std::move(v)) {}

  /**
   * @brief 整数型を int64 値として保持する。
   * @tparam Int bool 以外の整数型
   * @param v 入力値
   */
  template <typename Int>
  requires(std::is_integral_v<std::remove_cvref_t<Int>> && !std::is_same_v<std::remove_cvref_t<Int>, bool>)
  inja_value(Int v) : storage(static_cast<std::int64_t>(v)) {}

  /**
   * @brief 浮動小数型を double 値として保持する
   * @tparam Float 浮動小数型
   * @param v 入力値
   */
  template <typename Float>
  requires(std::is_floating_point_v<std::remove_cvref_t<Float>>)
  inja_value(Float v) : storage(static_cast<double>(v)) {}

  /**
   * @brief 値の等価性を判定する。
   *
   * variant の各型が等しいかどうかを判定する。
   * 数値型の型を跨いだ比較（int vs double）は equals_value を使用すること。
   */
  bool operator==(inja_value const& rhs) const {
    return storage == rhs.storage;
  }
};

/**
 * @brief 値が null か判定する。
 * @param v 対象値
 * @return null なら true
 */
[[nodiscard]] inline auto is_null(inja_value const& v) -> bool {
  return std::holds_alternative<inja_null>(v.storage);
}

/**
 * @brief 値を bool として取り出す。
 * @param v 対象値
 * @return bool 値
 * @throws inja_render_error 型が不一致の場合
 */
[[nodiscard]] inline auto as_bool(inja_value const& v) -> bool {
  if (auto const* p = std::get_if<bool>(&v.storage)) {
    return *p;
  }
  throw render_error{"value is not bool"};
}

/**
 * @brief 値を int64 として取り出す。
 * @param v 対象値
 * @return int64 値
 * @throws inja_render_error 型が不一致の場合
 */
[[nodiscard]] inline auto as_int(inja_value const& v) -> std::int64_t {
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return *p;
  }
  throw render_error{"value is not int"};
}

/**
 * @brief 値を double として取り出す。
 *
 * int64 は double に昇格して返す。
 *
 * @param v 対象値
 * @return 数値
 * @throws inja_render_error 数値型でない場合
 */
[[nodiscard]] inline auto as_double(inja_value const& v) -> double {
  if (auto const* p = std::get_if<double>(&v.storage)) {
    return *p;
  }
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return static_cast<double>(*p);
  }
  throw render_error{"value is not number"};
}

/**
 * @brief 値を文字列参照として取り出す。
 * @param v 対象値
 * @return 文字列参照
 * @throws inja_render_error 型が不一致の場合
 */
[[nodiscard]] inline auto as_string(inja_value const& v) -> std::string const& {
  if (auto const* p = std::get_if<std::string>(&v.storage)) {
    return *p;
  }
  throw render_error{"value is not string"};
}

/**
 * @brief 値を配列参照として取り出す。
 * @param v 対象値
 * @return 配列参照
 * @throws inja_render_error 型が不一致の場合
 */
[[nodiscard]] inline auto as_array(inja_value const& v) -> inja_array const& {
  if (auto const* p = std::get_if<inja_array>(&v.storage)) {
    return *p;
  }
  throw render_error{"value is not array"};
}

/**
 * @brief 値をオブジェクト参照として取り出す。
 * @param v 対象値
 * @return オブジェクト参照
 * @throws inja_render_error 型が不一致の場合
 */
[[nodiscard]] inline auto as_object(inja_value const& v) -> inja_object const& {
  if (auto const* p = std::get_if<inja_object>(&v.storage)) {
    return *p;
  }
  throw render_error{"value is not object"};
}

/**
 * @brief 初期化リストから配列値を生成する。
 * @param items 要素列
 * @return inja_value(配列)
 */
[[nodiscard]] inline auto make_array(std::initializer_list<inja_value> items) -> inja_value {
  return inja_value{inja_array{std::vector<inja_value>{items}}};
}

/**
 * @brief 任意の個数の引数から配列値を生成する。
 * @tparam Args 引数型
 * @param args 要素の並び
 * @return inja_value(配列)
 */
template <typename... Args>
requires(sizeof...(Args) != 1 || !requires { inja_array{std::declval<Args>()...}; })
[[nodiscard]] inline auto make_array(Args&&... args) -> inja_value {
  return inja_value{inja_array{std::vector<inja_value>{inja_value{std::forward<Args>(args)}...}}};
}

/**
 * @brief 整数配列を生成する。
 * @param v 整数ベクタ
 * @return inja_value(配列)
 */
[[nodiscard]] inline auto make_array(std::vector<std::int64_t> v) -> inja_value {
  return inja_value{inja_array{std::move(v)}};
}

/**
 * @brief 浮動小数配列を生成する。
 * @param v 浮動小数ベクタ
 * @return inja_value(配列)
 */
[[nodiscard]] inline auto make_array(std::vector<double> v) -> inja_value {
  return inja_value{inja_array{std::move(v)}};
}

/**
 * @brief 文字列配列を生成する。
 * @param v 文字列ベクタ
 * @return inja_value(配列)
 */
[[nodiscard]] inline auto make_array(std::vector<std::string> v) -> inja_value {
  return inja_value{inja_array{std::move(v)}};
}

/**
 * @brief 初期化リストからオブジェクト値を生成する。
 *
 * 事前に容量確保し、ハッシュテーブル再配置を最小化して高速化。
 *
 * @param items キー・値列
 * @return inja_value(オブジェクト)
 */
[[nodiscard]] inline auto make_object(std::initializer_list<std::pair<std::string, inja_value>> items) -> inja_value {
  auto out = inja_object{};
  out.reserve(items.size());
  for (auto const& [k, v] : items) {
    out.insert({k, v});
  }
  return inja_value{std::move(out)};
}

/**
 * @brief 任意の個数の引数からオブジェクト値を生成する。
 *
 * キーと値を交互に指定する。
 *
 * @tparam Args 引数型
 * @param args キーと値の並び
 * @return inja_value(オブジェクト)
 */
template <typename... Args>
requires(sizeof...(Args) % 2 == 0)
[[nodiscard]] inline auto make_object(Args&&... args) -> inja_value {
  auto out = inja_object{};
  if constexpr (sizeof...(Args) > 0) {
    out.reserve(sizeof...(Args) / 2);
    [&out](this auto self, auto&& k, auto&& v, auto&&... rest) {
      out.emplace(std::forward<decltype(k)>(k), std::forward<decltype(v)>(v));
      if constexpr (sizeof...(rest) > 0) {
        self(std::forward<decltype(rest)>(rest)...);
      }
    }(std::forward<Args>(args)...);
  }
  return inja_value{std::move(out)};
}

/**
 * @brief inja 互換寄りの真偽値変換
 *
 * false 扱い: null / false / 0 / 0.0 / 空文字 / 空配列 / 空オブジェクト
 * true 扱い: 上記以外
 *
 * @param v 対象値
 * @return 真偽値
 */
[[nodiscard]] inline auto truthy(inja_value const& v) -> bool {
  if (std::holds_alternative<inja_null>(v.storage)) {
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
  if (auto const* p = std::get_if<inja_array>(&v.storage)) {
    return std::visit([](auto const& vec) { return !vec.empty(); }, *p);
  }
  return !std::get<inja_object>(v.storage).empty();
}

/**
 * @brief 初期化リストから配列値を生成する
 * @param items 要素列
 * @return inja_value(配列)
 */
[[nodiscard]] inline auto array(std::initializer_list<inja_value> items) -> inja_value {
  return make_array(items);
}

/**
 * @brief 任意の個数の引数から配列値を生成する
 * @tparam Args 引数型
 * @param args 要素の並び
 * @return inja_value(配列)
 */
template <typename... Args>
[[nodiscard]] inline auto array(Args&&... args) -> inja_value {
  return make_array(std::forward<Args>(args)...);
}

/**
 * @brief 初期化リストからオブジェクト値を生成する
 *
 * 事前に容量確保し、ハッシュテーブル再配置を最小化して高速化。
 *
 * @param items キー・値列
 * @return inja_value(オブジェクト)
 */
[[nodiscard]] inline auto object(std::initializer_list<std::pair<std::string, inja_value>> items) -> inja_value {
  return make_object(items);
}

/**
 * @brief 任意の個数の引数からオブジェクト値を生成する
 *
 * キーと値を交互に指定する。
 *
 * @tparam Args 引数型
 * @param args キーと値の並び
 * @return inja_value(オブジェクト)
 */
template <typename... Args>
requires(sizeof...(Args) % 2 == 0)
[[nodiscard]] inline auto object(Args&&... args) -> inja_value {
  return make_object(std::forward<Args>(args)...);
}

/**
 * @brief 値をテンプレート出力用文字列に変換する
 *
 * 配列とオブジェクトは現状の簡易表現（`[array]`, `{object}`）を返す。
 *
 * @param v 対象値
 * @return 出力文字列
 */
[[nodiscard]] inline auto to_string(inja_value const& v) -> std::string {
  if (std::holds_alternative<inja_null>(v.storage)) {
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
  if (std::holds_alternative<inja_array>(v.storage)) {
    return "[array]";
  }
  return "{object}";
}

/**
 * @brief 文字列を大文字に変換する。
 *
 * @param str 入力文字列ビュー
 * @return 大文字化した文字列
 */
[[nodiscard]] inline auto fn_upper(std::string_view str) -> std::string {
  auto result = std::string{str};
  for (auto& c : result) {
    c = std::toupper(static_cast<unsigned char>(c));
  }
  return result;
}

/**
 * @brief 文字列を小文字に変換する。
 *
 * @param str 入力文字列ビュー
 * @return 小文字化した文字列
 */
[[nodiscard]] inline auto fn_lower(std::string_view str) -> std::string {
  auto result = std::string{str};
  for (auto& c : result) {
    c = std::tolower(static_cast<unsigned char>(c));
  }
  return result;
}

/**
 * @brief 文字列の先頭文字を大文字にする。
 *
 * @param str 入力文字列ビュー
 * @return 先頭文字を大文字にした文字列
 */
[[nodiscard]] inline auto fn_capitalize(std::string_view str) -> std::string {
  if (str.empty()) {
    return std::string{str};
  }
  auto result = std::string{str};
  result[0] = std::toupper(static_cast<unsigned char>(result[0]));
  return result;
}

/**
 * @brief 文字列中のすべての一致部分を置換する。
 *
 * @param str 入力文字列ビュー
 * @param old_str 検索する部分文字列
 * @param new_str 置換後の部分文字列
 * @return 一致をすべて置換した文字列
 */
[[nodiscard]] inline auto fn_replace(std::string_view str, std::string_view old_str, std::string_view new_str) -> std::string {
  auto result = std::string{str};
  auto pos = size_t{0};
  while ((pos = result.find(old_str, pos)) != std::string::npos) {
    result.replace(pos, old_str.length(), new_str);
    pos += new_str.length();
  }
  return result;
}

/**
 * @brief 配列の要素数を返す
 *
 * @param arr 入力配列
 * @return 要素数
 */
[[nodiscard]] inline auto fn_length(inja_array const& arr) -> std::int64_t {
  return std::visit([](auto const& vec) { return static_cast<std::int64_t>(vec.size()); }, arr);
}

/**
 * @brief 配列の先頭要素を返す
 *
 * @param arr 入力配列
 * @return 先頭要素（空なら例外を投げる）
 */
[[nodiscard]] inline auto fn_first(inja_array const& arr) -> inja_value {
  return std::visit([](auto const& vec) -> inja_value {
    if (vec.empty()) {
      throw render_error{"first() called on empty array"};
    }
    return inja_value{vec[0]};
  }, arr);
}

/**
 * @brief 配列の末尾要素を返す
 *
 * @param arr 入力配列
 * @return 末尾要素（空なら例外を投げる）
 */
[[nodiscard]] inline auto fn_last(inja_array const& arr) -> inja_value {
  return std::visit([](auto const& vec) -> inja_value {
    if (vec.empty()) {
      throw render_error{"last() called on empty array"};
    }
    return inja_value{vec[vec.size() - 1]};
  }, arr);
}

/**
 * @brief 配列の要素を文字列化して、指定された区切り文字で連結する。
 *
 * @param arr 入力配列
 * @param sep 区切り文字列
 * @return 連結結果
 */
[[nodiscard]] inline auto fn_join(inja_array const& arr, std::string_view sep) -> std::string {
  return std::visit([&](auto const& vec) -> std::string {
    if (vec.empty()) {
      return "";
    }

    auto result = std::string{};
    for (auto i = std::size_t{0}; i < vec.size(); ++i) {
      if (i > 0) {
        result += sep;
      }

      auto const& elem = vec[i];
      if constexpr (std::is_same_v<std::decay_t<decltype(elem)>, inja_value>) {
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
      } else if constexpr (std::is_same_v<std::decay_t<decltype(elem)>, std::string>) {
        result += elem;
      } else if constexpr (std::is_same_v<std::decay_t<decltype(elem)>, bool>) {
        result += elem ? "true" : "false";
      } else {
        result += std::to_string(elem);
      }
    }
    return result;
  }, arr);
}

/**
 * @brief 配列をソートする（新しい並び順のコピーを返す）
 *
 * @param arr 入力配列
 * @return ソート済みの新しい配列
 */
[[nodiscard]] inline auto fn_sort(inja_array const& arr) -> inja_array {
  return std::visit([](auto const& vec) -> inja_array {
    auto result = vec;
    using T = typename std::decay_t<decltype(vec)>::value_type;
    if constexpr (std::is_same_v<T, inja_value>) {
      std::sort(result.begin(), result.end(), [](inja_value const& a, inja_value const& b) {
        // 可能なら double に変換するヘルパーラムダ。
        auto to_double = [](inja_value const& v) -> std::optional<double> {
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

        // 文字列同士の比較。
        if (std::holds_alternative<std::string>(a.storage) && std::holds_alternative<std::string>(b.storage)) {
          return std::get<std::string>(a.storage) < std::get<std::string>(b.storage);
        }

        // 型が異なる場合は、数値を文字列より前に置く。
        if (a_num && std::holds_alternative<std::string>(b.storage)) {
          return true;
        }
        if (std::holds_alternative<std::string>(a.storage) && b_num) {
          return false;
        }

        return false;
      });
    } else {
      std::sort(result.begin(), result.end());
    }
    return inja_array{std::move(result)};
  }, arr);
}

/**
 * @brief 整数の範囲を生成する
 *
 * @param end 上限（含まない）
 * @return 配列 [0, 1, 2, ..., end-1]
 */
[[nodiscard]] inline auto fn_range(std::int64_t end) -> inja_array {
  auto result = std::vector<std::int64_t>{};
  if (end > 0) {
    result.reserve(static_cast<std::size_t>(end));
    for (auto i = std::int64_t{0}; i < end; ++i) {
      result.push_back(i);
    }
  }
  return inja_array{std::move(result)};
}

/**
 * @brief 開始値と終了値から整数の範囲を生成する
 *
 * @param start 開始値（含む）
 * @param end 終了値（含まない）
 * @return 配列 [start, start+1, ..., end-1]
 */
[[nodiscard]] inline auto fn_range(std::int64_t start, std::int64_t end) -> inja_array {
  auto result = std::vector<std::int64_t>{};
  if (end > start) {
    result.reserve(static_cast<std::size_t>(end - start));
    for (auto i = start; i < end; ++i) {
      result.push_back(i);
    }
  }
  return inja_array{std::move(result)};
}

/**
 * @brief 開始値・終了値・ステップから整数の範囲を生成する
 *
 * @param start 開始値（含む）
 * @param end 終了値（含まない）
 * @param step ステップ値
 * @return step 間隔の値を持つ配列
 */
[[nodiscard]] inline auto fn_range(std::int64_t start, std::int64_t end, std::int64_t step) -> inja_array {
  auto result = std::vector<std::int64_t>{};
  if (step > 0) {
    if (end > start) {
      result.reserve(static_cast<std::size_t>((end - start + step - 1) / step));
      for (auto i = start; i < end; i += step) {
        result.push_back(i);
      }
    }
  } else if (step < 0) {
    if (start > end) {
      result.reserve(static_cast<std::size_t>((start - end + (-step) - 1) / (-step)));
      for (auto i = start; i > end; i += step) {
        result.push_back(i);
      }
    }
  }
  return inja_array{std::move(result)};
}

/**
 * @brief 数値の絶対値を返す
 *
 * @param num 入力数値（int64_t または double）
 * @return 入力と同じ型の絶対値
 */
[[nodiscard]] inline auto fn_abs(inja_value const& num) -> inja_value {
  if (std::holds_alternative<std::int64_t>(num.storage)) {
    auto const val = std::get<std::int64_t>(num.storage);
    return inja_value{std::abs(val)};
  }
  if (std::holds_alternative<double>(num.storage)) {
    auto const val = std::get<double>(num.storage);
    return inja_value{std::abs(val)};
  }
  throw render_error{"abs() expects numeric argument"};
}

/**
 * @brief 数値を指定した小数桁で丸める
 *
 * @param num 入力数値（int64_t または double）
 * @param digits 小数桁数
 * @return 指定した小数桁で丸めた値
 */
[[nodiscard]] inline auto fn_round(inja_value const& num, inja_value const& digits) -> inja_value {
  double val = 0.0;
  if (std::holds_alternative<std::int64_t>(num.storage)) {
    val = static_cast<double>(std::get<std::int64_t>(num.storage));
  } else if (std::holds_alternative<double>(num.storage)) {
    val = std::get<double>(num.storage);
  } else {
    throw render_error{"round() expects numeric first argument"};
  }

  if (!std::holds_alternative<std::int64_t>(digits.storage)) {
    throw render_error{"round() expects integer second argument"};
  }

  auto const places = std::get<std::int64_t>(digits.storage);
  auto const factor = std::pow(10.0, static_cast<double>(places));
  return inja_value{std::round(val * factor) / factor};
}

/**
 * @brief 数値を小数 0 桁に丸める（単一引数版）
 *
 * @param num 入力数値（int64_t または double）
 * @return double として丸めた値
 */
[[nodiscard]] inline auto fn_round(inja_value const& num) -> inja_value {
  double val = 0.0;
  if (std::holds_alternative<std::int64_t>(num.storage)) {
    val = static_cast<double>(std::get<std::int64_t>(num.storage));
  } else if (std::holds_alternative<double>(num.storage)) {
    val = std::get<double>(num.storage);
  } else {
    throw render_error{"round() expects numeric argument"};
  }
  return inja_value{std::round(val)};
}

/**
 * @brief 数値配列の最大値を返す
 *
 * @param arr 入力配列
 * @return 最大値（すべて整数なら int64_t、それ以外は double）
 */
[[nodiscard]] inline auto fn_max(inja_array const& arr) -> inja_value {
  return std::visit([](auto const& vec) -> inja_value {
    if (vec.empty()) {
      throw render_error{"max() called on empty array"};
    }

    using T = typename std::decay_t<decltype(vec)>::value_type;
    if constexpr (std::is_same_v<T, inja_value>) {
      // 先に double が含まれるかを判定する。
      auto has_double = false;
      for (auto const& elem : vec) {
        if (std::holds_alternative<double>(elem.storage)) {
          has_double = true;
          break;
        }
      }

      if (has_double) {
        auto max_val = -std::numeric_limits<double>::infinity();
        for (auto const& elem : vec) {
          if (std::holds_alternative<std::int64_t>(elem.storage)) {
            max_val = std::max(max_val, static_cast<double>(std::get<std::int64_t>(elem.storage)));
          } else if (std::holds_alternative<double>(elem.storage)) {
            max_val = std::max(max_val, std::get<double>(elem.storage));
          } else {
            throw render_error{"max() array contains non-numeric value"};
          }
        }
        return inja_value{max_val};
      } else {
        auto max_int = std::numeric_limits<std::int64_t>::min();
        for (auto const& elem : vec) {
          if (std::holds_alternative<std::int64_t>(elem.storage)) {
            max_int = std::max(max_int, std::get<std::int64_t>(elem.storage));
          } else {
            throw render_error{"max() array contains non-numeric value"};
          }
        }
        return inja_value{max_int};
      }
    } else if constexpr (std::is_arithmetic_v<T>) {
      return inja_value{*std::max_element(vec.begin(), vec.end())};
    } else {
      throw render_error{"max() array contains non-numeric value"};
    }
  }, arr);
}

/**
 * @brief 数値配列の最小値を返す
 *
 * @param arr 入力配列
 * @return 最小値（すべて整数なら int64_t、それ以外は double）
 */
[[nodiscard]] inline auto fn_min(inja_array const& arr) -> inja_value {
  return std::visit([](auto const& vec) -> inja_value {
    if (vec.empty()) {
      throw render_error{"min() called on empty array"};
    }

    using T = typename std::decay_t<decltype(vec)>::value_type;
    if constexpr (std::is_same_v<T, inja_value>) {
      // 先に double が含まれるかを判定する。
      auto has_double = false;
      for (auto const& elem : vec) {
        if (std::holds_alternative<double>(elem.storage)) {
          has_double = true;
          break;
        }
      }

      if (has_double) {
        auto min_val = std::numeric_limits<double>::infinity();
        for (auto const& elem : vec) {
          if (std::holds_alternative<std::int64_t>(elem.storage)) {
            min_val = std::min(min_val, static_cast<double>(std::get<std::int64_t>(elem.storage)));
          } else if (std::holds_alternative<double>(elem.storage)) {
            min_val = std::min(min_val, std::get<double>(elem.storage));
          } else {
            throw render_error{"min() array contains non-numeric value"};
          }
        }
        return inja_value{min_val};
      } else {
        auto min_int = std::numeric_limits<std::int64_t>::max();
        for (auto const& elem : vec) {
          if (std::holds_alternative<std::int64_t>(elem.storage)) {
            min_int = std::min(min_int, std::get<std::int64_t>(elem.storage));
          } else {
            throw render_error{"min() array contains non-numeric value"};
          }
        }
        return inja_value{min_int};
      }
    } else if constexpr (std::is_arithmetic_v<T>) {
      return inja_value{*std::min_element(vec.begin(), vec.end())};
    } else {
      throw render_error{"min() array contains non-numeric value"};
    }
  }, arr);
}

/**
 * @brief 数値が偶数か判定する。
 *
 * @param num 入力整数
 * @return 偶数なら true、奇数なら false
 */
[[nodiscard]] inline auto fn_even(inja_value const& num) -> inja_value {
  if (!std::holds_alternative<std::int64_t>(num.storage)) {
    throw render_error{"even() expects integer argument"};
  }
  auto const val = std::get<std::int64_t>(num.storage);
  return inja_value{val % 2 == 0};
}

/**
 * @brief 数値が奇数か判定する。
 *
 * @param num 入力整数
 * @return 奇数なら true、偶数なら false
 */
[[nodiscard]] inline auto fn_odd(inja_value const& num) -> inja_value {
  if (!std::holds_alternative<std::int64_t>(num.storage)) {
    throw render_error{"odd() expects integer argument"};
  }
  auto const val = std::get<std::int64_t>(num.storage);
  return inja_value{val % 2 != 0};
}

/**
 * @brief ある数値が別の数値で割り切れるか判定する。
 *
 * @param num 被除数（整数）
 * @param divisor 除数（0 以外の整数）
 * @return 割り切れるなら true、それ以外は false
 */
[[nodiscard]] inline auto fn_divisibleBy(inja_value const& num, inja_value const& divisor) -> inja_value {
  if (!std::holds_alternative<std::int64_t>(num.storage)) {
    throw render_error{"divisibleBy() expects integer first argument"};
  }
  if (!std::holds_alternative<std::int64_t>(divisor.storage)) {
    throw render_error{"divisibleBy() expects integer second argument"};
  }

  auto const val = std::get<std::int64_t>(num.storage);
  auto const div = std::get<std::int64_t>(divisor.storage);

  if (div == 0) {
    throw render_error{"divisibleBy() divisor cannot be zero"};
  }

  return inja_value{val % div == 0};
}

/**
 * @brief 値を整数に変換する
 *
 * @param val 入力値（任意の型）
 * @return 整数変換結果
 * @throws inja_render_error 変換できない場合（オブジェクト型や変換不能な文字列）
 */
[[nodiscard]] inline auto fn_int(inja_value const& val) noexcept(false) -> inja_value {
  if (std::holds_alternative<std::int64_t>(val.storage)) {
    return val;
  }
  if (std::holds_alternative<double>(val.storage)) {
    return inja_value{static_cast<std::int64_t>(std::get<double>(val.storage))};
  }
  if (std::holds_alternative<bool>(val.storage)) {
    return inja_value{std::get<bool>(val.storage) ? 1 : 0};
  }
  if (std::holds_alternative<std::string>(val.storage)) {
    auto const& str = std::get<std::string>(val.storage);
    try {
      auto const result = std::stoll(str, nullptr, 10);
      return inja_value{result};
    } catch (...) {
      throw render_error{"int() cannot convert string to integer"};
    }
  }
  if (std::holds_alternative<inja_array>(val.storage)) {
    auto const& arr = std::get<inja_array>(val.storage);
    return inja_value{std::visit([](auto const& vec) { return static_cast<std::int64_t>(vec.size()); }, arr)};
  }
  if (std::holds_alternative<inja_null>(val.storage)) {
    return inja_value{0};
  }
  throw render_error{"int() cannot convert object type"};
}

/**
 * @brief 値を浮動小数に変換する
 *
 * @param val 入力値（任意の型）
 * @return 浮動小数変換結果
 * @throws inja_render_error 変換できない場合（オブジェクト型や変換不能な文字列）
 */
[[nodiscard]] inline auto fn_float(inja_value const& val) noexcept(false) -> inja_value {
  if (std::holds_alternative<double>(val.storage)) {
    return val;
  }
  if (std::holds_alternative<std::int64_t>(val.storage)) {
    return inja_value{static_cast<double>(std::get<std::int64_t>(val.storage))};
  }
  if (std::holds_alternative<bool>(val.storage)) {
    return inja_value{std::get<bool>(val.storage) ? 1.0 : 0.0};
  }
  if (std::holds_alternative<std::string>(val.storage)) {
    auto const& str = std::get<std::string>(val.storage);
    try {
      auto const result = std::stod(str);
      return inja_value{result};
    } catch (...) {
      throw render_error{"float() cannot convert string to float"};
    }
  }
  if (std::holds_alternative<inja_array>(val.storage)) {
    auto const& arr = std::get<inja_array>(val.storage);
    return inja_value{std::visit([](auto const& vec) { return static_cast<double>(vec.size()); }, arr)};
  }
  if (std::holds_alternative<inja_null>(val.storage)) {
    return inja_value{0.0};
  }
  throw render_error{"float() cannot convert object type"};
}

/**
 * @brief 値が文字列か判定する
 *
 * @param val 判定対象の値
 * @return inja_value 文字列型なら true、それ以外は false
 */
[[nodiscard]] inline auto fn_isString(inja_value const& val) -> inja_value {
  return inja_value{std::holds_alternative<std::string>(val.storage)};
}

/**
 * @brief 値が配列か判定する
 *
 * @param val 判定対象の値
 * @return inja_value 配列型なら true、それ以外は false
 */
[[nodiscard]] inline auto fn_isArray(inja_value const& val) -> inja_value {
  return inja_value{std::holds_alternative<inja_array>(val.storage)};
}

/**
 * @brief 値が数値型か判定する（int64_t または double）。
 *
 * @param val 判定対象の値
 * @return inja_value 数値型なら true、それ以外は false
 */
[[nodiscard]] inline auto fn_isNumber(inja_value const& val) -> inja_value {
  return inja_value{
    std::holds_alternative<std::int64_t>(val.storage) ||
    std::holds_alternative<double>(val.storage)
  };
}

/**
 * @brief 値がオブジェクトか判定する
 *
 * @param val 判定対象の値
 * @return inja_value オブジェクト型なら true、それ以外は false
 */
[[nodiscard]] inline auto fn_isObject(inja_value const& val) -> inja_value {
  return inja_value{std::holds_alternative<inja_object>(val.storage)};
}

/**
 * @brief 値が真偽値か判定する
 *
 * @param val 判定対象の値
 * @return inja_value bool 型なら true、それ以外は false
 */
[[nodiscard]] inline auto fn_isBoolean(inja_value const& val) -> inja_value {
  return inja_value{std::holds_alternative<bool>(val.storage)};
}

/**
 * @brief 値が浮動小数か判定する
 *
 * @param val 判定対象の値
 * @return inja_value double 型なら true、それ以外は false
 */
[[nodiscard]] inline auto fn_isFloat(inja_value const& val) -> inja_value {
  return inja_value{std::holds_alternative<double>(val.storage)};
}

/**
 * @brief 値が整数か判定する
 *
 * @param val 判定対象の値
 * @return inja_value int64_t 型なら true、それ以外は false
 */
[[nodiscard]] inline auto fn_isInteger(inja_value const& val) -> inja_value {
  return inja_value{std::holds_alternative<std::int64_t>(val.storage)};
}

/**
 * @brief 値が null か判定する
 *
 * @param val 判定対象の値
 * @return inja_value null なら true、それ以外は false
 */
[[nodiscard]] inline auto fn_isNone(inja_value const& val) -> inja_value {
  return inja_value{std::holds_alternative<inja_null>(val.storage)};
}

/**
 * @brief 値が空か判定する
 *
 * @param val 判定対象の値
 * @return inja_value 空なら true、それ以外は false
 */
[[nodiscard]] inline auto fn_isEmpty(inja_value const& val) -> inja_value {
  if (std::holds_alternative<inja_null>(val.storage)) {
    return inja_value{true};
  }
  if (auto const* p = std::get_if<std::string>(&val.storage)) {
    return inja_value{p->empty()};
  }
  if (auto const* p = std::get_if<inja_array>(&val.storage)) {
    return inja_value{std::visit([](auto const& vec) { return vec.empty(); }, *p)};
  }
  if (auto const* p = std::get_if<inja_object>(&val.storage)) {
    return inja_value{p->empty()};
  }
  return inja_value{false};
}

/**
 * @brief 配列を整数特化配列へ変換する。
 * @param v 変換対象の配列を持つ値
 * @return 整数特化配列を持つ inja_value
 * @throws render_error 変換できない要素が含まれる場合
 */
[[nodiscard]] inline auto fn_as_int_array(inja_value const& v) -> inja_value {
  if (!std::holds_alternative<inja_array>(v.storage)) {
    throw render_error{"as_int_array() expects array argument"};
  }
  auto const& arr = std::get<inja_array>(v.storage);
  return std::visit([](auto const& vec) -> inja_value {
    using T = typename std::decay_t<decltype(vec)>::value_type;
    if constexpr (std::is_same_v<T, std::int64_t>) {
      return inja_value{inja_array{vec}};
    } else {
      auto result = std::vector<std::int64_t>{};
      result.reserve(vec.size());
      for (auto const& elem : vec) {
        if constexpr (std::is_same_v<T, inja_value>) {
          if (auto const* p = std::get_if<std::int64_t>(&elem.storage)) {
            result.push_back(*p);
          } else {
            throw render_error{"as_int_array() element is not integer"};
          }
        } else {
          // double or string
          throw render_error{"as_int_array() element is not integer"};
        }
      }
      return inja_value{inja_array{std::move(result)}};
    }
  }, arr);
}

/**
 * @brief 配列を浮動小数特化配列へ変換する。
 * @param v 変換対象の配列を持つ値
 * @return 浮動小数特化配列を持つ inja_value
 * @throws render_error 変換できない要素が含まれる場合
 */
[[nodiscard]] inline auto fn_as_double_array(inja_value const& v) -> inja_value {
  if (!std::holds_alternative<inja_array>(v.storage)) {
    throw render_error{"as_double_array() expects array argument"};
  }
  auto const& arr = std::get<inja_array>(v.storage);
  return std::visit([](auto const& vec) -> inja_value {
    using T = typename std::decay_t<decltype(vec)>::value_type;
    if constexpr (std::is_same_v<T, double>) {
      return inja_value{inja_array{vec}};
    } else {
      auto result = std::vector<double>{};
      result.reserve(vec.size());
      for (auto const& elem : vec) {
        if constexpr (std::is_same_v<T, inja_value>) {
          if (auto const* p = std::get_if<double>(&elem.storage)) {
            result.push_back(*p);
          } else if (auto const* p = std::get_if<std::int64_t>(&elem.storage)) {
            result.push_back(static_cast<double>(*p));
          } else {
            throw render_error{"as_double_array() element is not number"};
          }
        } else if constexpr (std::is_same_v<T, std::int64_t>) {
          result.push_back(static_cast<double>(elem));
        } else {
          throw render_error{"as_double_array() element is not number"};
        }
      }
      return inja_value{inja_array{std::move(result)}};
    }
  }, arr);
}

/**
 * @brief 配列を文字列特化配列へ変換する。
 * @param v 変換対象の配列を持つ値
 * @return 文字列特化配列を持つ inja_value
 * @throws render_error 変換できない要素が含まれる場合
 */
[[nodiscard]] inline auto fn_as_string_array(inja_value const& v) -> inja_value {
  if (!std::holds_alternative<inja_array>(v.storage)) {
    throw render_error{"as_string_array() expects array argument"};
  }
  auto const& arr = std::get<inja_array>(v.storage);
  return std::visit([](auto const& vec) -> inja_value {
    using T = typename std::decay_t<decltype(vec)>::value_type;
    if constexpr (std::is_same_v<T, std::string>) {
      return inja_value{inja_array{vec}};
    } else {
      auto result = std::vector<std::string>{};
      result.reserve(vec.size());
      for (auto const& elem : vec) {
        if constexpr (std::is_same_v<T, inja_value>) {
          if (auto const* p = std::get_if<std::string>(&elem.storage)) {
            result.push_back(*p);
          } else {
            throw render_error{"as_string_array() element is not string"};
          }
        } else {
          throw render_error{"as_string_array() element is not string"};
        }
      }
      return inja_value{inja_array{std::move(result)}};
    }
  }, arr);
}

// ============ ユーティリティ関数 ============

/**
 * @brief default 関数で「空」とみなす値か判定する補助関数。
 *
 * @param val 入力値
 * @return null または空のコンテナ/文字列なら true、それ以外は false
 */
[[nodiscard]] inline auto is_empty_value(inja_value const& val) -> bool {
  if (std::holds_alternative<inja_null>(val.storage)) {
    return true;
  }
  if (auto const* p = std::get_if<std::string>(&val.storage)) {
    return p->empty();
  }
  if (auto const* p = std::get_if<inja_array>(&val.storage)) {
    return std::visit([](auto const& vec) { return vec.empty(); }, *p);
  }
  if (auto const* p = std::get_if<inja_object>(&val.storage)) {
    return p->empty();
  }
  return false;
}

/**
 * @brief 2つの inja_value が等しいか判定する
 *
 * @param lhs 左辺値
 * @param rhs 右辺値
 * @return true 等しければ true
 * @return false 等しくなければ false
 */
[[nodiscard]] inline auto values_equal(inja_value const& lhs, inja_value const& rhs) -> bool {
  if (std::holds_alternative<inja_null>(lhs.storage) && std::holds_alternative<inja_null>(rhs.storage)) {
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

/**
 * @brief 空でなければ値を返し、空ならフォールバックを返す
 *
 * @param val 判定対象の値
 * @param fallback val が空の場合に返す値
 * @return inja_value 非空なら val、そうでなければ fallback
 */
[[nodiscard]] inline auto fn_default(inja_value const& val, inja_value const& fallback) noexcept(false) -> inja_value {
  return is_empty_value(val) ? fallback : val;
}

/**
 * @brief 配列の指定インデックスの要素を取得する（0 始まり）
 *
 * @param arr 参照する配列
 * @param index 取得インデックス（末尾からの負のインデックスにも対応）
 * @return inja_value 指定位置の要素
 * @throws inja_render_error 不正な入力または範囲外の場合
 */
[[nodiscard]] inline auto fn_at(inja_value const& arr, inja_value const& index) noexcept(false) -> inja_value {
  if (!std::holds_alternative<inja_array>(arr.storage)) {
    throw render_error{"at() expects array as first argument"};
  }

  if (!std::holds_alternative<std::int64_t>(index.storage)) {
    throw render_error{"at() expects integer as index"};
  }

  auto const& array_variant = std::get<inja_array>(arr.storage);
  auto idx = std::get<std::int64_t>(index.storage);

  return std::visit([&](auto const& vec) -> inja_value {
    auto const size = static_cast<std::int64_t>(vec.size());
    if (idx < 0) {
      idx = size + idx;
    }

    if (idx < 0 || idx >= size) {
      throw render_error{"at() index out of bounds"};
    }

    return inja_value{vec[static_cast<std::size_t>(idx)]};
  }, array_variant);
}

/**
 * @brief 配列またはオブジェクトが存在するか判定する
 *
 * @param val 判定対象の値
 * @return inja_value 存在すれば true、存在すれば false
 */
[[nodiscard]] inline auto fn_exists(inja_value const& val) noexcept(false) -> inja_value {
  if (auto const* p = std::get_if<inja_array>(&val.storage)) {
    return inja_value{std::visit([](auto const& vec) { return !vec.empty(); }, *p)};
  }
  if (auto const* p = std::get_if<inja_object>(&val.storage)) {
    return inja_value{!p->empty()};
  }
  return inja_value{false};
}

/**
 * @brief 配列内に値が存在するか、またはオブジェクト値に一致するか判定する
 *
 * @param val 探す値
 * @param container 探索対象の配列またはオブジェクト
 * @return inja_value 配列で見つかるか、オブジェクトのいずれかの値に一致すれば true
 * @throws inja_render_error container が配列/オブジェクトでない場合
 */
[[nodiscard]] inline auto fn_existsIn(inja_value const& val, inja_value const& container) noexcept(false) -> inja_value {
  if (auto const* p = std::get_if<inja_array>(&container.storage)) {
    return std::visit([&](auto const& vec) -> inja_value {
      for (auto const& elem : vec) {
        if constexpr (std::is_same_v<std::decay_t<decltype(elem)>, inja_value>) {
          if (values_equal(val, elem)) {
            return inja_value{true};
          }
        } else {
          if (values_equal(val, inja_value{elem})) {
            return inja_value{true};
          }
        }
      }
      return inja_value{false};
    }, *p);
  }

  if (auto const* p = std::get_if<inja_object>(&container.storage)) {
    for (auto const& [k, v] : *p) {
      if (values_equal(val, v)) {
        return inja_value{true};
      }
    }
    return inja_value{false};
  }

  throw render_error{"existsIn() expects array or object as second argument"};
}

} // namespace frozenchars::inja
