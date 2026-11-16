#pragma once

#include "freeze.hpp"
#include "encoding.hpp"

#include <tuple>
#include <utility>

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
[[nodiscard]] auto consteval make_querystring_impl(Key&& key, Value&& value) noexcept {
  return concat(
    url_encode(freeze(key)), "=", url_encode(freeze(value))
  );
}

/**
 * @brief キー・バリューペアと残りの引数からクエリ文字列を生成する（再帰用）
 */
template <class Key, class Value, class... Tail>
requires (sizeof...(Tail) % 2 == 0 && Freezable<Key> && Freezable<Value>)
[[nodiscard]] auto consteval make_querystring_impl(Key&& key, Value&& value, Tail&&... tail) noexcept {
  return concat(
    url_encode(freeze(key)), "=", url_encode(freeze(value)),
    "&",
    make_querystring_impl(tail...)
  );
}

/**
 * @brief std::tuple かどうか（make_querystring のオーバーロード解決用）
 */
template <typename T>
inline constexpr bool is_std_tuple_v = false;

template <typename... Ts>
inline constexpr bool is_std_tuple_v<std::tuple<Ts...>> = true;

/**
 * @brief キー・バリューをまとめたタプルからクエリ文字列を生成する
 * @param args キーと値を交互に並べたタプル
 * @return "key1=val1&key2=val2" 形式の文字列
 */
template <FreezablePair T>
[[nodiscard]] auto consteval make_querystring_impl(T&& t) noexcept {
  return concat(
    url_encode(freeze(std::get<0>(t))), "=", url_encode(freeze(std::get<1>(t)))
  );
}

/**
 * @brief タプルと残りの引数からクエリ文字列を生成する（再帰用）
 */
template <FreezablePair Head, class... Tail>
[[nodiscard]] auto consteval make_querystring_impl(Head&& head, Tail&&... tail) noexcept {
  return concat(
    url_encode(freeze(std::get<0>(head))), "=", url_encode(freeze(std::get<1>(head))),
    "&",
    make_querystring_impl(tail...)
  );
}

/**
 * @brief タプルを index_sequence で展開してクエリ文字列を生成する
 *
 * @details std::apply は MSVC の定数評価で invoke に失敗するため使わない。
 * @param t キーと値を交互に並べたタプル
 * @param Is タプルのインデックス列
 * @return "?key1=val1&key2=val2" 形式の文字列
 */
template <typename... Args, std::size_t... Is>
[[nodiscard]] auto consteval make_querystring_tuple(std::tuple<Args...> const& t, std::index_sequence<Is...>) noexcept {
  return concat("?", make_querystring_impl(std::get<Is>(t)...));
}

} // namespace detail

/**
 * @brief クエリ文字列を生成するための関数
 *
 * @param args クエリパラメータのキーと値のペア 2つセットの引数か、std::tuple にまとめた引数を受け取る
 * @return クエリ文字列（先頭に '?' が付く）
 */
template <class... Args>
  requires((!detail::is_std_tuple_v<std::remove_cvref_t<Args>>) && ...)
[[nodiscard]] auto consteval make_querystring(Args&&... args) noexcept {
  return concat("?", detail::make_querystring_impl(args...));
}

/**
 * @brief クエリ文字列を生成するための関数
 *
 * @param t クエリパラメータのタプルまたは複数のキーと値のペア
 * @return クエリ文字列（先頭に '?' が付く）
 */
template<class ... Args>
[[nodiscard]] auto consteval make_querystring(std::tuple<Args...> const& t) noexcept {
  return detail::make_querystring_tuple(t, std::index_sequence_for<Args...>{});
}

/**
 * @brief クエリ文字列を生成する（非 const のタプル版）
 */
template <class... Args>
[[nodiscard]] auto consteval make_querystring(std::tuple<Args...>& t) noexcept {
  return make_querystring(static_cast<std::tuple<Args...> const&>(t));
}

/**
 * @brief クエリ文字列を生成する（右辺値のタプル版）
 */
template <class... Args>
[[nodiscard]] auto consteval make_querystring(std::tuple<Args...>&& t) noexcept {
  return make_querystring(static_cast<std::tuple<Args...> const&>(t));
}

} // namespace frozenchars
