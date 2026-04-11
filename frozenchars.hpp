#ifndef __FROZEN_CHARS_H__
#define __FROZEN_CHARS_H__

#include <array>
#include <algorithm>
#include <concepts>
#include <cstddef>
#include <cstdint>
#if defined(__has_include)
#  if __has_include(<format>)
#    include <format>
#  endif
#endif
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <utility>
#include <vector>

namespace frozenchars {

/*===============================================================================*\
 * ユーティリティ
\*===============================================================================*/

template <size_t N>
struct FrozenString;

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
  size_t length{N > 0 ? N - 1 : 0};

  constexpr FixedString(char const (&str)[N]) noexcept
  : length{N > 0 ? N - 1 : 0} {
    for (auto i = 0uz; i < N; ++i) {
      data[i] = str[i];
    }
  }
  constexpr FixedString(FrozenString<N> const& str) noexcept
  : length{str.length} {
    for (auto i = 0uz; i < N; ++i) {
      data[i] = str.buffer[i];
    }
  }
  auto constexpr sv() const noexcept {
    return std::string_view{data, length};
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

  consteval FrozenString() noexcept = default;
  consteval FrozenString(char const (&str)[N]) noexcept
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
};

namespace detail {

struct pipe_adaptor_tag {};

template <typename T>
concept PipeAdaptor = std::derived_from<std::remove_cvref_t<T>, pipe_adaptor_tag>;

template <typename T>
struct is_frozen_string : std::false_type {};

template <size_t N>
struct is_frozen_string<FrozenString<N>> : std::true_type {};

template <typename T>
inline constexpr auto is_frozen_string_v = is_frozen_string<std::remove_cvref_t<T>>::value;

template <size_t N>
consteval auto find_substring(FrozenString<N> const& str,
                              std::string_view needle,
                              std::size_t pos = 0uz) noexcept -> std::size_t {
  if (needle.empty()) {
    return pos <= str.length ? pos : std::string_view::npos;
  }
  if (needle.size() > str.length || pos > str.length - needle.size()) {
    return std::string_view::npos;
  }

  auto const last = str.length - needle.size();
  for (auto i = pos; i <= last; ++i) {
    auto matched = true;
    for (auto j = 0uz; j < needle.size(); ++j) {
      if (str.buffer[i + j] != needle[j]) {
        matched = false;
        break;
      }
    }
    if (matched) {
      return i;
    }
  }
  return std::string_view::npos;
}

} // namespace detail

template <size_t N, detail::PipeAdaptor Adaptor>
consteval auto operator|(FrozenString<N> const& lhs, Adaptor const& rhs)
  noexcept(noexcept(rhs(lhs))) {
  return rhs(lhs);
}

template <size_t Width, char Fill, size_t N>
auto consteval pad_left(FrozenString<N> const& str) noexcept;

template <size_t Width, char Fill, size_t N>
auto consteval pad_left(char const (&str)[N]) noexcept;

/**
 * @brief 文字列を指定回数繰り返した静的文字列を生成する関数
 *
 * @tparam Count 繰り返し回数
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 繰り返す文字列
 * @return auto consteval 繰り返された静的文字列
 */
template <size_t Count, size_t N>
auto consteval repeat(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval 繰り返された静的文字列
 */
template <size_t Count, size_t N>
auto consteval repeat(char const (&str)[N]) noexcept {
  return repeat<Count>(FrozenString{str});
}

/**
 * @brief 文字列を指定幅で右寄せした静的文字列を生成する関数
 *
 * @tparam Width 右寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval 右寄せされた静的文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval right(FrozenString<N> const& str) noexcept {
  return pad_left<Width, Fill>(str);
}

/**
 * @brief 文字列リテラルを指定幅で右寄せした静的文字列を生成する関数
 *
 * @tparam Width 右寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto consteval 右寄せされた静的文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval right(char const (&str)[N]) noexcept {
  return pad_left<Width, Fill>(str);
}

/**
 * @brief 文字列を指定幅で中央寄せした静的文字列を生成する関数
 *
 * @tparam Width 中央寄せ後の幅
 * @tparam Fill 埋める文字（デフォルト: 半角スペース）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval 中央寄せされた静的文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval center(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval 中央寄せされた静的文字列
 */
template <size_t Width, char Fill = ' ', size_t N>
auto consteval center(char const (&str)[N]) noexcept {
  return center<Width, Fill>(FrozenString{str});
}

/**
 * @brief 文字列をすべて大文字に変換した静的文字列を生成する関数
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval 大文字に変換された静的文字列
 */
template <size_t N>
auto consteval toupper(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval 大文字に変換された静的文字列
 */
template <size_t N>
auto consteval toupper(char const (&str)[N]) noexcept {
  return toupper(FrozenString{str});
}

/**
 * @brief 文字列をすべて小文字に変換した静的文字列を生成する関数
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval 小文字に変換された静的文字列
 */
template <size_t N>
auto consteval tolower(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval 小文字に変換された静的文字列
 */
template <size_t N>
auto consteval tolower(char const (&str)[N]) noexcept {
  return tolower(FrozenString{str});
}

/**
 * @brief 文字列の部分文字列を生成する関数
 *
 * @tparam Pos 開始位置
 * @tparam Len 文字数。負の場合は Pos の左側から abs(Len) 文字
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval 部分文字列
 */
template <size_t Pos, std::ptrdiff_t Len, size_t N>
auto consteval substr(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval 部分文字列
 */
template <size_t Pos, std::ptrdiff_t Len, size_t N>
auto consteval substr(char const (&str)[N]) noexcept {
  return substr<Pos, Len>(FrozenString{str});
}

/**
 * @brief 文字列の部分文字列を生成する関数
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param pos 開始位置
 * @param len 文字数。負の場合は pos の左側から abs(len) 文字
 * @return auto consteval 部分文字列
 */
template <size_t N>
auto consteval substr(FrozenString<N> const& str, std::size_t pos, std::ptrdiff_t len) noexcept {
  auto res = FrozenString<N>{};
  auto const requested_len = len >= 0
    ? static_cast<size_t>(len)
    : static_cast<size_t>(-(len + 1)) + 1uz;
  auto const anchor = std::min(pos, str.length);
  auto start = anchor;
  auto actual_len = 0uz;

  if (len >= 0) {
    actual_len = anchor < str.length ? std::min(requested_len, str.length - anchor) : 0uz;
  } else {
    start = anchor > requested_len ? anchor - requested_len : 0uz;
    actual_len = anchor - start;
  }

  for (auto i = 0uz; i < actual_len; ++i) {
    res.buffer[i] = str.buffer[start + i];
  }
  res.buffer[actual_len] = '\0';
  res.length = actual_len;
  return res;
}

namespace detail {

/**
 * @brief ASCII 空白文字かどうかを判定する関数
 *
 * @param c 判定する文字
 * @return auto consteval 空白なら true
 */
auto constexpr is_whitespace(char c) noexcept {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

template <bool TrimLeft, bool TrimRight, char TrimChar, size_t N>
auto consteval trim_copy(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval 16進数字なら true
 */
auto consteval is_hex_digit(char c) noexcept {
  return (c >= '0' && c <= '9')
    || (c >= 'a' && c <= 'f')
    || (c >= 'A' && c <= 'F');
}

/**
 * @brief 16進数字1文字を 0..15 に変換する関数
 *
 * @param c 変換する16進数字
 * @return auto consteval 変換結果
 */
auto consteval hex_digit_to_value(char c) {
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
 * @return auto consteval 変換結果
 */
auto consteval parse_hex_byte(char hi, char lo) {
  if (!is_hex_digit(hi) || !is_hex_digit(lo)) {
    throw std::invalid_argument("parse_hex_color: invalid hex digit");
  }
  return static_cast<std::uint8_t>((hex_digit_to_value(hi) << 4u) | hex_digit_to_value(lo));
}

/**
 * @brief 16進数字1文字を nibble 複製して 1byte に変換する関数
 *
 * @param c 変換する16進数字
 * @return auto consteval 変換結果
 */
auto consteval parse_hex_shorthand_byte(char c) {
  auto const value = hex_digit_to_value(c);
  return static_cast<std::uint8_t>((value << 4u) | value);
}

/**
 * @brief 区切り判定関数でトークン数を数える関数
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval トークン数
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
 * @brief 整数字句を指定整数型に変換する関数
 *
 * @tparam Int 変換先の整数型
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param token 変換するトークン
 * @return auto consteval 変換結果
 */
template <Numeric Number = int, size_t N>
  requires (!std::same_as<std::remove_cv_t<Number>, bool>)
auto consteval parse_int_token(FrozenString<N> const& token) {
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

} // namespace detail

/**
 * @brief 文字列を区切り判定関数で分割したときのトークン数を返す関数
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval トークン数
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split_count(FrozenString<N> const& str) noexcept {
  return detail::split_count_impl<IsDelimiter>(str);
}

/**
 * @brief 文字列リテラルを区切り判定関数で分割したときのトークン数を返す関数
 *
 * @tparam IsDelimiter 区切り文字判定関数（デフォルト: 空白判定）
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto consteval トークン数
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split_count(char const (&str)[N]) noexcept {
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
 * @return auto consteval 分割結果の配列
 */
template <size_t Count, auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval 分割結果の配列（未使用要素は空文字）
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval 分割結果の配列（未使用要素は空文字）
 */
template <auto IsDelimiter = detail::is_whitespace, size_t N>
  requires std::predicate<decltype(IsDelimiter), char>
auto consteval split(char const (&str)[N]) noexcept {
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
 * @return auto consteval 分割・数値変換結果の配列（未使用要素は0）
 */
template <auto IsDelimiter = detail::is_whitespace, Numeric Int = int, size_t N>
  requires (std::predicate<decltype(IsDelimiter), char>
            && !std::same_as<std::remove_cv_t<Int>, bool>)
auto consteval split_numbers(FrozenString<N> const& str) {
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
 * @return auto consteval 分割・数値変換結果の配列（未使用要素は0）
 */
template <Numeric Int, size_t N>
  requires (!std::same_as<std::remove_cv_t<Int>, bool>)
auto consteval split_numbers(FrozenString<N> const& str) {
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
 * @return auto consteval 分割・数値変換結果の配列（未使用要素は0）
 */
template <auto IsDelimiter = detail::is_whitespace, Numeric Int = int, size_t N>
  requires (std::predicate<decltype(IsDelimiter), char>
            && !std::same_as<std::remove_cv_t<Int>, bool>)
auto consteval split_numbers(char const (&str)[N]) {
  return split_numbers<IsDelimiter, Int>(FrozenString{str});
}

/**
 * @brief 文字列リテラルを空白区切りで分割し、1回の呼び出しで指定数値型配列へ変換する関数
 *
 * @tparam Int 解析する数値型
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto consteval 分割・数値変換結果ের 配列（未使用要素は0）
 */
template <Numeric Int, size_t N>
  requires (!std::same_as<std::remove_cv_t<Int>, bool>)
auto consteval split_numbers(char const (&str)[N]) {
  return split_numbers<detail::is_whitespace, Int>(FrozenString{str});
}

/**
 * @brief `#RGB` / `#RRGGBB` 形式の色文字列を RGB タプルへ変換する関数
 *
 * @param str 対象文字列
 * @return auto consteval `(r, g, b)` の順に並んだタプル
 */
auto consteval parse_hex_rgb(std::string_view str) {
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
 * @return auto consteval `(r, g, b)` の順に並んだタプル
 */
template <size_t N>
auto consteval parse_hex_rgb(char const (&str)[N]) {
  return parse_hex_rgb(std::string_view{str, N - 1});
}

/**
 * @brief `#RGBA` / `#RRGGBBAA` 形式の色文字列を RGBA タプルへ変換する関数
 *
 * @param str 対象文字列
 * @return auto consteval `(r, g, b, a)` の順に並んだタプル
 */
auto consteval parse_hex_rgba(std::string_view str) {
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
 * @return auto consteval `(r, g, b, a)` の順に並んだタプル
 */
template <size_t N>
auto consteval parse_hex_rgba(char const (&str)[N]) {
  return parse_hex_rgba(std::string_view{str, N - 1});
}

/**
 * @brief RGB タプルを BGR タプルへ並び替える関数
 *
 * @tparam R 赤チャネル型
 * @tparam G 緑チャネル型
 * @tparam B 青チャネル型
 * @param rgb `(r, g, b)` の順のタプル
 * @return auto consteval `(b, g, r)` の順のタプル
 */
template <typename R, typename G, typename B>
auto consteval to_bgr(std::tuple<R, G, B> const& rgb) {
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
 * @return auto consteval `(b, g, r, a)` の順のタプル
 */
template <typename R, typename G, typename B, typename A>
auto consteval to_bgra(std::tuple<R, G, B, A> const& rgba) {
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
 * @return auto consteval `(a, b, g, r)` の順のタプル
 */
template <typename R, typename G, typename B, typename A>
auto consteval to_abgr(std::tuple<R, G, B, A> const& rgba) {
  return std::tuple<A, B, G, R>{std::get<3>(rgba), std::get<2>(rgba), std::get<1>(rgba), std::get<0>(rgba)};
}

/**
 * @brief 文字列の先頭を大文字に、残りを小文字に変換した静的文字列を生成する関数
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval 先頭大文字・残り小文字に変換された静的文字列
 */
template <size_t N>
auto consteval capitalize(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval 先頭大文字・残り小文字に変換された静的文字列
 */
template <size_t N>
auto consteval capitalize(char const (&str)[N]) noexcept {
  return capitalize(FrozenString{str});
}

/**
 * @brief camelCase/PascalCase文字列をsnake_caseに変換した静的文字列を生成する関数
 * 大文字の前にアンダースコアを挿入し、すべての文字を小文字に変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval snake_caseに変換された静的文字列
 */
template <size_t N>
auto consteval to_snake_case(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval snake_caseに変換された静的文字列
 */
template <size_t N>
auto consteval to_snake_case(char const (&str)[N]) noexcept {
  return to_snake_case(FrozenString{str});
}

/**
 * @brief snake_case文字列をcamelCaseに変換した静的文字列を生成する関数
 * アンダースコアを除去し、アンダースコアに続く文字を大文字に変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval camelCaseに変換された静的文字列
 */
template <size_t N>
auto consteval to_camel_case(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval camelCaseに変換された静的文字列
 */
template <size_t N>
auto consteval to_camel_case(char const (&str)[N]) noexcept {
  return to_camel_case(FrozenString{str});
}

/**
 * @brief snake_case文字列をPascalCaseに変換した静的文字列を生成する関数
 * アンダースコアを除去し、アンダースコアに続く文字および先頭の文字を大文字に変換する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval PascalCaseに変換された静的文字列
 */
template <size_t N>
auto consteval to_pascal_case(FrozenString<N> const& str) noexcept {
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
 * @return auto consteval PascalCaseに変換された静的文字列
 */
template <size_t N>
auto consteval to_pascal_case(char const (&str)[N]) noexcept {
  return to_pascal_case(FrozenString{str});
}

/**
 * @brief 各行の先頭の空白を指定個数分削除する静的文字列を生成する関数
 * 空白がなくなった行についてはそれ以上削除しない
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param n 削除する空白の数
 * @return auto consteval 変換された静的文字列
 */
template <size_t N>
auto consteval remove_leading_spaces(FrozenString<N> const& str, size_t n) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  while (i < str.length) {
    auto spaces = 0uz;
    while (i < str.length && str.buffer[i] == ' ' && spaces < n) {
      ++i;
      ++spaces;
    }
    while (i < str.length && str.buffer[i] != '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
    if (i < str.length && str.buffer[i] == '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval remove_leading_spaces(char const (&str)[N], size_t n) noexcept {
  return remove_leading_spaces(FrozenString{str}, n);
}

/**
 * @brief 指定されたコメント開始文字列で始まる行を削除する静的文字列を生成する関数
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param comment_seq コメント開始文字列 (デフォルト: "#")
 * @return auto consteval 変換された静的文字列
 */
template <size_t N>
auto consteval remove_comment_lines(FrozenString<N> const& str, std::string_view comment_seq = "#") noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto const sv = str.sv();

  while (i < str.length) {
    auto const line_start = i;
    while (i < str.length && str.buffer[i] != '\n') {
      ++i;
    }
    auto const line_end = i;
    auto const line = sv.substr(line_start, line_end - line_start);

    if (!line.starts_with(comment_seq)) {
      for (auto j = line_start; j < line_end; ++j) {
        res.buffer[offset++] = str.buffer[j];
      }
      if (i < str.length && str.buffer[i] == '\n') {
        res.buffer[offset++] = str.buffer[i];
      }
    }
    if (i < str.length && str.buffer[i] == '\n') {
      ++i;
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval remove_comment_lines(char const (&str)[N], std::string_view comment_seq = "#") noexcept {
  return remove_comment_lines(FrozenString{str}, comment_seq);
}

/**
 * @brief 指定された文字列以降行末までを削除した静的文字列を生成する関数
 * 指定された文字列直前に空白文字が連続している場合はそれも削除します。
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @param comment_seq コメント開始文字列 (デフォルト: "#")
 * @return auto consteval 変換された静的文字列
 */
template <size_t N>
auto consteval remove_comments(FrozenString<N> const& str, std::string_view comment_seq = "#") noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto const sv = str.sv();

  while (i < str.length) {
    auto const line_start = i;
    while (i < str.length && str.buffer[i] != '\n') {
      ++i;
    }
    auto const line_end = i;
    auto const line = sv.substr(line_start, line_end - line_start);

    auto const comment_pos = line.find(comment_seq);
    if (comment_pos == std::string_view::npos) {
      // コメントなし
      for (auto j = line_start; j < line_end; ++j) {
        res.buffer[offset++] = str.buffer[j];
      }
    } else {
      // コメントあり。直前の空白も削除
      auto last_non_space = comment_pos;
      while (last_non_space > 0 && detail::is_whitespace(line[last_non_space - 1])) {
        --last_non_space;
      }
      for (auto j = 0uz; j < last_non_space; ++j) {
        res.buffer[offset++] = line[j];
      }
    }

    if (i < str.length && str.buffer[i] == '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval remove_comments(char const (&str)[N], std::string_view comment_seq = "#") noexcept {
  return remove_comments(FrozenString{str}, comment_seq);
}

/**
 * @brief すべての行を結合する静的文字列を生成する関数
 * 行を結合した際に、行末と行頭どちらにもスペースがない場合にはスペースを入れる
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval 結合された静的文字列
 */
template <size_t N>
auto consteval join_lines(FrozenString<N> const& str) noexcept {
  constexpr auto OUT_CAP = (N > 0 ? N - 1 : 0) * 2 + 1;
  auto res = FrozenString<OUT_CAP>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto first_line = true;

  while (i < str.length) {
    if (!first_line) {
      auto needs_space = true;
      if (offset > 0 && (res.buffer[offset - 1] == ' ' || res.buffer[offset - 1] == '\t')) {
        needs_space = false;
      }
      if (i < str.length && (str.buffer[i] == ' ' || str.buffer[i] == '\t')) {
        needs_space = false;
      }
      if (needs_space) {
        res.buffer[offset++] = ' ';
      }
    }

    while (i < str.length && str.buffer[i] != '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
    if (i < str.length && str.buffer[i] == '\n') {
      ++i;
    }
    first_line = false;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval join_lines(char const (&str)[N]) noexcept {
  return join_lines(FrozenString{str});
}

/**
 * @brief すべての行末の空白を削除する静的文字列を生成する関数
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval 変換された静的文字列
 */
template <size_t N>
auto consteval trim_trailing_spaces(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  while (i < str.length) {
    auto line_start = i;
    while (i < str.length && str.buffer[i] != '\n') {
      ++i;
    }
    auto line_end = i;
    auto last_non_space = line_end;
    while (last_non_space > line_start && detail::is_whitespace(str.buffer[last_non_space - 1])) {
      --last_non_space;
    }
    for (auto j = line_start; j < last_non_space; ++j) {
      res.buffer[offset++] = str.buffer[j];
    }
    if (i < str.length && str.buffer[i] == '\n') {
      res.buffer[offset++] = str.buffer[i++];
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval trim_trailing_spaces(char const (&str)[N]) noexcept {
  return trim_trailing_spaces(FrozenString{str});
}

/**
 * @brief すべての空行を削除する静的文字列を生成する関数
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto consteval 変換された静的文字列
 */
template <size_t N>
auto consteval remove_empty_lines(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  while (i < str.length) {
    auto line_start = i;
    while (i < str.length && str.buffer[i] != '\n') {
      ++i;
    }
    if (i > line_start) {
      for (auto j = line_start; j < i; ++j) {
        res.buffer[offset++] = str.buffer[j];
      }
      if (i < str.length && str.buffer[i] == '\n') {
        res.buffer[offset++] = str.buffer[i++];
      }
    } else {
      if (i < str.length && str.buffer[i] == '\n') {
        ++i;
      }
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

template <size_t N>
auto consteval remove_empty_lines(char const (&str)[N]) noexcept {
  return remove_empty_lines(FrozenString{str});
}

namespace literals {

/**
 * @brief 文字列リテラルを FrozenString に変換するリテラル演算子
 *
 * @tparam FS 固定長文字列
 * @return auto consteval FrozenString に変換された文字列
 */
template <FixedString FS>
auto consteval operator""_fs() noexcept {
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
 * @return auto consteval 結合された FrozenString
 */
template <size_t N, size_t M>
auto consteval operator+(char const (&lhs)[N], FrozenString<M> const& rhs) noexcept {
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
 * @return auto consteval 変換された文字列とその長さのペア
 */
auto consteval to_dec_chars(long long v) noexcept {
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
 * @return auto consteval 変換された文字列とその長さのペア
 */
auto consteval to_hex_chars(long long value) noexcept {
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
 * @return auto consteval 変換された文字列とその長さのペア
 */
auto consteval to_bin_chars(long long value) noexcept {
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
 * @return auto consteval 変換された文字列とその長さのペア
 */
auto consteval to_oct_chars(long long value) noexcept {
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
 * @return auto consteval 変換された文字列とその長さのペア
 */
auto consteval to_float_chars(double value, int precision) noexcept {
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
 * @return auto consteval 変換された 0..255 の値
 */
template <typename T>
auto consteval to_u8(T const v) noexcept {
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
 * @return auto consteval 変換された FrozenString<257>
 */
template <typename Elem>
auto consteval freeze_from_ptr(Elem const* arg) noexcept {
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
 * @return auto consteval 変換された FrozenString<257>
 */
template <typename Elem, size_t Extent>
auto consteval freeze_from_span(std::span<Elem const, Extent> arg) noexcept {
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
 * @return auto consteval 変換された FrozenString<257>
 */
auto consteval freeze_from_sv(std::string_view s) noexcept {
  auto res = FrozenString<257>{};
  auto const len = std::min(s.size(), res.buffer.size() - 1);
  for (auto i = 0uz; i < len; ++i) {
    res.buffer[i] = s[i];
  }
  res.buffer[len] = '\0';
  res.length = len;
  return res;
}

} // namespace detail

/*===============================================================================*\
 * 各種データ型に対応したfreeze関数の定義
\*===============================================================================*/

/*-------------------------------------------------------------------------------*\
 * FrozenString自体の場合はそのまま返す
\*/
template <size_t N>
auto consteval freeze(FrozenString<N> const& arg) noexcept {
  return arg;
}

/*-------------------------------------------------------------------------------*\
 * 文字列リテラル
\*/
template <size_t N>
auto consteval freeze(char const (&arg)[N]) noexcept {
  return FrozenString<N>{arg};
}

/*-------------------------------------------------------------------------------*\
 * 各種C文字列ポインタ
\*/
auto consteval freeze(char const* arg) noexcept {
  return detail::freeze_from_ptr(arg);
}
auto consteval freeze(char* arg) noexcept {
  return freeze(static_cast<char const*>(arg));
}
auto consteval freeze(signed char const* arg) noexcept {
  return detail::freeze_from_ptr(arg);
}
auto consteval freeze(signed char* arg) noexcept {
  return freeze(static_cast<signed char const*>(arg));
}
auto consteval freeze(unsigned char const* arg) noexcept {
  return detail::freeze_from_ptr(arg);
}
auto consteval freeze(unsigned char* arg) noexcept {
  return freeze(static_cast<unsigned char const*>(arg));
}

/*-------------------------------------------------------------------------------*\
 * 各種char型のspan
\*/
template <size_t Extent>
auto consteval freeze(std::span<char const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto consteval freeze(std::span<char, Extent> arg) noexcept {
  return freeze(std::span<char const, Extent>{arg});
}
template <size_t Extent>
auto consteval freeze(std::span<signed char const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto consteval freeze(std::span<signed char, Extent> arg) noexcept {
  return freeze(std::span<signed char const, Extent>{arg});
}
template <size_t Extent>
auto consteval freeze(std::span<unsigned char const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto consteval freeze(std::span<unsigned char, Extent> arg) noexcept {
  return freeze(std::span<unsigned char const, Extent>{arg});
}
template <size_t Extent>
auto consteval freeze(std::span<std::byte const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto consteval freeze(std::span<std::byte, Extent> arg) noexcept {
  return freeze(std::span<std::byte const, Extent>{arg});
}

/*-------------------------------------------------------------------------------*\
 * 各種char型のarray
\*/
template <size_t N>
auto consteval freeze(std::array<char, N> const& arg) noexcept {
  return freeze(std::span<char const, N>{arg});
}
template <size_t N>
auto consteval freeze(std::array<signed char, N> const& arg) noexcept {
  return freeze(std::span<signed char const, N>{arg});
}
template <size_t N>
auto consteval freeze(std::array<unsigned char, N> const& arg) noexcept {
  return freeze(std::span<unsigned char const, N>{arg});
}
template <size_t N>
auto consteval freeze(std::array<std::byte, N> const& arg) noexcept {
  return freeze(std::span<std::byte const, N>{arg});
}

/*-------------------------------------------------------------------------------*\
 * 各種char型のvector
\*/
template <typename Alloc>
auto consteval freeze(std::vector<char, Alloc>& arg) noexcept {
  return freeze(std::span<char>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<char, Alloc> const& arg) noexcept {
  return freeze(std::span<char const>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<signed char, Alloc>& arg) noexcept {
  return freeze(std::span<signed char>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<signed char, Alloc> const& arg) noexcept {
  return freeze(std::span<signed char const>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<unsigned char, Alloc>& arg) noexcept {
  return freeze(std::span<unsigned char>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<unsigned char, Alloc> const& arg) noexcept {
  return freeze(std::span<unsigned char const>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<std::byte, Alloc>& arg) noexcept {
  return freeze(std::span<std::byte>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<std::byte, Alloc> const& arg) noexcept {
  return freeze(std::span<std::byte const>{arg.data(), arg.size()});
}

/*-------------------------------------------------------------------------------*\
 * nullptrは非対応とする
\*/
auto consteval freeze(decltype(nullptr)) noexcept = delete;

template <char TrimChar = ' ', size_t N>
auto consteval ltrim(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<true, false, TrimChar>(str);
}

template <char TrimChar = ' ', size_t N>
auto consteval rtrim(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<false, true, TrimChar>(str);
}

template <char TrimChar = ' ', size_t N>
auto consteval trim(FrozenString<N> const& str) noexcept {
  return detail::trim_copy<true, true, TrimChar>(str);
}

template <char TrimChar = ' ', size_t N>
auto consteval ltrim(char const (&str)[N]) noexcept {
  return ltrim<TrimChar>(FrozenString{str});
}

template <char TrimChar = ' ', size_t N>
auto consteval rtrim(char const (&str)[N]) noexcept {
  return rtrim<TrimChar>(FrozenString{str});
}

template <char TrimChar = ' ', size_t N>
auto consteval trim(char const (&str)[N]) noexcept {
  return trim<TrimChar>(FrozenString{str});
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
auto consteval ltrim(Ptr&& str) noexcept {
  return ltrim<TrimChar>(freeze(str));
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
auto consteval rtrim(Ptr&& str) noexcept {
  return rtrim<TrimChar>(freeze(str));
}

template <char TrimChar = ' ', typename Ptr>
  requires (std::same_as<std::remove_cvref_t<Ptr>, char const*>
            || std::same_as<std::remove_cvref_t<Ptr>, char*>)
auto consteval trim(Ptr&& str) noexcept {
  return trim<TrimChar>(freeze(str));
}

// join - zero args: returns empty string
template <FixedString Delim>
auto consteval join() noexcept {
  return FrozenString<1>{};
}

// join - homogeneous std::array of FrozenStrings
template <FixedString Delim, size_t ElemN, size_t Count>
auto consteval join(std::array<FrozenString<ElemN>, Count> const& arr) noexcept {
  if constexpr (Count == 0) {
    return FrozenString<1>{};
  } else {
    auto constexpr DELIM_LEN = Delim.sv().size();
    auto constexpr MAX_ELEM = ElemN > 0 ? ElemN - 1 : 0;
    auto constexpr OUT_CAP = Count * MAX_ELEM + (Count - 1) * DELIM_LEN + 1;
    auto res = FrozenString<OUT_CAP>{};
    auto offset = 0uz;
    for (auto i = 0uz; i < Count; ++i) {
      if (i > 0) {
        for (auto const c : Delim.sv()) {
          res.buffer[offset++] = c;
        }
      }
      for (auto j = 0uz; j < arr[i].length; ++j) {
        res.buffer[offset++] = arr[i].buffer[j];
      }
    }
    res.buffer[offset] = '\0';
    res.length = offset;
    return res;
  }
}

// join - variadic heterogeneous FrozenStrings
template <FixedString Delim, size_t N0, size_t... Ns>
auto consteval join(FrozenString<N0> const& first, FrozenString<Ns> const&... rest) noexcept {
  auto constexpr DELIM_LEN = Delim.sv().size();
  auto constexpr COUNT = 1uz + sizeof...(Ns);
  auto constexpr MAX_ELEMS = (N0 > 0 ? N0 - 1 : 0) + (0uz + ... + (Ns > 0 ? Ns - 1 : 0));
  auto constexpr DELIM_TOTAL = COUNT > 1 ? (COUNT - 1) * DELIM_LEN : 0uz;
  auto constexpr OUT_CAP = MAX_ELEMS + DELIM_TOTAL + 1;

  auto res = FrozenString<OUT_CAP>{};
  auto offset = 0uz;

  for (auto i = 0uz; i < first.length; ++i) {
    res.buffer[offset++] = first.buffer[i];
  }
  ([&](auto const& elem) {
    for (auto const c : Delim.sv()) {
      res.buffer[offset++] = c;
    }
    for (auto i = 0uz; i < elem.length; ++i) {
      res.buffer[offset++] = elem.buffer[i];
    }
  }(rest), ...);

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

// pad_left - FrozenString (add Fill chars on left; delegates to right<>)
template <size_t Width, char Fill = ' ', size_t N>
auto consteval pad_left(FrozenString<N> const& str) noexcept {
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

// pad_left - string literal
template <size_t Width, char Fill = ' ', size_t N>
auto consteval pad_left(char const (&str)[N]) noexcept {
  return pad_left<Width, Fill>(FrozenString{str});
}

// pad_left - Integral (default Fill = '0')
// Defined after freeze(Integral) to ensure visibility
// (see below, near the end of the namespace)

// pad_right - FrozenString (add Fill chars on right)
template <size_t Width, char Fill = ' ', size_t N>
auto consteval pad_right(FrozenString<N> const& str) noexcept {
  auto constexpr OUT_CAP = N > (Width + 1) ? N : (Width + 1);
  auto res = FrozenString<OUT_CAP>{};
  auto const out_len = std::max(str.length, Width);
  auto const right_pad = out_len - str.length;
  for (auto i = 0uz; i < str.length; ++i) {
    res.buffer[i] = str.buffer[i];
  }
  for (auto i = 0uz; i < right_pad; ++i) {
    res.buffer[str.length + i] = Fill;
  }
  res.buffer[out_len] = '\0';
  res.length = out_len;
  return res;
}

// pad_right - string literal
template <size_t Width, char Fill = ' ', size_t N>
auto consteval pad_right(char const (&str)[N]) noexcept {
  return pad_right<Width, Fill>(FrozenString{str});
}

// pad_right - Integral (default Fill = '0')
// Defined after freeze(Integral) to ensure visibility
// (see below, near the end of the namespace)

// replace - first occurrence of From with To
template <FixedString From, FixedString To, size_t N>
auto consteval replace(FrozenString<N> const& str) noexcept {
  static_assert(From.sv().size() > 0, "replace: From must not be empty");
  auto constexpr FROM_LEN = From.sv().size();
  auto constexpr TO_LEN = To.sv().size();
  auto constexpr EXTRA = TO_LEN > FROM_LEN ? TO_LEN - FROM_LEN : 0uz;
  auto constexpr OUT_CAP = N + EXTRA;

  auto res = FrozenString<OUT_CAP>{};
  auto const from_sv = From.sv();
  auto const to_sv = To.sv();
  auto const match_pos = detail::find_substring(str, from_sv);
  if (match_pos == std::string_view::npos) {
    for (auto i = 0uz; i < str.length; ++i) {
      res.buffer[i] = str.buffer[i];
    }
    res.buffer[str.length] = '\0';
    res.length = str.length;
    return res;
  }
  auto offset = 0uz;
  for (auto i = 0uz; i < match_pos; ++i) {
    res.buffer[offset++] = str.buffer[i];
  }
  for (auto const c : to_sv) {
    res.buffer[offset++] = c;
  }
  auto after = match_pos + FROM_LEN;
  while (after < str.length) {
    res.buffer[offset++] = str.buffer[after++];
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

// replace - string literal
template <FixedString From, FixedString To, size_t N>
auto consteval replace(char const (&str)[N]) noexcept {
  return replace<From, To>(FrozenString{str});
}

// replace_all - all non-overlapping occurrences of From with To (left-to-right)
template <FixedString From, FixedString To, size_t N>
auto consteval replace_all(FrozenString<N> const& str) noexcept {
  static_assert(From.sv().size() > 0, "replace_all: From must not be empty");
  auto constexpr FROM_LEN = From.sv().size();
  auto constexpr TO_LEN = To.sv().size();
  auto constexpr MAX_STR_LEN = N > 0 ? N - 1 : 0;
  auto constexpr MAX_MATCHES = MAX_STR_LEN / FROM_LEN;
  auto constexpr EXTRA = TO_LEN > FROM_LEN ? (TO_LEN - FROM_LEN) * MAX_MATCHES : 0uz;
  auto constexpr OUT_CAP = N + EXTRA;

  auto res = FrozenString<OUT_CAP>{};
  auto const from_sv = From.sv();
  auto const to_sv = To.sv();
  auto offset = 0uz;
  auto pos = 0uz;
  while (pos < str.length) {
    auto const found = detail::find_substring(str, from_sv, pos);
    if (found == std::string_view::npos) {
      while (pos < str.length) {
        res.buffer[offset++] = str.buffer[pos++];
      }
      break;
    }
    while (pos < found) {
      res.buffer[offset++] = str.buffer[pos++];
    }
    for (auto const c : to_sv) {
      res.buffer[offset++] = c;
    }
    pos = found + FROM_LEN;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

// replace_all - string literal
template <FixedString From, FixedString To, size_t N>
auto consteval replace_all(char const (&str)[N]) noexcept {
  return replace_all<From, To>(FrozenString{str});
}

namespace ops {

struct trim_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::trim(str);
  }
};

struct ltrim_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::ltrim(str);
  }
};

struct rtrim_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::rtrim(str);
  }
};

struct toupper_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::toupper(str);
  }
};

struct tolower_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::tolower(str);
  }
};

struct substr_adaptor : detail::pipe_adaptor_tag {
  std::size_t pos;
  std::ptrdiff_t len;

  constexpr substr_adaptor(std::size_t p, std::ptrdiff_t l) noexcept
  : pos(p), len(l)
  {}

  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::substr(str, pos, len);
  }
};

struct capitalize_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::capitalize(str);
  }
};

struct to_snake_case_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_snake_case(str);
  }
};

struct to_camel_case_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_camel_case(str);
  }
};

struct to_pascal_case_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_pascal_case(str);
  }
};

struct remove_leading_spaces_adaptor : detail::pipe_adaptor_tag {
  size_t n;
  constexpr remove_leading_spaces_adaptor(size_t count) noexcept : n(count) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_leading_spaces(str, n);
  }
};

struct remove_comment_lines_adaptor : detail::pipe_adaptor_tag {
  std::string_view comment_seq;
  constexpr remove_comment_lines_adaptor(std::string_view seq = "#") noexcept : comment_seq(seq) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_comment_lines(str, comment_seq);
  }
};

struct remove_comments_adaptor : detail::pipe_adaptor_tag {
  std::string_view comment_seq;
  constexpr remove_comments_adaptor(std::string_view seq = "#") noexcept : comment_seq(seq) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_comments(str, comment_seq);
  }
};

struct join_lines_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::join_lines(str);
  }
};

struct trim_trailing_spaces_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::trim_trailing_spaces(str);
  }
};

struct remove_empty_lines_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_empty_lines(str);
  }
};

inline constexpr trim_adaptor trim{};
inline constexpr ltrim_adaptor ltrim{};
inline constexpr rtrim_adaptor rtrim{};
inline constexpr toupper_adaptor toupper{};
inline constexpr tolower_adaptor tolower{};
inline constexpr capitalize_adaptor capitalize{};
inline constexpr to_snake_case_adaptor to_snake_case{};
inline constexpr to_camel_case_adaptor to_camel_case{};
inline constexpr to_pascal_case_adaptor to_pascal_case{};
inline constexpr join_lines_adaptor join_lines{};
inline constexpr trim_trailing_spaces_adaptor trim_trailing_spaces{};
inline constexpr remove_empty_lines_adaptor remove_empty_lines{};

consteval auto substr(std::size_t pos, std::ptrdiff_t len) noexcept {
  return substr_adaptor{pos, len};
}

consteval auto remove_leading_spaces(size_t n) noexcept {
  return remove_leading_spaces_adaptor{n};
}

consteval auto remove_comment_lines(std::string_view comment_seq = "#") noexcept {
  return remove_comment_lines_adaptor{comment_seq};
}

consteval auto remove_comments(std::string_view comment_seq = "#") noexcept {
  return remove_comments_adaptor{comment_seq};
}

// join adaptor: works on std::array<FrozenString<ElemN>, Count>
template <FixedString Delim>
struct join_adaptor : detail::pipe_adaptor_tag {
  template <size_t ElemN, size_t Count>
  consteval auto operator()(std::array<FrozenString<ElemN>, Count> const& arr) const noexcept {
    return frozenchars::join<Delim>(arr);
  }
};

template <FixedString Delim>
inline constexpr join_adaptor<Delim> join{};

template <size_t ElemN, size_t Count, FixedString Delim>
consteval auto operator|(std::array<FrozenString<ElemN>, Count> const& lhs,
                         join_adaptor<Delim> const& rhs) noexcept(noexcept(rhs(lhs))) {
  return rhs(lhs);
}

// pad_left adaptor: pads left with Fill (default ' ')
template <size_t Width, char Fill = ' '>
struct pad_left_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::pad_left<Width, Fill>(str);
  }
};

template <size_t Width, char Fill = ' '>
inline constexpr pad_left_adaptor<Width, Fill> pad_left{};

// pad_right adaptor: pads right with Fill (default ' ')
template <size_t Width, char Fill = ' '>
struct pad_right_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::pad_right<Width, Fill>(str);
  }
};

template <size_t Width, char Fill = ' '>
inline constexpr pad_right_adaptor<Width, Fill> pad_right{};

// replace adaptor: replaces first occurrence of From with To
template <FixedString From, FixedString To>
struct replace_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::replace<From, To>(str);
  }
};

template <FixedString From, FixedString To>
inline constexpr replace_adaptor<From, To> replace{};

// replace_all adaptor: replaces all non-overlapping occurrences of From with To
template <FixedString From, FixedString To>
struct replace_all_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::replace_all<From, To>(str);
  }
};

template <FixedString From, FixedString To>
inline constexpr replace_all_adaptor<From, To> replace_all{};

} // namespace ops

/*-------------------------------------------------------------------------------*\
 * 数値対応
\*/

/**
 * @brief Bin タグを受け取って整数を2進数表現の文字列に変換する
 *
 * @param arg 変換する整数を含む Bin タグ
 * @return auto consteval 変換後の静的文字列
 */
auto consteval freeze(Bin const& arg) noexcept {
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
 * @return auto consteval 変換後の静的文字列
 */
auto consteval freeze(Oct const& arg) noexcept {
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
 * @return auto consteval 変換後の静的文字列
 */
auto consteval freeze(Hex const& arg) noexcept {
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
 * @return auto consteval 変換後の静的文字列
 */
auto consteval freeze(Precision const& arg) noexcept {
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
 * @return auto consteval 変換後の静的文字列
 */
template <Integral T>
auto consteval freeze(T const& arg) noexcept {
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
auto consteval freeze(T const& arg) noexcept {
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
auto consteval freeze(T const& arg) noexcept {
  auto const s = std::string_view{arg};
  return detail::freeze_from_sv(s);
}

/*-------------------------------------------------------------------------------*\
 * 未対応型はコンパイルエラー
\*/
template <typename T>
auto consteval freeze(T const&) noexcept = delete;

// pad_left - non-integral freezable values
template <size_t Width, char Fill = ' ', typename T>
  requires (!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
auto consteval pad_left(T const& v) noexcept {
  return pad_left<Width, Fill>(freeze(v));
}

// pad_right - non-integral freezable values
template <size_t Width, char Fill = ' ', typename T>
  requires (!Integral<std::remove_cvref_t<T>> && requires(T const& v) { freeze(v); })
auto consteval pad_right(T const& v) noexcept {
  return pad_right<Width, Fill>(freeze(v));
}

template <FixedString Delim, typename First, typename... Rest>
  requires (requires(First const& v) { freeze(v); }
            && (... && requires(Rest const& v) { freeze(v); })
            && (!detail::is_frozen_string_v<First> || (... || !detail::is_frozen_string_v<Rest>)))
auto consteval join(First const& first, Rest const&... rest) noexcept {
  return join<Delim>(freeze(first), freeze(rest)...);
}

// pad_left - Integral (default Fill = '0'); defined after freeze(Integral)
template <size_t Width, char Fill = '0', Integral T>
auto consteval pad_left(T const& v) noexcept {
  return pad_left<Width, Fill>(freeze(v));
}

// pad_right - Integral (default Fill = '0'); defined after freeze(Integral)
template <size_t Width, char Fill = '0', Integral T>
auto consteval pad_right(T const& v) noexcept {
  return pad_right<Width, Fill>(freeze(v));
}

/**
 * @brief 引数で渡された値をすべて FrozenString に変換し結合する
 *
 * @tparam Args 可変引数の型
 * @param args 結合する引数
 * @return auto consteval 結合後の静的文字列
 */
template <typename... Args>
auto consteval concat(Args const&... args) noexcept {
  return (freeze(args) + ...);
}

namespace detail {

/**
 * @brief 型情報を保持するための単純な構造体
 */
template <typename T>
struct type_identity {
  using type = T;
};

struct unknown_type {};

/**
 * @brief 文字列トークンを対応する型に変換するヘルパー関数
 *
 * @tparam S 判定対象の FrozenString
 */
template <auto S>
consteval auto map_string_to_type() {
  auto constexpr s = S.sv();
  if constexpr (s == "bool") return type_identity<bool>{};
  else if constexpr (s == "char") return type_identity<char>{};
  else if constexpr (s == "int") return type_identity<int>{};
  else if constexpr (s == "uint" || s == "unsigned") return type_identity<unsigned int>{};
  else if constexpr (s == "long") return type_identity<long>{};
  else if constexpr (s == "ulong") return type_identity<unsigned long>{};
  else if constexpr (s == "float") return type_identity<float>{};
  else if constexpr (s == "double") return type_identity<double>{};
  else if constexpr (s == "string" || s == "str") return type_identity<std::string>{};
  else if constexpr (s == "string_view" || s == "sv") return type_identity<std::string_view>{};
  else if constexpr (s == "void") return type_identity<void>{};
  else if constexpr (s == "size_t" || s == "sz") return type_identity<std::size_t>{};
  // 固定幅整数
  else if constexpr (s == "int8_t" || s == "int8") return type_identity<std::int8_t>{};
  else if constexpr (s == "int16_t" || s == "int16") return type_identity<std::int16_t>{};
  else if constexpr (s == "int32_t" || s == "int32") return type_identity<std::int32_t>{};
  else if constexpr (s == "int64_t" || s == "int64") return type_identity<std::int64_t>{};
  else if constexpr (s == "uint8_t" || s == "uint8") return type_identity<std::uint8_t>{};
  else if constexpr (s == "uint16_t" || s == "uint16") return type_identity<std::uint16_t>{};
  else if constexpr (s == "uint32_t" || s == "uint32") return type_identity<std::uint32_t>{};
  else if constexpr (s == "uint64_t" || s == "uint64") return type_identity<std::uint64_t>{};
  else return type_identity<unknown_type>{};
}

/**
 * @brief 閉じ括弧 ']' を探す。括弧の深さを考慮する。
 */
template <auto S>
consteval std::size_t find_closing_bracket() {
  auto constexpr sv = S.sv();
  std::size_t depth = 0;
  for (std::size_t i = 0; i < sv.size(); ++i) {
    if (sv[i] == '[') ++depth;
    else if (sv[i] == ']') {
      if (--depth == 0) return i;
    }
  }
  return std::string_view::npos;
}

/**
 * @brief トップレベルのカンマ ',' を探す。括弧の深さを考慮する。
 */
template <auto S>
consteval std::size_t find_top_level_comma() {
  auto constexpr sv = S.sv();
  std::size_t depth = 0;
  for (std::size_t i = 0; i < sv.size(); ++i) {
    if (sv[i] == '[') ++depth;
    else if (sv[i] == ']') --depth;
    else if (sv[i] == ',' && depth == 0) return i;
  }
  return std::string_view::npos;
}

} // namespace detail

/**
 * @brief 文字列トークンを対応する型に変換するメタ関数
 * @tparam S 判定対象の文字列
 */
template <auto S>
struct type_mapping {
  using type = typename decltype(detail::map_string_to_type<S>())::type;
};

template <bool EmptyMeansVoid, auto Str>
consteval auto parse_to_tuple_impl() {
  auto constexpr trimmed = trim(Str);

  if constexpr (trimmed.length == 0) {
    if constexpr (EmptyMeansVoid) {
      return detail::type_identity<std::tuple<void>>{};
    } else {
      return detail::type_identity<std::tuple<>>{};
    }
  } else if constexpr (trimmed.buffer[0] == '[') {
    auto constexpr closing_pos = detail::find_closing_bracket<trimmed>();
    static_assert(closing_pos != std::string_view::npos, "Missing matching ']'"
    );

    auto constexpr inner = substr(trimmed, 1, static_cast<std::ptrdiff_t>(closing_pos - 1));
    using BaseInnerTuple = typename decltype(parse_to_tuple_impl<false, inner>())::type;

    auto constexpr opt_info = [](auto const& s, size_t pos) {
      size_t i = pos + 1;
      while (i < s.length && detail::is_whitespace(s.buffer[i])) ++i;
      bool found = (i < s.length && s.buffer[i] == '?');
      return std::pair{found, found ? i : pos};
    }(trimmed, closing_pos);

    auto constexpr is_opt = opt_info.first;
    auto constexpr search_start = opt_info.second;

    using InnerTuple = std::conditional_t<is_opt, std::optional<BaseInnerTuple>, BaseInnerTuple>;

    auto constexpr next_comma = trimmed.sv().find(',', search_start);
    if constexpr (next_comma == std::string_view::npos) {
      return detail::type_identity<std::tuple<InnerTuple>>{};
    } else {
      auto constexpr rest = substr(trimmed, next_comma + 1, static_cast<std::ptrdiff_t>(trimmed.length - next_comma - 1));
      using RestTuple = typename decltype(parse_to_tuple_impl<true, rest>())::type;
      using Combined = decltype(std::tuple_cat(std::declval<std::tuple<InnerTuple>>(), std::declval<RestTuple>()));
      return detail::type_identity<Combined>{};
    }
  } else {
    auto constexpr comma_pos = detail::find_top_level_comma<trimmed>();

    if constexpr (comma_pos == std::string_view::npos) {
      auto constexpr is_opt = (trimmed.length > 0 && trimmed.buffer[trimmed.length - 1] == '?');
      if constexpr (is_opt) {
        auto constexpr name = trim(substr(trimmed, 0, static_cast<std::ptrdiff_t>(trimmed.length - 1)));
        using T = typename type_mapping<name>::type;
        static_assert(!std::is_same_v<T, detail::unknown_type>, "Unknown type name before '?'");
        static_assert(!std::is_same_v<T, void>, "'void?' is not supported");
        return detail::type_identity<std::tuple<std::optional<T>>>{};
      } else {
        using T = typename type_mapping<trimmed>::type;
        static_assert(!std::is_same_v<T, detail::unknown_type>, "Unknown type name");
        return detail::type_identity<std::tuple<T>>{};
      }
    } else {
      auto constexpr token = trim(substr(trimmed, 0, comma_pos));
      auto constexpr is_opt = (token.length > 0 && token.buffer[token.length - 1] == '?');
      auto constexpr rest_str = substr(trimmed, comma_pos + 1, static_cast<std::ptrdiff_t>(trimmed.length - comma_pos - 1));
      using RestTuple = typename decltype(parse_to_tuple_impl<true, rest_str>())::type;

      if constexpr (token.length == 0) {
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<void>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      } else if constexpr (is_opt) {
        auto constexpr name = trim(substr(token, 0, static_cast<std::ptrdiff_t>(token.length - 1)));
        using T = typename type_mapping<name>::type;
        static_assert(!std::is_same_v<T, detail::unknown_type>, "Unknown type name before '?'");
        static_assert(!std::is_same_v<T, void>, "'void?' is not supported");
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<std::optional<T>>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      } else {
        using T = typename type_mapping<token>::type;
        static_assert(!std::is_same_v<T, detail::unknown_type>, "Unknown type name");
        using Combined = decltype(std::tuple_cat(std::declval<std::tuple<T>>(), std::declval<RestTuple>()));
        return detail::type_identity<Combined>{};
      }
    }
  }
}

/**
 * @brief 固定文字列をパースして型のリスト（std::tuple）を保持する type_identity を生成する
 * 入れ子構造 [...] にも対応。末尾の '?' による std::optional にも対応。
 *
 * @tparam Str 入力文字列 (FrozenString)
 */
template <auto Str>
consteval auto parse_to_tuple() {
  return parse_to_tuple_impl<false, Str>();
}

}

#ifdef __cpp_lib_format
namespace std {

template <size_t N>
struct formatter<frozenchars::FrozenString<N>, char> {
  formatter<std::string_view, char> delegate_{};

  constexpr auto parse(format_parse_context& ctx) {
    return delegate_.parse(ctx);
  }

  template <typename FormatContext>
  auto format(frozenchars::FrozenString<N> const& value, FormatContext& ctx) const {
    return delegate_.format(value.sv(), ctx);
  }
};

} // namespace std
#endif

#endif /* __FROZEN_CHARS_H__ */
