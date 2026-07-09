#pragma once

#include <array>
#include <cstddef>
#include <limits>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "concepts.hpp"
#include "detail/number_conv.hpp"
#include "freeze.hpp"
#include "literals.hpp"
#include "string.hpp"

namespace frozenchars::detail {

struct format_spec {
  char fill = ' ';
  char align = '\0';
  char sign = '\0';
  bool zero = false;
  int width = 0;
  int precision = -1;
  char type = '\0';
};

/**
 * @brief フォーマット指定子文字列を解析する
 */
[[nodiscard]] consteval auto parse_spec(char const* data, size_t /*len*/, size_t start, size_t end) noexcept
  -> format_spec
{
  format_spec spec{};
  auto i = start;
  if (i < end && data[i] == '{') ++i;
  if (i >= end) return spec;
  if (data[i] == ':') ++i;
  if (i >= end) return spec;

  if (data[i] != '<' && data[i] != '>' && data[i] != '^') {
    if (i + 1 < end) {
      auto next = data[i + 1];
      if (next == '<' || next == '>' || next == '^') {
        spec.fill = data[i];
        spec.align = next;
        i += 2;
      }
    }
  }
  if (spec.align == '\0' && i < end) {
    if (data[i] == '<' || data[i] == '>' || data[i] == '^') {
      spec.align = data[i];
      ++i;
    }
  }

  if (i < end) {
    if (data[i] == '+') { spec.sign = '+'; ++i; }
    else if (data[i] == '-') { spec.sign = '-'; ++i; }
    else if (data[i] == ' ') { spec.sign = ' '; ++i; }
  }

  if (i < end && data[i] == '0') { spec.zero = true; ++i; }

  if (i < end && data[i] >= '1' && data[i] <= '9') {
    int w = 0;
    while (i < end && data[i] >= '0' && data[i] <= '9') {
      w = w * 10 + (data[i] - '0');
      ++i;
    }
    if (w > 9999) w = 9999;
    spec.width = w;
  }

  if (i < end && data[i] == '.') {
    ++i;
    int p = 0;
    while (i < end && data[i] >= '0' && data[i] <= '9') {
      p = p * 10 + (data[i] - '0');
      ++i;
    }
    if (p > 9999) p = 9999;
    spec.precision = p;
  }

  if (i < end) {
    spec.type = data[i];
    ++i;
  }

  return spec;
}

[[nodiscard]] consteval auto count_fields(char const* data, size_t len) noexcept -> size_t {
  auto count = 0uz;
  auto i = 0uz;
  while (i < len) {
    if (data[i] == '{') {
      if (i + 1 < len && data[i + 1] == '{') {
        i += 2;
      } else {
        ++count;
        ++i;
        while (i < len && data[i] != '}') ++i;
        ++i;
      }
    } else if (data[i] == '}') {
      i += (i + 1 < len && data[i + 1] == '}') ? 2 : 1;
    } else {
      ++i;
    }
  }
  return count;
}

[[nodiscard]] consteval auto count_literal_chars(char const* data, size_t len) noexcept -> size_t {
  auto count = 0uz;
  auto i = 0uz;
  while (i < len) {
    if (data[i] == '{') {
      if (i + 1 < len && data[i + 1] == '{') {
        count += 1; i += 2;
      } else {
        ++i;
        while (i < len && data[i] != '}') ++i;
        ++i;
      }
    } else if (data[i] == '}') {
      if (i + 1 < len && data[i + 1] == '}') {
        count += 1; i += 2;
      } else {
        ++i;
      }
    } else {
      count += 1; ++i;
    }
  }
  return count;
}

consteval void validate_format(char const* data, size_t len) {
  size_t i = 0;
  while (i < len) {
    if (data[i] == '{') {
      if (i + 1 < len && data[i + 1] == '{') {
        i += 2;
      } else {
        ++i;
        while (i < len && data[i] != '}') ++i;
        if (i >= len) throw "frozen_format: unmatched '{' in format string";
        ++i;
      }
    } else if (data[i] == '}') {
      if (i + 1 < len && data[i + 1] == '}') {
        i += 2;
      } else {
        throw "frozen_format: unmatched '}' in format string";
      }
    } else {
      ++i;
    }
  }
}

/**
 * @brief 浮動小数点数を固定小数点形式でバッファに書き込む
 */
template <typename Buf>
constexpr auto format_float_fixed(Buf& buf, size_t pos, double value, int precision) noexcept -> size_t {
  auto written = 0uz;
  if (value < 0) { buf[pos + written] = '-'; ++written; value = -value; }

  auto int_part = static_cast<unsigned long long>(value);
  auto [int_data, int_len] = to_dec_chars(static_cast<long long>(int_part));
  for (auto j = 0uz; j < int_len; ++j) {
    buf[pos + written] = int_data[j]; ++written;
  }

  if (precision > 0) {
    buf[pos + written] = '.'; ++written;
    double frac = value - static_cast<double>(int_part);
    for (int p = 0; p < precision; ++p) {
      frac *= 10.0;
      auto digit = static_cast<int>(frac);
      buf[pos + written] = static_cast<char>('0' + digit); ++written;
      frac -= static_cast<double>(digit);
    }
  }
  return written;
}

/**
 * @brief 浮動小数点数を科学計数法でバッファに書き込む
 */
template <typename Buf>
constexpr auto format_float_scientific(Buf& buf, size_t pos, double value, int precision) noexcept -> size_t {
  auto written = 0uz;
  if (value < 0) { buf[pos + written] = '-'; ++written; value = -value; }

  if (value == 0.0) {
    buf[pos + written] = '0'; ++written;
    if (precision > 0) {
      buf[pos + written] = '.'; ++written;
      for (int p = 0; p < precision; ++p) { buf[pos + written] = '0'; ++written; }
    }
    buf[pos + written] = 'e'; ++written;
    buf[pos + written] = '+'; ++written;
    buf[pos + written] = '0'; ++written;
    buf[pos + written] = '0'; ++written;
    return written;
  }

  int exponent = 0;
  while (value >= 10.0) { value /= 10.0; ++exponent; }
  while (value < 1.0 && value > 0.0) { value *= 10.0; --exponent; }

  auto first_digit = static_cast<int>(value);
  buf[pos + written] = static_cast<char>('0' + first_digit); ++written;
  value -= static_cast<double>(first_digit);

  if (precision > 0) {
    buf[pos + written] = '.'; ++written;
    for (int p = 0; p < precision; ++p) {
      value *= 10.0;
      auto digit = static_cast<int>(value);
      buf[pos + written] = static_cast<char>('0' + digit); ++written;
      value -= static_cast<double>(digit);
    }
  }

  buf[pos + written] = 'e'; ++written;
  if (exponent >= 0) { buf[pos + written] = '+'; ++written; }
  else { buf[pos + written] = '-'; ++written; exponent = -exponent; }

  if (exponent < 10) { buf[pos + written] = '0'; ++written; }
  auto [exp_data, exp_len] = to_dec_chars(static_cast<long long>(exponent));
  for (auto j = 0uz; j < exp_len; ++j) {
    buf[pos + written] = exp_data[j]; ++written;
  }

  return written;
}

/**
 * @brief 浮動小数点数を汎用形式でバッファに書き込む
 */
template <typename Buf>
constexpr auto format_float_general(Buf& buf, size_t pos, double value, int precision) noexcept -> size_t {
  if (precision < 0) precision = 6;

  char fixed_buf[128]{};
  char sci_buf[128]{};
  auto fixed_len = format_float_fixed(fixed_buf, 0, value, precision);
  auto sci_len = format_float_scientific(sci_buf, 0, value, precision);

  auto trim_len = fixed_len;
  while (trim_len > 1 && fixed_buf[trim_len - 1] == '0') --trim_len;
  if (trim_len > 1 && fixed_buf[trim_len - 1] == '.') --trim_len;

  bool use_sci = (sci_len < trim_len) || (trim_len <= 0);
  auto src = use_sci ? sci_buf : fixed_buf;
  auto src_len = use_sci ? sci_len : trim_len;
  for (auto j = 0uz; j < src_len; ++j) {
    buf[pos + j] = src[j];
  }
  return src_len;
}

template <typename Buf>
constexpr auto write_sign(Buf& buf, size_t pos, bool is_negative, char sign_mode) noexcept -> size_t {
  if (is_negative) { buf[pos] = '-'; return 1; }
  if (sign_mode == '+') { buf[pos] = '+'; return 1; }
  if (sign_mode == ' ') { buf[pos] = ' '; return 1; }
  return 0;
}

template <typename Buf, typename T>
constexpr auto format_field(Buf& buf, size_t pos, T const& arg, format_spec const& spec) noexcept -> size_t {
  auto start = pos;

  if constexpr (std::same_as<std::remove_cvref_t<T>, bool>) {
    std::string_view sv = arg ? "true" : "false";
    size_t sv_len = sv.size();
    size_t field_size = static_cast<size_t>(std::max(spec.width, static_cast<int>(sv_len)));
    size_t pad = field_size - sv_len;
    auto fill = spec.fill;
    auto align = spec.align;
    if (align == '\0') align = '<';

    if (align == '>') {
      for (size_t p = 0; p < pad; ++p) { buf[pos] = fill; ++pos; }
      for (size_t j = 0; j < sv_len; ++j) { buf[pos] = sv[j]; ++pos; }
    } else if (align == '<') {
      for (size_t j = 0; j < sv_len; ++j) { buf[pos] = sv[j]; ++pos; }
      for (size_t p = 0; p < pad; ++p) { buf[pos] = fill; ++pos; }
    } else {
      auto left = pad / 2;
      auto right = pad - left;
      for (size_t p = 0; p < left; ++p) { buf[pos] = fill; ++pos; }
      for (size_t j = 0; j < sv_len; ++j) { buf[pos] = sv[j]; ++pos; }
      for (size_t p = 0; p < right; ++p) { buf[pos] = fill; ++pos; }
    }
  } else if constexpr (std::same_as<std::remove_cvref_t<T>, char>) {
    std::string_view sv(&arg, 1);
    size_t sv_len = 1;
    size_t field_size = static_cast<size_t>(std::max(spec.width, static_cast<int>(sv_len)));
    size_t pad = field_size - sv_len;
    auto fill = spec.fill;
    auto align = spec.align;
    if (align == '\0') align = '<';

    if (align == '>') {
      for (size_t p = 0; p < pad; ++p) { buf[pos] = fill; ++pos; }
      buf[pos] = arg; ++pos;
    } else if (align == '<') {
      buf[pos] = arg; ++pos;
      for (size_t p = 0; p < pad; ++p) { buf[pos] = fill; ++pos; }
    } else {
      auto left = pad / 2;
      auto right = pad - left;
      for (size_t p = 0; p < left; ++p) { buf[pos] = fill; ++pos; }
      buf[pos] = arg; ++pos;
      for (size_t p = 0; p < right; ++p) { buf[pos] = fill; ++pos; }
    }
  } else if constexpr (Integral<T>) {
    bool is_neg;
    unsigned long long uv;
    if constexpr (std::is_signed_v<T>) {
      auto const v = static_cast<long long>(arg);
      is_neg = v < 0;
      uv = static_cast<unsigned long long>(v < 0 ? -(v + 1) + 1 : v);
    } else {
      is_neg = false;
      uv = static_cast<unsigned long long>(arg);
    }
    auto type = spec.type;
    if (type == '\0') type = 'd';

    // Get raw number without sign (use unsigned conversion)
    std::array<char, 65> raw_data{};
    size_t raw_len = 0;
    if (type == 'x' || type == 'X') {
      auto [d, l] = to_hex_chars(uv);
      for (auto j = 0uz; j < l; ++j) raw_data[j] = d[j];
      raw_len = l;
    } else if (type == 'o') {
      auto [d, l] = to_oct_chars(uv);
      for (auto j = 0uz; j < l; ++j) raw_data[j] = d[j];
      raw_len = l;
    } else if (type == 'b' || type == 'B') {
      auto [d, l] = to_bin_chars(uv);
      for (auto j = 0uz; j < l; ++j) raw_data[j] = d[j];
      raw_len = l;
    } else {
      auto [d, l] = to_dec_chars(uv);
      for (auto j = 0uz; j < l; ++j) raw_data[j] = d[j];
      raw_len = l;
    }

    if (type == 'X' || type == 'B') {
      for (auto j = 0uz; j < raw_len; ++j) {
        if (raw_data[j] >= 'a' && raw_data[j] <= 'z')
          raw_data[j] = static_cast<char>(raw_data[j] - 'a' + 'A');
      }
    }

    auto sign_size = write_sign(buf, pos, is_neg, spec.sign);

    if (spec.zero) {
      pos += sign_size;
      auto content = sign_size + raw_len;
      auto pad = (spec.width > static_cast<int>(content)) ? spec.width - content : 0;
      for (size_t p = 0; p < static_cast<size_t>(pad); ++p) { buf[pos] = '0'; ++pos; }
      for (auto j = 0uz; j < raw_len; ++j) { buf[pos] = raw_data[j]; ++pos; }
    } else {
      auto content = sign_size + raw_len;
      auto field = static_cast<size_t>(std::max(spec.width, static_cast<int>(content)));
      auto pad = field - content;
      auto fill = spec.fill;
      auto align = spec.align;
      if (align == '\0') align = '>';

      if (align == '>') {
        for (size_t p = 0; p < pad; ++p) { buf[pos] = fill; ++pos; }
        pos += sign_size;
        for (auto j = 0uz; j < raw_len; ++j) { buf[pos] = raw_data[j]; ++pos; }
      } else if (align == '<') {
        pos += sign_size;
        for (auto j = 0uz; j < raw_len; ++j) { buf[pos] = raw_data[j]; ++pos; }
        for (size_t p = 0; p < pad; ++p) { buf[pos] = fill; ++pos; }
      } else {
        auto left = pad / 2;
        auto right = pad - left;
        for (size_t p = 0; p < left; ++p) { buf[pos] = fill; ++pos; }
        pos += sign_size;
        for (auto j = 0uz; j < raw_len; ++j) { buf[pos] = raw_data[j]; ++pos; }
        for (size_t p = 0; p < right; ++p) { buf[pos] = fill; ++pos; }
      }
    }

  } else if constexpr (FloatingPoint<T>) {
    double v = static_cast<double>(arg);
    auto type = spec.type;
    if (type == '\0') type = 'g';
    int prec = spec.precision;
    if (prec < 0) prec = 6;

    char raw[128]{};
    size_t raw_len;
    if (type == 'f' || type == 'F') {
      raw_len = detail::format_float_fixed(raw, 0, v, prec);
      if (type == 'F') {
        for (auto j = 0uz; j < raw_len; ++j) {
          if (raw[j] == 'e') raw[j] = 'E';
        }
      }
    } else if (type == 'e' || type == 'E') {
      raw_len = detail::format_float_scientific(raw, 0, v, prec);
      if (type == 'E') {
        for (auto j = 0uz; j < raw_len; ++j) {
          if (raw[j] == 'e') raw[j] = 'E';
        }
      }
    } else {
      raw_len = detail::format_float_general(raw, 0, v, prec);
      if (type == 'G') {
        for (auto j = 0uz; j < raw_len; ++j) {
          if (raw[j] == 'e') raw[j] = 'E';
        }
      }
    }

    bool has_sign = (raw_len > 0 && raw[0] == '-');
    size_t content_start = has_sign ? 1 : 0;
    size_t content_len = raw_len - content_start;

    char sign_char = '\0';
    if (has_sign) sign_char = '-';
    else if (spec.sign == '+') sign_char = '+';
    else if (spec.sign == ' ') sign_char = ' ';

    size_t sign_size = (sign_char != '\0') ? 1 : 0;
    size_t total_content = sign_size + content_len;
    size_t field_size = static_cast<size_t>(std::max(spec.width, static_cast<int>(total_content)));
    size_t pad = field_size - total_content;
    auto fill = spec.fill;
    auto align = spec.align;
    if (align == '\0') align = '>';

    if (spec.zero) {
      if (sign_char != '\0') { buf[pos] = sign_char; ++pos; }
      for (size_t p = 0; p < pad; ++p) { buf[pos] = '0'; ++pos; }
      for (auto j = content_start; j < raw_len; ++j) { buf[pos] = raw[j]; ++pos; }
    } else if (align == '>') {
      for (size_t p = 0; p < pad; ++p) { buf[pos] = fill; ++pos; }
      if (sign_char != '\0') { buf[pos] = sign_char; ++pos; }
      for (auto j = content_start; j < raw_len; ++j) { buf[pos] = raw[j]; ++pos; }
    } else if (align == '<') {
      if (sign_char != '\0') { buf[pos] = sign_char; ++pos; }
      for (auto j = content_start; j < raw_len; ++j) { buf[pos] = raw[j]; ++pos; }
      for (size_t p = 0; p < pad; ++p) { buf[pos] = fill; ++pos; }
    } else {
      auto left = pad / 2;
      auto right = pad - left;
      for (size_t p = 0; p < left; ++p) { buf[pos] = fill; ++pos; }
      if (sign_char != '\0') { buf[pos] = sign_char; ++pos; }
      for (auto j = content_start; j < raw_len; ++j) { buf[pos] = raw[j]; ++pos; }
      for (size_t p = 0; p < right; ++p) { buf[pos] = fill; ++pos; }
    }

  } else {
    std::string_view sv;
    if constexpr (std::same_as<std::remove_cvref_t<T>, bool>) {
      sv = arg ? "true" : "false";
    } else if constexpr (std::same_as<std::remove_cvref_t<T>, char>) {
      sv = std::string_view(&arg, 1);
    } else if constexpr (requires { arg.sv(); }) {
      sv = arg.sv();
    } else if constexpr (requires { std::string_view(arg); }) {
      sv = std::string_view(arg);
    }

    size_t sv_len = sv.size();
    if (spec.precision >= 0 && static_cast<size_t>(spec.precision) < sv_len) {
      sv_len = spec.precision;
    }
    if (spec.type == 'c') sv_len = 1;

    size_t field_size = static_cast<size_t>(std::max(spec.width, static_cast<int>(sv_len)));
    size_t pad = field_size - sv_len;
    auto fill = spec.fill;
    auto align = spec.align;
    if (align == '\0') align = '<';

    if (align == '>') {
      for (size_t p = 0; p < pad; ++p) { buf[pos] = fill; ++pos; }
      for (size_t j = 0; j < sv_len; ++j) { buf[pos] = sv[j]; ++pos; }
    } else if (align == '<') {
      for (size_t j = 0; j < sv_len; ++j) { buf[pos] = sv[j]; ++pos; }
      for (size_t p = 0; p < pad; ++p) { buf[pos] = fill; ++pos; }
    } else {
      auto left = pad / 2;
      auto right = pad - left;
      for (size_t p = 0; p < left; ++p) { buf[pos] = fill; ++pos; }
      for (size_t j = 0; j < sv_len; ++j) { buf[pos] = sv[j]; ++pos; }
      for (size_t p = 0; p < right; ++p) { buf[pos] = fill; ++pos; }
    }
  }

  return pos - start;
}

} // namespace frozenchars::detail

namespace frozenchars {

/**
 * @brief コンパイル時フォーマット関数。std::format 互換の構文で文字列を生成する。
 *
 * @details 戻り値は FrozenString<4096>（固定バッファ）。実際の内容は .sv() で取得。
 * 呼び出し例: frozen_format<"value={}"_fs>(42)
 *
 * @tparam Fmt フォーマット文字列（FrozenString NTTP）
 * @tparam Args 引数の型パラメータパック
 * @param args フォーマット引数
 * @return auto FrozenString<4096>（実際の長さは .length に格納）
 */
template <FrozenString Fmt, typename... Args>
[[nodiscard]] consteval auto frozen_format(Args const&... args) noexcept {
  constexpr size_t len = Fmt.size();
  detail::validate_format(Fmt.buffer.data(), len);
  constexpr auto num_fields = detail::count_fields(Fmt.buffer.data(), len);
  static_assert(num_fields == sizeof...(Args),
    "frozen_format: number of arguments does not match number of format fields");

  auto tup = std::tie(args...);
  auto result = FrozenString<4096>{};
  size_t pos = 0;
  size_t field_idx = 0;

  for (size_t i = 0; i < len; ++i) {
    if (Fmt.buffer[i] == '{') {
      if (i + 1 < len && Fmt.buffer[i + 1] == '{') {
        result.buffer[pos++] = '{';
        ++i;
      } else {
        auto start = i;
        ++i;
        while (i < len && Fmt.buffer[i] != '}') ++i;
        auto spec = detail::parse_spec(Fmt.buffer.data(), len, start, i);

        [&]<size_t... Is>(std::index_sequence<Is...>) {
          ((field_idx == Is ? (pos += detail::format_field(result.buffer, pos, std::get<Is>(tup), spec), 0) : 0), ...);
        }(std::make_index_sequence<sizeof...(Args)>{});

        ++field_idx;
      }
    } else if (Fmt.buffer[i] == '}') {
      if (i + 1 < len && Fmt.buffer[i + 1] == '}') {
        result.buffer[pos++] = '}';
        ++i;
      } else {
        result.buffer[pos++] = '}';
      }
    } else {
      result.buffer[pos++] = Fmt.buffer[i];
    }
  }

  result.buffer[pos] = '\0';
  result.length = pos;
  return result;
}

} // namespace frozenchars
