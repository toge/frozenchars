#pragma once

#include "frozenchars/config.hpp"

#include <array>
#if __STDC_HOSTED__ == 1 && defined(__has_include) && __has_include(<charconv>)
#include <charconv>
#define FROZENCHARS_DETAIL_HAS_CHARCONV 1
#else
#define FROZENCHARS_DETAIL_HAS_CHARCONV 0
#endif
#include <limits>
#include <ranges>
#include <utility>
#include <algorithm>

namespace frozenchars::detail {

/**
 * @brief 10進数整数を文字列に変換する
 *
 * @param v 変換する整数
 * @return auto 変換された文字列とその長さのペア
 */
template <typename T>
  requires std::same_as<T, long long> || std::same_as<T, unsigned long long>
[[nodiscard]] auto constexpr to_dec_chars(T v) noexcept {
  auto buffer = std::array<char, 21>{};
#if FROZENCHARS_DETAIL_HAS_CHARCONV
  auto const [end, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), v);
  if (ec != std::errc{}) {
    return std::pair{buffer, 0uz};
  }
  return std::pair{buffer, static_cast<std::size_t>(end - buffer.data())};
#else
  if (v == 0) {
    buffer[0] = '0';
    return std::pair{buffer, 1uz};
  }
  bool neg = false;
  unsigned long long uv;
  if constexpr (std::is_signed_v<T>) {
    if (v < 0) { neg = true; uv = static_cast<unsigned long long>(-(v)); }
    else uv = static_cast<unsigned long long>(v);
  } else {
    uv = static_cast<unsigned long long>(v);
  }
  auto i = 0uz;
  auto tmp = uv;
  while (tmp > 0) { ++i; tmp /= 10; }
  auto len = i + (neg ? 1 : 0);
  auto pos = len;
  tmp = uv;
  while (tmp > 0) { buffer[--pos] = static_cast<char>('0' + (tmp % 10)); tmp /= 10; }
  if (neg) buffer[0] = '-';
  return std::pair{buffer, len};
#endif
}

/**
 * @brief 16進数整数を文字列に変換する
 *
 * @param value 変換する整数
 * @return auto 変換文字列とその長さのペア
 */
[[nodiscard]] auto constexpr to_hex_chars(unsigned long long value) noexcept {
  auto buffer = std::array<char, 17>{};
#if FROZENCHARS_DETAIL_HAS_CHARCONV
  auto const [end, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), value, 16);
  if (ec != std::errc{}) {
    return std::pair{buffer, 0uz};
  }
  return std::pair{buffer, static_cast<std::size_t>(end - buffer.data())};
#else
  if (value == 0) { buffer[0] = '0'; return std::pair{buffer, 1uz}; }
  auto i = 0uz;
  auto tmp = value;
  while (tmp > 0) { ++i; tmp >>= 4; }
  auto len = i;
  auto pos = len;
  tmp = value;
  while (tmp > 0) { auto d = tmp & 0xF; buffer[--pos] = d < 10 ? static_cast<char>('0' + d) : static_cast<char>('a' + (d - 10)); tmp >>= 4; }
  return std::pair{buffer, len};
#endif
}

/**
 * @brief 16進数整数を文字列に変換する（符号付き版）
 */
[[nodiscard]] auto constexpr to_hex_chars(long long value) noexcept {
  return to_hex_chars(static_cast<unsigned long long>(value));
}

/**
 * @brief 2進数整数を文字列に変換する
 *
 * @param v 変換する整数
 * @return auto 変換文字列とその長さのペア
 */
[[nodiscard]] auto constexpr to_bin_chars(unsigned long long v) noexcept {
  auto buffer = std::array<char, 65>{};
  if (v == 0) {
    buffer[0] = '0';
    return std::pair{buffer, 1uz};
  }
  auto i = 0uz;
  while (v > 0) {
    buffer[i++] = '0' + static_cast<char>(v % 2); v /= 2;
  }
  for (auto const j : std::views::iota(0uz, i / 2)) {
    std::swap(buffer[j], buffer[i - j - 1]);
  }
  return std::pair{buffer, i};
}

/**
 * @brief 2進数整数を文字列に変換する（符号付き版）
 */
[[nodiscard]] auto constexpr to_bin_chars(long long value) noexcept {
  return to_bin_chars(static_cast<unsigned long long>(value));
}

/**
 * @brief 8進数整数を文字列に変換する
 *
 * @param v 変換する整数
 * @return auto 変換された文字列とその長さのペア
 */
[[nodiscard]] auto constexpr to_oct_chars(unsigned long long v) noexcept {
  auto buffer = std::array<char, 23>{};
  if (v == 0) {
    buffer[0] = '0';
    return std::pair{buffer, 1uz};
  }
  auto i = 0uz;
  while (v > 0) {
    buffer[i++] = '0' + static_cast<char>(v % 8); v /= 8;
  }
  for (auto const j : std::views::iota(0uz, i / 2)) {
    std::swap(buffer[j], buffer[i - j - 1]);
  }
  return std::pair{buffer, i};
}

/**
 * @brief 8進数整数を文字列に変換する（符号付き版）
 */
[[nodiscard]] auto constexpr to_oct_chars(long long value) noexcept {
  return to_oct_chars(static_cast<unsigned long long>(value));
}

/**
 * @brief 浮動小数点数を文字列に変換する（簡易固定小数点）
 *
 * @param value 変換する浮動小数点数
 * @param precision 小数点以下の桁数
 * @return auto 変換文字列とその長さのペア
 */
[[nodiscard]] auto constexpr to_float_chars(double value, int precision) noexcept {
  auto buffer = std::array<char, 48>{};
  auto i = 0uz;

  if (value != value || value > std::numeric_limits<double>::max() || value < -std::numeric_limits<double>::max()) {
    return std::pair{buffer, i};
  }

  if (value < 0) {
    buffer[i++] = '-'; value = -value;
  }

  if (value >= static_cast<double>(std::numeric_limits<long long>::max())) {
    return std::pair{buffer, i};
  }

  auto integral = static_cast<long long>(value);

  // 小数部は 10^p 倍して四捨五入した整数として扱う（1 桁ずつ切り捨てると
  // 0.57 → "0.56" のように二進表現の誤差が出る）。long long に収まる 18 桁までを
  // 丸め対象とし、それより下位の桁は 0 で埋める。
  auto const p = std::min(std::max(0, precision), static_cast<int>(buffer.size()) - 22);
  auto const round_digits = std::min(p, 18);
  auto scale = 1LL;
  for (auto const _ : std::views::iota(0, round_digits)) {
    scale *= 10;
  }
  auto const scaled = (value - static_cast<double>(integral)) * static_cast<double>(scale) + 0.5;
  auto frac_digits = static_cast<long long>(scaled);
  if (frac_digits >= scale) {
    // 0.999.. が繰り上がって整数部に桁上がりする
    frac_digits -= scale;
    ++integral;
  }

  auto [int_data, int_len] = to_dec_chars(integral);
  for (auto const j : std::views::iota(0uz, int_len)) {
    buffer[i++] = int_data[j];
  }

  if (p > 0) {
    buffer[i++] = '.';
    for (auto d = round_digits - 1; d >= 0; --d) {
      buffer[i + static_cast<size_t>(d)] = static_cast<char>('0' + frac_digits % 10);
      frac_digits /= 10;
    }
    i += static_cast<size_t>(round_digits);
    for (auto const _ : std::views::iota(round_digits, p)) {
      buffer[i++] = '0';
    }
  }
  return std::pair{buffer, i};
}

} // namespace frozenchars::detail
