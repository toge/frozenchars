#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <charconv>
#include <system_error>
#include <limits>

#include "string.hpp"

namespace frozenchars {

/**
 * @brief 文字列を数値に変換する
 *
 * @tparam T 変換後の型
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換後の数値
 */
template <typename T, size_t N>
[[nodiscard]] auto constexpr parse_number(FrozenString<N> const& str) -> T {
  auto const sv = str.sv();
  if (sv.empty()) {
    throw std::invalid_argument("empty string");
  }

  size_t start = 0;
  bool neg = false;
  if (sv[0] == '-') {
    neg = true;
    start = 1;
  } else if (sv[0] == '+') {
    start = 1;
  }

  if (start >= sv.size()) {
    throw std::invalid_argument("no digits");
  }

  // 整数型の場合は std::from_chars を使用して変換する
  if constexpr (std::integral<T>) {
    if constexpr (std::unsigned_integral<T>) {
      if (neg) {
        throw std::out_of_range("negative value for unsigned type");
      }
    }

    int base = 10;
    if (sv.size() > start + 1 && sv[start] == '0') {
      if (sv.size() > start + 2 && (sv[start + 1] == 'x' || sv[start + 1] == 'X')) {
        base = 16;
        start += 2;
      } else if (sv.size() > start + 2 && (sv[start + 1] == 'b' || sv[start + 1] == 'B')) {
        base = 2;
        start += 2;
      } else {
        base = 8;
        start += 1;
      }
    }

    if (start >= sv.size() && base != 10) {
      throw std::invalid_argument("missing digits after prefix");
    }

    if constexpr (std::same_as<T, unsigned long long>) {
      T res = 0;
      auto const [ptr, ec] = std::from_chars(sv.data() + start, sv.data() + sv.size(), res, base);
      if (ec == std::errc::invalid_argument) {
        throw std::invalid_argument("invalid digit");
      }
      if (ec == std::errc::result_out_of_range) {
        throw std::out_of_range("overflow");
      }
      if (ptr != sv.data() + sv.size()) {
        throw std::invalid_argument("invalid digit");
      }
      return res;
    } else if constexpr (std::unsigned_integral<T>) {
      unsigned long long res = 0;
      auto const [ptr, ec] = std::from_chars(sv.data() + start, sv.data() + sv.size(), res, base);
      if (ec == std::errc::invalid_argument) {
        throw std::invalid_argument("invalid digit");
      }
      if (ec == std::errc::result_out_of_range) {
        throw std::out_of_range("overflow");
      }
      if (ptr != sv.data() + sv.size()) {
        throw std::invalid_argument("invalid digit");
      }
      if (res > static_cast<unsigned long long>(std::numeric_limits<T>::max())) {
        throw std::out_of_range("overflow");
      }
      return static_cast<T>(res);
    } else {
      long long res = 0;
      auto const [ptr, ec] = std::from_chars(sv.data() + start, sv.data() + sv.size(), res, base);
      if (ec == std::errc::invalid_argument) {
        throw std::invalid_argument("invalid digit");
      }
      if (ec == std::errc::result_out_of_range) {
        throw std::out_of_range("overflow");
      }
      if (ptr != sv.data() + sv.size()) {
        throw std::invalid_argument("invalid digit");
      }
      if (neg) {
        if (res < 0 || static_cast<unsigned long long>(res) > static_cast<unsigned long long>(std::numeric_limits<T>::max()) + 1ULL) {
          throw std::out_of_range("underflow");
        }
        return static_cast<T>(-res);
      } else {
        if (res < 0 || res > std::numeric_limits<T>::max()) {
          throw std::out_of_range("overflow");
        }
        return static_cast<T>(res);
      }
    }
  // 実数型の場合はランタイムではstd::from_chars, コンパイル時は独自実装で変換する
  } else if constexpr (std::floating_point<T>) {
    // ランタイムパス: std::from_chars でロケール非依存かつIEEE754準拠の正確な変換を行う。
    if (!std::is_constant_evaluated()) {
      // from_chars は先頭 '+' を受理しないため1文字スキップする。'-' は from_chars が処理する。
      // ponytail: general フォーマット固定。16進float "0x1.8p3" は解さないが依存箇所なし。
      char const* first    = sv.data() + (sv[0] == '+' ? 1 : 0);
      char const* last     = sv.data() + sv.size();
      T           result{};
      auto const [ptr, ec] = std::from_chars(first, last, result);
      if (ec == std::errc::invalid_argument || ptr != last) {
        throw std::invalid_argument("invalid float");
      }
      if (ec == std::errc::result_out_of_range) {
        throw std::out_of_range("float overflow");
      }
      return result;
    }

    // コンパイル時パス: uint64_t で整数部・小数部を蓄積して精度を向上させる。
    // float/double への変換は最後に1回だけ行う。
    auto constexpr pow10 = [](int n) -> T {
      T result = 1;
      T base   = 10;
      while (n > 0) {
        if (n & 1) {
          result *= base;
        }
        base *= base;
        n >>= 1;
      }
      return result;
    };

    size_t i        = start;
    bool has_digits = false;

    // 整数部を uint64_t で蓄積（2^64 ≈ 1.8×10^19 まで厳密）
    std::uint64_t int_part   = 0;
    int           extra_exp  = 0; // uint64_t オーバーフロー時の桁数補正
    while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
      if (int_part <= (UINT64_MAX - 9) / 10) {
        int_part = int_part * 10 + static_cast<std::uint64_t>(sv[i] - '0');
      } else {
        ++extra_exp;
      }
      ++i;
      has_digits = true;
    }
    T res = static_cast<T>(int_part);
    if (extra_exp > 0) {
      res *= pow10(extra_exp);
    }

    // 小数部を uint64_t で蓄積し、最後に1回の除算で精度を確保する
    if (i < sv.size() && sv[i] == '.') {
      ++i;
      std::uint64_t frac_digits = 0;
      int           frac_count  = 0;
      while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
        if (frac_count < 18) {
          frac_digits = frac_digits * 10 + static_cast<std::uint64_t>(sv[i] - '0');
          ++frac_count;
        }
        // 18桁以降は丸め誤差が小さいため切り捨て
        ++i;
        has_digits = true;
      }
      if (frac_count > 0) {
        res += static_cast<T>(frac_digits) / pow10(frac_count);
      }
    }

    int exp = 0;
    if (i < sv.size() && (sv[i] == 'e' || sv[i] == 'E')) {
      ++i;
      bool eneg = false;
      if (i < sv.size() && sv[i] == '-') {
        eneg = true; ++i;
      } else if (i < sv.size() && sv[i] == '+') {
        ++i;
      }
      bool has_exp_digits = false;
      while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
        exp = exp * 10 + static_cast<int>(sv[i] - '0');
        ++i;
        has_exp_digits = true;
      }
      if (!has_exp_digits) {
        throw std::invalid_argument("invalid exponent");
      }
      if (eneg) {
        exp = -exp;
      }
    }
    if (!has_digits || i < sv.size()) {
      throw std::invalid_argument("invalid float");
    }
    T final_res = neg ? -res : res;
    if (exp != 0) {
      if (exp > 0) {
        final_res *= pow10(exp);
      } else {
        final_res /= pow10(-exp);
      }
    }
    if (final_res == std::numeric_limits<T>::infinity() || final_res == -std::numeric_limits<T>::infinity()) {
      throw std::out_of_range("float overflow");
    }
    return final_res;
  } else {
    throw std::invalid_argument("unsupported type");
  }
}

template <typename T, size_t N>
[[nodiscard]] auto constexpr parse_number(char const (&str)[N]) -> T {
  return parse_number<T>(FrozenString{str});
}

} // namespace frozenchars
