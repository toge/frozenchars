#pragma once

#include "frozen_string.hpp"
#include <cstddef>
#if defined(__has_include) && __has_include(<format>)
#  include <format>
#endif
#include <string_view>

#ifdef __cpp_lib_format
namespace std {

template <size_t N>
struct formatter<frozenchars::FrozenString<N>, char> {
  formatter<std::string_view, char> delegate_{};

  constexpr auto parse(format_parse_context& ctx) {
    return delegate_.parse(ctx);
  }

  template <typename FormatContext>
  auto format(frozenchars::FrozenString<N> const& value, FormatContext& ctx) const {
    return delegate_.format(value.sv(), ctx);
  }
};

} // namespace std
#endif
