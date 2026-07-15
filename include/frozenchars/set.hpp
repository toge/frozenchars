#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string_view>
#include <ranges>
#include <span>
#include <type_traits>

#include "string.hpp"
#include "map.hpp"

namespace frozenchars {

/**
 * @brief コンパイル時に固定されたキー集合を表すクラス
 *
 * @tparam Keys 集合に含まれるキー文字列（FrozenString の非型テンプレート引数）
 */
template <FrozenString... Keys>
class frozen_set {
  static_assert(sizeof...(Keys) > 0, "frozen_set requires at least one key");
  static_assert(!detail::has_duplicate_keys<Keys...>(), "frozen_set keys must be unique");

public:
  using key_type        = std::string_view;
  using value_type      = std::string_view;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;

  /**
   * @brief キー集合を走査するランダムアクセスイテレータ
   * @details operator* で std::string_view（キー）を返す。
   *          frozen_set はすべて static constexpr で保持するため、owner ポインタを持たず添字のみで動作する。
   */
  class iterator {
  public:
    using value_type        = std::string_view;
    using reference         = const std::string_view&;
    using pointer           = const std::string_view*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    constexpr iterator() noexcept = default;
    constexpr explicit iterator(size_type index) noexcept : index_{index} {}

    constexpr auto operator*()  const noexcept -> reference { return detail::lookup_index<Keys...>::key_views_[index_]; }
    constexpr auto operator->() const noexcept -> pointer   { return &detail::lookup_index<Keys...>::key_views_[index_]; }
    constexpr auto operator[](difference_type n) const noexcept -> reference { return detail::lookup_index<Keys...>::key_views_[index_ + n]; }

    constexpr auto operator++()    noexcept -> iterator& { ++index_; return *this; }
    constexpr auto operator++(int) noexcept -> iterator  { auto t = *this; ++index_; return t; }
    constexpr auto operator--()    noexcept -> iterator& { --index_; return *this; }
    constexpr auto operator--(int) noexcept -> iterator  { auto t = *this; --index_; return t; }
    constexpr auto operator+=(difference_type n) noexcept -> iterator& { index_ += n; return *this; }
    constexpr auto operator-=(difference_type n) noexcept -> iterator& { index_ -= n; return *this; }

    friend constexpr auto operator+(iterator it, difference_type n) noexcept -> iterator { return it += n; }
    friend constexpr auto operator+(difference_type n, iterator it) noexcept -> iterator { return it += n; }
    friend constexpr auto operator-(iterator it, difference_type n) noexcept -> iterator { return it -= n; }
    friend constexpr auto operator-(iterator a, iterator b) noexcept -> difference_type {
      return static_cast<difference_type>(a.index_) - static_cast<difference_type>(b.index_);
    }

    friend constexpr bool operator==(iterator const& a, iterator const& b) noexcept { return a.index_ == b.index_; }
    friend constexpr auto operator<=>(iterator const& a, iterator const& b) noexcept { return a.index_ <=> b.index_; }

  private:
    size_type index_{0};
  };

  using const_iterator = iterator;

  /** @brief キーの総数を返す */
  static constexpr auto size()  noexcept -> size_type { return sizeof...(Keys); }
  /** @brief 常に false（frozen_set は少なくとも1つのキーを要求する） */
  static constexpr auto empty() noexcept -> bool { return false; }

  /**
   * @brief 指定されたキーが集合に含まれるかを判定する
   * @param key 検索するキー
   * @return キーが存在すれば true
   */
  static constexpr auto contains(std::string_view key) noexcept -> bool {
    return lookup_::find_index_raw(key) != size();
  }

  /**
   * @brief 指定されたキーの出現回数を返す
   * @param key 検索するキー
   * @return キーが存在すれば 1、存在しなければ 0
   */
  static constexpr auto count(std::string_view key) noexcept -> size_type {
    return lookup_::find_index_raw(key) != size() ? 1uz : 0uz;
  }

  /**
   * @brief 指定されたキーを検索する
   * @param key 検索するキー
   * @return キーを指すイテレータ。見つからない場合は end()
   */
  static constexpr auto find(std::string_view key) noexcept -> iterator {
    auto const i = lookup_::find_index_raw(key);
    return i != size() ? iterator{i} : end();
  }

  /** @brief 宣言順の先頭イテレータ */
  static constexpr auto begin()  noexcept -> iterator { return iterator{0}; }
  /** @brief 宣言順の終端イテレータ */
  static constexpr auto end()    noexcept -> iterator { return iterator{size()}; }
  /** @copydoc begin() */
  static constexpr auto cbegin() noexcept -> iterator { return begin(); }
  /** @copydoc end() */
  static constexpr auto cend()   noexcept -> iterator { return end(); }

  /**
   * @brief 辞書順にソートされたキー配列を取得する
   * @return ソート済みキーの std::span
   */
  static constexpr auto keys() noexcept -> std::span<const std::string_view, size()> {
    return lookup_::sorted_key_views_;
  }

  /**
   * @brief 宣言順のキー配列を取得する
   * @return 宣言順キーの std::span
   */
  static constexpr auto keys_in_declaration_order() noexcept -> std::span<const std::string_view, size()> {
    return lookup_::key_views_;
  }

  /**
   * @brief この frozen_set と同じキー集合を持つ frozen_map の型エイリアス
   * @tparam T マップの値型
   */
  template <typename T>
  using map_type = frozen_map<T, Keys...>;

private:
  using lookup_ = detail::lookup_index<Keys...>;
};

/**
 * @brief frozen_set から frozen_map を構築する
 * @tparam T マップの値型
 * @tparam Keys frozen_set のキー集合
 * @param タグディスパッチ用の frozen_set 値
 * @param values キーに対応する値の配列
 * @return Keys をキー、values を値とする frozen_map
 */
template <typename T, FrozenString... Keys>
constexpr auto to_frozen_map(frozen_set<Keys...>, std::array<T, sizeof...(Keys)> values)
  -> frozen_map<T, Keys...> {
  return frozen_map<T, Keys...>{std::move(values)};
}

} // namespace frozenchars
