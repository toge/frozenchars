#pragma once

#include "frozen_string.hpp"
#include "string_ops.hpp"
#include "case_conv.hpp"
#include "multiline.hpp"
#include "encoding.hpp"
#include "regex_comment.hpp"
#include <array>
#include <cstddef>
#include <string_view>

namespace frozenchars::ops {

template <auto Pred = detail::is_space_char>
struct trim_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::trim_if<Pred>(str);
  }
  template <size_t N>
  consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::trim_if<Pred>(FrozenString{str});
  }
};

template <auto Pred = detail::is_space_char>
struct ltrim_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::ltrim_if<Pred>(str);
  }
  template <size_t N>
  consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::ltrim_if<Pred>(FrozenString{str});
  }
};

template <auto Pred = detail::is_space_char>
struct rtrim_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::rtrim_if<Pred>(str);
  }
  template <size_t N>
  consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::rtrim_if<Pred>(FrozenString{str});
  }
};

struct toupper_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::toupper(str);
  }
  template <size_t N>
  consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::toupper(FrozenString{str});
  }
};

struct tolower_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::tolower(str);
  }
  template <size_t N>
  consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::tolower(FrozenString{str});
  }
};

template <auto Pred = detail::is_space_char>
struct collapse_spaces_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::collapse_spaces_if<Pred>(str);
  }
  template <size_t N>
  consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::collapse_spaces_if<Pred>(FrozenString{str});
  }
};

struct substr_adaptor : pipe_adaptor_base {
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

struct capitalize_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::capitalize(str);
  }
};

struct to_snake_case_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_snake_case(str);
  }
};

struct to_camel_case_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_camel_case(str);
  }
};

struct to_pascal_case_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_pascal_case(str);
  }
};

template <auto Pred = detail::is_space_char>
struct remove_leading_spaces_adaptor : pipe_adaptor_base {
  size_t n;
  constexpr remove_leading_spaces_adaptor(size_t count = 0) noexcept : n(count) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_leading_spaces_if<Pred>(str, n);
  }
  consteval auto operator()(size_t count) const noexcept {
    return remove_leading_spaces_adaptor<Pred>{count};
  }
};

struct remove_comment_lines_adaptor : pipe_adaptor_base {
  std::string_view comment_seq;
  constexpr remove_comment_lines_adaptor(std::string_view seq = "#") noexcept : comment_seq(seq) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_comment_lines(str, comment_seq);
  }
};

struct remove_comments_adaptor : pipe_adaptor_base {
  std::string_view comment_seq;
  constexpr remove_comments_adaptor(std::string_view seq = "#") noexcept : comment_seq(seq) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_comments(str, comment_seq);
  }
};

template <auto Pred = detail::is_space_char>
struct remove_trailing_spaces_adaptor : pipe_adaptor_base {
  size_t n;
  constexpr remove_trailing_spaces_adaptor(size_t count = 0) noexcept : n(count) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_trailing_spaces_if<Pred>(str, n);
  }
  consteval auto operator()(size_t count) const noexcept {
    return remove_trailing_spaces_adaptor<Pred>{count};
  }
};

struct remove_range_comments_adaptor : pipe_adaptor_base {
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

struct remove_regex_comment_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_regex_comment(str);
  }

  template <size_t N>
  consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::remove_regex_comment(FrozenString{str});
  }
};

struct join_lines_adaptor : pipe_adaptor_base {
  std::string_view sep;
  constexpr join_lines_adaptor(std::string_view s = "") noexcept : sep(s) {}

  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const {
    return frozenchars::join_lines(str, sep);
  }

  consteval auto operator()(std::string_view s) const noexcept {
    return join_lines_adaptor{s};
  }
};

template <FrozenString Sep>
struct join_lines_nttp_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::join_lines<Sep>(str);
  }
};

struct trim_trailing_spaces_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::trim_trailing_spaces(str);
  }
};

struct remove_empty_lines_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_empty_lines(str);
  }
};

template <size_t M>
struct prefix_lines_adaptor : pipe_adaptor_base {
  FrozenString<M> prefix;
  constexpr prefix_lines_adaptor(FrozenString<M> p) noexcept : prefix(p) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::prefix_lines(str, prefix);
  }
};

template <size_t M>
struct postfix_lines_adaptor : pipe_adaptor_base {
  FrozenString<M> postfix;
  constexpr postfix_lines_adaptor(FrozenString<M> p) noexcept : postfix(p) {}
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::postfix_lines(str, postfix);
  }
};

template <size_t M1, size_t M2>
struct surround_lines_adaptor : pipe_adaptor_base {
  FrozenString<M1> prefix;
  FrozenString<M2> postfix;
  constexpr surround_lines_adaptor(FrozenString<M1> pr, FrozenString<M2> po) noexcept
  : prefix(pr), postfix(po) {}

  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::surround_lines(str, prefix, postfix);
  }
};

struct url_encode_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::url_encode(str);
  }
};

struct url_decode_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::url_decode(str);
  }
};

struct base64_encode_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::base64_encode(str);
  }
};

struct base64_decode_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::base64_decode(str);
  }
};

inline constexpr trim_adaptor<> trim{};
inline constexpr ltrim_adaptor<> ltrim{};
inline constexpr rtrim_adaptor<> rtrim{};
inline constexpr toupper_adaptor toupper{};
inline constexpr tolower_adaptor tolower{};
inline constexpr collapse_spaces_adaptor<> collapse_spaces{};
inline constexpr capitalize_adaptor capitalize{};
inline constexpr to_snake_case_adaptor to_snake_case{};
inline constexpr to_camel_case_adaptor to_camel_case{};
inline constexpr to_pascal_case_adaptor to_pascal_case{};
inline constexpr join_lines_adaptor join_lines{};
template <FrozenString Sep>
inline constexpr join_lines_nttp_adaptor<Sep> join_lines_nttp{};
inline constexpr trim_trailing_spaces_adaptor trim_trailing_spaces{};
inline constexpr remove_empty_lines_adaptor remove_empty_lines{};
inline constexpr url_encode_adaptor url_encode{};
inline constexpr url_decode_adaptor url_decode{};
inline constexpr base64_encode_adaptor base64_encode{};
inline constexpr base64_decode_adaptor base64_decode{};
inline constexpr remove_leading_spaces_adaptor<> remove_leading_spaces{};
inline constexpr remove_trailing_spaces_adaptor<> remove_trailing_spaces{};
inline constexpr remove_regex_comment_adaptor remove_regex_comment{};

struct data_t {};

inline constexpr data_t data{};

// Predicate variants
template <auto Pred> inline constexpr trim_adaptor<Pred> trim_if{};
template <auto Pred> inline constexpr ltrim_adaptor<Pred> ltrim_if{};
template <auto Pred> inline constexpr rtrim_adaptor<Pred> rtrim_if{};
template <auto Pred> inline constexpr collapse_spaces_adaptor<Pred> collapse_spaces_if{};
template <auto Pred> inline constexpr remove_leading_spaces_adaptor<Pred> remove_leading_spaces_if{};
template <auto Pred> inline constexpr remove_trailing_spaces_adaptor<Pred> remove_trailing_spaces_if{};

template <size_t M>
consteval auto prefix_lines(FrozenString<M> const& prefix) noexcept {
  return prefix_lines_adaptor<M>{prefix};
}

template <size_t M>
consteval auto postfix_lines(FrozenString<M> const& postfix) noexcept {
  return postfix_lines_adaptor<M>{postfix};
}

template <size_t M1, size_t M2>
consteval auto surround_lines(FrozenString<M1> const& prefix, FrozenString<M2> const& postfix) noexcept {
  return surround_lines_adaptor<M1, M2>{prefix, postfix};
}

template <size_t M>
consteval auto surround_lines(FrozenString<M> const& both) noexcept {
  return surround_lines_adaptor<M, M>{both, both};
}

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
struct join_adaptor : pipe_adaptor_base {
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

template <size_t N>
auto constexpr operator|(FrozenString<N>& lhs, data_t) noexcept {
  return lhs.data();
}

template <size_t N>
auto constexpr operator|(FrozenString<N> const& lhs, data_t) noexcept {
  return lhs.data();
}

template <size_t N>
auto constexpr operator|(FrozenString<N>&& lhs, data_t) noexcept {
  return lhs.data();
}

template <size_t N>
auto constexpr operator|(FrozenString<N> const&& lhs, data_t) noexcept {
  return lhs.data();
}

template <size_t Width, char Fill = ' '>
struct pad_left_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::pad_left<Width, Fill>(str);
  }
  template <Integral T>
  consteval auto operator()(T const& v) const noexcept {
    return frozenchars::pad_left<Width, Fill>(v);
  }
};

template <size_t Width, char Fill = ' '>
inline constexpr pad_left_adaptor<Width, Fill> pad_left{};

template <size_t Width, char Fill = ' '>
struct pad_right_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::pad_right<Width, Fill>(str);
  }
  template <Integral T>
  consteval auto operator()(T const& v) const noexcept {
    return frozenchars::pad_right<Width, Fill>(v);
  }
};

template <size_t Width, char Fill = ' '>
inline constexpr pad_right_adaptor<Width, Fill> pad_right{};

template <FrozenString From, FrozenString To>
struct replace_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::replace<From, To>(str);
  }
};

template <FrozenString From, FrozenString To>
inline constexpr replace_adaptor<From, To> replace{};

template <FrozenString From, FrozenString To>
struct replace_all_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::replace_all<From, To>(str);
  }
};

template <FrozenString From, FrozenString To>
inline constexpr replace_all_adaptor<From, To> replace_all{};

/**
 * @brief 数値型に対してアダプタを適用するパイプ演算子
 */
template <Integral T, PipeAdaptor Adaptor>
auto consteval operator|(T const& lhs, Adaptor const& rhs) noexcept(noexcept(rhs(lhs))) {
  return rhs(lhs);
}

} // namespace frozenchars::ops
