#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace frozenchars::inja {

// 配列・オブジェクトは再帰構造になるため、value_view 内でポインタ間接参照する
// （std::variant は直接再帰を許さない）。非所有ビューのため生ポインタで十分。
struct array_view;
struct object_view;

/**
 * @brief 非所有値ビュー（ホットパス専用）
 *
 * 文字列は std::string_view で所有しない。葉を覗くためだけの一時オブジェクト。
 * 配列・オブジェクトは再帰構造のためポインタ経由で保持する。
 */
struct value_view {
  std::variant<std::monostate, bool, std::int64_t, double, std::string_view, array_view const*, object_view const*> storage{std::monostate{}};
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
  std::variant<std::monostate, bool, std::int64_t, double, std::string, std::vector<temp_value>, std::map<std::string, temp_value>> storage{std::monostate{}};

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

// ============================================================
// fixed_string - NTTP 対応固定長文字列型
// ============================================================

/**
 * @brief テンプレート引数として文字列リテラルを渡すための NTTP 対応固定長文字列型。
 *
 * structured type の要件（全メンバ public、参照なし）を満たす。
 * コンパイラが `std::string_view` を構造的型として扱わない環境（libstdc++ 16 など）でも
 * 使用可能な代替。
 */
template <std::size_t N>
struct fixed_string {
  char data[N]{};

  constexpr fixed_string() noexcept = default;

  constexpr fixed_string(char const (&s)[N]) noexcept {
    for (auto i = 0uz; i < N; ++i) {
      data[i] = s[i];
    }
  }

  [[nodiscard]] constexpr auto sv() const noexcept -> std::string_view {
    auto len = std::size_t{0};
    while (len < N && data[len] != '\0') {
      ++len;
    }
    return {data, len};
  }

  [[nodiscard]] constexpr auto operator==(std::string_view s) const noexcept -> bool { return sv() == s; }

  template <std::size_t M>
  [[nodiscard]] constexpr auto operator==(fixed_string<M> const& other) const noexcept -> bool {
    return sv() == other.sv();
  }

  auto operator<=>(fixed_string const&) const = default;
};

template <std::size_t N>
fixed_string(char const (&)[N]) -> fixed_string<N>;

}  // namespace frozenchars::inja
