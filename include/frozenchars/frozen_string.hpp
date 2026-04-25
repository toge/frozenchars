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
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 */
template <size_t N>
struct FrozenString {
  // buffer は常にゼロ初期化されることを保証する
  static_assert(std::is_trivially_default_constructible_v<std::array<char, 1>>,
    "buffer zero-initialization relies on aggregate initialization");

  std::array<char, N> buffer{};
  size_t length{N > 0 ? N - 1 : 0};

  consteval FrozenString() noexcept = default;
  consteval FrozenString(char const (&str)[N]) noexcept
  : length{N > 0 ? N - 1 : 0} {
    for (auto i = 0uz; i < N; ++i) {
      buffer[i] = str[i];
    }
  }

  // @note buffer はメンバ初期化子 {} によりゼロ初期化済み。
  //       copy_len 以降の要素は明示的に書かないが、値は保証された '\0' のまま。
  template <size_t M>
  consteval FrozenString(FrozenString<M> const& other) noexcept
  : length{std::min(other.length, N > 0 ? N - 1 : 0)} {
    auto const copy_len = length;
    for (auto i = 0uz; i < copy_len; ++i) {
      buffer[i] = other.buffer[i];
    }
    if constexpr (N > 0) {
      buffer[copy_len] = '\0';
    }
  }

  auto constexpr sv() const noexcept {
    return std::string_view{buffer.data(), length};
  }

  explicit constexpr operator std::string_view() const noexcept {
    return sv();
  }

  auto constexpr begin() const noexcept { return buffer.data(); }
  auto constexpr end()   const noexcept { return buffer.data() + length; }

  auto constexpr operator[](std::size_t i) const noexcept -> char const& { return buffer[i]; }
  auto constexpr operator[](std::size_t i)       noexcept -> char&       { return buffer[i]; }

  auto constexpr size()  const noexcept { return length; }
  auto constexpr empty() const noexcept { return length == 0; }

  auto constexpr data() const noexcept { return buffer.data(); }
  auto constexpr data()       noexcept { return buffer.data(); }

  // FrozenString同士の結合
  template <size_t M>
  auto consteval operator+(FrozenString<M> const& other) const noexcept {
    FrozenString<N + M - 1> res{};
    auto offset = 0uz;
    for (auto const c : this->sv()) {
      res.buffer[offset++] = c;
    }
    for (auto const c : other.sv()) {
      res.buffer[offset++] = c;
    }
    res.buffer[offset] = '\0';
    res.length = offset;
    return res;
  }

  // 文字列リテラルとの結合
  template <size_t M>
  auto consteval operator+(char const (&rhs)[M]) const noexcept {
    return *this + FrozenString<M>{rhs};
  }

  // 1文字の追加
  auto consteval operator+(char rhs) const noexcept {
    FrozenString<N + 1> res{};
    auto offset = 0uz;
    for (auto const c : this->sv()) {
      res.buffer[offset++] = c;
    }
    res.buffer[offset++] = rhs;
    res.buffer[offset] = '\0';
    res.length = offset;
    return res;
  }
};

template <size_t N>
auto operator<<(std::ostream& os, FrozenString<N> const& value) -> std::ostream& {
  return os << value.sv();
}

} // namespace frozenchars

/**
 * @brief 文字列リテラルと FrozenString を結合する
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @tparam M FrozenString の長さ (終端文字'\0'を含む)
 * @param rhs 結合する FrozenString
 * @return auto 結合結果
 */
template <size_t N, size_t M>
auto consteval operator+(char const (&lhs)[N], frozenchars::FrozenString<M> const& rhs) noexcept {
  frozenchars::FrozenString<N + M - 1> res{};
  auto offset = 0uz;
  for (auto i = 0uz; i < N - 1; ++i) {
    res.buffer[offset++] = lhs[i];
  }
  for (auto const c : rhs.sv()) {
    res.buffer[offset++] = c;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}
