#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace frozenchars::inja {

// 配列・オブジェクトは再帰構造になるため、value_view 内で unique_ptr 間接参照する
// （std::variant は直接再帰を許さない）。
struct array_view;
struct object_view;

/**
 * @brief 非所有値ビュー（ホットパス専用）
 *
 * 文字列は std::string_view で所有しない。葉を覗くためだけの一時オブジェクト。
 * 配列・オブジェクトは再帰構造のため std::unique_ptr 経由で保持する。
 */
struct value_view {
  std::variant<
    std::monostate,
    bool,
    std::int64_t,
    double,
    std::string_view,
    std::unique_ptr<array_view const>,
    std::unique_ptr<object_view const>
  > storage{std::monostate{}};
};

/**
 * @brief 配列の非所有ビュー
 */
struct array_view {
  std::span<value_view const> elements;
};

/**
 * @brief オブジェクトの非所有ビュー
 */
struct object_view {
  std::span<std::pair<std::string_view, value_view const> const> entries;
};

/**
 * @brief 関数境界専用の一時値（限定的に boxing）
 *
 * 文字列は std::string 所有（関数戻り値がコンテキストライフタイムに依存できないため）。
 * inja_value よりは小さい（inja_array の variant 4 種が 1 種に統合）。
 */
struct temp_value {
  std::variant<
    std::monostate,
    bool,
    std::int64_t,
    double,
    std::string,
    std::vector<temp_value>,
    std::map<std::string, temp_value>
  > storage{std::monostate{}};

  temp_value() = default;

  template <typename T>
  requires(!std::same_as<std::remove_cvref_t<T>, temp_value>)
  temp_value(T&& v) : storage(std::forward<T>(v)) {}
};

/**
 * @brief テンプレート評価時の実行時エラー
 */
class render_error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

}  // namespace frozenchars::inja
