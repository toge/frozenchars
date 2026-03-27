#ifndef __FROZEN_CHARS_H__
#define __FROZEN_CHARS_H__

#include <array>
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <vector>

namespace frozenchars {

/*===============================================================================*\
 * ユーティリティ
\*===============================================================================*/

/**
 * @brief 整数型を文字列化するためのタグ
 *
 * @tparam T 対象となる型
 */
template <typename T>
concept Integral = std::is_integral_v<T>;

/**
 * @brief 浮動小数点型を文字列化するためのタグ
 *
 * @tparam T 対象となる型
 */
template <typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

/**
 * @brief 数値型（整数または浮動小数点）
 *
 * @tparam T 対象となる型
 */
template <typename T>
concept Numeric = Integral<T> || FloatingPoint<T>;

/**
 * @brief 整数値を16進数表現するためのタグ
 *
 */
struct Hex {
  long long value;
  constexpr Hex(Integral auto v)
  : value(v)
  {}
};

/**
 * @brief 整数値を2進数表現するためのタグ
 *
 */
struct Bin {
  long long value;
  constexpr Bin(Integral auto v)
  : value(v)
  {}
};

/**
 * @brief 整数値を8進数表現するためのタグ
 *
 */
struct Oct {
  long long value;
  constexpr Oct(Integral auto v)
  : value(v)
  {}
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

/**
 * @brief 固定長文字列を表す構造体
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 */
template <size_t N>
struct FixedString {
  char data[N]{};
  constexpr FixedString(char const (&str)[N]) noexcept {
    for (auto i = 0uz; i < N; ++i) {
      data[i] = str[i];
    }
  }
  auto constexpr sv() const noexcept {
    return std::string_view{data, N - 1};
  }
};

/**
 * @brief 静的文字列を表す構造体
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 */
template <size_t N>
struct FrozenString {
  std::array<char, N> buffer{};
  size_t length{N > 0 ? N - 1 : 0};

  constexpr FrozenString() noexcept = default;
  constexpr FrozenString(char const (&str)[N]) noexcept
  : length{N > 0 ? N - 1 : 0} {
    for (auto i = 0uz; i < N; ++i) {
      buffer[i] = str[i];
    }
  }

  auto constexpr sv() const noexcept {
    return std::string_view{buffer.data(), length};
  }

  // FrozenString同士の結合
  template <size_t M>
  auto constexpr operator+(FrozenString<M> const& other) const noexcept {
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
  auto constexpr operator+(char const (&rhs)[M]) const noexcept {
    return *this + FrozenString<M>{rhs};
  }
};

/**
 * @brief 文字列を指定回数繰り返した静的文字列を生成する関数
 *
 * @tparam Count 繰り返し回数
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 繰り返す文字列
 * @return auto constexpr 繰り返された静的文字列
 */
template <size_t Count, size_t N>
auto constexpr repeat(FrozenString<N> const& str) noexcept {
  auto constexpr UNIT_LEN = N > 0 ? N - 1 : 0;
  auto constexpr NEW_SIZE = UNIT_LEN * Count + 1;

  auto res = FrozenString<NEW_SIZE>{};
  auto offset = 0uz;
  auto const src = str.sv();

  for (auto i = 0uz; i < Count; ++i) {
    for (auto const c : src) {
      res.buffer[offset++] = c;
    }
  }
  if constexpr (NEW_SIZE > 0) {
    res.buffer[NEW_SIZE - 1] = '\0';
  }
  return res;
}

/**
 * @brief 文字列リテラルを繰り返した文字列を生成する関数
 *
 * @tparam Count 繰り返し回数
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 繰り返す文字列リテラル
 * @return auto constexpr 繰り返された静的文字列
 */
template <size_t Count, size_t N>
auto constexpr repeat(char const (&str)[N]) noexcept {
  return repeat<Count>(FrozenString{str});
}

/**
 * @brief 文字列を指定幅で右寄せした静的文字列を生成する関数
 *
 * @tparam Width 右寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr 右寄せされた静的文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto constexpr right(FrozenString<N> const& str) noexcept {
  auto constexpr OUT_CAP = N > (Width + 1) ? N : (Width + 1);
  auto res = FrozenString<OUT_CAP>{};

  auto const out_len = std::max(str.length, Width);
  auto const left_pad = out_len - str.length;

  for (auto i = 0uz; i < left_pad; ++i) {
    res.buffer[i] = Fill;
  }
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[left_pad + i] = str.buffer[i];
  }

  res.buffer[out_len] = '\0';
  res.length = out_len;
  return res;
}

/**
 * @brief 文字列リテラルを指定幅で右寄せした静的文字列を生成する関数
 *
 * @tparam Width 右寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr 右寄せされた静的文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto constexpr right(char const (&str)[N]) noexcept {
  return right<Width, Fill>(FrozenString{str});
}

/**
 * @brief 文字列を指定幅で中央寄せした静的文字列を生成する関数
 *
 * @tparam Width 中央寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr 中央寄せされた静的文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto constexpr center(FrozenString<N> const& str) noexcept {
  auto constexpr OUT_CAP = N > (Width + 1) ? N : (Width + 1);
  auto res = FrozenString<OUT_CAP>{};

  auto const out_len = std::max(str.length, Width);
  auto const total_pad = out_len - str.length;
  auto const left_pad = total_pad / 2;
  auto const right_pad = total_pad - left_pad;

  for (auto i = 0uz; i < left_pad; ++i) {
    res.buffer[i] = Fill;
  }
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[left_pad + i] = str.buffer[i];
  }
  for (auto i = 0uz; i < right_pad; ++i) {
    res.buffer[left_pad + str.length + i] = Fill;
  }

  res.buffer[out_len] = '\0';
  res.length = out_len;
  return res;
}

/**
 * @brief 文字列リテラルを指定幅で中央寄せした静的文字列を生成する関数
 *
 * @tparam Width 中央寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr 中央寄せされた静的文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto constexpr center(char const (&str)[N]) noexcept {
  return center<Width, Fill>(FrozenString{str});
}

/**
 * @brief 文字列をすべて大文字に変換した静的文字列を生成する関数
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr 大文字に変換された静的文字列
 */
template <size_t N>
auto constexpr toupper(FrozenString<N> const& str) noexcept {
  auto res = str;
  for (auto i = 0uz; i < res.length; ++i) {
    auto const c = res.buffer[i];
    if (c >= 'a' && c <= 'z') {
      res.buffer[i] = static_cast<char>(c - ('a' - 'A'));
    }
  }
  return res;
}

/**
 * @brief 文字列リテラルをすべて大文字に変換した静的文字列を生成する関数
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr 大文字に変換された静的文字列
 */
template <size_t N>
auto constexpr toupper(char const (&str)[N]) noexcept {
  return toupper(FrozenString{str});
}

/**
 * @brief 文字列をすべて小文字に変換した静的文字列を生成する関数
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr 小文字に変換された静的文字列
 */
template <size_t N>
auto constexpr tolower(FrozenString<N> const& str) noexcept {
  auto res = str;
  for (auto i = 0uz; i < res.length; ++i) {
    auto const c = res.buffer[i];
    if (c >= 'A' && c <= 'Z') {
      res.buffer[i] = static_cast<char>(c + ('a' - 'A'));
    }
  }
  return res;
}

/**
 * @brief 文字列リテラルをすべて小文字に変換した静的文字列を生成する関数
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr 小文字に変換された静的文字列
 */
template <size_t N>
auto constexpr tolower(char const (&str)[N]) noexcept {
  return tolower(FrozenString{str});
}

/**
 * @brief 文字列の部分文字列を生成する関数
 *
 * @tparam Pos 開始位置
 * @tparam Len 文字数。負の場合は Pos の左側から abs(Len) 文字
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr 部分文字列
 */
template <size_t Pos, std::ptrdiff_t Len, size_t N>
auto constexpr substr(FrozenString<N> const& str) noexcept {
  auto constexpr REQUESTED_LEN = Len >= 0 ? static_cast<size_t>(Len) : static_cast<size_t>(-Len);
  auto res = FrozenString<REQUESTED_LEN + 1>{};
  auto const anchor = std::min(Pos, str.length);
  auto start = anchor;
  auto actual_len = 0uz;

  if constexpr (Len >= 0) {
    actual_len = anchor < str.length ? std::min(REQUESTED_LEN, str.length - anchor) : 0uz;
  } else {
    start = anchor > REQUESTED_LEN ? anchor - REQUESTED_LEN : 0uz;
    actual_len = anchor - start;
  }

  for (auto i = 0uz; i < actual_len; ++i) {
    res.buffer[i] = str.buffer[start + i];
  }
  res.buffer[actual_len] = '\0';
  res.length = actual_len;
  return res;
}

/**
 * @brief 文字列リテラルの部分文字列を生成する関数
 *
 * @tparam Pos 開始位置
 * @tparam Len 文字数。負の場合は Pos の左側から abs(Len) 文字
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr 部分文字列
 */
template <size_t Pos, std::ptrdiff_t Len, size_t N>
auto constexpr substr(char const (&str)[N]) noexcept {
  return substr<Pos, Len>(FrozenString{str});
}

namespace detail {

  /**
   * @brief ASCII 空白文字かどうかを判定する関数
   *
   * @param c 判定する文字
   * @return auto constexpr 空白なら true
   */
  auto constexpr is_whitespace(char c) noexcept {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
  }

  template <bool TrimLeft, bool TrimRight, char TrimChar, size_t N>
  auto constexpr trim_copy(FrozenString<N> const& str) noexcept {
    auto res = FrozenString<N>{};
    auto start = 0uz;
    auto end = str.length;

    if constexpr (TrimLeft) {
      while (start < str.length && str.buffer[start] == TrimChar) {
        ++start;
      }
    }

    if constexpr (TrimRight) {
      while (end > start && str.buffer[end - 1] == TrimChar) {
        --end;
      }
    }

    auto const out_len = end - start;
    for (auto i = 0uz; i < out_len; ++i) {
      res.buffer[i] = str.buffer[start + i];
    }
    res.buffer[out_len] = '\0';
    res.length = out_len;
    return res;
  }

  /**
   * @brief ASCII の16進数字かどうかを判定する関数
   *
   * @param c 判定する文字
   * @return auto constexpr 16進数字なら true
   */
  auto constexpr is_hex_digit(char c) noexcept {
    return (c >= '0' && c <= '9')
      || (c >= 'a' && c <= 'f')
      || (c >= 'A' && c <= 'F');
  }

  /**
   * @brief 16進数字1文字を 0..15 に変換する関数
   *
   * @param c 変換する16進数字
   * @return auto constexpr 変換結果
   */
  auto constexpr hex_digit_to_value(char c) {
    if (c >= '0' && c <= '9') {
      return static_cast<std::uint8_t>(c - '0');
    }
    if (c >= 'a' && c <= 'f') {
      return static_cast<std::uint8_t>(10 + (c - 'a'));
    }
    if (c >= 'A' && c <= 'F') {
      return static_cast<std::uint8_t>(10 + (c - 'A'));
    }
    throw std::invalid_argument("parse_hex_color: invalid hex digit");
  }

  /**
   * @brief 16進数字2文字を 1byte に変換する関数
   *
   * @param hi 上位4bitを表す16進数字
   * @param lo 下位4bitを表す16進数字
   * @return auto constexpr 変換結果
   */
  auto constexpr parse_hex_byte(char hi, char lo) {
    if (!is_hex_digit(hi) || !is_hex_digit(lo)) {
      throw std::invalid_argument("parse_hex_color: invalid hex digit");
    }
    return static_cast<std::uint8_t>((hex_digit_to_value(hi) << 4u) | hex_digit_to_value(lo));
  }

  /**
   * @brief 16進数字1文字を nibble 複製して 1byte に変換する関数
   *
   * @param c 変換する16進数字
   * @return auto constexpr 変換結果
   */
  auto constexpr parse_hex_shorthand_byte(char c) {
    auto const value = hex_digit_to_value(c);
    return static_cast<std::uint8_t>((value << 4u) | value);
  }

  /**
   * @brief 区切り判定関数でトークン数を数える関数
   *
   * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
   * @tparam N 文字列の長さ (終端文字'\0'を含む)
   * @param str 対象文字列
   * @return auto constexpr トークン数
   */
  template <auto IsDelimiter = is_whitespace, size_t N>
    requires std::predicate<decltype(IsDelimiter), char>
  auto constexpr split_count_impl(FrozenString<N> const& str) noexcept {
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
   * @brief 整数字句を指定整数型に変換する関数
   *
   * @tparam Int 変換先の整数型
   * @tparam N 文字列の長さ (終端文字'\0'を含む)
   * @param token 変換するトークン
   * @return auto constexpr 変換結果
   */
  template <Numeric Number = int, size_t N>
    requires (!std::same_as<std::remove_cv_t<Number>, bool>)
  auto constexpr parse_int_token(FrozenString<N> const& token) {
    using Result = std::remove_cv_t<Number>;

    if (token.length == 0) {
      throw std::invalid_argument("split_numbers: empty token");
    }

    if constexpr (Integral<Result>) {
      using Unsigned = std::make_unsigned_t<Result>;

      auto pos = 0uz;
      auto negative = false;
      if (token.buffer[pos] == '+' || token.buffer[pos] == '-') {
        negative = token.buffer[pos] == '-';
        ++pos;
      }
      if (pos == token.length) {
        throw std::invalid_argument("split_numbers: sign without digits");
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
        if (c < '0' || c > '9') {
          throw std::invalid_argument("split_numbers: non-numeric token");
        }

        auto const digit = static_cast<Unsigned>(c - '0');
        if (value > (limit - digit) / 10u) {
          throw std::out_of_range("split_numbers: integer out of range");
        }
        value = static_cast<Unsigned>(value * 10u + digit);
      }

      if (negative) {
        if constexpr (!IS_SIGNED) {
          throw std::out_of_range("split_numbers: negative token for unsigned type");
        } else {
          auto constexpr NEGATIVE_LIMIT = static_cast<Unsigned>(MAX_POSITIVE + 1u);
          if (value == NEGATIVE_LIMIT) {
            return std::numeric_limits<Result>::min();
          }
          return static_cast<Result>(-static_cast<Result>(value));
        }
      }

      return static_cast<Result>(value);
    } else {
      auto pos = 0uz;
      auto negative = false;
      if (token.buffer[pos] == '+' || token.buffer[pos] == '-') {
        negative = token.buffer[pos] == '-';
        ++pos;
      }
      if (pos == token.length) {
        throw std::invalid_argument("split_numbers: sign without digits");
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
        throw std::invalid_argument("split_numbers: non-numeric token");
      }

      if (pos < token.length && (token.buffer[pos] == 'e' || token.buffer[pos] == 'E')) {
        ++pos;
        auto exp_negative = false;
        if (pos < token.length && (token.buffer[pos] == '+' || token.buffer[pos] == '-')) {
          exp_negative = token.buffer[pos] == '-';
          ++pos;
        }

        if (pos == token.length || token.buffer[pos] < '0' || token.buffer[pos] > '9') {
          throw std::invalid_argument("split_numbers: invalid exponent");
        }

        auto exponent = 0;
        while (pos < token.length && token.buffer[pos] >= '0' && token.buffer[pos] <= '9') {
          auto const digit = token.buffer[pos] - '0';
          if (exponent > (std::numeric_limits<int>::max() - digit) / 10) {
            throw std::out_of_range("split_numbers: floating exponent out of range");
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
              throw std::out_of_range("split_numbers: floating value out of range");
            }
          }
        }
      }

      if (pos != token.length) {
        throw std::invalid_argument("split_numbers: non-numeric token");
      }

      if (negative) {
        value = -value;
      }

      auto const min_value = static_cast<long double>(std::numeric_limits<Result>::lowest());
      auto const max_value = static_cast<long double>(std::numeric_limits<Result>::max());
      if (value < min_value || value > max_value) {
        throw std::out_of_range("split_numbers: floating value out of range");
      }

      return static_cast<Result>(value);
    }
  }

}

/**
 * @brief 文字列を区切り判定関数で分割したときのトークン数を返す関数
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr トークン数
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto constexpr split_count(FrozenString<N> const& str) noexcept {
  return detail::split_count_impl<IsDelimiter>(str);
}

/**
 * @brief 文字列リテラルを区切り判定関数で分割したときのトークン数を返す関数
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr トークン数
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto constexpr split_count(char const (&str)[N]) noexcept {
  return split_count<IsDelimiter>(FrozenString{str});
}

/**
 * @brief 文字列を区切り判定関数で分割して std::array に変換する関数
 * `Count` より多いトークンは切り捨て、足りない要素は空文字列のまま残る
 *
 * @tparam Count 返却する配列の要素数
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr 分割結果の配列
 */
template <size_t Count, auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto constexpr split(FrozenString<N> const& str) noexcept {
  auto res = std::array<FrozenString<N>, Count>{};
  for (auto& token : res) {
    token.length = 0;
  }
  auto const token_count = split_count<IsDelimiter>(str);
  auto const token_limit = std::min(Count, token_count);
  auto src = 0uz;
  auto dst = 0uz;

  while (src < str.length && dst < token_limit) {
    while (src < str.length && IsDelimiter(str.buffer[src])) {
      ++src;
    }
    if (src >= str.length) {
      break;
    }

    auto token_len = 0uz;
    while (src < str.length && !IsDelimiter(str.buffer[src])) {
      res[dst].buffer[token_len++] = str.buffer[src++];
    }
    res[dst].buffer[token_len] = '\0';
    res[dst].length = token_len;
    ++dst;
  }

  return res;
}

/**
 * @brief 文字列を区切り判定関数で分割し、1回の呼び出しで結果を返す関数
 * split_count(...) を内部で呼び出して、必要分までトークンを格納する
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr 分割結果の配列（未使用要素は空文字）
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto constexpr split(FrozenString<N> const& str) noexcept {
  auto res = std::array<FrozenString<N>, N>{};
  for (auto& token : res) {
    token.length = 0;
  }

  auto const token_count = split_count<IsDelimiter>(str);
  auto src = 0uz;
  auto dst = 0uz;

  while (src < str.length && dst < token_count) {
    while (src < str.length && IsDelimiter(str.buffer[src])) {
      ++src;
    }
    if (src >= str.length) {
      break;
    }

    auto token_len = 0uz;
    while (src < str.length && !IsDelimiter(str.buffer[src])) {
      res[dst].buffer[token_len++] = str.buffer[src++];
    }
    res[dst].buffer[token_len] = '\0';
    res[dst].length = token_len;
    ++dst;
  }

  return res;
}

/**
 * @brief 文字列リテラルを区切り判定関数で分割し、1回の呼び出しで結果を返す関数
 * split_count(...) を内部で呼び出して、必要分までトークンを格納する
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr 分割結果の配列（未使用要素は空文字）
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto constexpr split(char const (&str)[N]) noexcept {
  return split<IsDelimiter>(FrozenString{str});
}

/**
 * @brief 文字列を区切り判定関数で分割し、1回の呼び出しで数値配列へ変換する関数
 * split_count(...) を内部で呼び出して、必要分まで数値変換する
 *
 * @tparam Int 解析する数値型（デフォルト: int）
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr 分割・数値変換結果の配列（未使用要素は0）
 */
template <auto IsDelimiter = detail::is_whitespace, Numeric Int = int, size_t N>
  requires (std::predicate<decltype(IsDelimiter), char>
            && !std::same_as<std::remove_cv_t<Int>, bool>)
auto constexpr split_numbers(FrozenString<N> const& str) {
  using Result = std::remove_cv_t<Int>;
  auto res = std::array<Result, N>{};
  auto const tokens = split<IsDelimiter>(str);
  auto const token_count = split_count<IsDelimiter>(str);
  for (auto i = 0uz; i < token_count; ++i) {
    if (tokens[i].length == 0) {
      continue;
    }
    res[i] = detail::parse_int_token<Result>(tokens[i]);
  }
  return res;
}

/**
 * @brief 文字列を空白区切りで分割し、1回の呼び出しで指定数値型配列へ変換する関数
 *
 * @tparam Int 解析する数値型
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr 分割・数値変換結果の配列（未使用要素は0）
 */
template <Numeric Int, size_t N>
  requires (!std::same_as<std::remove_cv_t<Int>, bool>)
auto constexpr split_numbers(FrozenString<N> const& str) {
  return split_numbers<detail::is_whitespace, Int>(str);
}

/**
 * @brief 文字列リテラルを区切り判定関数で分割し、1回の呼び出しで数値配列へ変換する関数
 * split_count(...) を内部で呼び出して、必要分まで数値変換する
 *
 * @tparam Int 解析する数値型（デフォルト: int）
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr 分割・数値変換結果の配列（未使用要素は0）
 */
template <auto IsDelimiter = detail::is_whitespace, Numeric Int = int, size_t N>
  requires (std::predicate<decltype(IsDelimiter), char>
            && !std::same_as<std::remove_cv_t<Int>, bool>)
auto constexpr split_numbers(char const (&str)[N]) {
  return split_numbers<IsDelimiter, Int>(FrozenString{str});
}

/**
 * @brief 文字列リテラルを空白区切りで分割し、1回の呼び出しで指定数値型配列へ変換する関数
 *
 * @tparam Int 解析する数値型
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr 分割・数値変換結果の配列（未使用要素は0）
 */
template <Numeric Int, size_t N>
  requires (!std::same_as<std::remove_cv_t<Int>, bool>)
auto constexpr split_numbers(char const (&str)[N]) {
  return split_numbers<detail::is_whitespace, Int>(FrozenString{str});
}

/**
 * @brief `#RGB` / `#RRGGBB` 形式の色文字列を RGB タプルへ変換する関数
 *
 * @param str 対象文字列
 * @return auto constexpr `(r, g, b)` の順に並んだタプル
 */
auto constexpr parse_hex_rgb(std::string_view str) {
  if (str.empty() || str[0] != '#' || (str.size() != 4 && str.size() != 7)) {
    throw std::invalid_argument("parse_hex_rgb: expected #RGB or #RRGGBB");
  }

  if (str.size() == 4) {
    return std::tuple{
      detail::parse_hex_shorthand_byte(str[1]),
      detail::parse_hex_shorthand_byte(str[2]),
      detail::parse_hex_shorthand_byte(str[3])
    };
  }

  return std::tuple{
    detail::parse_hex_byte(str[1], str[2]),
    detail::parse_hex_byte(str[3], str[4]),
    detail::parse_hex_byte(str[5], str[6])
  };
}

/**
 * @brief 文字列リテラル版の `#RGB` / `#RRGGBB` パーサ
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr `(r, g, b)` の順に並んだタプル
 */
template <size_t N>
auto constexpr parse_hex_rgb(char const (&str)[N]) {
  return parse_hex_rgb(std::string_view{str, N - 1});
}

/**
 * @brief `#RGBA` / `#RRGGBBAA` 形式の色文字列を RGBA タプルへ変換する関数
 *
 * @param str 対象文字列
 * @return auto constexpr `(r, g, b, a)` の順に並んだタプル
 */
auto constexpr parse_hex_rgba(std::string_view str) {
  if (str.empty() || str[0] != '#' || (str.size() != 5 && str.size() != 9)) {
    throw std::invalid_argument("parse_hex_rgba: expected #RGBA or #RRGGBBAA");
  }

  if (str.size() == 5) {
    return std::tuple{
      detail::parse_hex_shorthand_byte(str[1]),
      detail::parse_hex_shorthand_byte(str[2]),
      detail::parse_hex_shorthand_byte(str[3]),
      detail::parse_hex_shorthand_byte(str[4])
    };
  }

  return std::tuple{
    detail::parse_hex_byte(str[1], str[2]),
    detail::parse_hex_byte(str[3], str[4]),
    detail::parse_hex_byte(str[5], str[6]),
    detail::parse_hex_byte(str[7], str[8])
  };
}

/**
 * @brief 文字列リテラル版の `#RGBA` / `#RRGGBBAA` パーサ
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr `(r, g, b, a)` の順に並んだタプル
 */
template <size_t N>
auto constexpr parse_hex_rgba(char const (&str)[N]) {
  return parse_hex_rgba(std::string_view{str, N - 1});
}

/**
 * @brief RGB タプルを BGR タプルへ並び替える関数
 *
 * @tparam R 赤チャネル型
 * @tparam G 緑チャネル型
 * @tparam B 青チャネル型
 * @param rgb `(r, g, b)` の順のタプル
 * @return auto constexpr `(b, g, r)` の順のタプル
 */
template <typename R, typename G, typename B>
auto constexpr to_bgr(std::tuple<R, G, B> const& rgb) {
  return std::tuple<B, G, R>{std::get<2>(rgb), std::get<1>(rgb), std::get<0>(rgb)};
}

/**
 * @brief RGBA タプルを BGRA タプルへ並び替える関数
 *
 * @tparam R 赤チャネル型
 * @tparam G 緑チャネル型
 * @tparam B 青チャネル型
 * @tparam A アルファチャネル型
 * @param rgba `(r, g, b, a)` の順のタプル
 * @return auto constexpr `(b, g, r, a)` の順のタプル
 */
template <typename R, typename G, typename B, typename A>
auto constexpr to_bgra(std::tuple<R, G, B, A> const& rgba) {
  return std::tuple<B, G, R, A>{std::get<2>(rgba), std::get<1>(rgba), std::get<0>(rgba), std::get<3>(rgba)};
}

/**
 * @brief RGBA タプルを ABGR タプルへ並び替える関数
 *
 * @tparam R 赤チャネル型
 * @tparam G 緑チャネル型
 * @tparam B 青チャネル型
 * @tparam A アルファチャネル型
 * @param rgba `(r, g, b, a)` の順のタプル
 * @return auto constexpr `(a, b, g, r)` の順のタプル
 */
template <typename R, typename G, typename B, typename A>
auto constexpr to_abgr(std::tuple<R, G, B, A> const& rgba) {
  return std::tuple<A, B, G, R>{std::get<3>(rgba), std::get<2>(rgba), std::get<1>(rgba), std::get<0>(rgba)};
}

/**
 * @brief 文字列の先頭を大文字に、残りを小文字に変換した静的文字列を生成する関数
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr 先頭大文字・残り小文字に変換された静的文字列
 */
template <size_t N>
auto constexpr capitalize(FrozenString<N> const& str) noexcept {
  auto res = tolower(str);
  if (res.length > 0) {
    auto const c = res.buffer[0];
    if (c >= 'a' && c <= 'z') {
      res.buffer[0] = static_cast<char>(c - ('a' - 'A'));
    }
  }
  return res;
}

/**
 * @brief 文字列リテラルの先頭を大文字に、残りを小文字に変換した静的文字列を生成する関数
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr 先頭大文字・残り小文字に変換された静的文字列
 */
template <size_t N>
auto constexpr capitalize(char const (&str)[N]) noexcept {
  return capitalize(FrozenString{str});
}

/**
 * @brief camelCase/PascalCase文字列をsnake_caseに変換した静的文字列を生成する関数
 * 大文字の前にアンダースコアを挿入し、すべての文字を小文字に変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr snake_caseに変換された静的文字列
 */
template <size_t N>
auto constexpr to_snake_case(FrozenString<N> const& str) noexcept {
  constexpr auto OUT_CAP = 2 * (N > 0 ? N - 1 : 0) + 1;
  auto res = FrozenString<OUT_CAP>{};
  auto offset = 0uz;
  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    if (c >= 'A' && c <= 'Z' && i > 0) {
      res.buffer[offset++] = '_';
    }
    res.buffer[offset++] = (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief camelCase/PascalCase文字列リテラルをsnake_caseに変換した静的文字列を生成する関数
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr snake_caseに変換された静的文字列
 */
template <size_t N>
auto constexpr to_snake_case(char const (&str)[N]) noexcept {
  return to_snake_case(FrozenString{str});
}

/**
 * @brief snake_case文字列をcamelCaseに変換した静的文字列を生成する関数
 * アンダースコアを除去し、アンダースコアに続く文字を大文字に変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr camelCaseに変換された静的文字列
 */
template <size_t N>
auto constexpr to_camel_case(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto next_upper = false;
  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    if (c == '_') {
      next_upper = true;
    } else if (next_upper) {
      res.buffer[offset++] = (c >= 'a' && c <= 'z') ? static_cast<char>(c - ('a' - 'A')) : c;
      next_upper = false;
    } else {
      res.buffer[offset++] = c;
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief snake_case文字列リテラルをcamelCaseに変換した静的文字列を生成する関数
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr camelCaseに変換された静的文字列
 */
template <size_t N>
auto constexpr to_camel_case(char const (&str)[N]) noexcept {
  return to_camel_case(FrozenString{str});
}

/**
 * @brief snake_case文字列をPascalCaseに変換した静的文字列を生成する関数
 * アンダースコアを除去し、アンダースコアに続く文字および先頭の文字を大文字に変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto constexpr PascalCaseに変換された静的文字列
 */
template <size_t N>
auto constexpr to_pascal_case(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto next_upper = true;
  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    if (c == '_') {
      next_upper = true;
    } else if (next_upper) {
      res.buffer[offset++] = (c >= 'a' && c <= 'z') ? static_cast<char>(c - ('a' - 'A')) : c;
      next_upper = false;
    } else {
      res.buffer[offset++] = c;
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief snake_case文字列リテラルをPascalCaseに変換した静的文字列を生成する関数
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto constexpr PascalCaseに変換された静的文字列
 */
template <size_t N>
auto constexpr to_pascal_case(char const (&str)[N]) noexcept {
  return to_pascal_case(FrozenString{str});
}

namespace literals {

/**
 * @brief 文字列リテラルを FrozenString に変換するリテラル演算子
 *
 * @tparam FS 固定長文字列
 * @return auto constexpr FrozenString に変換された文字列
 */
template <FixedString FS>
auto constexpr operator""_fs() noexcept {
  auto res = FrozenString<FS.sv().size() + 1>{};
  auto const s = FS.sv();
  for (auto i = 0uz; i < s.size(); ++i) {
    res.buffer[i] = s[i];
  }
  res.buffer[s.size()] = '\0';
  return res;
}

}

/**
 * @brief 文字列リテラルと FrozenString を結合する演算子
 * FrozenString + 文字列リテラルについては、FrozenString内で定義されている
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @tparam M FrozenString の長さ (終端文字'\0'を含む)
 * @param rhs 結合する FrozenString
 * @return auto constexpr 結合された FrozenString
 */
template <size_t N, size_t M>
auto constexpr operator+(char const (&lhs)[N], FrozenString<M> const& rhs) noexcept {
  auto res = FrozenString<N + M - 1>{};
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

namespace detail {

  /**
   * @brief 10進数整数を文字列に変換する関数
   *
   * @param v 変換する整数
   * @return auto constexpr 変換された文字列とその長さのペア
   */
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

  /**
   * @brief 16進数整数を文字列に変換する関数
   *
   * @param value 変換する整数
   * @return auto constexpr 変換された文字列とその長さのペア
   */
  auto constexpr to_hex_chars(long long value) noexcept {
    auto buffer = std::array<char, 17>{};
    auto v = static_cast<unsigned long long>(value);
    if (v == 0) {
      buffer[0] = '0';
      return std::pair{buffer, 1uz};
    }
    auto i = 0uz;
    auto constexpr digits = "0123456789abcdef";
    while (v > 0) {
      buffer[i++] = digits[v % 16]; v /= 16;
    }
    for (auto const j : std::views::iota(0uz, i / 2)) {
      std::swap(buffer[j], buffer[i - j - 1]);
    }
    return std::pair{buffer, i};
  }

  /**
   * @brief 2進数整数を文字列に変換する関数
   *
   * @param value 変換する整数
   * @return auto constexpr 変換された文字列とその長さのペア
   */
  auto constexpr to_bin_chars(long long value) noexcept {
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
   * @brief 8進数整数を文字列に変換する関数
   *
   * @param value 変換する整数
   * @return auto constexpr 変換された文字列とその長さのペア
   */
  auto constexpr to_oct_chars(long long value) noexcept {
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
   * @brief 浮動小数点数を文字列に変換する関数（簡易固定小数点）
   *
   * @param value 変換する浮動小数点数
   * @param precision 小数点以下の桁数
   * @return auto constexpr 変換された文字列とその長さのペア
   */
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

  /**
   * @brief 1要素を 0..255 の値として扱うための共通変換関数
   * - std::byte は std::to_integer<unsigned char> を使用
   * - それ以外は unsigned char へキャスト
   *
   * @tparam T 変換する要素の型
   * @param v 変換する要素
   * @return auto constexpr 変換された 0..255 の値
   */
  template <typename T>
  auto constexpr to_u8(T const v) noexcept {
    if constexpr (std::same_as<std::remove_cv_t<T>, std::byte>) {
      return std::to_integer<unsigned char>(v);
    } else {
      return static_cast<unsigned char>(v);
    }
  }

  /**
   * @brief ヌル終端ポインタを FrozenString<257> に変換する関数
   * - nullptr は空文字とする
   * - '\0' もしくは 256 文字で打ち切り
   *
   * @tparam Elem 変換する要素の型
   * @param arg 変換するヌル終端ポインタ
   * @return auto constexpr 変換された FrozenString<257>
   */
  template <typename Elem>
  auto constexpr freeze_from_ptr(Elem const* arg) noexcept {
    auto res = FrozenString<257>{};
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

  /**
   * @brief span を FrozenString<257> に変換する関数
   * - 先頭から 0 値までを文字列として扱う
   * - 0 がなくても最大 256 文字までコピー
   *
   * @tparam Elem 変換する要素の型
   * @tparam Extent span の長さ
   * @param arg 変換する span
   * @return auto constexpr 変換された FrozenString<257>
   */
  template <typename Elem, size_t Extent>
  auto constexpr freeze_from_span(std::span<Elem const, Extent> arg) noexcept {
    auto res = FrozenString<257>{};
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

  /**
   * @brief string_view を FrozenString<257> に変換する関数
   * - 終端は長さベース
   * - 最大 256 文字までコピー
   *
   * @param s 変換する string_view
   * @return auto constexpr 変換された FrozenString<257>
   */
  auto constexpr freeze_from_sv(std::string_view s) noexcept {
    auto res = FrozenString<257>{};
    auto const len = std::min(s.size(), res.buffer.size() - 1);
    for (auto i = 0uz; i < len; ++i) {
      res.buffer[i] = s[i];
    }
    res.buffer[len] = '\0';
    res.length = len;
    return res;
  }
}

/*===============================================================================*\
 * 各種データ型に対応したfreeze関数の定義
\*===============================================================================*/

/*-------------------------------------------------------------------------------*\
 * FrozenString自体の場合はそのまま返す
\*/
template <size_t N>
auto constexpr freeze(FrozenString<N> const& arg) noexcept {
  return arg;
}

/*-------------------------------------------------------------------------------*\
 * 文字列リテラル
\*/
template <size_t N>
auto constexpr freeze(char const (&arg)[N]) noexcept {
  return FrozenString<N>{arg};
}

/*-------------------------------------------------------------------------------*\
 * 各種C文字列ポインタ
\*/
auto constexpr freeze(char const* arg) noexcept {
  return detail::freeze_from_ptr(arg);
}
auto constexpr freeze(char* arg) noexcept {
  return freeze(static_cast<char const*>(arg));
}
auto constexpr freeze(signed char const* arg) noexcept {
  return detail::freeze_from_ptr(arg);
}
auto constexpr freeze(signed char* arg) noexcept {
  return freeze(static_cast<signed char const*>(arg));
}
auto constexpr freeze(unsigned char const* arg) noexcept {
  return detail::freeze_from_ptr(arg);
}
auto constexpr freeze(unsigned char* arg) noexcept {
  return freeze(static_cast<unsigned char const*>(arg));
}

/*-------------------------------------------------------------------------------*\
 * 各種char型のspan
\*/
template <size_t Extent>
auto constexpr freeze(std::span<char const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto constexpr freeze(std::span<char, Extent> arg) noexcept {
  return freeze(std::span<char const, Extent>{arg});
}
template <size_t Extent>
auto constexpr freeze(std::span<signed char const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto constexpr freeze(std::span<signed char, Extent> arg) noexcept {
  return freeze(std::span<signed char const, Extent>{arg});
}
template <size_t Extent>
auto constexpr freeze(std::span<unsigned char const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto constexpr freeze(std::span<unsigned char, Extent> arg) noexcept {
  return freeze(std::span<unsigned char const, Extent>{arg});
}
template <size_t Extent>
auto constexpr freeze(std::span<std::byte const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto constexpr freeze(std::span<std::byte, Extent> arg) noexcept {
  return freeze(std::span<std::byte const, Extent>{arg});
}

/*-------------------------------------------------------------------------------*\
 * 各種char型のarray
\*/
template <size_t N>
auto constexpr freeze(std::array<char, N> const& arg) noexcept {
  return freeze(std::span<char const, N>{arg});
}
template <size_t N>
auto constexpr freeze(std::array<signed char, N> const& arg) noexcept {
  return freeze(std::span<signed char const, N>{arg});
}
template <size_t N>
auto constexpr freeze(std::array<unsigned char, N> const& arg) noexcept {
  return freeze(std::span<unsigned char const, N>{arg});
}
template <size_t N>
auto constexpr freeze(std::array<std::byte, N> const& arg) noexcept {
  return freeze(std::span<std::byte const, N>{arg});
}

/*-------------------------------------------------------------------------------*\
 * 各種char型のvector
\*/
template <typename Alloc>
auto constexpr freeze(std::vector<char, Alloc>& arg) noexcept {
  return freeze(std::span<char>{arg.data(), arg.size()});
}
template <typename Alloc>
auto constexpr freeze(std::vector<char, Alloc> const& arg) noexcept {
  return freeze(std::span<char const>{arg.data(), arg.size()});
}
template <typename Alloc>
auto constexpr freeze(std::vector<signed char, Alloc>& arg) noexcept {
  return freeze(std::span<signed char>{arg.data(), arg.size()});
}
template <typename Alloc>
auto constexpr freeze(std::vector<signed char, Alloc> const& arg) noexcept {
  return freeze(std::span<signed char const>{arg.data(), arg.size()});
}
template <typename Alloc>
auto constexpr freeze(std::vector<unsigned char, Alloc>& arg) noexcept {
  return freeze(std::span<unsigned char>{arg.data(), arg.size()});
}
template <typename Alloc>
auto constexpr freeze(std::vector<unsigned char, Alloc> const& arg) noexcept {
  return freeze(std::span<unsigned char const>{arg.data(), arg.size()});
}
template <typename Alloc>
auto constexpr freeze(std::vector<std::byte, Alloc>& arg) noexcept {
  return freeze(std::span<std::byte>{arg.data(), arg.size()});
}
template <typename Alloc>
auto constexpr freeze(std::vector<std::byte, Alloc> const& arg) noexcept {
  return freeze(std::span<std::byte const>{arg.data(), arg.size()});
}

/*-------------------------------------------------------------------------------*\
 * nullptrは非対応とする
\*/
auto constexpr freeze(decltype(nullptr)) noexcept = delete;

template <char TrimChar = ' ', size_t N>
auto constexpr ltrim(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<true, false, TrimChar>(str);
}

template <char TrimChar = ' ', size_t N>
auto constexpr rtrim(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<false, true, TrimChar>(str);
}

template <char TrimChar = ' ', size_t N>
auto constexpr trim(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<true, true, TrimChar>(str);
}

template <char TrimChar = ' ', size_t N>
auto constexpr ltrim(char const (&str)[N]) noexcept {
  return ltrim<TrimChar>(FrozenString{str});
}

template <char TrimChar = ' ', size_t N>
auto constexpr rtrim(char const (&str)[N]) noexcept {
  return rtrim<TrimChar>(FrozenString{str});
}

template <char TrimChar = ' ', size_t N>
auto constexpr trim(char const (&str)[N]) noexcept {
  return trim<TrimChar>(FrozenString{str});
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
auto constexpr ltrim(Ptr&& str) noexcept {
  return ltrim<TrimChar>(freeze(str));
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
auto constexpr rtrim(Ptr&& str) noexcept {
  return rtrim<TrimChar>(freeze(str));
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
auto constexpr trim(Ptr&& str) noexcept {
  return trim<TrimChar>(freeze(str));
}

/*-------------------------------------------------------------------------------*\
 * 数値対応
\*/

/**
 * @brief Bin タグを受け取って整数を2進数表現の文字列に変換する
 *
 * @param arg 変換する整数を含む Bin タグ
 * @return auto constexpr 変換後の静的文字列
 */
auto constexpr freeze(Bin const& arg) noexcept {
  auto const p = detail::to_bin_chars(arg.value);
  auto res = FrozenString<65>{};
  for (auto i = 0uz; i < p.second; ++i) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

/**
 * @brief Oct タグを受け取って整数を8進数表現の文字列に変換する
 *
 * @param arg 変換する整数を含む Oct タグ
 * @return auto constexpr 変換後の静的文字列
 */
auto constexpr freeze(Oct const& arg) noexcept {
  auto const p = detail::to_oct_chars(arg.value);
  auto res = FrozenString<23>{};
  for (auto i = 0uz; i < p.second; ++i) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

/**
 * @brief Hex タグを受け取って整数を16進数表現の文字列に変換する
 *
 * @param arg 変換する整数を含む Hex タグ
 * @return auto constexpr 変換後の静的文字列
 */
auto constexpr freeze(Hex const& arg) noexcept {
  auto const p = detail::to_hex_chars(arg.value);
  auto res = FrozenString<17>{};
  for (auto i = 0uz; i < p.second; ++i) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

/**
 * @brief Precision タグを受け取って浮動小数点数を指定精度の文字列に変換する
 *
 * @param arg 変換する浮動小数点数を含む Precision タグ
 * @return auto constexpr 変換後の静的文字列
 */
auto constexpr freeze(Precision const& arg) noexcept {
  auto const p = detail::to_float_chars(arg.value, arg.precision);
  auto res = FrozenString<49>{};
  for (auto i = 0uz; i < p.second; ++i) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

/**
 * @brief 整数型を受け取って10進数表現の文字列に変換する
 *
 * @tparam T 整数型
 * @param arg 変換する整数
 * @return auto constexpr 変換後の静的文字列
 */
template <Integral T>
auto constexpr freeze(T const& arg) noexcept {
  auto const p = detail::to_dec_chars(static_cast<long long>(arg));
  auto res = FrozenString<21>{};
  for (auto i = 0uz; i < p.second; ++i) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

template <FloatingPoint T>
auto constexpr freeze(T const& arg) noexcept {
  return freeze(Precision(arg, 2)); // デフォルト精度2
}

/*-------------------------------------------------------------------------------*\
 * string_view 変換可能型
\*/
template <typename T>
  requires (std::is_convertible_v<T, std::string_view>
            && !Integral<std::remove_cvref_t<T>>
            && !FloatingPoint<std::remove_cvref_t<T>>
            && !std::same_as<std::remove_cvref_t<T>, Hex>
            && !std::same_as<std::remove_cvref_t<T>, Precision>)
auto constexpr freeze(T const& arg) noexcept {
  auto const s = std::string_view{arg};
  return detail::freeze_from_sv(s);
}

/*-------------------------------------------------------------------------------*\
 * 未対応型はコンパイルエラー
\*/
template <typename T>
auto constexpr freeze(T const&) noexcept = delete;

/**
 * @brief 引数で渡された値をすべて FrozenString に変換し結合する
 *
 * @tparam Args 可変引数の型
 * @param args 結合する引数
 * @return auto constexpr 結合後の静的文字列
 */
template <typename... Args>
auto constexpr concat(Args const&... args) noexcept {
  return (freeze(args) + ...);
}

}

#endif /* __FROZEN_CHARS_H__ */
