#pragma once

#include <array>
#include <cstddef>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <version>

#if defined(__cpp_lib_constexpr_format)
#include <format>
#endif

#include "concepts.hpp"
#include "detail/number_conv.hpp"
#include "string.hpp"

namespace frozenchars::detail {

/**
 * @brief フォーマット指定子（{...} 内のオプション）を保持する構造体
 */
struct format_spec {
  char fill = ' ';         // 埋め文字（{fill}{align} の形で指定、デフォルトは空白）
  char align = '\0';       // 配置 '<'（左）'#'（右）'^'（中央）、未指定は '\0'
  char sign = '\0';        // 符号表示 '+'(常に) '-'(負のみ) ' '(負で空白) 、未指定は '\0'
  bool zero = false;       // 数値の左側を '0' で埋めるか（width と共に使用）
  int width = 0;           // フィールド最小幅（0 は指定なし）
  int precision = -1;      // 浮動小数点/文字列の精度（-1 は指定なし）
  char type = '\0';        // 変換型 'd'/'x'/'o'/'b'/'f'/'e'/'g' 等、未指定は '\0'
};

/**
 * @brief フォーマット指定子文字列を解析する
 *
 * @param data フォーマット文字列全体へのポインタ
 * @param len フォーマット文字列の長さ（現在は未使用）
 * @param start 解析開始位置（'{' の位置）
 * @param end 解析終了位置（'}' の位置）
 * @return format_spec 解析結果の指定子
 */
[[nodiscard]] consteval auto parse_spec(char const* data, size_t /*len*/, size_t start, size_t end) noexcept -> format_spec {
  format_spec spec{};
  auto i = start;
  if (i < end && data[i] == '{') ++i;
  if (i >= end) return spec;

  // '{' の直後の数字は std::format ではフィールドインデックス指定だが、本実装は
  // インデックス指定をサポートしない。幅指定として黙って解釈すると
  // std::format と異なる結果になるため、診断して落とす。
  if (i < end && data[i] >= '0' && data[i] <= '9') {
    FROZENCHARS_CONSTEVAL_FAIL("frozen_format: explicit field index is not supported (use {} or {} in order)");
  }

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
    if (w > 9999) w = 9999;  // 幅の上限（巨大な幅によるバッファ確保の暴走を防ぐ）
    spec.width = w;
  }

  if (i < end && data[i] == '.') {
    ++i;
    int p = 0;
    while (i < end && data[i] >= '0' && data[i] <= '9') {
      p = p * 10 + (data[i] - '0');
      ++i;
    }
    if (p > 9999) p = 9999;  // 精度の上限（同上）
    spec.precision = p;
  }

  if (i < end) {
    spec.type = data[i];
    ++i;
  }

  return spec;
}

/**
 * @brief フォーマット文字列の 1 走査でフィールド数・リテラル数・整合性を返す
 *
 * @param data フォーマット文字列
 * @param len 文字列長
 * @return struct{size_t num_fields, size_t num_literal_chars} 解析結果
 * @throw 対応が取れない '{' や '}' の場合 consteval throw
 */
struct format_scan_result {
  size_t num_fields;
  size_t num_literal_chars;
};
[[nodiscard]] consteval auto scan_format(char const* data, size_t len) noexcept -> format_scan_result {
  size_t fields = 0;
  size_t literal = 0;
  size_t i = 0;
  while (i < len) {
    if (data[i] == '{') {
      if (i + 1 < len && data[i + 1] == '{') {
        literal += 1; i += 2;
      } else {
        ++fields;
        ++i;
        while (i < len && data[i] != '}') ++i;
        if (i >= len) FROZENCHARS_CONSTEVAL_FAIL("frozen_format: unmatched '{' in format string");
        ++i;
      }
    } else if (data[i] == '}') {
      if (i + 1 < len && data[i + 1] == '}') {
        literal += 1; i += 2;
      } else {
        FROZENCHARS_CONSTEVAL_FAIL("frozen_format: unmatched '}' in format string");
      }
    } else {
      literal += 1; ++i;
    }
  }
  return {fields, literal};
}

/**
 * @brief 浮動小数点表記の末尾 0（と小数点）を除去する
 *
 * 指数部 'e...' はそのまま保持する。
 *
 * @param buf 対象バッファ（長さを詰めた位置へ指数部を前詰めする）
 * @param len 元の長さ
 * @return size_t 詰めた後の長さ
 */
template <size_t N>
constexpr auto trim_float_repr(std::array<char, N>& buf, size_t len) noexcept -> size_t {
  auto epos = len;
  for (auto i = 0uz; i < len; ++i) {
    if (buf[i] == 'e') {
      epos = i;
      break;
    }
  }
  auto dot = epos;
  for (auto i = 0uz; i < epos; ++i) {
    if (buf[i] == '.') {
      dot = i;
      break;
    }
  }
  auto new_len = epos;
  if (dot != epos) {
    while (new_len > dot + 1 && buf[new_len - 1] == '0')
      --new_len;
    if (new_len == dot + 1)
      --new_len;
  }
  for (auto i = epos; i < len; ++i) {
    buf[new_len++] = buf[i];
  }
  return new_len;
}

template <typename Buf>
constexpr auto format_float_scientific(Buf& buf, size_t pos, double value, int precision) noexcept -> size_t;

/**
 * @brief 浮動小数点数を固定小数点形式でバッファに書き込む
 *
 * 小数部は最近接偶数丸め（std::format と同じ）。long long で正確に表せない
 * 巨大値は科学計数法へフォールバックする。
 */
template <typename Buf>
constexpr auto format_float_fixed(Buf& buf, size_t pos, double value, int precision) noexcept -> size_t {
  auto const write_str = [&](std::string_view s) noexcept {
    auto n = 0uz;
    for (auto const ch : s) {
      buf[pos + n] = ch;
      ++n;
    }
    return n;
  };
  if (value != value)
    return write_str("nan");
  if (value > std::numeric_limits<double>::max())
    return write_str("inf");
  if (value < -std::numeric_limits<double>::max())
    return write_str("-inf");

  auto written = 0uz;
  if (detail::is_negative(value)) {
    buf[pos + written] = '-';
    ++written;
    value = -value;
  }

  // long long で正確に表せない値は科学計数法で出力する
  if (value >= static_cast<double>(std::numeric_limits<long long>::max())) {
    return written + format_float_scientific(buf, pos + written, value, precision);
  }

  auto const p = std::min(std::max(precision, 0), 18);
  auto scale = 1LL;
  for (int k = 0; k < p; ++k)
    scale *= 10;
  auto integral = static_cast<long long>(value);
  auto frac_scaled = (value - static_cast<double>(integral)) * static_cast<double>(scale);
  auto frac = static_cast<long long>(frac_scaled);
  auto const rem = frac_scaled - static_cast<double>(frac);
  auto const parity = (p > 0 ? frac : integral) & 1;
  if (rem > 0.5 || (rem == 0.5 && parity != 0))
    ++frac;
  if (frac >= scale) {
    frac -= scale;
    ++integral;
  }

  auto [int_data, int_len] = to_dec_chars(integral);
  for (auto j = 0uz; j < int_len; ++j) {
    buf[pos + written] = int_data[j];
    ++written;
  }

  if (precision > 0) {
    buf[pos + written] = '.'; ++written;
    for (int d = p - 1; d >= 0; --d) {
      buf[pos + written + static_cast<size_t>(d)] = static_cast<char>('0' + frac % 10);
      frac /= 10;
    }
    written += static_cast<size_t>(p);
    for (int d = p; d < precision; ++d) {
      buf[pos + written] = '0';
      ++written;
    }
  }
  return written;
}

/**
 * @brief 浮動小数点数を科学計数法(例:1.2e+03)でバッファに書き込む
 *
 * 仮数部は最近接偶数丸め。繰り上がりで 9.99 -> 1.00e+01 のように指数が増える。
 */
template <typename Buf>
constexpr auto format_float_scientific(Buf& buf, size_t pos, double value, int precision) noexcept -> size_t {
  auto const write_str = [&](std::string_view s) noexcept {
    auto n = 0uz;
    for (auto const ch : s) {
      buf[pos + n] = ch;
      ++n;
    }
    return n;
  };
  if (value != value)
    return write_str("nan");
  if (value > std::numeric_limits<double>::max())
    return write_str("inf");
  if (value < -std::numeric_limits<double>::max())
    return write_str("-inf");

  auto written = 0uz;
  if (detail::is_negative(value)) {
    buf[pos + written] = '-';
    ++written;
    value = -value;
  }

  int exponent = 0;
  if (value != 0.0) {
    while (value >= 10.0) {
      value /= 10.0;
      ++exponent;
    }
    while (value < 1.0) {
      value *= 10.0;
      --exponent;
    }
  }

  auto const p = std::min(std::max(precision, 0), 18);
  auto scale = 1LL;
  for (int k = 0; k < p; ++k)
    scale *= 10;
  auto first = static_cast<int>(value);
  auto frac_scaled = (value - static_cast<double>(first)) * static_cast<double>(scale);
  auto frac = static_cast<long long>(frac_scaled);
  auto const rem = frac_scaled - static_cast<double>(frac);
  auto const parity = (p > 0 ? frac : first) & 1;
  if (rem > 0.5 || (rem == 0.5 && parity != 0))
    ++frac;
  if (frac >= scale) {
    frac -= scale;
    ++first;
    if (first >= 10) {
      first = 1;
      ++exponent;
    }
  }

  buf[pos + written] = static_cast<char>('0' + first);
  ++written;
  if (precision > 0) {
    buf[pos + written] = '.'; ++written;
    for (int d = p - 1; d >= 0; --d) {
      buf[pos + written + static_cast<size_t>(d)] = static_cast<char>('0' + frac % 10);
      frac /= 10;
    }
    written += static_cast<size_t>(p);
    for (int d = p; d < precision; ++d) {
      buf[pos + written] = '0';
      ++written;
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
 *
 * 固定小数点と科学計数法の短い方（指数が範囲外なら科学計数法）を選び、
 * 末尾の不要な 0 を除去する。
 *
 * @tparam WorkSize 作業バッファの容量（精度 + 余裕ぶん。呼び出し側の仕様から決める）
 * @tparam Buf 出力バッファ
 */
template <size_t WorkSize, typename Buf>
constexpr auto format_float_general(Buf& buf, size_t pos, double value, int precision) noexcept -> size_t {
  // 明示精度は有効桁数（std::format の 'g' と同じ規則）。未指定は従来どおり 6 桁の小数。
  bool const explicit_precision = precision >= 0;
  if (precision < 0) precision = 6;
  if (explicit_precision)
    precision = std::min(std::max(precision, 1), 18) - 1; // 科学計数法の小数桁数

  std::array<char, WorkSize> fixed_buf{};  // 固定小数点表現の作業バッファ
  std::array<char, WorkSize> sci_buf{};    // 科学計数法表現の作業バッファ
  auto const sci_len = format_float_scientific(sci_buf, 0, value, precision);
  auto const sci_trim = trim_float_repr(sci_buf, sci_len);

  // 科学計数法の 10 進指数（'e' が無ければ 0）
  auto exp10 = 0;
  for (auto i = 0uz; i < sci_trim; ++i) {
    if (sci_buf[i] != 'e')
      continue;
    auto j = i + 1;
    bool neg = false;
    if (j < sci_trim && (sci_buf[j] == '+' || sci_buf[j] == '-')) {
      neg = sci_buf[j] == '-';
      ++j;
    }
    while (j < sci_trim && sci_buf[j] >= '0' && sci_buf[j] <= '9') {
      exp10 = exp10 * 10 + (sci_buf[j] - '0');
      ++j;
    }
    if (neg)
      exp10 = -exp10;
    break;
  }

  // 明示精度: 指数が -4 以上かつ有効桁数未満なら固定小数点、それ以外は科学計数法
  if (explicit_precision) {
    bool const use_sci = exp10 < -4 || exp10 >= precision + 1;
    auto const src_len = use_sci ? sci_trim : trim_float_repr(fixed_buf, format_float_fixed(fixed_buf, 0, value, precision - exp10));
    auto const src = use_sci ? sci_buf.data() : fixed_buf.data();
    for (auto j = 0uz; j < src_len; ++j) {
      buf[pos + j] = src[j];
    }
    return src_len;
  }

  auto const fixed_len = format_float_fixed(fixed_buf, 0, value, precision);
  auto const fixed_trim = trim_float_repr(fixed_buf, fixed_len);

  bool use_sci = (exp10 < -4) || (exp10 >= 6 && sci_trim <= fixed_trim);
  if (value != 0 && [&] {
        for (auto i = 0uz; i < fixed_trim; ++i) {
          if (fixed_buf[i] >= '1' && fixed_buf[i] <= '9')
            return false;
        }
        return true;
      }()) {
    use_sci = true; // 固定小数点では 0 に丸まる非ゼロ値
  }

  auto const src = use_sci ? sci_buf.data() : fixed_buf.data();
  auto const src_len = use_sci ? sci_trim : fixed_trim;
  for (auto j = 0uz; j < src_len; ++j) {
    buf[pos + j] = src[j];
  }
  return src_len;
}

/**
 * @brief フィールド先頭に符号文字を書き込む
 *
 * @param buf 出力バッファ
 * @param pos 書き込み位置
 * @param is_negative 値が負かどうか
 * @param sign_mode 符号表示モード（'+' / ' ' / その他は負のみ）
 * @return size_t 書き込んだバイト数（符号なしなら 0）
 */
template <typename Buf>
constexpr auto write_sign(Buf& buf, size_t pos, bool is_negative, char sign_mode, size_t max_pos) noexcept -> size_t {
  if (pos < max_pos) {
    if (is_negative) { buf[pos] = '-'; return 1; }
    if (sign_mode == '+') { buf[pos] = '+'; return 1; }
    if (sign_mode == ' ') { buf[pos] = ' '; return 1; }
  }
  return 0;
}

/**
 * @brief フォーマット文字列に現れる最大の精度指定を返す
 *
 * 浮動小数点数の作業バッファを精度に追従させるために使う。
 *
 * @param data フォーマット文字列
 * @param len 文字列長
 * @return size_t 最大精度（精度指定がなければ 0）
 */
[[nodiscard]] consteval auto max_precision_in(char const* data, size_t len) noexcept -> size_t {
  auto max_precision = 0uz;
  for (auto i = 0uz; i + 1 < len; ++i) {
    if (data[i] != '.') continue;
    auto j = i + 1;
    auto p = 0u;
    while (j < len && data[j] >= '0' && data[j] <= '9') {
      p = p * 10 + static_cast<unsigned>(data[j] - '0');
      if (p > 9999) p = 9999;
      ++j;
    }
    if (j > i + 1 && p > max_precision) max_precision = p;
  }
  return max_precision;
}

/**
 * @brief 1 つの引数を指定子に従ってバッファへ書き込む
 *
 * @tparam WorkSize 浮動小数点数の作業バッファ容量（精度 + 余裕ぶん）
 * @tparam Buf 出力バッファ
 * @tparam T 引数の型
 * @param buf 出力バッファ
 * @param pos 書き込み開始位置
 * @param arg フォーマットする引数
 * @param spec フォーマット指定子
 * @return size_t 書き込んだ文字数
 */
template <size_t WorkSize, typename Buf, typename T>
constexpr auto format_field(Buf& buf, size_t pos, T const& arg, format_spec const& spec, size_t max_pos) noexcept -> size_t {
  auto start = pos;

  // 真偽値: "true"/"false" を文字列として配置・埋め文字処理する
  if constexpr (std::same_as<std::remove_cvref_t<T>, bool>) {
    std::string_view sv = arg ? "true" : "false";
    size_t sv_len = sv.size();
    size_t field_size = static_cast<size_t>(std::max(spec.width, static_cast<int>(sv_len)));
    size_t pad = field_size - sv_len;
    auto fill = spec.fill;
    auto align = spec.align;
    if (align == '\0') align = '<';

    if (align == '>') {
      for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
      for (size_t j = 0; j < sv_len; ++j) { if (pos < max_pos) buf[pos] = sv[j]; ++pos; }
    } else if (align == '<') {
      for (size_t j = 0; j < sv_len; ++j) { if (pos < max_pos) buf[pos] = sv[j]; ++pos; }
      for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
    } else {
      auto left = pad / 2;
      auto right = pad - left;
      for (size_t p = 0; p < left; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
      for (size_t j = 0; j < sv_len; ++j) { if (pos < max_pos) buf[pos] = sv[j]; ++pos; }
      for (size_t p = 0; p < right; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
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
      for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
      if (pos < max_pos) { buf[pos] = arg; } ++pos;
    } else if (align == '<') {
      if (pos < max_pos) { buf[pos] = arg; } ++pos;
      for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
    } else {
      auto left = pad / 2;
      auto right = pad - left;
      for (size_t p = 0; p < left; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
      if (pos < max_pos) { buf[pos] = arg; } ++pos;
      for (size_t p = 0; p < right; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
    }
  // 整数: 符号判定・基数変換・符号/ゼロ埋め/配置を行う
  } else if constexpr (Integral<T>) {
    bool is_neg;
    unsigned long long uv;
    if constexpr (std::is_signed_v<T>) {
      auto const v = static_cast<long long>(arg);
      is_neg = v < 0;
      // 負数を符号なしへ変換（LLONG_MIN の符号付きオーバーフローを避けるため加算前に unsigned へキャスト）
      uv = v < 0 ? static_cast<unsigned long long>(-(v + 1)) + 1ull : static_cast<unsigned long long>(v);
    } else {
      is_neg = false;
      uv = static_cast<unsigned long long>(arg);
    }
    auto type = spec.type;
    if (type == '\0') type = 'd';  // 型未指定は 10 進数

    // 符号なしで生の数値文字列を生成（基数に応じて変換関数を切り替え）
    std::array<char, 65> raw_data{};  // 64bit 整数の最大桁(64) + 余裕分
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

    char sign_char = '\0';
    if (is_neg) {
      sign_char = '-';
    } else if (spec.sign == '+') {
      sign_char = '+';
    } else if (spec.sign == ' ') {
      sign_char = ' ';
    }
    auto sign_size = (sign_char != '\0') ? 1uz : 0uz;

    if (spec.zero) {
      if (sign_size > 0 && pos < max_pos) { buf[pos] = sign_char; }
      pos += sign_size;
      auto content = sign_size + raw_len;
      auto pad = (spec.width > static_cast<int>(content)) ? spec.width - content : 0;
      for (size_t p = 0; p < static_cast<size_t>(pad); ++p) { if (pos < max_pos) buf[pos] = '0'; ++pos; }
      for (auto j = 0uz; j < raw_len; ++j) { if (pos < max_pos) buf[pos] = raw_data[j]; ++pos; }
    } else {
      auto content = sign_size + raw_len;
      auto field = static_cast<size_t>(std::max(spec.width, static_cast<int>(content)));
      auto pad = field - content;
      auto fill = spec.fill;
      auto align = spec.align;
      if (align == '\0') align = '>';

      if (align == '>') {
        for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
        if (sign_size > 0 && pos < max_pos) { buf[pos] = sign_char; ++pos; }
        for (auto j = 0uz; j < raw_len; ++j) { if (pos < max_pos) buf[pos] = raw_data[j]; ++pos; }
      } else if (align == '<') {
        if (sign_size > 0 && pos < max_pos) { buf[pos] = sign_char; ++pos; }
        for (auto j = 0uz; j < raw_len; ++j) { if (pos < max_pos) buf[pos] = raw_data[j]; ++pos; }
        for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
      } else {
        auto left = pad / 2;   // 中央揃え: 左側の余白（端数は右に回す）
        auto right = pad - left;
        for (size_t p = 0; p < left; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
        if (sign_size > 0 && pos < max_pos) { buf[pos] = sign_char; ++pos; }
        for (auto j = 0uz; j < raw_len; ++j) { if (pos < max_pos) buf[pos] = raw_data[j]; ++pos; }
        for (size_t p = 0; p < right; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
      }
    }

  // 浮動小数点: 型別（f/e/g）の表現生成と符号・配置処理
  } else if constexpr (FloatingPoint<T>) {
    double v = static_cast<double>(arg);
    auto type = spec.type;
    if (type == '\0') type = 'g';  // 型未指定は汎用形式
    int prec = spec.precision;
    if (prec < 0) prec = 6;  // 精度未指定は 6

    std::array<char, WorkSize> raw{};  // 変換結果の作業バッファ（精度 + 余裕ぶん）
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
      raw_len = detail::format_float_general<WorkSize>(raw, 0, v, spec.precision);
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
      if (sign_char != '\0' && pos < max_pos) { buf[pos] = sign_char; ++pos; }
      for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = '0'; ++pos; }
      for (auto j = content_start; j < raw_len; ++j) { if (pos < max_pos) buf[pos] = raw[j]; ++pos; }
    } else if (align == '>') {
      for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
      if (sign_char != '\0' && pos < max_pos) { buf[pos] = sign_char; ++pos; }
      for (auto j = content_start; j < raw_len; ++j) { if (pos < max_pos) buf[pos] = raw[j]; ++pos; }
    } else if (align == '<') {
      if (sign_char != '\0' && pos < max_pos) { buf[pos] = sign_char; ++pos; }
      for (auto j = content_start; j < raw_len; ++j) { if (pos < max_pos) buf[pos] = raw[j]; ++pos; }
      for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
    } else {
      auto left = pad / 2;
      auto right = pad - left;
      for (size_t p = 0; p < left; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
      if (sign_char != '\0' && pos < max_pos) { buf[pos] = sign_char; ++pos; }
      for (auto j = content_start; j < raw_len; ++j) { if (pos < max_pos) buf[pos] = raw[j]; ++pos; }
      for (size_t p = 0; p < right; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
    }

  } else {
    // その他（文字列・ FrozenString 等）: sv() または string_view 変換で取得し配置する
    std::string_view sv;
    if constexpr (requires { arg.sv(); }) {
      sv = arg.sv();
    } else if constexpr (requires { std::string_view(arg); }) {
      sv = std::string_view(arg);
    }

    size_t sv_len = sv.size();
    if (spec.precision >= 0 && static_cast<size_t>(spec.precision) < sv_len) {
      sv_len = spec.precision;
    }
    if (spec.type == 'c' && sv_len > 1)
      sv_len = 1;

    size_t field_size = static_cast<size_t>(std::max(spec.width, static_cast<int>(sv_len)));
    size_t pad = field_size - sv_len;
    auto fill = spec.fill;
    auto align = spec.align;
    if (align == '\0') align = '<';

    if (align == '>') {
      for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
      for (size_t j = 0; j < sv_len; ++j) { if (pos < max_pos) buf[pos] = sv[j]; ++pos; }
    } else if (align == '<') {
      for (size_t j = 0; j < sv_len; ++j) { if (pos < max_pos) buf[pos] = sv[j]; ++pos; }
      for (size_t p = 0; p < pad; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
    } else {
      auto left = pad / 2;
      auto right = pad - left;
      for (size_t p = 0; p < left; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
      for (size_t j = 0; j < sv_len; ++j) { if (pos < max_pos) buf[pos] = sv[j]; ++pos; }
      for (size_t p = 0; p < right; ++p) { if (pos < max_pos) buf[pos] = fill; ++pos; }
    }
  }

  return pos - start;
}

} // namespace frozenchars::detail

namespace frozenchars {

/**
 * @brief コンパイル時フォーマット関数。std::format 互換の構文で文字列を生成する。
 *
 * @details 戻り値は FrozenString<Capacity>。実際の内容は .sv() で取得。
 * Capacity を省略した場合は 4096 となり、4096 文字を超える出力も明示的に Capacity を
 * 指定すれば扱える（例: frozen_format<"big {}"_fs, 9000>(huge_string)）。
 * 出力が Capacity - 1 文字を超える場合は末尾を切り詰める。
 *
 * 呼び出し例: frozen_format<"value={}"_fs>(42)
 *            frozen_format<"big {}"_fs, 9000>(huge_string)
 *
 * @tparam Fmt フォーマット文字列（FrozenString NTTP）
 * @tparam Capacity 出力バッファの最大サイズ（終端 '\0' を含む、デフォルト 4096）
 * @tparam Args 引数の型パラメータパック
 * @param args フォーマット引数
 * @return auto FrozenString<Capacity>（実際の長さは .length に格納）
 */
template <FrozenString Fmt, size_t Capacity = 4096, typename... Args>
[[nodiscard]] consteval auto frozen_format(Args const&... args) noexcept {
  constexpr size_t len = Fmt.size();
  // 1 走査でフィールド数・リテラル数・整合性をまとめて検証
  constexpr auto scan = detail::scan_format(Fmt.buffer.data(), len);
  static_assert(scan.num_fields == sizeof...(Args),
    "frozen_format: number of arguments does not match number of format fields");

  auto tup = std::tie(args...);
  // 内部バッファも Capacity 連動とし、format_field に Capacity - 1 を渡して境界内に収める
  auto result = FrozenString<Capacity>{};
  size_t pos = 0;
  size_t field_idx = 0;

  for (size_t i = 0; i < len; ++i) {
    if (Fmt.buffer[i] == '{') {
      if (i + 1 < len && Fmt.buffer[i + 1] == '{') {
        if (pos < Capacity - 1) result.buffer[pos] = '{';
        ++pos;
        ++i;
      } else {
        auto start = i;
        ++i;
        while (i < len && Fmt.buffer[i] != '}') ++i;
        auto spec = detail::parse_spec(Fmt.buffer.data(), len, start, i);
        // 浮動小数点数の作業バッファは書式文字列中の最大精度に追従させる
        //（固定長にすると {:.200f} のような大きな精度でバッファ外へ書き込む）
        constexpr size_t work_size = detail::max_precision_in(Fmt.buffer.data(), len) + 64uz;

        [&]<size_t... Is>(std::index_sequence<Is...>) {
          ((field_idx == Is ? (pos += detail::format_field<work_size>(result.buffer, pos, std::get<Is>(tup), spec, Capacity - 1), 0) : 0), ...);
        }(std::make_index_sequence<sizeof...(Args)>{});

        ++field_idx;
      }
    } else if (Fmt.buffer[i] == '}') {
      if (i + 1 < len && Fmt.buffer[i + 1] == '}') {
        if (pos < Capacity - 1) result.buffer[pos] = '}';
        ++pos;
        ++i;
      } else {
        if (pos < Capacity - 1) result.buffer[pos] = '}';
        ++pos;
      }
    } else {
      if (pos < Capacity - 1) result.buffer[pos] = Fmt.buffer[i];
      ++pos;
    }
  }

  result.buffer[pos < Capacity ? pos : Capacity - 1] = '\0';
  result.length = pos < Capacity ? pos : Capacity - 1;
  return result;
}

/**
 * @brief std::format 完全準拠のフォーマット関数。
 *
 * @details frozen_format と異なり、std::format / std::format_to_n をそのまま使用するため、
 * std::format の全ての型・指定子（ロケール非依存）に対応する。戻り値は FrozenString<Capacity>。
 * Capacity を省略した場合は 4096 となり、4096 文字を超える出力も明示的に Capacity を
 * 指定すれば扱える（例: frozen_std_format<"big {}"_fs, 9000>(huge_string)）。
 *
 * 本関数は __cpp_lib_constexpr_format が定義されている環境（C++26 対応コンパイラで
 * constexpr std::format が実装済み）でのみ提供される。
 *
 * 呼び出し例: frozen_std_format<"value={}"_fs>(42)
 *            frozen_std_format<"big {}"_fs, 9000>(huge_string)
 *
 * @tparam Fmt フォーマット文字列（FrozenString NTTP）
 * @tparam Capacity 出力バッファの最大サイズ（終端 '\0' を含む、デフォルト 4096）
 * @tparam Args 引数の型パラメータパック
 * @param args フォーマット引数
 * @return FrozenString<Capacity> フォーマット済み文字列（.length に格納）
 */
#if defined(__cpp_lib_constexpr_format)
template <FrozenString Fmt, size_t Capacity = 4096, typename... Args>
[[nodiscard]] consteval auto frozen_std_format(Args const&... args) noexcept {
  constexpr std::string_view fmt = Fmt.sv();
  constexpr auto size = std::formatted_size(fmt, args...);
  static_assert(size < Capacity, "frozen_std_format: output exceeds Capacity");
  auto result = FrozenString<Capacity>{};
  auto const [ptr, count] = std::format_to_n(result.buffer.data(), Capacity - 1, fmt, args...);
  result.length = static_cast<size_t>(count);
  *ptr = '\0';
  return result;
}
#endif

} // namespace frozenchars
