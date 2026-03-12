#ifndef __FROZEN_CHARS_H__
#define __FROZEN_CHARS_H__

#include <array>
#include <cstddef>
#include <span>
#include <vector>
#include <string_view>
#include <ranges>
#include <algorithm>
#include <concepts>
#include <type_traits>

namespace frozenchars {

/*===============================================================================*\
 * ユーティリティ
\*===============================================================================*/

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
  auto constexpr sv() const noexcept {
    return std::string_view{data, N - 1};
  }
};

/**
 * @brief 16進数表現用のタグ
 *
 */
struct Hex {
  long long value;
  constexpr Hex(Integral auto v) : value(v) {}
};

/**
 * @brief 浮動小数点数の精度指定用のタグ
 *
 */
struct Precision {
  double value;
  int precision;
  constexpr Precision(FloatingPoint auto v, int p = 2)
  : value(static_cast<double>(v)), precision(p)
  {}
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

  auto constexpr sv() const noexcept {
    return std::string_view{buffer.data(), length};
  }

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
  if constexpr (new_size > 0) {
    res.buffer[new_size - 1] = '\0';
  }
  return res;
}

template <size_t Count, size_t N>
auto constexpr repeat(char const (&rhs)[N]) noexcept {
  return repeat<Count>(StaticString<N>{rhs});
}

// リテラル演算子 _ss
template <FixedString FS>
auto constexpr operator""_ss() noexcept {
  auto res = StaticString<FS.sv().size() + 1>{};
  auto const s = FS.sv();
  for (auto const i : std::views::iota(0uz, s.size())) {
    res.buffer[i] = s[i];
  }
  res.buffer[s.size()] = '\0';
  return res;
}

// 左辺リテラル + StaticString
template <size_t N, size_t M>
auto constexpr operator+(char const (&lhs)[N], StaticString<M> const& rhs) noexcept {
  auto res = StaticString<N + M - 1>{};
  auto offset = 0uz;
  for (auto const i : std::views::iota(0uz, N - 1)) {
    res.buffer[offset++] = lhs[i];
  }
  for (auto const c : rhs.sv()) {
    res.buffer[offset++] = c;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

// --- 数値変換エンジン ---

namespace detail {
  // 10進数整数変換
  auto constexpr to_dec_chars(long long v) noexcept {
    auto buffer = std::array<char, 21>{};
    if (v == 0) {
      buffer[0] = '0';
      return std::pair{buffer, 1uz};
    }
    auto const neg = v < 0;
    auto val = neg ? (v == -9223372036854775807LL - 1 ? 9223372036854775807LL : -v) : v;
    auto i = 0uz;
    while (val > 0) {
      buffer[i++] = static_cast<char>('0' + (val % 10));
      val /= 10;
    }
    if (neg) {
      buffer[i++] = '-';
    }
    for (auto const j : std::views::iota(0uz, i / 2)) {
      std::swap(buffer[j], buffer[i - j - 1]);
    }
    return std::pair{buffer, i};
  }

  // 16進数変換
  auto constexpr to_hex_chars(long long value) noexcept {
    auto buffer = std::array<char, 19>{'0', 'x'};
    auto v = static_cast<unsigned long long>(value);
    if (v == 0) {
      buffer[2] = '0';
      return std::pair{buffer, 3uz};
    }
    auto i = 2uz;
    auto constexpr digits = "0123456789abcdef";
    while (v > 0) {
      buffer[i++] = digits[v % 16]; v /= 16;
    }
    for (auto const j : std::views::iota(0uz, (i - 2) / 2)) {
      std::swap(buffer[j + 2], buffer[i - j - 1]);
    }
    return std::pair{buffer, i};
  }

  // 浮動小数点数（簡易固定小数点）変換
  auto constexpr to_float_chars(double value, int precision) noexcept {
    auto buffer = std::array<char, 48>{};
    auto i = 0uz;
    if (value < 0) {
      buffer[i++] = '-'; value = -value;
    }

    auto integral = static_cast<long long>(value);
    auto [int_data, int_len] = to_dec_chars(integral);
    for (auto const j : std::views::iota(0uz, int_len)) {
      buffer[i++] = int_data[j];
    }

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

  // 1要素を 0..255 の値として扱うための共通変換。
  // std::byte は to_integer、それ以外は unsigned char へキャストする。
  template <typename T>
  auto constexpr to_u8(T const v) noexcept {
    if constexpr (std::same_as<std::remove_cv_t<T>, std::byte>) {
      return std::to_integer<unsigned char>(v);
    } else {
      return static_cast<unsigned char>(v);
    }
  }

  // ヌル終端ポインタを StaticString<257> に変換する。
  // - nullptr は空文字
  // - '\0' もしくは 256 文字で打ち切り
  template <typename Elem>
  auto constexpr make_static_from_ptr(Elem const* arg) noexcept {
    auto res = StaticString<257>{};
    if (arg == nullptr) {
      res.buffer[0] = '\0';
      res.length = 0;
      return res;
    }

    auto len = 0uz;
    for (; len < res.buffer.size() - 1; ++len) {
      auto const byte = to_u8(arg[len]);
      if (byte == 0u) {
        break;
      }
      res.buffer[len] = static_cast<char>(byte);
    }
    res.buffer[len] = '\0';
    res.length = len;
    return res;
  }

  // span を StaticString<257> に変換する。
  // - 先頭から 0 値までを文字列として扱う
  // - 0 がなくても最大 256 文字までコピー
  template <typename Elem, size_t Extent>
  auto constexpr make_static_from_span(std::span<Elem const, Extent> arg) noexcept {
    auto res = StaticString<257>{};
    auto const max_len = std::min(arg.size(), res.buffer.size() - 1);
    auto len = 0uz;
    for (; len < max_len; ++len) {
      auto const byte = to_u8(arg[len]);
      if (byte == 0u) {
        break;
      }
      res.buffer[len] = static_cast<char>(byte);
    }
    res.buffer[len] = '\0';
    res.length = len;
    return res;
  }

  // string_view を StaticString<257> に変換する（終端は長さベース）。
  auto constexpr make_static_from_sv(std::string_view s) noexcept {
    auto res = StaticString<257>{};
    auto const len = std::min(s.size(), res.buffer.size() - 1);
    for (auto const i : std::views::iota(0uz, len)) {
      res.buffer[i] = s[i];
    }
    res.buffer[len] = '\0';
    res.length = len;
    return res;
  }
}

// --- ファクトリ ---

// 文字列リテラル

template <size_t N>
auto constexpr make_static(char const (&arg)[N]) noexcept {
  return StaticString<N>{arg};
}

// C文字列ポインタ

auto constexpr make_static(char const* arg) noexcept {
  return detail::make_static_from_ptr(arg);
}

auto constexpr make_static(char* arg) noexcept {
  return make_static(static_cast<char const*>(arg));
}

auto constexpr make_static(signed char const* arg) noexcept {
  return detail::make_static_from_ptr(arg);
}

auto constexpr make_static(signed char* arg) noexcept {
  return make_static(static_cast<signed char const*>(arg));
}

auto constexpr make_static(unsigned char const* arg) noexcept {
  return detail::make_static_from_ptr(arg);
}

auto constexpr make_static(unsigned char* arg) noexcept {
  return make_static(static_cast<unsigned char const*>(arg));
}

// span / array 系

template <size_t Extent>
auto constexpr make_static(std::span<char const, Extent> arg) noexcept {
  return detail::make_static_from_span(arg);
}

template <size_t Extent>
auto constexpr make_static(std::span<char, Extent> arg) noexcept {
  return make_static(std::span<char const, Extent>{arg});
}

template <size_t N>
auto constexpr make_static(std::array<char, N> const& arg) noexcept {
  return make_static(std::span<char const, N>{arg});
}

template <size_t Extent>
auto constexpr make_static(std::span<signed char const, Extent> arg) noexcept {
  return detail::make_static_from_span(arg);
}

template <size_t Extent>
auto constexpr make_static(std::span<signed char, Extent> arg) noexcept {
  return make_static(std::span<signed char const, Extent>{arg});
}

template <size_t N>
auto constexpr make_static(std::array<signed char, N> const& arg) noexcept {
  return make_static(std::span<signed char const, N>{arg});
}

template <size_t Extent>
auto constexpr make_static(std::span<unsigned char const, Extent> arg) noexcept {
  return detail::make_static_from_span(arg);
}

template <size_t Extent>
auto constexpr make_static(std::span<unsigned char, Extent> arg) noexcept {
  return make_static(std::span<unsigned char const, Extent>{arg});
}

template <size_t N>
auto constexpr make_static(std::array<unsigned char, N> const& arg) noexcept {
  return make_static(std::span<unsigned char const, N>{arg});
}

template <size_t Extent>
auto constexpr make_static(std::span<std::byte const, Extent> arg) noexcept {
  return detail::make_static_from_span(arg);
}

template <size_t Extent>
auto constexpr make_static(std::span<std::byte, Extent> arg) noexcept {
  return make_static(std::span<std::byte const, Extent>{arg});
}

template <size_t N>
auto constexpr make_static(std::array<std::byte, N> const& arg) noexcept {
  return make_static(std::span<std::byte const, N>{arg});
}

// vector 系

template <typename Alloc>
auto constexpr make_static(std::vector<char, Alloc>& arg) noexcept {
  return make_static(std::span<char>{arg.data(), arg.size()});
}

template <typename Alloc>
auto constexpr make_static(std::vector<char, Alloc> const& arg) noexcept {
  return make_static(std::span<char const>{arg.data(), arg.size()});
}

template <typename Alloc>
auto constexpr make_static(std::vector<signed char, Alloc>& arg) noexcept {
  return make_static(std::span<signed char>{arg.data(), arg.size()});
}

template <typename Alloc>
auto constexpr make_static(std::vector<signed char, Alloc> const& arg) noexcept {
  return make_static(std::span<signed char const>{arg.data(), arg.size()});
}

template <typename Alloc>
auto constexpr make_static(std::vector<unsigned char, Alloc>& arg) noexcept {
  return make_static(std::span<unsigned char>{arg.data(), arg.size()});
}

template <typename Alloc>
auto constexpr make_static(std::vector<unsigned char, Alloc> const& arg) noexcept {
  return make_static(std::span<unsigned char const>{arg.data(), arg.size()});
}

template <typename Alloc>
auto constexpr make_static(std::vector<std::byte, Alloc>& arg) noexcept {
  return make_static(std::span<std::byte>{arg.data(), arg.size()});
}

template <typename Alloc>
auto constexpr make_static(std::vector<std::byte, Alloc> const& arg) noexcept {
  return make_static(std::span<std::byte const>{arg.data(), arg.size()});
}

// 無効入力
auto constexpr make_static(decltype(nullptr)) noexcept = delete;

// 数値フォーマット

auto constexpr make_static(Hex const& arg) noexcept {
  auto const p = detail::to_hex_chars(arg.value);
  auto res = StaticString<19>{};
  for (auto const i : std::views::iota(0uz, p.second)) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

auto constexpr make_static(Precision const& arg) noexcept {
  auto const p = detail::to_float_chars(arg.value, arg.precision);
  auto res = StaticString<49>{};
  for (auto const i : std::views::iota(0uz, p.second)) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

template <Integral T>
auto constexpr make_static(T const& arg) noexcept {
  auto const p = detail::to_dec_chars(static_cast<long long>(arg));
  auto res = StaticString<21>{};
  for (auto const i : std::views::iota(0uz, p.second)) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

template <FloatingPoint T>
auto constexpr make_static(T const& arg) noexcept {
  return make_static(Precision(arg, 2)); // デフォルト精度2
}

// string_view 変換可能型

template <typename T>
  requires (std::is_convertible_v<T, std::string_view>
            && !Integral<std::remove_cvref_t<T>>
            && !FloatingPoint<std::remove_cvref_t<T>>
            && !std::same_as<std::remove_cvref_t<T>, Hex>
            && !std::same_as<std::remove_cvref_t<T>, Precision>)
auto constexpr make_static(T const& arg) noexcept {
  auto const s = std::string_view{arg};
  return detail::make_static_from_sv(s);
}

// フォールバック（未対応型はコンパイルエラー）
template <typename T>
auto constexpr make_static(T const&) noexcept = delete;

/**
 * 一括で文字列と数値を結合する
 */
template <typename... Args>
auto constexpr concat(Args const&... args) noexcept {
  return (make_static(args) + ...);
}

}

#endif /* __FROZEN_CHARS_H__ */
