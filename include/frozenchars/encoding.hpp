#pragma once

#include <cstddef>
#include <cstdint>

#include "string.hpp"
#include "string_ops.hpp"
#include "detail/char_utils.hpp"

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

} // namespace frozenchars
