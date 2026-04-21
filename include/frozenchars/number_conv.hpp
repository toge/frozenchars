#pragma once

#include "frozen_string.hpp"
#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <charconv>
#include <system_error>
#include <limits>

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
auto constexpr parse_number(FrozenString<N> const& str) -> T {
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

  if constexpr (std::integral<T>) {
    T res = 0;
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
      // Handle cases like "0x" or "0b" which might be invalid depending on requirements
      // But usually at least one digit is expected after prefix.
      // test_parse_number.cpp seems to expect exception for missing_hex ("0x")
      throw std::invalid_argument("missing digits after prefix");
    }

    for (size_t i = start; i < sv.size(); ++i) {
      int digit = 0;
      if (sv[i] >= '0' && sv[i] <= '9') digit = sv[i] - '0';
      else if (sv[i] >= 'a' && sv[i] <= 'f') digit = sv[i] - 'a' + 10;
      else if (sv[i] >= 'A' && sv[i] <= 'F') digit = sv[i] - 'A' + 10;
      else throw std::invalid_argument("invalid digit");

      if (digit >= base) throw std::invalid_argument("digit out of base range");

      if (neg) {
        if (__builtin_mul_overflow(res, static_cast<T>(base), &res) || __builtin_sub_overflow(res, static_cast<T>(digit), &res)) {
          throw std::out_of_range("overflow");
        }
      } else {
        if (__builtin_mul_overflow(res, static_cast<T>(base), &res) || __builtin_add_overflow(res, static_cast<T>(digit), &res)) {
          throw std::out_of_range("overflow");
        }
      }
    }
    return res;
  } else if constexpr (std::floating_point<T>) {
    T res = 0;
    size_t i = start;
    bool has_digits = false;
    while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
      res = res * 10 + (sv[i] - '0');
      ++i;
      has_digits = true;
    }
    if (i < sv.size() && sv[i] == '.') {
      ++i;
      T frac = 0.1;
      while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
        res += (sv[i] - '0') * frac;
        frac /= 10.0;
        ++i;
        has_digits = true;
      }
    }
    if (i < sv.size() && (sv[i] == 'e' || sv[i] == 'E')) {
      ++i;
      bool eneg = false;
      if (i < sv.size() && sv[i] == '-') {
        eneg = true; ++i;
      } else if (i < sv.size() && sv[i] == '+') {
        ++i;
      }
      int exp = 0;
      bool has_exp_digits = false;
      while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
        if (__builtin_mul_overflow(exp, 10, &exp) || __builtin_add_overflow(exp, static_cast<int>(sv[i] - '0'), &exp)) {
          exp = 1000; // Force overflow later
        }
        ++i;
        has_exp_digits = true;
      }
      if (!has_exp_digits) {
        throw std::invalid_argument("invalid exponent");
      }
      if (eneg) {
        for (int j = 0; j < exp; ++j) {
          res /= 10.0;
          if (res == 0) {
            break;
          }
        }
      } else {
        for (int j = 0; j < exp; ++j) {
          res *= 10.0;
          if (res > std::numeric_limits<T>::max() || res < -std::numeric_limits<T>::max()) {
            throw std::out_of_range("float overflow");
          }
        }
      }
    }
    if (!has_digits || i < sv.size()) {
      throw std::invalid_argument("invalid float");
    }
    T final_res = neg ? -res : res;
    if (final_res == std::numeric_limits<T>::infinity() || final_res == -std::numeric_limits<T>::infinity()) {
      throw std::out_of_range("float overflow");
    }
    return final_res;
  } else {
    throw std::invalid_argument("unsupported type");
  }
}

template <typename T, size_t N>
auto constexpr parse_number(char const (&str)[N]) -> T {
  return parse_number<T>(FrozenString{str});
}

} // namespace frozenchars
