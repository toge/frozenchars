#pragma once

#include <array>
#include <concepts>
#include <optional>
#include <string_view>
#include <utility>

#include "map.hpp"

namespace frozenchars {

/**
 * @brief キー・値の両方向で探索できる連想コンテナ（frozen bimap）
 *
 * キー側は frozen_map の O(1) ハッシュ探索を継承し、値側は O(n) の線形走査で逆引きする。
 * 逆引きは先頭一致（同じ値が複数ある場合は最初のキー）を返す。
 *
 * @tparam T 値の型
 * @tparam Keys キー列（FrozenString NTTP、重複不可）
 */
template <typename T, FrozenString... Keys>
class frozen_bimap : public frozen_map<T, Keys...> {
  static_assert(sizeof...(Keys) > 0, "frozen_bimap requires at least one key");
  static_assert(!detail::has_duplicate_keys<Keys...>(), "frozen_bimap keys must be unique");

public:
  using frozen_map<T, Keys...>::frozen_map;

  /**
   * @brief 値から対応するキーを逆引きする（先頭一致のみ）
   *
   * @param value 検索する値
   * @return std::optional<std::string_view> 対応するキー、未検出は std::nullopt
   */
  [[nodiscard]] constexpr auto by_value(T const& value) const
      -> std::optional<std::string_view> requires std::equality_comparable<T> {
    for (auto it = this->begin(); it != this->end(); ++it) {
      auto const& [key, mapped] = *it;
      if (mapped == value) {
        return key;
      }
    }
    return std::nullopt;
  }

  /**
   * @brief 値が存在するかを判定する
   *
   * @param value 検索する値
   * @return bool 存在すれば true
   */
  [[nodiscard]] constexpr auto contains_value(T const& value) const -> bool
      requires std::equality_comparable<T> {
    return by_value(value).has_value();
  }
};

/**
 * @brief 値配列から frozen_bimap を構築する
 *
 * @tparam T 値の型
 * @tparam Keys キー列（FrozenString NTTP）
 * @param values キー順に対応する値の配列
 * @return frozen_bimap<T, Keys...> 構築されたバイマップ
 */
template <typename T, FrozenString... Keys>
constexpr auto make_frozen_bimap(std::array<T, sizeof...(Keys)> values) -> frozen_bimap<T, Keys...> {
  return frozen_bimap<T, Keys...>{std::move(values)};
}

/**
 * @brief pair-like 要素群から frozen_bimap を構築する
 *
 * @tparam T 値の型
 * @tparam Keys キー列（FrozenString NTTP）
 * @tparam Entries 要素の型パラメータパック
 * @param entries キー・値ペアの要素群
 * @return frozen_bimap<T, Keys...> 構築されたバイマップ
 */
template <typename T, FrozenString... Keys, typename... Entries>
  requires((detail::PairLikeEntry<Entries> && ...) &&
           (std::convertible_to<decltype(detail::pair_like_get<0>(std::declval<Entries>())), std::string_view> && ...) &&
           (std::convertible_to<decltype(detail::pair_like_get<1>(std::declval<Entries>())), T> && ...))
constexpr auto make_frozen_bimap(Entries&&... entries) -> frozen_bimap<T, Keys...> {
  static_assert(sizeof...(Keys) == sizeof...(Entries), "make_frozen_bimap requires exactly one entry per key");
  auto arr = std::array<frozen_map_entry<T>, sizeof...(Keys)>{
    frozen_map_entry<T>{detail::pair_like_get<0>(std::forward<Entries>(entries)), detail::pair_like_get<1>(std::forward<Entries>(entries))}...
  };
  return frozen_bimap<T, Keys...>{std::move(arr)};
}

/**
 * @brief kv リテラル群から frozen_bimap を構築する（コンパイル時、キーは型から抽出）
 *
 * @tparam T 値の型
 * @tparam KVs kv リテラルのパラメータパック
 * @return frozen_bimap<T, ...> 構築されたバイマップ
 */
template <typename T, kv... KVs>
constexpr auto make_frozen_bimap_kv() -> frozen_bimap<T, KVs.key...> {
  return frozen_bimap<T, KVs.key...>{ std::array<T, sizeof...(KVs)>{ KVs.value... } };
}

} // namespace frozenchars
