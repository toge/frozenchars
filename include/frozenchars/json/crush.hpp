#pragma once

#include <algorithm>
#include <string>
#include <string_view>

#include "detail/crush_detail.hpp"
#include "frozenchars/string.hpp"
#include "frozenchars/literals.hpp"

namespace frozenchars::json {

namespace detail {
  template <FrozenString Input>
  [[nodiscard]] consteval auto crushed_size() -> size_t {
    auto const input_sv = Input.sv();
    auto u16 = frozenchars::json::detail::utf8_to_utf16(input_sv);
    u16.erase(std::remove(u16.begin(), u16.end(), frozenchars::json::detail::JSON_CRUSH_DELIMITER), u16.end());
    auto const swapped = frozenchars::json::detail::json_crush_swap<char16_t>(u16, true);
    auto const result = frozenchars::json::detail::js_crush_utf16<char16_t>(swapped);
    auto output_u16 = result.crushed;
    if (!result.split.empty()) {
      output_u16.push_back(frozenchars::json::detail::JSON_CRUSH_DELIMITER);
      output_u16.append(result.split);
    }
    output_u16.push_back(u'_');
    auto const output_u8 = frozenchars::json::detail::utf16_to_utf8(output_u16);
    return output_u8.size() + 1;
  }
}

template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto crush() -> FrozenString<detail::crushed_size<Input>()> {
  constexpr auto N = detail::crushed_size<Input>();
  auto const input_sv = Input.sv();

  auto u16 = frozenchars::json::detail::utf8_to_utf16(input_sv);
  u16.erase(std::remove(u16.begin(), u16.end(), frozenchars::json::detail::JSON_CRUSH_DELIMITER), u16.end());

  auto const swapped = frozenchars::json::detail::json_crush_swap<char16_t>(u16, true);

  auto const result = frozenchars::json::detail::js_crush_utf16<char16_t>(swapped);

  auto output_u16 = result.crushed;
  if (!result.split.empty()) {
    output_u16.push_back(frozenchars::json::detail::JSON_CRUSH_DELIMITER);
    output_u16.append(result.split);
  }
  output_u16.push_back(u'_');

  auto const output_u8 = frozenchars::json::detail::utf16_to_utf8(output_u16);

  auto res = FrozenString<N>{};
  for (size_t i = 0; i < output_u8.size() && i < N - 1; ++i) {
    res.buffer[i] = output_u8[i];
  }
  res.buffer[output_u8.size()] = '\0';
  res.length = output_u8.size();
  return res;
}

} // namespace frozenchars::json
