#pragma once

#include <concepts>
#include <cstddef>
#include <type_traits>

namespace frozenchars {

template <size_t N>
struct FrozenString;

namespace detail {

struct pipe_adaptor_tag {};

template <typename T>
concept PipeAdaptor = std::derived_from<std::remove_cvref_t<T>, pipe_adaptor_tag>;

} // namespace detail

template <size_t N, detail::PipeAdaptor Adaptor>
auto consteval operator|(FrozenString<N> const& lhs, Adaptor const& rhs) noexcept(noexcept(rhs(lhs))) {
  return rhs(lhs);
}

} // namespace frozenchars
