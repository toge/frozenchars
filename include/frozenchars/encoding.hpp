#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "string.hpp"
#include "string_ops.hpp"
#include "detail/char_utils.hpp"

namespace frozenchars::detail {

/**
 * @brief 先頭バイトから UTF-8 シーケンスのバイト数を求める
 *
 * @param c シーケンスの先頭バイト
 * @return size_t バイト数（1〜4）。不正なリーディングバイトは 1（フェイルソフト）
 */
constexpr auto utf8_char_length(char const c) noexcept -> size_t {
  auto const uc = static_cast<unsigned char>(c);
  if (uc < 0x80) {
    return 1;
  }
  if ((uc & 0xE0) == 0xC0) {
    return 2;
  }
  if ((uc & 0xF0) == 0xE0) {
    return 3;
  }
  if ((uc & 0xF8) == 0xF0) {
    return 4;
  }
  return 1;
}

/**
 * @brief 指定位置から UTF-8 シーケンスを 1 符号点に復号する
 *
 * 継続バイトが不正な場合は先頭バイトのみを 1 符号点として扱う（フェイルソフト）。
 *
 * @param str 復号元の文字列
 * @param pos 復号開始位置（バイト単位）
 * @param consumed 消費したバイト数（出力）
 * @param codepoint 復号した符号点（出力）
 * @return bool pos が範囲内なら true
 */
constexpr auto utf8_decode_at(std::string_view str, size_t pos, size_t& consumed, std::uint32_t& codepoint) noexcept -> bool {
  if (pos >= str.size()) {
    return false;
  }

  auto const first = static_cast<unsigned char>(str[pos]);
  if (first < 0x80) {
    codepoint = first;
    consumed = 1;
    return true;
  }

  if ((first & 0xE0) == 0xC0 && pos + 1 < str.size()) {
    auto const second = static_cast<unsigned char>(str[pos + 1]);
    if ((second & 0xC0) != 0x80) {
      codepoint = first;
      consumed = 1;
      return true;
    }
    codepoint = ((first & 0x1F) << 6) | (second & 0x3F);
    consumed = 2;
    return true;
  }

  if ((first & 0xF0) == 0xE0 && pos + 2 < str.size()) {
    auto const second = static_cast<unsigned char>(str[pos + 1]);
    auto const third = static_cast<unsigned char>(str[pos + 2]);
    if ((second & 0xC0) != 0x80 || (third & 0xC0) != 0x80) {
      codepoint = first;
      consumed = 1;
      return true;
    }
    codepoint = ((first & 0x0F) << 12) | ((second & 0x3F) << 6) | (third & 0x3F);
    consumed = 3;
    return true;
  }

  if ((first & 0xF8) == 0xF0 && pos + 3 < str.size()) {
    auto const second = static_cast<unsigned char>(str[pos + 1]);
    auto const third = static_cast<unsigned char>(str[pos + 2]);
    auto const fourth = static_cast<unsigned char>(str[pos + 3]);
    if ((second & 0xC0) != 0x80 || (third & 0xC0) != 0x80 || (fourth & 0xC0) != 0x80) {
      codepoint = first;
      consumed = 1;
      return true;
    }
    codepoint = ((first & 0x07) << 18) | ((second & 0x3F) << 12) | ((third & 0x3F) << 6) | (fourth & 0x3F);
    consumed = 4;
    return true;
  }

  codepoint = first;
  consumed = 1;
  return true;
}

/**
 * @brief 文字列を符号点列に変換する
 *
 * @tparam MaxN 入力バッファ長（終端含む、配列サイズにも使用）
 * @param str 対象文字列
 * @return std::array<std::uint32_t, MaxN> 符号点配列（length 以降は 0）
 */
template <size_t MaxN>
constexpr auto utf8_codepoints(FrozenString<MaxN> const& str) noexcept -> std::array<std::uint32_t, MaxN> {
  auto codepoints = std::array<std::uint32_t, MaxN>{};
  auto count = 0uz;
  auto i = 0uz;
  while (i < str.length) {
    size_t consumed = 0;
    std::uint32_t codepoint = 0;
    (void)utf8_decode_at(str.sv(), i, consumed, codepoint);
    codepoints[count++] = codepoint;
    i += consumed;
  }
  return codepoints;
}

/**
 * @brief 文字列を符号点列に変換する（符号点数も取得する版）
 *
 * @tparam MaxN 入力バッファ長（終端含む、配列サイズにも使用）
 * @param str 対象文字列
 * @param count 変換した符号点数（出力）
 * @return std::array<std::uint32_t, MaxN> 符号点配列（count 以降は 0）
 */
template <size_t MaxN>
constexpr auto utf8_codepoints(FrozenString<MaxN> const& str, size_t& count) noexcept -> std::array<std::uint32_t, MaxN> {
  auto codepoints = std::array<std::uint32_t, MaxN>{};
  count = 0;
  auto i = 0uz;
  while (i < str.length) {
    size_t consumed = 0;
    std::uint32_t codepoint = 0;
    (void)utf8_decode_at(str.sv(), i, consumed, codepoint);
    codepoints[count++] = codepoint;
    i += consumed;
  }
  return codepoints;
}

/**
 * @brief 符号点を UTF-8 バイト列にエンコードする
 *
 * @param codepoint エンコードする符号点
 * @param out エンコード結果の格納先（最大 4 バイト）
 * @param length 書き込んだバイト数（出力）
 */
constexpr auto utf8_encode_codepoint(std::uint32_t codepoint, std::array<char, 4>& out, size_t& length) noexcept -> void {
  if (codepoint <= 0x7F) {
    out[0] = static_cast<char>(codepoint);
    length = 1;
    return;
  }
  if (codepoint <= 0x7FF) {
    out[0] = static_cast<char>(0xC0 | (codepoint >> 6));
    out[1] = static_cast<char>(0x80 | (codepoint & 0x3F));
    length = 2;
    return;
  }
  if (codepoint <= 0xFFFF) {
    out[0] = static_cast<char>(0xE0 | (codepoint >> 12));
    out[1] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
    out[2] = static_cast<char>(0x80 | (codepoint & 0x3F));
    length = 3;
    return;
  }
  out[0] = static_cast<char>(0xF0 | (codepoint >> 18));
  out[1] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
  out[2] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
  out[3] = static_cast<char>(0x80 | (codepoint & 0x3F));
  length = 4;
}

/**
 * @brief 指定位置のバイトが有効な UTF-8 シーケンスかを判定し、その長さを返す
 *
 * オーバーロング・サロゲート・U+10FFFF 超えは不正と判定する。
 *
 * @param str 検査対象の文字列
 * @param pos 判定開始位置
 * @return size_t 有効なら長さ（2〜4）、不正・ASCII・孤立バイトなら 0
 */
[[nodiscard]] constexpr auto valid_utf8_seq_length(std::string_view const str, size_t const pos) noexcept -> size_t {
  if (pos >= str.size()) {
    return 0;
  }
  auto const first = static_cast<unsigned char>(str[pos]);
  auto const is_cont = [&](size_t const at) noexcept {
    return at < str.size() && (static_cast<unsigned char>(str[at]) & 0xC0) == 0x80;
  };
  if (first >= 0xC2 && first <= 0xDF) {
    return is_cont(pos + 1) ? 2 : 0;
  }
  if (first >= 0xE0 && first <= 0xEF) {
    if (!is_cont(pos + 1)) {
      return 0;
    }
    auto const second = static_cast<unsigned char>(str[pos + 1]);
    if (first == 0xE0 && second < 0xA0) {
      return 0;  // オーバーロング
    }
    if (first == 0xED && second > 0x9F) {
      return 0;  // サロゲート
    }
    return is_cont(pos + 2) ? 3 : 0;
  }
  if (first >= 0xF0 && first <= 0xF4) {
    if (!is_cont(pos + 1)) {
      return 0;
    }
    auto const second = static_cast<unsigned char>(str[pos + 1]);
    if (first == 0xF0 && second < 0x90) {
      return 0;  // オーバーロング
    }
    if (first == 0xF4 && second > 0x8F) {
      return 0;  // U+10FFFF 超え
    }
    return (is_cont(pos + 2) && is_cont(pos + 3)) ? 4 : 0;
  }
  return 0;
}

/**
 * @brief 16 進数字の読み取り結果
 */
struct hex_digits_result {
  bool valid;             ///< 1 桁以上読み取れたか
  std::uint32_t value;    ///< 読み取った値
  std::size_t count;      ///< 読み取った桁数
};

/**
 * @brief 文字列の指定位置から 16 進数字を最大 max_digits 桁読み取る
 *
 * @param str 読み取り元
 * @param i 読み取り開始位置
 * @param max_digits 読み取り上限桁数
 * @return hex_digits_result 結果（有効桁数が 0 の場合は valid=false）
 */
[[nodiscard]] consteval auto parse_hex_digits(std::string_view const str, size_t const i, size_t const max_digits) noexcept -> hex_digits_result {
  std::uint32_t value = 0;
  auto count = 0uz;
  while (count < max_digits && i + count < str.size() && is_hex_digit(str[i + count])) {
    value = (value << 4) | hex_digit_to_value(str[i + count]);
    ++count;
  }
  return {count > 0, value, count};
}

/**
 * @brief NTTP 文字列中に不正な \u/\U エスケープ（サロゲート・U+10FFFF 超え）が含まれるか判定する
 *
 * @tparam Str 検査対象の文字列（FrozenString NTTP）
 * @return bool 含まれれば true
 */
template <FrozenString Str>
[[nodiscard]] consteval auto has_invalid_codepoint_escape() noexcept -> bool {
  auto const s = Str.sv();
  for (auto i = 0uz; i + 1 < s.size(); ++i) {
    if (s[i] != '\\') {
      continue;
    }
    auto const esc = s[i + 1];
    if (esc == 'u') {
      auto const r = parse_hex_digits(s, i + 2, 4);
      if (r.valid && r.count == 4 && (r.value >= 0xD800 && r.value <= 0xDFFF)) {
        return true;
      }
    }
    if (esc == 'U') {
      auto const r = parse_hex_digits(s, i + 2, 8);
      if (r.valid && r.count == 8 && r.value > 0x10FFFF) {
        return true;
      }
    }
  }
  return false;
}

} // namespace frozenchars::detail

namespace frozenchars {

/**
 * @brief 文字列を16進エンコードする
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列 (バッファサイズは 2 * length + 1)
 */
template <size_t N>
[[nodiscard]] auto consteval hex_encode(FrozenString<N> const& str) noexcept {
  constexpr auto OUT_CAP = 2 * (N > 0 ? N - 1 : 0) + 1;
  auto res = FrozenString<OUT_CAP>{};
  auto const s = str.sv();
  auto offset = 0uz;

  for (auto const c : s) {
    auto const byte = static_cast<std::uint8_t>(c);
    res.buffer[offset++] = detail::value_to_hex_digit(byte >> 4);
    res.buffer[offset++] = detail::value_to_hex_digit(byte & 0x0F);
  }

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列リテラルを16進エンコードする
 */
template <size_t N>
[[nodiscard]] auto consteval hex_encode(char const (&str)[N]) noexcept {
  return hex_encode(FrozenString{str});
}

/**
 * @brief 文字列を16進デコードする
 */
template <size_t N>
[[nodiscard]] auto consteval hex_decode(FrozenString<N> const& str) noexcept {
  auto const s = str.sv();
  auto res = FrozenString<N>{}; // 元のバッファサイズを流用（デコード後は必ず小さくなる）
  auto offset = 0uz;

  for (auto i = 0uz; i + 1 < s.size(); i += 2) {
    res.buffer[offset++] = static_cast<char>(detail::parse_hex_byte(s[i], s[i + 1]));
  }

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列リテラルを16進デコードする
 */
template <size_t N>
[[nodiscard]] auto consteval hex_decode(char const (&str)[N]) noexcept {
  return hex_decode(FrozenString{str});
}

/**
 * @brief 文字列を16進エンコードする (NTTP版)
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval hex_encode() noexcept {
  return shrink_to_fit<hex_encode(Str)>();
}

/**
 * @brief 文字列を16進デコードする (NTTP版)
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval hex_decode() noexcept {
  return shrink_to_fit<hex_decode(Str)>();
}

/**
 * @brief 文字列をアスキー形式に変換する (16進エンコード)
 */
template <size_t N>
[[nodiscard]] auto consteval to_ascii(FrozenString<N> const& str) noexcept {
  return hex_encode(str);
}

/**
 * @brief 文字列リテラルをアスキー形式に変換する (16進エンコード)
 */
template <size_t N>
[[nodiscard]] auto consteval to_ascii(char const (&str)[N]) noexcept {
  return hex_encode(str);
}

/**
 * @brief 文字列をアスキー形式に変換する (16進エンコード、NTTP版)
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval to_ascii() noexcept {
  return hex_encode<Str>();
}

/**
 * @brief アスキー形式から文字列を復元する (16進デコード)
 */
template <size_t N>
[[nodiscard]] auto consteval from_ascii(FrozenString<N> const& str) noexcept {
  return hex_decode(str);
}

/**
 * @brief 文字列リテラルをアスキー形式から復元する (16進デコード)
 */
template <size_t N>
[[nodiscard]] auto consteval from_ascii(char const (&str)[N]) noexcept {
  return hex_decode(str);
}

/**
 * @brief アスキー形式から文字列を復元する (16進デコード、NTTP版)
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval from_ascii() noexcept {
  return hex_decode<Str>();
}

/**
 * @brief 文字列をURLエンコードする
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列 (バッファサイズは最悪ケース 3 * length + 1)
 */
template <size_t N>
[[nodiscard]] auto consteval url_encode(FrozenString<N> const& str) noexcept {
  constexpr auto OUT_CAP = 3 * (N > 0 ? N - 1 : 0) + 1;
  auto res = FrozenString<OUT_CAP>{};
  auto offset = 0uz;
  for (auto const c : str.sv()) {
    if (detail::is_unreserved(c)) {
      res.buffer[offset++] = c;
    } else {
      auto const byte = static_cast<std::uint8_t>(c);
      res.buffer[offset++] = '%';
      res.buffer[offset++] = detail::value_to_hex_digit(byte >> 4);
      res.buffer[offset++] = detail::value_to_hex_digit(byte & 0x0F);
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列リテラルをURLエンコードする
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 変換文字列
 */
template <size_t N>
[[nodiscard]] auto consteval url_encode(char const (&str)[N]) noexcept {
  return url_encode(FrozenString{str});
}

/**
 * @brief 文字列をURLエンコードする（NTTP版・正確なバッファサイズ）
 *
 * @tparam Str 変換対象の FrozenString（NTTPとして渡す）
 * @return auto 変換文字列
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval url_encode() noexcept {
  constexpr auto OUT_LEN = detail::count_url_encoded_size(Str);
  auto res = FrozenString<OUT_LEN + 1>{};
  auto offset = 0uz;
  for (auto const c : Str.sv()) {
    if (detail::is_unreserved(c)) {
      res.buffer[offset++] = c;
    } else {
      auto const byte = static_cast<std::uint8_t>(c);
      res.buffer[offset++] = '%';
      res.buffer[offset++] = detail::value_to_hex_digit(byte >> 4);
      res.buffer[offset++] = detail::value_to_hex_digit(byte & 0x0F);
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列をURLデコードする
 * `%XX` 形式を元の文字に変換します。`+` はスペースに変換されます。
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列
 */
template <size_t N>
[[nodiscard]] auto consteval url_decode(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto const s = str.sv();
  for (auto i = 0uz; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size() && detail::is_hex_digit(s[i + 1]) && detail::is_hex_digit(s[i + 2])) {
      res.buffer[offset++] = static_cast<char>(detail::parse_hex_byte(s[i + 1], s[i + 2]));
      i += 2;
    } else if (s[i] == '+') {
      res.buffer[offset++] = ' ';
    } else {
      res.buffer[offset++] = s[i];
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列リテラルをURLデコードする
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 変換文字列
 */
template <size_t N>
[[nodiscard]] auto consteval url_decode(char const (&str)[N]) noexcept {
  return url_decode(FrozenString{str});
}

/**
 * @brief 文字列をURLデコードする（NTTP版・正確なバッファサイズ）
 *
 * @tparam Str 変換対象の FrozenString（NTTPとして渡す）
 * @return auto 変換文字列
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval url_decode() noexcept {
  constexpr auto OUT_LEN = detail::count_url_decoded_size(Str);
  auto res = FrozenString<OUT_LEN + 1>{};
  auto offset = 0uz;
  auto const s = Str.sv();
  for (auto i = 0uz; i < s.size(); ++i) {
    if (s[i] == '%' && i + 2 < s.size() && detail::is_hex_digit(s[i + 1]) && detail::is_hex_digit(s[i + 2])) {
      res.buffer[offset++] = static_cast<char>(detail::parse_hex_byte(s[i + 1], s[i + 2]));
      i += 2;
    } else if (s[i] == '+') {
      res.buffer[offset++] = ' ';
    } else {
      res.buffer[offset++] = s[i];
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列をBase64エンコードする
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列 (バッファサイズは ((length + 2) / 3) * 4 + 1)
 */
template <size_t N>
[[nodiscard]] auto consteval base64_encode(FrozenString<N> const& str) noexcept {
  constexpr auto OUT_CAP = detail::count_base64_encoded_size(N > 0 ? N - 1 : 0) + 1;
  auto res = FrozenString<OUT_CAP>{};
  auto const s = str.sv();
  auto offset = 0uz;

  for (auto i = 0uz; i < s.size(); i += 3) {
    auto const b1 = static_cast<std::uint8_t>(s[i]);
    auto const b2 = (i + 1 < s.size()) ? static_cast<std::uint8_t>(s[i + 1]) : 0;
    auto const b3 = (i + 2 < s.size()) ? static_cast<std::uint8_t>(s[i + 2]) : 0;

    res.buffer[offset++] = detail::base64_chars[b1 >> 2];
    res.buffer[offset++] = detail::base64_chars[((b1 & 0x03) << 4) | (b2 >> 4)];
    res.buffer[offset++] = (i + 1 < s.size()) ? detail::base64_chars[((b2 & 0x0F) << 2) | (b3 >> 6)] : '=';
    res.buffer[offset++] = (i + 2 < s.size()) ? detail::base64_chars[b3 & 0x3F] : '=';
  }

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列リテラルをBase64エンコードする
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 変換文字列
 */
template <size_t N>
[[nodiscard]] auto consteval base64_encode(char const (&str)[N]) noexcept {
  return base64_encode(FrozenString{str});
}

/**
 * @brief 文字列をBase64エンコードする（NTTP版・正確なバッファサイズ）
 *
 * @tparam Str 変換対象 of FrozenString（NTTPとして渡す）
 * @return auto 変換文字列
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval base64_encode() noexcept {
  return shrink_to_fit<base64_encode(Str)>();
}

/**
 * @brief 文字列をBase64デコードする
 * 不正な文字は無視されます。
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換後のバイナリ/文字列
 */
template <size_t N>
[[nodiscard]] auto consteval base64_decode(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{}; // デコード後は必ず元のサイズ以下になる
  auto const s = str.sv();
  auto offset = 0uz;
  auto count = 0uz;
  std::uint32_t buffer = 0;

  for (auto const c : s) {
    if (c == '=') break;
    auto const val = detail::base64_char_to_value(c);
    if (val == 255) continue;

    buffer = (buffer << 6) | val;
    count++;

    if (count == 4) {
      res.buffer[offset++] = static_cast<char>((buffer >> 16) & 0xFF);
      res.buffer[offset++] = static_cast<char>((buffer >> 8) & 0xFF);
      res.buffer[offset++] = static_cast<char>(buffer & 0xFF);
      count = 0;
      buffer = 0;
    }
  }

  if (count == 2) {
    res.buffer[offset++] = static_cast<char>((buffer >> 4) & 0xFF);
  } else if (count == 3) {
    res.buffer[offset++] = static_cast<char>((buffer >> 10) & 0xFF);
    res.buffer[offset++] = static_cast<char>((buffer >> 2) & 0xFF);
  }

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列リテラルをBase64デコードする
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto 変換文字列
 */
template <size_t N>
[[nodiscard]] auto consteval base64_decode(char const (&str)[N]) noexcept {
  return base64_decode(FrozenString{str});
}

/**
 * @brief 文字列をBase64デコードする（NTTP版・正確なバッファサイズ）
 *
 * @tparam Str 変換対象 of FrozenString（NTTPとして渡す）
 * @return auto 変換文字列
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval base64_decode() noexcept {
  constexpr auto OUT_LEN = detail::count_base64_decoded_size(Str);
  auto res = FrozenString<OUT_LEN + 1>{};
  auto const s = Str.sv();
  auto offset = 0uz;
  auto count = 0uz;
  std::uint32_t buffer = 0;

  for (auto const c : s) {
    if (c == '=') break;
    auto const val = detail::base64_char_to_value(c);
    if (val == 255) continue;

    buffer = (buffer << 6) | val;
    count++;

    if (count == 4) {
      res.buffer[offset++] = static_cast<char>((buffer >> 16) & 0xFF);
      res.buffer[offset++] = static_cast<char>((buffer >> 8) & 0xFF);
      res.buffer[offset++] = static_cast<char>(buffer & 0xFF);
      count = 0;
      buffer = 0;
    }
  }

  if (count == 2) {
    res.buffer[offset++] = static_cast<char>((buffer >> 4) & 0xFF);
  } else if (count == 3) {
    res.buffer[offset++] = static_cast<char>((buffer >> 10) & 0xFF);
    res.buffer[offset++] = static_cast<char>((buffer >> 2) & 0xFF);
  }

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

// ===== HTML entity encode/decode =====

namespace detail {

/**
 * @brief HTML 名前付きエンティティを1文字に変換する
 *
 * @param sv 入力文字列
 * @param i '&' の位置
 * @param consumed 消費した文字数（出力）
 * @return char 変換後の文字。未知のエンティティの場合は '\0'
 */
auto constexpr html_decode_entity(std::string_view sv, size_t i, size_t& consumed) noexcept -> char {
  // &lt; - 4 chars
  if (i + 3 < sv.size() && sv[i + 1] == 'l' && sv[i + 2] == 't' && sv[i + 3] == ';') {
    consumed = 4;
    return '<';
  }
  // &gt; - 4 chars
  if (i + 3 < sv.size() && sv[i + 1] == 'g' && sv[i + 2] == 't' && sv[i + 3] == ';') {
    consumed = 4;
    return '>';
  }
  // &amp; - 5 chars
  if (i + 4 < sv.size() && sv[i + 1] == 'a' && sv[i + 2] == 'm' && sv[i + 3] == 'p' && sv[i + 4] == ';') {
    consumed = 5;
    return '&';
  }
  // &quot; - 6 chars
  if (i + 5 < sv.size() && sv[i + 1] == 'q' && sv[i + 2] == 'u' && sv[i + 3] == 'o' && sv[i + 4] == 't' && sv[i + 5] == ';') {
    consumed = 6;
    return '"';
  }
  // &#39; - 5 chars (decimal for ')
  if (i + 4 < sv.size() && sv[i + 1] == '#' && sv[i + 2] == '3' && sv[i + 3] == '9' && sv[i + 4] == ';') {
    consumed = 5;
    return '\'';
  }
  // &#x27; - 6 chars (hex for ')
  if (i + 5 < sv.size() && sv[i + 1] == '#' && sv[i + 2] == 'x' && sv[i + 3] == '2' && sv[i + 4] == '7' && sv[i + 5] == ';') {
    consumed = 6;
    return '\'';
  }
  return '\0';
}

} // namespace detail

/**
 * @brief 文字列をHTMLエンコードする
 *
 * 以下の文字を HTML エンティティに変換します:
 * - `&` → `&amp;`
 * - `<` → `&lt;`
 * - `>` → `&gt;`
 * - `"` → `&quot;`
 * - `'` → `&#39;`
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto エンコード後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval html_encode(FrozenString<N> const& str) noexcept {
  constexpr auto OUT_CAP = 6 * (N > 0 ? N - 1 : 0) + 1;
  auto res = FrozenString<OUT_CAP>{};
  auto offset = 0uz;

  for (auto const c : str.sv()) {
    switch (c) {
    case '&':
      res.buffer[offset++] = '&'; res.buffer[offset++] = 'a';
      res.buffer[offset++] = 'm'; res.buffer[offset++] = 'p';
      res.buffer[offset++] = ';';
      break;
    case '<':
      res.buffer[offset++] = '&'; res.buffer[offset++] = 'l';
      res.buffer[offset++] = 't'; res.buffer[offset++] = ';';
      break;
    case '>':
      res.buffer[offset++] = '&'; res.buffer[offset++] = 'g';
      res.buffer[offset++] = 't'; res.buffer[offset++] = ';';
      break;
    case '"':
      res.buffer[offset++] = '&'; res.buffer[offset++] = 'q';
      res.buffer[offset++] = 'u'; res.buffer[offset++] = 'o';
      res.buffer[offset++] = 't'; res.buffer[offset++] = ';';
      break;
    case '\'':
      res.buffer[offset++] = '&'; res.buffer[offset++] = '#';
      res.buffer[offset++] = '3'; res.buffer[offset++] = '9';
      res.buffer[offset++] = ';';
      break;
    default:
      res.buffer[offset++] = c;
      break;
    }
  }

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列リテラルをHTMLエンコードする
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto エンコード後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval html_encode(char const (&str)[N]) noexcept {
  return html_encode(FrozenString{str});
}

/**
 * @brief 文字列をHTMLエンコードする（NTTP版・正確なバッファサイズ）
 *
 * @tparam Str 変換対象の FrozenString（NTTPとして渡す）
 * @return auto エンコード後の文字列
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval html_encode() noexcept {
  return shrink_to_fit<html_encode(Str)>();
}

/**
 * @brief 文字列をHTMLデコードする
 *
 * HTML エンティティを元の文字に戻します:
 * - `&amp;` → `&`
 * - `&lt;` → `<`
 * - `&gt;` → `>`
 * - `&quot;` → `"`
 * - `&#39;` → `'`
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto デコード後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval html_decode(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto const s = str.sv();

  for (auto i = 0uz; i < s.size(); ++i) {
    if (s[i] == '&') {
      size_t consumed = 0;
      auto const decoded = detail::html_decode_entity(s, i, consumed);
      if (decoded != '\0') {
        res.buffer[offset++] = decoded;
        i += consumed - 1;
        continue;
      }
    }
    res.buffer[offset++] = s[i];
  }

  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列リテラルをHTMLデコードする
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return auto デコード後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval html_decode(char const (&str)[N]) noexcept {
  return html_decode(FrozenString{str});
}

/**
 * @brief 文字列をHTMLデコードする（NTTP版・正確なバッファサイズ）
 *
 * @tparam Str 変換対象の FrozenString（NTTPとして渡す）
 * @return auto デコード後の文字列
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval html_decode() noexcept {
  return shrink_to_fit<html_decode(Str)>();
}

/**
 * @brief UTF-8 文字列のコードポイント数を計算する
 *
 * UTF-8 エンコードされた文字列を走査し、有効なコードポイントの個数を返します。
 * 不正なバイト列は1バイトを1コードポイントとしてカウントします（フェイルソフト）。
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return size_t コードポイント数
 */
template <size_t N>
[[nodiscard]] auto consteval utf8_length(FrozenString<N> const& str) noexcept -> size_t {
  auto count = 0uz;
  auto i = 0uz;

  while (i < str.length) {
    auto const c = static_cast<unsigned char>(str.buffer[i]);
    if (c < 0x80) {
      i += 1;
    } else if (c < 0xC0) {
      // Continuation byte without leading byte - invalid, skip
      i += 1;
    } else if (c < 0xE0) {
      i += 2;
    } else if (c < 0xF0) {
      i += 3;
    } else if (c < 0xF8) {
      i += 4;
    } else {
      // Invalid byte, skip
      i += 1;
    }
    ++count;
  }

  return count;
}

/**
 * @brief 文字列リテラルのUTF-8コードポイント数を計算する
 *
 * @tparam N 文字列リテラルの長さ (終端文字'\0'を含む)
 * @param str 対象文字列リテラル
 * @return size_t コードポイント数
 */
template <size_t N>
[[nodiscard]] auto consteval utf8_length(char const (&str)[N]) noexcept -> size_t {
  return utf8_length(FrozenString{str});
}

/**
 * @brief UTF-8 文字列のコードポイント数を計算する（NTTP版）
 *
 * @tparam Str 変換対象の FrozenString（NTTPとして渡す）
 * @return size_t コードポイント数
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval utf8_length() noexcept -> size_t {
  return utf8_length(Str);
}

/**
 * @brief UTF-8 文字列を符号点単位で部分切り出しする
 *
 * @tparam Start 開始符号点インデックス
 * @tparam Count 切り出す符号点数
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return FrozenString<N> 切り出した文字列（バッファサイズは入力と同じ）
 */
template <size_t Start, size_t Count, size_t N>
[[nodiscard]] auto consteval utf8_substr(FrozenString<N> const& str) noexcept -> FrozenString<N> {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto emitted = 0uz;
  while (i < str.length && emitted < Start + Count) {
    size_t consumed = 0;
    std::uint32_t codepoint = 0;
    (void)detail::utf8_decode_at(str.sv(), i, consumed, codepoint);
    if (emitted >= Start) {
      for (auto j = 0uz; j < consumed; ++j) {
        res.buffer[offset++] = str.buffer[i + j];
      }
    }
    i += consumed;
    ++emitted;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief UTF-8 文字列を符号点単位で部分切り出しする（文字列リテラル版）
 */
template <size_t Start, size_t Count, size_t N>
[[nodiscard]] auto consteval utf8_substr(char const (&str)[N]) noexcept -> FrozenString<N> {
  return utf8_substr<Start, Count>(FrozenString{str});
}

/**
 * @brief UTF-8 文字列を符号点単位で部分切り出しする（NTTP版）
 *
 * @tparam Start 開始符号点インデックス
 * @tparam Count 切り出す符号点数
 * @tparam Str 対象文字列（FrozenString NTTP）
 * @return FrozenString<Str.length> 切り出した文字列
 */
template <size_t Start, size_t Count, auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval utf8_substr() noexcept -> FrozenString<Str.length> {
  return utf8_substr<Start, Count>(Str);
}

/**
 * @brief UTF-8 文字列を符号点単位で逆順にする
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return FrozenString<N> 逆順にした文字列
 */
template <size_t N>
[[nodiscard]] auto consteval utf8_reverse(FrozenString<N> const& str) noexcept -> FrozenString<N> {
  auto const codepoints = detail::utf8_codepoints(str);
  auto count = 0uz;
  auto const all = detail::utf8_codepoints(str, count);
  (void)all;
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  for (auto i = count; i-- > 0;) {
    auto bytes = std::array<char, 4>{};
    size_t length = 0;
    detail::utf8_encode_codepoint(codepoints[i], bytes, length);
    for (auto j = 0uz; j < length; ++j) {
      res.buffer[offset++] = bytes[j];
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief UTF-8 文字列を符号点単位で逆順にする（文字列リテラル版）
 */
template <size_t N>
[[nodiscard]] auto consteval utf8_reverse(char const (&str)[N]) noexcept -> FrozenString<N> {
  return utf8_reverse(FrozenString{str});
}

/**
 * @brief UTF-8 文字列を符号点単位で逆順にする（NTTP版）
 *
 * @tparam Str 対象文字列（FrozenString NTTP）
 * @return FrozenString<Str.length> 逆順にした文字列
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval utf8_reverse() noexcept -> FrozenString<Str.length> {
  return utf8_reverse(Str);
}

/**
 * @brief Unicode 符号点を UTF-8 文字列に変換する
 *
 * @tparam Codepoint 変換する符号点（0〜0x10FFFF）
 * @return FrozenString<5> UTF-8 エンコードした文字列（最大 4 バイト + 終端）
 */
template <auto Codepoint>
[[nodiscard]] auto consteval codepoint_to_utf8() noexcept -> FrozenString<4 + 1> {
  auto out = std::array<char, 4>{};
  auto length = 0uz;
  detail::utf8_encode_codepoint(static_cast<std::uint32_t>(Codepoint), out, length);
  auto res = FrozenString<5>{};
  for (auto i = 0uz; i < length; ++i) {
    res.buffer[i] = out[i];
  }
  res.buffer[length] = '\0';
  res.length = length;
  return res;
}

/**
 * @brief 文字列を C エスケープシーケンスに変換する
 *
 * 制御文字（\a \b \f \n \r \t \v \0）・引用符・バックスラッシュを 2 文字のエスケープに変換する。
 * それ以外の非表示文字・不正 UTF-8 バイトは \xHH（16 進大文字）、有効な UTF-8 シーケンスは
 * 符号点に復号して \uXXXX（BMP）または \UXXXXXXXX（補助）に変換する。
 *
 * @tparam N 入力バッファ長（終端含む）
 * @param str 対象文字列
 * @return FrozenString<4 * N + 1> エスケープ後の文字列（最悪 1 バイトが \xNN の 4 文字になるため）
 */
template <size_t N>
[[nodiscard]] auto consteval escape_c(FrozenString<N> const& str) noexcept -> FrozenString<4 * N + 1> {
  auto res = FrozenString<4 * N + 1>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto emit = [&](char escaped) {
    res.buffer[offset++] = '\\';
    res.buffer[offset++] = escaped;
  };
  auto emit_hex_digits = [&](std::uint32_t value, size_t const digits) {
    for (auto shift = static_cast<int>(digits * 4 - 4); shift >= 0; shift -= 4) {
      res.buffer[offset++] = detail::value_to_hex_digit((value >> shift) & 0xF);
    }
  };
  while (i < str.length) {
    auto const c = str.buffer[i];
    switch (c) {
    case '\a': emit('a'); ++i; break;
    case '\b': emit('b'); ++i; break;
    case '\f': emit('f'); ++i; break;
    case '\v': emit('v'); ++i; break;
    case '\n': emit('n'); ++i; break;
    case '\r': emit('r'); ++i; break;
    case '\t': emit('t'); ++i; break;
    case '\0': emit('0'); ++i; break;
    case '\'': emit('\''); ++i; break;
    case '"': emit('"'); ++i; break;
    case '\\': emit('\\'); ++i; break;
    default: {
      auto const uc = static_cast<unsigned char>(c);
      if (uc >= 0x80) {
        auto const seq_len = detail::valid_utf8_seq_length(str.sv(), i);
        if (seq_len != 0) {
          std::uint32_t codepoint = 0;
          size_t consumed = 0;
          (void)detail::utf8_decode_at(str.sv(), i, consumed, codepoint);
          if (codepoint <= 0xFFFF) {
            emit('u');
            emit_hex_digits(codepoint, 4);
          } else {
            emit('U');
            emit_hex_digits(codepoint, 8);
          }
          i += seq_len;
        } else {
          emit('x');
          emit_hex_digits(uc, 2);
          ++i;
        }
      } else if (c <= 0x1F || c == 0x7F) {
        emit('x');
        emit_hex_digits(uc, 2);
        ++i;
      } else {
        res.buffer[offset++] = c;
        ++i;
      }
      break;
    }
    }
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief 文字列を C エスケープシーケンスに変換する（文字列リテラル版）
 */
template <size_t N>
[[nodiscard]] auto consteval escape_c(char const (&str)[N]) noexcept -> FrozenString<4 * N + 1> {
  return escape_c(FrozenString{str});
}

/**
 * @brief escape_c の NTTP 版
 *
 * @tparam Str 対象文字列（FrozenString NTTP）
 * @return auto エスケープ後の文字列
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval escape_c() noexcept {
  return escape_c(FrozenString{Str});
}

/**
 * @brief C エスケープシーケンスを元の文字列に変換する
 *
 * \a \b \f \n \r \t \v \0 \\ \' \" に加え、\xHH（16 進 1〜2 桁）、\uXXXX（BMP 符号点）、
 * \UXXXXXXXX（補助符号点）を復号する。不正な \u/\U シーケンス（サロゲート・U+10FFFF 超え・
 * 非 16 進・桁不足）は元のシーケンスをそのまま保持する。
 *
 * @tparam N 入力バッファ長（終端含む）
 * @param str 対象文字列
 * @return FrozenString<N> アンエスケープ後の文字列（出力は入力長以下）
 */
template <size_t N>
[[nodiscard]] auto consteval unescape_c(FrozenString<N> const& str) noexcept -> FrozenString<N> {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  auto preserve = [&](char escaped, std::size_t const count) {
    res.buffer[offset++] = '\\';
    res.buffer[offset++] = escaped;
    for (auto k = 0uz; k < count; ++k) {
      res.buffer[offset++] = str.buffer[i + 1 + k];
    }
    i += count;
  };
  auto encode_codepoint = [&](std::uint32_t const codepoint) {
    auto out = std::array<char, 4>{};
    size_t len = 0;
    detail::utf8_encode_codepoint(codepoint, out, len);
    for (auto k = 0uz; k < len; ++k) {
      res.buffer[offset++] = out[k];
    }
  };
  while (i < str.length) {
    auto const c = str.buffer[i];
    if (c == '\\' && i + 1 < str.length) {
      auto const escaped = str.buffer[++i];
      switch (escaped) {
      case 'a': res.buffer[offset++] = '\a'; break;
      case 'b': res.buffer[offset++] = '\b'; break;
      case 'f': res.buffer[offset++] = '\f'; break;
      case 'v': res.buffer[offset++] = '\v'; break;
      case 'n': res.buffer[offset++] = '\n'; break;
      case 'r': res.buffer[offset++] = '\r'; break;
      case 't': res.buffer[offset++] = '\t'; break;
      case '0': res.buffer[offset++] = '\0'; break;
      case '\\': res.buffer[offset++] = '\\'; break;
      case '\'': res.buffer[offset++] = '\''; break;
      case '"': res.buffer[offset++] = '"'; break;
      case 'x': {
        auto const r = detail::parse_hex_digits(str.sv(), i + 1, 2);
        if (!r.valid) {
          preserve('x', 0);
        } else {
          res.buffer[offset++] = static_cast<char>(r.value);
          i += r.count;
        }
        break;
      }
      case 'u': {
        auto const r = detail::parse_hex_digits(str.sv(), i + 1, 4);
        if (!r.valid || r.count != 4 || (r.value >= 0xD800 && r.value <= 0xDFFF)) {
          preserve('u', r.valid ? r.count : 0);
        } else {
          encode_codepoint(r.value);
          i += r.count;
        }
        break;
      }
      case 'U': {
        auto const r = detail::parse_hex_digits(str.sv(), i + 1, 8);
        if (!r.valid || r.count != 8 || r.value > 0x10FFFF) {
          preserve('U', r.valid ? r.count : 0);
        } else {
          encode_codepoint(r.value);
          i += r.count;
        }
        break;
      }
      default:
        res.buffer[offset++] = '\\';
        res.buffer[offset++] = escaped;
        break;
      }
    } else {
      res.buffer[offset++] = c;
    }
    ++i;
  }
  res.buffer[offset] = '\0';
  res.length = offset;
  return res;
}

/**
 * @brief C エスケープシーケンスを元の文字列に変換する（文字列リテラル版）
 */
template <size_t N>
[[nodiscard]] auto consteval unescape_c(char const (&str)[N]) noexcept -> FrozenString<N> {
  return unescape_c(FrozenString{str});
}

/**
 * @brief unescape_c の NTTP 版
 *
 * 不正な \u/\U シーケンス（サロゲート・U+10FFFF 超え）はコンパイルエラーになる。
 *
 * @tparam Str 対象文字列（FrozenString NTTP）
 * @return auto アンエスケープ後の文字列
 */
template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval unescape_c() noexcept {
  static_assert(!detail::has_invalid_codepoint_escape<Str>(),
    "frozenchars: unescape_c: invalid \\u/\\U code point (surrogate or out of range)");
  return unescape_c(FrozenString{Str});
}

} // namespace frozenchars
