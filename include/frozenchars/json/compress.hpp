#pragma once

#include <string>
#include <string_view>

#include "detail/compress_detail.hpp"
#include "crush.hpp"
#include "../string.hpp"
#include "../literals.hpp"

namespace frozenchars::json {

namespace detail {
  template <FrozenString Input>
  [[nodiscard]] consteval auto compressed_size() -> size_t {
    auto const parsed = frozenchars::json::detail::parse_json(Input.sv());
    auto const result = frozenchars::json::detail::compress_to_string(parsed);
    return result.size() + 1;
  }
}

template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto compress() -> FrozenString<detail::compressed_size<Input>()> {
  constexpr auto N = detail::compressed_size<Input>();
  auto const parsed = frozenchars::json::detail::parse_json(Input.sv());
  auto const result = frozenchars::json::detail::compress_to_string(parsed);

  auto res = FrozenString<N>{};
  for (size_t i = 0; i < result.size() && i < N - 1; ++i) {
    res.buffer[i] = result[i];
  }
  res.buffer[result.size()] = '\0';
  res.length = result.size();
  return res;
}

template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto crush_compress() {
  // compress
  auto const parsed = frozenchars::json::detail::parse_json(Input.sv());
  auto const compressed_str = frozenchars::json::detail::compress_to_string(parsed);

  // crush
  auto u16 = frozenchars::json::detail::utf8_to_utf16(compressed_str);
  u16.erase(std::remove(u16.begin(), u16.end(), frozenchars::json::detail::JSON_CRUSH_DELIMITER), u16.end());
  auto const swapped = frozenchars::json::detail::json_crush_swap<char16_t>(u16, true);
  auto const crush_result = frozenchars::json::detail::js_crush_utf16<char16_t>(swapped);
  auto output_u16 = crush_result.crushed;
  if (!crush_result.split.empty()) {
    output_u16.push_back(frozenchars::json::detail::JSON_CRUSH_DELIMITER);
    output_u16.append(crush_result.split);
  }
  output_u16.push_back(u'_');
  auto const output_u8 = frozenchars::json::detail::utf16_to_utf8(output_u16);

  constexpr auto N = Input.length * 4 + 4;
  auto res = FrozenString<N>{};
  for (size_t i = 0; i < output_u8.size() && i < N - 1; ++i) {
    res.buffer[i] = output_u8[i];
  }
  res.buffer[output_u8.size()] = '\0';
  res.length = output_u8.size();
  return res;
}

} // namespace frozenchars::json
