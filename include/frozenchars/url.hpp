#pragma once

#include <utility>

#include "freeze.hpp"
#include "encoding.hpp"

namespace frozenchars {

namespace detail {

/**
 * @brief Tがfreeze可能かを判定するコンセプト
 */
template<typename T>
concept Freezable = requires(T const& t) {
  { freeze(t).sv() } -> std::convertible_to<std::string_view>;
};

/**
 * @brief Tが2つの値をもちそれぞれがfreeze可能かを判定するコンセプト
  - std::get<0>(t) と std::get<1>(t) がfreeze可能であること
  - 例えば std::pair や std::tuple の要素がfreeze可能であればこのコンセプトを満たす
 */
template<typename T>
concept FreezablePair = requires(T const& t) {
  { freeze(std::get<0>(t)).sv() } -> std::convertible_to<std::string_view>;
  { freeze(std::get<1>(t)).sv() } -> std::convertible_to<std::string_view>;
  requires std::tuple_size_v<std::remove_cvref_t<T>> == 2;
};

/**
 * @brief キー・バリューペアからクエリ文字列を生成する
 * @param args キーと値を交互に並べた引数列（偶数個必須）
 * @return "key1=val1&key2=val2" 形式の文字列
 */
template <class Key, class Value>
requires (Freezable<Key> && Freezable<Value>)
auto consteval make_querystring_impl(Key&& key, Value&& value) {
  return concat(
    url_encode(freeze(key)), "=", url_encode(freeze(value))
  );
}

template <class Key, class Value, class... Tail>
requires (sizeof...(Tail) % 2 == 0 && Freezable<Key> && Freezable<Value>)
auto consteval make_querystring_impl(Key&& key, Value&& value, Tail&&... tail) {
  return concat(
    url_encode(freeze(key)), "=", url_encode(freeze(value)),
    "&",
    make_querystring_impl(tail...)
  );
}

/**
 * @brief キー・バリューをまとめたタプルからクエリ文字列を生成する
 * @param args キーと値を交互に並べたタプル
 * @return "key1=val1&key2=val2" 形式の文字列
 */
template <FreezablePair T>
auto consteval make_querystring_impl(T&& t) {
  return concat(
    url_encode(freeze(std::get<0>(t))), "=", url_encode(freeze(std::get<1>(t)))
  );
}

template <FreezablePair Head, class... Tail>
auto consteval make_querystring_impl(Head&& head, Tail&&... tail) {
  return concat(
    url_encode(freeze(std::get<0>(head))), "=", url_encode(freeze(std::get<1>(head))),
    "&",
    make_querystring_impl(tail...)
  );
}

} // namespace detail

/**
 * @brief クエリ文字列を生成するための関数
 *
 * @param args クエリパラメータのキーと値のペア 2つセットの引数か、std::tuple にまとめた引数を受け取る
 * @return クエリ文字列（先頭に '?' が付く）
 */
template<class ... Args>
auto consteval make_querystring(Args&&... args) {
  return concat("?", detail::make_querystring_impl(args...));
}

/**
 * @brief クエリ文字列を生成するための関数
 *
 * @param t クエリパラメータのタプルまたは複数のキーと値のペア
 * @return クエリ文字列（先頭に '?' が付く）
 */
template<class ... Args>
auto consteval make_querystring(std::tuple<Args...> const& t) {
  return concat("?", std::apply([](auto&&... args) {
    return detail::make_querystring_impl(args...);
  }, t));
}

}
