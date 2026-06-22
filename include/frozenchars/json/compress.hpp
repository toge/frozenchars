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

namespace detail {
  template <FrozenString Input>
  struct crush_compress_helper {
    static constexpr auto compressed = compress<Input>();
    static constexpr auto result = crush<compressed>();
  };
}

template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto crush_compress() {
  return detail::crush_compress_helper<Input>::result;
}

} // namespace frozenchars::json
