#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace frozenchars::json::detail {

/// JSON-Crush で crush 結果と分割文字列を区切るデリミタ（制御文字 U+0001）
inline constexpr auto JSON_CRUSH_DELIMITER = char16_t{0x0001};

/**
 * @brief JSON-Crush の置換に使用可能な UTF-16 文字テーブル
 *
 * @details URI エンコードしても安全な文字を優先度順に集めた 222 要素の配列。
 * 前半で URI 非安全（=エンコードで長くなる）文字を降順に、後半で URI 安全文字を
 * 降順に格納する。'\\'(92) は特殊扱いのため除外する。
 */
inline constexpr auto replacement_characters_utf16 = [] {
  std::array<char16_t, 222> chars{};
  size_t idx = 0;
  for (int i = 254; i >= 32; --i) {
    if (i == 92) continue;
    bool in_uri_safe = false;
    std::u16string_view const unescaped = u"-_.!~*'()";
    in_uri_safe = (i >= 48 && i <= 57) || (i >= 65 && i <= 90) || (i >= 97 && i <= 122) ||
                  unescaped.find(static_cast<char16_t>(i)) != std::u16string_view::npos;
    if (!in_uri_safe) chars[idx++] = static_cast<char16_t>(i);
  }
  for (int i = 126; i > 0; --i) {
    bool in_uri_safe = (i >= 48 && i <= 57) || (i >= 65 && i <= 90) || (i >= 97 && i <= 122);
    std::u16string_view const unescaped = u"-_.!~*'()";
    in_uri_safe = in_uri_safe || unescaped.find(static_cast<char16_t>(i)) != std::u16string_view::npos;
    if (in_uri_safe) chars[idx++] = static_cast<char16_t>(i);
  }
  return chars;
}();

/**
 * @brief UTF-8 バイト列から1つのコードポイントをデコードする
 *
 * @param input UTF-8 バイト列
 * @param index 開始位置。デコードしたバイト数だけ進める（1〜4バイト）
 * @return char32_t デコードしたコードポイント
 * @throw std::runtime_error 不正な UTF-8 シーケンスの場合
 */
inline constexpr auto decode_utf8 = [](std::string_view const input, size_t& index) -> char32_t {
  auto const lead = static_cast<uint8_t>(input[index]);
  // 1バイト（ASCII）
  if (lead < 0x80) { return static_cast<char32_t>(input[index++]); }
  // 2バイトシーケンス
  if ((lead & 0xE0) == 0xC0) {
    auto const c1 = static_cast<uint8_t>(input[index + 1]);
    if ((c1 & 0xC0) != 0x80) throw std::runtime_error("invalid UTF-8");
    index += 2;
    return (static_cast<char32_t>(lead & 0x1F) << 6) | static_cast<char32_t>(c1 & 0x3F);
  }
  // 3バイトシーケンス
  if ((lead & 0xF0) == 0xE0) {
    auto const c1 = static_cast<uint8_t>(input[index + 1]);
    auto const c2 = static_cast<uint8_t>(input[index + 2]);
    if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) throw std::runtime_error("invalid UTF-8");
    index += 3;
    return (static_cast<char32_t>(lead & 0x0F) << 12) |
           (static_cast<char32_t>(c1 & 0x3F) << 6) |
           static_cast<char32_t>(c2 & 0x3F);
  }
  // 4バイトシーケンス
  if ((lead & 0xF8) == 0xF0) {
    auto const c1 = static_cast<uint8_t>(input[index + 1]);
    auto const c2 = static_cast<uint8_t>(input[index + 2]);
    auto const c3 = static_cast<uint8_t>(input[index + 3]);
    if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80)
      throw std::runtime_error("invalid UTF-8");
    index += 4;
    return (static_cast<char32_t>(lead & 0x07) << 18) |
           (static_cast<char32_t>(c1 & 0x3F) << 12) |
           (static_cast<char32_t>(c2 & 0x3F) << 6) |
           static_cast<char32_t>(c3 & 0x3F);
  }
  throw std::runtime_error("invalid UTF-8");
};

/**
 * @brief UTF-8 文字列を UTF-16 文字列に変換する
 *
 * @param input UTF-8 バイト列
 * @return std::u16string 変換した UTF-16 文字列（BMP 外はサロゲートペアで表現）
 * @throw std::runtime_error 不正な UTF-8 シーケンスの場合
 */
inline constexpr auto utf8_to_utf16 = [](std::string_view const input) -> std::u16string {
  auto output = std::u16string{};
  output.reserve(input.size());
  size_t idx = 0;
  while (idx < input.size()) {
    auto const cp = decode_utf8(input, idx);
    if (cp < 0x10000) {
      output.push_back(static_cast<char16_t>(cp));
    } else {
      // BMP 外はサロゲートペアに分割

      output.push_back(static_cast<char16_t>(0xD800 + ((cp - 0x10000) >> 10)));
      output.push_back(static_cast<char16_t>(0xDC00 + ((cp - 0x10000) & 0x3FF)));
    }
  }
  return output;
};

/**
 * @brief UTF-16 文字列を UTF-8 文字列に変換する
 *
 * @param input UTF-16 文字列
 * @return std::string 変換した UTF-8 バイト列
 * @throw std::runtime_error 対になっていないサロゲートがある場合
 */
inline constexpr auto utf16_to_utf8 = [](std::u16string_view const input) -> std::string {
  auto output = std::string{};
  output.reserve(input.size());
  // 1つのコードポイントを UTF-8 バイト列として追記する
  auto append_utf8 = [&](char32_t cp) {
    if (cp < 0x80) {
      output.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
      output.push_back(static_cast<char>(0xC0 | (cp >> 6)));
      output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
      output.push_back(static_cast<char>(0xE0 | (cp >> 12)));
      output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
      output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
      output.push_back(static_cast<char>(0xF0 | (cp >> 18)));
      output.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
      output.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
      output.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
  };
  for (size_t i = 0; i < input.size(); ++i) {
    auto const v = input[i];
    // 上位サロゲートなら次の下位サロゲートと合成してコードポイントを復元
    if (v >= 0xD800 && v <= 0xDBFF) {
      if (i + 1 >= input.size() || input[i + 1] < 0xDC00 || input[i + 1] > 0xDFFF)
        throw std::runtime_error("invalid UTF-16");
      auto const cp = 0x10000 + ((static_cast<char32_t>(v - 0xD800) << 10) | (input[i + 1] - 0xDC00));
      append_utf8(cp);
      ++i;
    } else {
      append_utf8(static_cast<char32_t>(v));
    }
  }
  return output;
};

} // namespace frozenchars::json::detail
