#pragma once

#include "frozen_string.hpp"
#include "freeze.hpp"
#include "detail/pipe.hpp"

#if __has_include (<ctll/fixed_string.hpp>)
#include <ctll/fixed_string.hpp>

namespace frozenchars {

namespace ops {

struct to_ctre_t : frozenchars::detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(frozenchars::FrozenString<N> const& str) const {
    if (str.length != N - 1) {
      throw "FrozenStringのactual lengthがbufferサイズN-1と一致しません。to_ctre<expr>() を使用してください。";
    }
    return ctll::fixed_string<N - 1>(ctll::construct_from_pointer, str.data());
  }
};
inline constexpr to_ctre_t to_ctre{};

} // namespace ops

template <auto S>
  requires detail::is_frozen_string_v<decltype(S)>
consteval auto to_ctre() noexcept {
  return ctll::fixed_string<S.length>(ctll::construct_from_pointer, S.data());
}

} // namespace frozenchars

#endif // __has_include (<ctll/fixed_string.hpp>)
