#pragma once

#include "concepts.hpp"
#include "detail/pipe.hpp"
#include <array>
#include <cstddef>
#include <ostream>
#include <string_view>

namespace frozenchars {

/**
 * @brief 静的文字列
 * このライブラリでの基本的な文字列型
 * @tparam N 文字列の最大長 (終端文字'\0'を含む)
 */
template <size_t N>
struct FrozenString {
  std::array<char, N> buffer{};
  size_t length = 0;

  /**
   * @brief デフォルトコンストラクタ
   */
  constexpr FrozenString() = default;

  /**
   * @brief 文字列リテラルからのコンストラクタ
   * @param str 文字列リテラル
   */
  constexpr FrozenString(char const (&str)[N]) noexcept {
    for (size_t i = 0; i < N - 1; ++i) {
      buffer[i] = str[i];
    }
    buffer[N - 1] = '\0';
    length = N - 1;
  }

  /**
   * @brief string_view を取得する
   * @return std::string_view
   */
  [[nodiscard]] constexpr auto sv() const noexcept -> std::string_view {
    return {buffer.data(), length};
  }

  /**
   * @brief 互換性のための string_view 変換
   */
  [[nodiscard]] constexpr operator std::string_view() const noexcept {
    return sv();
  }
};

namespace detail {
  template <typename T>
  struct is_frozen_string : std::false_type {};

  template <std::size_t N>
  struct is_frozen_string<FrozenString<N>> : std::true_type {};

  template <typename T>
  inline constexpr bool is_frozen_string_v = is_frozen_string<std::remove_cvref_t<T>>::value;
}

template <typename T>
concept FrozenStringLike = detail::is_frozen_string_v<T>;

/**
 * @brief 文字列リテラルから FrozenString を作成する
 */
template <size_t N>
constexpr auto freeze(char const (&str)[N]) noexcept -> FrozenString<N> {
  return FrozenString<N>{str};
}

} // namespace frozenchars
