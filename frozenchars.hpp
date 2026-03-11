#ifndef __FROZEN_CHARS_H__
#define __FROZEN_CHARS_H__

#include <array>
#include <string_view>
#include <ranges>
#include <algorithm>
#include <iostream>
#include <concepts>
#include <numeric>
#include <type_traits>

// --- 前方宣言とユーティリティ ---

template <typename T>
concept Integral = std::is_integral_v<T>;

template <typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

template <size_t N>
struct FixedString {
  char data[N]{};
  constexpr FixedString(char const (&str)[N]) noexcept {
    for (auto const i : std::views::iota(0uz, N)) {
      data[i] = str[i];
    }
  }
  auto constexpr sv() const noexcept -> std::string_view { return {data, N - 1}; }
};

/**
 * 特殊なフォーマット指定用のタグ
 */
struct Hex {
  long long value;
  constexpr Hex(Integral auto v) : value(static_cast<long long>(v)) {}
};

struct Precision {
  double value;
  int precision;
  constexpr Precision(FloatingPoint auto v, int p = 2) : value(static_cast<double>(v)), precision(p) {}
};

// --- StaticString 本体 ---

template <size_t N>
struct StaticString {
  std::array<char, N> buffer{};
  size_t length{N > 0 ? N - 1 : 0};

  constexpr StaticString() noexcept = default;

  constexpr StaticString(char const (&str)[N]) noexcept : length{N > 0 ? N - 1 : 0} {
    for (auto const i : std::views::iota(0uz, N)) {
      buffer[i] = str[i];
    }
  }

  auto constexpr sv() const noexcept -> std::string_view { return {buffer.data(), length}; }

  // 1. StaticString + StaticString
  template <size_t M>
  auto constexpr operator+(StaticString<M> const& other) const noexcept {
    StaticString<N + M - 1> res{};
    auto offset = 0uz;
    for (auto const c : this->sv()) res.buffer[offset++] = c;
    for (auto const c : other.sv()) res.buffer[offset++] = c;
    res.buffer[offset] = '\0';
    res.length = offset;
    return res;
  }

  // 2. StaticString + 文字列リテラル
  template <size_t M>
  auto constexpr operator+(char const (&rhs)[M]) const noexcept {
    return *this + StaticString<M>{rhs};
  }
};

/**
 * 繰り返しのための非メンバ関数テンプレート
 * StaticString<N> * Constant<3> のような形にするか、
 * サイズを計算するために count をテンプレート引数で受け取ります
 */
template <size_t Count, size_t N>
auto constexpr repeat(StaticString<N> const& str) noexcept {
  auto constexpr unit_len = N > 0 ? N - 1 : 0;
  auto constexpr new_size = unit_len * Count + 1;

  StaticString<new_size> res{};
  auto offset = 0uz;
  auto const src = str.sv();

  for (auto const _ : std::views::iota(0uz, Count)) {
    for (auto const c : src) {
      res.buffer[offset++] = c;
    }
  }
  if constexpr (new_size > 0) res.buffer[new_size - 1] = '\0';
  return res;
}

template <size_t Count, size_t N>
auto constexpr repeat(char const (&rhs)[N]) noexcept {
  return repeat<Count>(StaticString<N>{rhs});
}

// リテラル演算子 _ss
template <FixedString FS>
auto constexpr operator""_ss() noexcept {
  StaticString<FS.sv().size() + 1> res{};
  auto const s = FS.sv();
  for (auto const i : std::views::iota(0uz, s.size())) res.buffer[i] = s[i];
  res.buffer[s.size()] = '\0';
  return res;
}

// 左辺リテラル + StaticString
template <size_t N, size_t M>
auto constexpr operator+(char const (&lhs)[N], StaticString<M> const& rhs) noexcept {
  StaticString<N + M - 1> res{};
  auto offset = 0uz;
  for (auto const i : std::views::iota(0uz, N - 1)) res.buffer[offset++] = lhs[i];
  for (auto const c : rhs.sv()) res.buffer[offset++] = c;
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

// --- 数値変換エンジン ---

namespace detail {
  // 10進数整数変換
  auto constexpr to_dec_chars(long long v) noexcept {
    std::array<char, 21> buffer{};
    if (v == 0) {
      buffer[0] = '0';
      return std::pair{buffer, 1uz};
    }
    auto const neg = v < 0;
    auto val = neg ? (v == -9223372036854775807LL - 1 ? 9223372036854775807LL : -v) : v;
    auto i = 0uz;
    while (val > 0) { buffer[i++] = static_cast<char>('0' + (val % 10)); val /= 10; }
    if (neg) buffer[i++] = '-';
    for (auto const j : std::views::iota(0uz, i / 2)) std::swap(buffer[j], buffer[i - j - 1]);
    return std::pair{buffer, i};
  }

  // 16進数変換
  auto constexpr to_hex_chars(long long value) noexcept {
    std::array<char, 19> buffer{'0', 'x'};
    auto v = static_cast<unsigned long long>(value);
    if (v == 0) {
      buffer[2] = '0';
      return std::pair{buffer, 3uz};
    }
    auto i = 2uz;
    auto constexpr digits = "0123456789abcdef";
    while (v > 0) { buffer[i++] = digits[v % 16]; v /= 16; }
    for (auto const j : std::views::iota(0uz, (i - 2) / 2)) std::swap(buffer[j + 2], buffer[i - j - 1]);
    return std::pair{buffer, i};
  }

  // 浮動小数点数（簡易固定小数点）変換
  auto constexpr to_float_chars(double value, int precision) noexcept {
    std::array<char, 48> buffer{};
    auto i = 0uz;
    if (value < 0) { buffer[i++] = '-'; value = -value; }

    auto integral = static_cast<long long>(value);
    auto [int_data, int_len] = to_dec_chars(integral);
    for (auto const j : std::views::iota(0uz, int_len)) buffer[i++] = int_data[j];

    auto p = std::max(0, precision);
    if (p > 0 && i < buffer.size()) {
      buffer[i++] = '.';
      auto const room = static_cast<int>(buffer.size() - i);
      p = std::min(p, room);

      double frac = value - static_cast<double>(integral);
      for (auto const _ : std::views::iota(0, p)) {
        frac *= 10.0;
        int digit = static_cast<int>(frac);
        buffer[i++] = static_cast<char>('0' + digit);
        frac -= digit;
      }
    }
    return std::pair{buffer, i};
  }
}

// --- ファクトリ ---

template <size_t N>
auto constexpr make_static(char const (&arg)[N]) noexcept {
  return StaticString<N>{arg};
}

auto constexpr make_static(auto const& arg) noexcept {
  using T = std::remove_cvref_t<decltype(arg)>;

  if constexpr (std::same_as<T, Hex>) {
    auto const p = detail::to_hex_chars(arg.value);
    StaticString<19> res{};
    for (auto const i : std::views::iota(0uz, p.second)) res.buffer[i] = p.first[i];
    res.buffer[p.second] = '\0';
    res.length = p.second;
    return res;
  } else if constexpr (std::same_as<T, Precision>) {
    auto const p = detail::to_float_chars(arg.value, arg.precision);
    StaticString<49> res{};
    for (auto const i : std::views::iota(0uz, p.second)) res.buffer[i] = p.first[i];
    res.buffer[p.second] = '\0';
    res.length = p.second;
    return res;
  } else if constexpr (std::is_integral_v<T>) {
    auto const p = detail::to_dec_chars(static_cast<long long>(arg));
    StaticString<21> res{};
    for (auto const i : std::views::iota(0uz, p.second)) res.buffer[i] = p.first[i];
    res.buffer[p.second] = '\0';
    res.length = p.second;
    return res;
  } else if constexpr (FloatingPoint<T>) {
    return make_static(Precision(arg, 2)); // デフォルト精度2
  } else if constexpr (std::is_convertible_v<T, std::string_view>) {
    auto const s = std::string_view{arg};
    StaticString<257> res{};
    auto const len = std::min(s.size(), res.buffer.size() - 1);
    for (auto const i : std::views::iota(0uz, len)) res.buffer[i] = s[i];
    res.buffer[len] = '\0';
    res.length = len;
    return res;
  } else {
    static_assert(std::is_same_v<T, void>, "Unsupported type for make_static");
  }
}

/**
 * 可変引数による一括結合
 */
template <typename... Args>
auto constexpr concat(Args const&... args) noexcept {
  return (make_static(args) + ...);
}

#endif /* __FROZEN_CHARS_H__ */
