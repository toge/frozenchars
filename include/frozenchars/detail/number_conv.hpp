#pragma once

#include <array>
#include <charconv>
#include <cmath>
#include <limits>
#include <ranges>
#include <utility>
#include <algorithm>

namespace frozenchars::detail {

/**
 * @brief 10進数整数を文字列に変換する
 *
 * @param v 変換する整数
 * @return auto consteval 変換された文字列とその長さのペア
 */
[[nodiscard]] auto consteval to_dec_chars(long long v) noexcept {
  auto buffer = std::array<char, 21>{};
  auto const [end, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), v);
  if (ec != std::errc{}) {
    return std::pair{buffer, 0uz};
  }
  return std::pair{buffer, static_cast<std::size_t>(end - buffer.data())};
}

/**
 * @brief 16進数整数を文字列に変換する
 *
 * @param value 変換する整数
 * @return auto 変換文字列とその長さのペア
 */
[[nodiscard]] auto consteval to_hex_chars(long long value) noexcept {
  auto buffer = std::array<char, 17>{};
  auto const [end, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(),
                                       static_cast<unsigned long long>(value), 16);
  if (ec != std::errc{}) {
    return std::pair{buffer, 0uz};
  }
  return std::pair{buffer, static_cast<std::size_t>(end - buffer.data())};
}

/**
 * @brief 2進数整数を文字列に変換する
 *
 * @param value 変換する整数
 * @return auto 変換文字列とその長さのペア
 */
[[nodiscard]] auto consteval to_bin_chars(long long value) noexcept {
  auto buffer = std::array<char, 65>{};
  auto v = static_cast<unsigned long long>(value);
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
 * @brief 8進数整数を文字列に変換する
 *
 * @param value 変換する整数
 * @return auto 変換された文字列とその長さのペア
 */
[[nodiscard]] auto consteval to_oct_chars(long long value) noexcept {
  auto buffer = std::array<char, 23>{};
  auto v = static_cast<unsigned long long>(value);
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
 * @brief 浮動小数点数を文字列に変換する（簡易固定小数点）
 *
 * @param value 変換する浮動小数点数
 * @param precision 小数点以下の桁数
 * @return auto 変換文字列とその長さのペア
 */
[[nodiscard]] auto consteval to_float_chars(double value, int const precision) noexcept {
  auto buffer = std::array<char, 48>{};
  auto i = 0uz;

  if (!std::isfinite(value)) {
    return std::pair{buffer, i};
  }

  if (value < 0) {
    buffer[i++] = '-'; value = -value;
  }

  if (value >= static_cast<double>(std::numeric_limits<long long>::max())) {
    return std::pair{buffer, i};
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

} // namespace frozenchars::detail
