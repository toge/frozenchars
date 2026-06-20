#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace frozenchars::json::detail {

inline constexpr auto JSON_CRUSH_DELIMITER = char16_t{0x0001};

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

inline constexpr auto decode_utf8 = [](std::string_view const input, size_t& index) -> char32_t {
  auto const lead = static_cast<uint8_t>(input[index]);
  if (lead < 0x80) { return static_cast<char32_t>(input[index++]); }
  if ((lead & 0xE0) == 0xC0) {
    auto const c1 = static_cast<uint8_t>(input[index + 1]);
    if ((c1 & 0xC0) != 0x80) throw std::runtime_error("invalid UTF-8");
    index += 2;
    return (static_cast<char32_t>(lead & 0x1F) << 6) | static_cast<char32_t>(c1 & 0x3F);
  }
  if ((lead & 0xF0) == 0xE0) {
    auto const c1 = static_cast<uint8_t>(input[index + 1]);
    auto const c2 = static_cast<uint8_t>(input[index + 2]);
    if ((c1 & 0xC0) != 0x80 || (c2 & 0xC0) != 0x80) throw std::runtime_error("invalid UTF-8");
    index += 3;
    return (static_cast<char32_t>(lead & 0x0F) << 12) |
           (static_cast<char32_t>(c1 & 0x3F) << 6) |
           static_cast<char32_t>(c2 & 0x3F);
  }
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

inline constexpr auto utf8_to_utf16 = [](std::string_view const input) -> std::u16string {
  auto output = std::u16string{};
  output.reserve(input.size());
  size_t idx = 0;
  while (idx < input.size()) {
    auto const cp = decode_utf8(input, idx);
    if (cp < 0x10000) {
      output.push_back(static_cast<char16_t>(cp));
    } else {
      output.push_back(static_cast<char16_t>(0xD800 + ((cp - 0x10000) >> 10)));
      output.push_back(static_cast<char16_t>(0xDC00 + ((cp - 0x10000) & 0x3FF)));
    }
  }
  return output;
};

inline constexpr auto utf16_to_utf8 = [](std::u16string_view const input) -> std::string {
  auto output = std::string{};
  output.reserve(input.size());
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
