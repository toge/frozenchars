#pragma once

#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <string_view>
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
  } else if constexpr (std::floating_point<T>) {
    if (!std::is_constant_evaluated()) {
      T res = 0;
      auto const [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), res);
      if (ec == std::errc::invalid_argument) {
        throw std::invalid_argument("invalid float");
      }
      if (ec == std::errc::result_out_of_range) {
        throw std::out_of_range("float overflow");
      }
      if (ptr != sv.data() + sv.size()) {
        throw std::invalid_argument("invalid float");
      }
      return res;
    }
    T res = 0;
    size_t i = start;
    bool has_digits = false;
    while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
      res = res * 10 + static_cast<T>(sv[i] - '0');
      ++i;
      has_digits = true;
    }
    if (i < sv.size() && sv[i] == '.') {
      ++i;
      T frac = 1;
      while (i < sv.size() && sv[i] >= '0' && sv[i] <= '9') {
        frac /= 10;
        res += static_cast<T>(sv[i] - '0') * frac;
        ++i;
        has_digits = true;
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
      if (eneg) exp = -exp;
    }
    if (!has_digits || i < sv.size()) {
      throw std::invalid_argument("invalid float");
    }
    T final_res = neg ? -res : res;
    if (exp != 0) {
      auto constexpr pow10 = [](int n) -> T {
        T result = 1;
        T base = 10;
        while (n > 0) {
          if (n & 1) result *= base;
          base *= base;
          n >>= 1;
        }
        return result;
      };
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
