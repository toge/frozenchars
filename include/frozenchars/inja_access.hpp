#pragma once

#include "inja_types.hpp"
#include <glaze/glaze.hpp>
#include <string_view>
#include <type_traits>

namespace frozenchars::inja {

namespace detail {

template <typename T, fixed_string Segment, std::size_t I = 0>
consteval auto find_key_index() -> std::size_t {
  using U = std::remove_cvref_t<T>;
  constexpr auto keys = glz::reflect<U>::keys;
  if constexpr (I >= keys.size()) {
    throw "accessor: key not found";
  } else {
    if (std::string_view{keys[I]} == Segment.sv()) {
      return I;
    }
    return find_key_index<T, Segment, I + 1>();
  }
}

}  // namespace detail

/**
 * @brief 単一セグメントのアクセサ
 *
 * `accessor<T, "field">::resolve(obj)` で `obj.field` への const 参照を返す。
 */
template <typename T, fixed_string Segment>
struct accessor;

template <typename T, fixed_string Segment>
  requires requires { glz::reflect<std::remove_cvref_t<T>>::keys; }
struct accessor<T, Segment> {
  using U = std::remove_cvref_t<T>;
  static constexpr auto index = detail::find_key_index<U, Segment>();
  using field_type = std::remove_cvref_t<decltype(glz::get<index>(glz::to_tie(std::declval<U&>())))>;

  [[nodiscard]] static auto resolve(U const& obj) -> field_type const& {
    return glz::get<index>(glz::to_tie(obj));
  }
};

}  // namespace frozenchars::inja
