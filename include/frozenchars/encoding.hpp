#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "string.hpp"
#include "string_ops.hpp"
#include "detail/char_utils.hpp"

namespace frozenchars::detail {

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

template <size_t N>
[[nodiscard]] auto consteval to_ascii(char const (&str)[N]) noexcept {
  return hex_encode(str);
}

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

template <size_t N>
[[nodiscard]] auto consteval from_ascii(char const (&str)[N]) noexcept {
  return hex_decode(str);
}

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

template <size_t Start, size_t Count, size_t N>
[[nodiscard]] auto consteval utf8_substr(char const (&str)[N]) noexcept -> FrozenString<N> {
  return utf8_substr<Start, Count>(FrozenString{str});
}

template <size_t Start, size_t Count, auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval utf8_substr() noexcept -> FrozenString<Str.length> {
  return utf8_substr<Start, Count>(Str);
}

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

template <size_t N>
[[nodiscard]] auto consteval utf8_reverse(char const (&str)[N]) noexcept -> FrozenString<N> {
  return utf8_reverse(FrozenString{str});
}

template <auto Str>
  requires detail::is_frozen_string_v<decltype(Str)>
[[nodiscard]] auto consteval utf8_reverse() noexcept -> FrozenString<Str.length> {
  return utf8_reverse(Str);
}

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

template <size_t N>
[[nodiscard]] auto consteval escape_c(FrozenString<N> const& str) noexcept -> FrozenString<2 * N + 1> {
  auto res = FrozenString<2 * N + 1>{};
  auto offset = 0uz;
  for (auto const c : str.sv()) {
    switch (c) {
    case '\\':
      res.buffer[offset++] = '\\';
      res.buffer[offset++] = '\\';
      break;
    case '\n':
      res.buffer[offset++] = '\\';
      res.buffer[offset++] = 'n';
      break;
    case '\r':
      res.buffer[offset++] = '\\';
      res.buffer[offset++] = 'r';
      break;
    case '\t':
      res.buffer[offset++] = '\\';
      res.buffer[offset++] = 't';
      break;
    case '\0':
      res.buffer[offset++] = '\\';
      res.buffer[offset++] = '0';
      break;
    case '\'':
      res.buffer[offset++] = '\\';
      res.buffer[offset++] = '\'';
      break;
    case '"':
      res.buffer[offset++] = '\\';
      res.buffer[offset++] = '"';
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

template <size_t N>
[[nodiscard]] auto consteval escape_c(char const (&str)[N]) noexcept -> FrozenString<2 * N + 1> {
  return escape_c(FrozenString{str});
}

template <size_t N>
[[nodiscard]] auto consteval unescape_c(FrozenString<N> const& str) noexcept -> FrozenString<N> {
  auto res = FrozenString<N>{};
  auto offset = 0uz;
  auto i = 0uz;
  while (i < str.length) {
    auto const c = str.buffer[i];
    if (c == '\\' && i + 1 < str.length) {
      auto const escaped = str.buffer[++i];
      switch (escaped) {
      case 'n':
        res.buffer[offset++] = '\n';
        break;
      case 'r':
        res.buffer[offset++] = '\r';
        break;
      case 't':
        res.buffer[offset++] = '\t';
        break;
      case '0':
        res.buffer[offset++] = '\0';
        break;
      case '\\':
        res.buffer[offset++] = '\\';
        break;
      case '\'':
        res.buffer[offset++] = '\'';
        break;
      case '"':
        res.buffer[offset++] = '"';
        break;
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

template <size_t N>
[[nodiscard]] auto consteval unescape_c(char const (&str)[N]) noexcept -> FrozenString<N> {
  return unescape_c(FrozenString{str});
}

} // namespace frozenchars
