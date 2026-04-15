#pragma once

#include "char_utils.hpp"
#include <concepts>
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace frozenchars::detail {

enum class integer_base_mode {
  decimal_only,
  autodetect
};

/**
 * @brief 区切り判定関数でトークン数を数える
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto トークン数
 */
template <auto IsDelimiter = is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split_count_impl(FrozenString<N> const& str) noexcept {
  auto count = 0uz;
  auto in_token = false;
  for (auto i = 0uz; i < str.length; ++i) {
    if (IsDelimiter(str.buffer[i])) {
      in_token = false;
    } else if (!in_token) {
      in_token = true;
      ++count;
    }
  }
  return count;
}

/**
 * @brief 数値字句を指定数値型に変換する
 *
 * @tparam Number 変換先の数値型
 * @tparam BaseMode 整数の基数解釈モード
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param token 変換するトークン
 * @return auto 変換結果
 */
template <typename Number = int, integer_base_mode BaseMode = integer_base_mode::decimal_only, size_t N>
  requires (!std::same_as<std::remove_cv_t<Number>, bool>)
constexpr auto parse_number_token(FrozenString<N> const& token) {
  using Result = std::remove_cv_t<Number>;

  auto constexpr invalid = "parse_number: invalid number";
  auto constexpr out_of_range = "parse_number: out of range";

  if (token.length == 0) {
    throw std::invalid_argument(invalid);
  }

  // 整数値へ変換
  if constexpr (std::is_integral_v<Result>) {
    using Unsigned = std::make_unsigned_t<Result>;

    auto pos = 0uz;
    auto negative = false;
    if (token.buffer[pos] == '+' || token.buffer[pos] == '-') {
      negative = token.buffer[pos] == '-';
      ++pos;
    }
    if (pos == token.length) {
      throw std::invalid_argument(invalid);
    }

    auto base = Unsigned{10};
    if constexpr (BaseMode == integer_base_mode::autodetect) {
      if (token.buffer[pos] == '0' && pos + 1 < token.length) {
        if (token.buffer[pos + 1] == 'x' || token.buffer[pos + 1] == 'X') {
          base = 16;
          pos += 2;
        } else if (token.buffer[pos + 1] == 'b' || token.buffer[pos + 1] == 'B') {
          base = 2;
          pos += 2;
        } else {
          base = 8;
          ++pos;
        }
      }
    }
    if (pos == token.length) {
      throw std::invalid_argument(invalid);
    }

    auto value = Unsigned{0};
    auto constexpr IS_SIGNED = std::numeric_limits<Result>::is_signed;
    auto constexpr MAX_POSITIVE = static_cast<Unsigned>(std::numeric_limits<Result>::max());
    auto const limit = [&]() constexpr {
      if constexpr (IS_SIGNED) {
        auto constexpr NEGATIVE_LIMIT = static_cast<Unsigned>(MAX_POSITIVE + 1u);
        return negative ? NEGATIVE_LIMIT : MAX_POSITIVE;
      } else {
        return MAX_POSITIVE;
      }
    }();

    for (; pos < token.length; ++pos) {
      auto const c = token.buffer[pos];
      auto const digit = [&]() constexpr -> Unsigned {
        if (c >= '0' && c <= '9') {
          return static_cast<Unsigned>(c - '0');
        }
        if (c >= 'a' && c <= 'f') {
          return static_cast<Unsigned>(10 + (c - 'a'));
        }
        if (c >= 'A' && c <= 'F') {
          return static_cast<Unsigned>(10 + (c - 'A'));
        }
        return base;
      }();
      if (digit >= base) {
        throw std::invalid_argument(invalid);
      }
      if (value > (limit - digit) / base) {
        throw std::out_of_range(out_of_range);
      }
      value = static_cast<Unsigned>(value * base + digit);
    }

    if (negative) {
      if constexpr (!IS_SIGNED) {
        throw std::out_of_range(out_of_range);
      } else {
        auto constexpr NEGATIVE_LIMIT = static_cast<Unsigned>(MAX_POSITIVE + 1u);
        if (value == NEGATIVE_LIMIT) {
          return std::numeric_limits<Result>::min();
        }
        return static_cast<Result>(-static_cast<Result>(value));
      }
    }

    return static_cast<Result>(value);
  // 実数値へ変換
  } else {
    auto pos = 0uz;
    auto negative = false;
    if (token.buffer[pos] == '+' || token.buffer[pos] == '-') {
      negative = token.buffer[pos] == '-';
      ++pos;
    }
    if (pos == token.length) {
      throw std::invalid_argument(invalid);
    }

    auto value = 0.0L;
    auto has_digits = false;

    while (pos < token.length && token.buffer[pos] >= '0' && token.buffer[pos] <= '9') {
      has_digits = true;
      value = value * 10.0L + static_cast<long double>(token.buffer[pos] - '0');
      ++pos;
    }

    if (pos < token.length && token.buffer[pos] == '.') {
      ++pos;
      auto place = 0.1L;
      while (pos < token.length && token.buffer[pos] >= '0' && token.buffer[pos] <= '9') {
        has_digits = true;
        value += static_cast<long double>(token.buffer[pos] - '0') * place;
        place *= 0.1L;
        ++pos;
      }
    }

    if (!has_digits) {
      throw std::invalid_argument(invalid);
    }

    if (pos < token.length && (token.buffer[pos] == 'e' || token.buffer[pos] == 'E')) {
      ++pos;
      auto exp_negative = false;
      if (pos < token.length && (token.buffer[pos] == '+' || token.buffer[pos] == '-')) {
        exp_negative = token.buffer[pos] == '-';
        ++pos;
      }

      if (pos == token.length || token.buffer[pos] < '0' || token.buffer[pos] > '9') {
        throw std::invalid_argument(invalid);
      }

      auto exponent = 0;
      while (pos < token.length && token.buffer[pos] >= '0' && token.buffer[pos] <= '9') {
        auto const digit = token.buffer[pos] - '0';
        if (exponent > (std::numeric_limits<int>::max() - digit) / 10) {
          throw std::out_of_range(out_of_range);
        }
        exponent = exponent * 10 + digit;
        ++pos;
      }

      if (exp_negative) {
        for (auto i = 0; i < exponent; ++i) {
          value /= 10.0L;
        }
      } else {
        auto const max_abs = static_cast<long double>(std::numeric_limits<Result>::max());
        for (auto i = 0; i < exponent; ++i) {
          value *= 10.0L;
          if (value > max_abs) {
            throw std::out_of_range(out_of_range);
          }
        }
      }
    }

    if (pos != token.length) {
      throw std::invalid_argument(invalid);
    }

    if (negative) {
      value = -value;
    }

    auto const min_value = static_cast<long double>(std::numeric_limits<Result>::lowest());
    auto const max_value = static_cast<long double>(std::numeric_limits<Result>::max());
    if (value < min_value || value > max_value) {
      throw std::out_of_range(out_of_range);
    }

    return static_cast<Result>(value);
  }
}

} // namespace frozenchars::detail
