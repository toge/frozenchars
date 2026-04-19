#pragma once

#include "frozen_string.hpp"
#include "string_ops.hpp"
#include "case_conv.hpp"
#include "multiline.hpp"
#include "encoding.hpp"
#include <array>
#include <cstddef>
#include <string_view>

namespace frozenchars::ops {

struct trim_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::trim(str);
  }
};

struct ltrim_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::ltrim(str);
  }
};

struct rtrim_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::rtrim(str);
  }
};

struct toupper_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::toupper(str);
  }
};

struct tolower_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::tolower(str);
  }
};

struct substr_adaptor : detail::pipe_adaptor_tag {
  std::size_t pos;
  std::ptrdiff_t len;

  constexpr substr_adaptor(std::size_t p, std::ptrdiff_t l) noexcept
  : pos(p), len(l)
  {}

  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::substr(str, pos, len);
  }
};

struct capitalize_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::capitalize(str);
  }
};

struct to_snake_case_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_snake_case(str);
  }
};

struct to_camel_case_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_camel_case(str);
  }
};

struct to_pascal_case_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_pascal_case(str);
  }
};

struct remove_leading_spaces_adaptor : detail::pipe_adaptor_tag {
  size_t n;
  constexpr remove_leading_spaces_adaptor(size_t count = 0) noexcept : n(count) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_leading_spaces(str, n);
  }
  consteval auto operator()(size_t count) const noexcept {
    return remove_leading_spaces_adaptor{count};
  }
};

struct remove_comment_lines_adaptor : detail::pipe_adaptor_tag {
  std::string_view comment_seq;
  constexpr remove_comment_lines_adaptor(std::string_view seq = "#") noexcept : comment_seq(seq) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_comment_lines(str, comment_seq);
  }
};

struct remove_comments_adaptor : detail::pipe_adaptor_tag {
  std::string_view comment_seq;
  constexpr remove_comments_adaptor(std::string_view seq = "#") noexcept : comment_seq(seq) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_comments(str, comment_seq);
  }
};

struct remove_trailing_spaces_adaptor : detail::pipe_adaptor_tag {
  size_t n;
  constexpr remove_trailing_spaces_adaptor(size_t count = 0) noexcept : n(count) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_trailing_spaces(str, n);
  }
  consteval auto operator()(size_t count) const noexcept {
    return remove_trailing_spaces_adaptor{count};
  }
};

struct remove_range_comments_adaptor : detail::pipe_adaptor_tag {
  std::string_view start_seq;
  std::string_view end_seq;
  constexpr remove_range_comments_adaptor(std::string_view start, std::string_view end) noexcept
  : start_seq(start), end_seq(end)
  {}

  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_range_comments(str, start_seq, end_seq);
  }
};

struct join_lines_adaptor : detail::pipe_adaptor_tag {
  std::string_view sep;
  constexpr join_lines_adaptor(std::string_view s = "") noexcept : sep(s) {}

  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::join_lines(str, sep);
  }

  consteval auto operator()(std::string_view s) const noexcept {
    return join_lines_adaptor{s};
  }
};

struct trim_trailing_spaces_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::trim_trailing_spaces(str);
  }
};

struct remove_empty_lines_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_empty_lines(str);
  }
};

struct url_encode_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::url_encode(str);
  }
};

struct url_decode_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::url_decode(str);
  }
};

struct base64_encode_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::base64_encode(str);
  }
};

struct base64_decode_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::base64_decode(str);
  }
};

inline constexpr trim_adaptor trim{};
inline constexpr ltrim_adaptor ltrim{};
inline constexpr rtrim_adaptor rtrim{};
inline constexpr toupper_adaptor toupper{};
inline constexpr tolower_adaptor tolower{};
inline constexpr capitalize_adaptor capitalize{};
inline constexpr to_snake_case_adaptor to_snake_case{};
inline constexpr to_camel_case_adaptor to_camel_case{};
inline constexpr to_pascal_case_adaptor to_pascal_case{};
inline constexpr join_lines_adaptor join_lines{};
inline constexpr trim_trailing_spaces_adaptor trim_trailing_spaces{};
inline constexpr remove_empty_lines_adaptor remove_empty_lines{};
inline constexpr url_encode_adaptor url_encode{};
inline constexpr url_decode_adaptor url_decode{};
inline constexpr base64_encode_adaptor base64_encode{};
inline constexpr base64_decode_adaptor base64_decode{};
inline constexpr remove_leading_spaces_adaptor remove_leading_spaces{};
inline constexpr remove_trailing_spaces_adaptor remove_trailing_spaces{};

consteval auto substr(std::size_t pos, std::ptrdiff_t len) noexcept {
  return substr_adaptor{pos, len};
}

consteval auto remove_comment_lines(std::string_view comment_seq = "#") noexcept {
  return remove_comment_lines_adaptor{comment_seq};
}

consteval auto remove_comments(std::string_view comment_seq = "#") noexcept {
  return remove_comments_adaptor{comment_seq};
}

consteval auto remove_range_comments(std::string_view start_seq, std::string_view end_seq) noexcept {
  return remove_range_comments_adaptor{start_seq, end_seq};
}

template <FrozenString Delim>
struct join_adaptor : detail::pipe_adaptor_tag {
  template <size_t ElemN, size_t Count>
  consteval auto operator()(std::array<FrozenString<ElemN>, Count> const& arr) const noexcept {
    return frozenchars::join<Delim>(arr);
  }
};

template <FrozenString Delim>
inline constexpr join_adaptor<Delim> join{};

template <size_t ElemN, size_t Count, FrozenString Delim>
consteval auto operator|(std::array<FrozenString<ElemN>, Count> const& lhs,
                         join_adaptor<Delim> const& rhs) noexcept(noexcept(rhs(lhs))) {
  return rhs(lhs);
}

template <size_t Width, char Fill = ' '>
struct pad_left_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::pad_left<Width, Fill>(str);
  }
};

template <size_t Width, char Fill = ' '>
inline constexpr pad_left_adaptor<Width, Fill> pad_left{};

template <size_t Width, char Fill = ' '>
struct pad_right_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::pad_right<Width, Fill>(str);
  }
};

template <size_t Width, char Fill = ' '>
inline constexpr pad_right_adaptor<Width, Fill> pad_right{};

template <FrozenString From, FrozenString To>
struct replace_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::replace<From, To>(str);
  }
};

template <FrozenString From, FrozenString To>
inline constexpr replace_adaptor<From, To> replace{};

template <FrozenString From, FrozenString To>
struct replace_all_adaptor : detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::replace_all<From, To>(str);
  }
};

template <FrozenString From, FrozenString To>
inline constexpr replace_all_adaptor<From, To> replace_all{};

} // namespace frozenchars::ops
