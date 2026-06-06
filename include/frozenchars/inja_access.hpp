#pragma once

#include "inja_types.hpp"
#include <glaze/glaze.hpp>
#include <string_view>
#include <type_traits>
#include <utility>

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
 * @brief パスのコンパイル時アクセサ
 *
 * `accessor<T, "a", "b", "c">::resolve(obj)` で `obj.a.b.c` への const 参照を返す。
 * 末端 1 セグメント版を partial specialisation し、多段版は再帰的に組み合わせる。
 */
template <typename T, fixed_string... Segments>
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

/**
 * @brief 多段パスのアクセサ（再帰）
 *
 * 先頭セグメントでフィールドを取得し、残りのセグメントで再帰的に accessor を適用する。
 */
template <typename T, fixed_string Head, fixed_string... Rest>
  requires(sizeof...(Rest) > 0) && requires { glz::reflect<std::remove_cvref_t<T>>::keys; }
struct accessor<T, Head, Rest...> {
  using head = accessor<T, Head>;
  using tail = accessor<typename head::field_type, Rest...>;

  [[nodiscard]] static auto resolve(std::remove_cvref_t<T> const& obj) -> typename tail::field_type const& {
    return tail::resolve(head::resolve(obj));
  }
};

}  // namespace frozenchars::inja
