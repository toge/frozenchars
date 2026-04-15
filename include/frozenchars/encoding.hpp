#pragma once

#include "frozen_string.hpp"
#include "detail/char_utils.hpp"
#include <cstddef>
#include <cstdint>

namespace frozenchars {

/**
 * @brief 文字列をURLエンコードする
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return auto 変換文字列 (バッファサイズは最悪ケース 3 * length + 1)
 */
template <size_t N>
auto consteval url_encode(FrozenString<N> const& str) noexcept {
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
auto consteval url_encode(char const (&str)[N]) noexcept {
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
auto consteval url_encode() noexcept {
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
auto consteval url_decode(FrozenString<N> const& str) noexcept {
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
auto consteval url_decode(char const (&str)[N]) noexcept {
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
auto consteval url_decode() noexcept {
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
auto consteval base64_encode(FrozenString<N> const& str) noexcept {
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
auto consteval base64_encode(char const (&str)[N]) noexcept {
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
auto consteval base64_encode() noexcept {
  return base64_encode(Str);
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
auto consteval base64_decode(FrozenString<N> const& str) noexcept {
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
auto consteval base64_decode(char const (&str)[N]) noexcept {
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
auto consteval base64_decode() noexcept {
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

} // namespace frozenchars
