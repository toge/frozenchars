#pragma once

#include <cstddef>
#include <iterator>
#include <span>
#include <string_view>

#include "string.hpp"
#include "trie_index.hpp"

namespace frozenchars {

/**
 * @brief トライ木構造を利用したコンパイル時キーの集合
 * @tparam Keys キー文字列の NTTP パック
 */
template <FrozenString... Keys>
class frozen_trie_set {
  static_assert(sizeof...(Keys) > 0, "frozen_trie_set requires at least one key");
  // duplicate check happens in frozen_trie_index

public:
  using key_type        = std::string_view;
  using value_type      = std::string_view;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;

  /**
   * @brief frozen_trie_set のランダムアクセスイテレータ
   */
  class iterator {
  public:
    using value_type        = std::string_view;
    using reference         = const std::string_view&;
    using pointer           = const std::string_view*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    constexpr iterator() noexcept = default;
    /**
     * @param index キーインデックス
     */
    constexpr explicit iterator(size_type index) noexcept : index_{index} {}

    constexpr auto operator*()  const noexcept -> reference { return frozen_trie_index<Keys...>::k_key_views_[index_]; }
    constexpr auto operator->() const noexcept -> pointer   { return &frozen_trie_index<Keys...>::k_key_views_[index_]; }
    constexpr auto operator[](difference_type n) const noexcept -> reference { return frozen_trie_index<Keys...>::k_key_views_[index_ + n]; }

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
    size_type index_{0};  ///< 現在のキーインデックス
  };

  using const_iterator = iterator;

  static constexpr auto size()  noexcept -> size_type { return sizeof...(Keys); }
  static constexpr auto empty() noexcept -> bool { return false; }

  /**
   * @brief キーの存在確認
   * @param key 検索キー
   */
  static constexpr auto contains(std::string_view key) noexcept -> bool {
    return lookup_::find(key) != size();
  }

  /**
   * @brief キーに対応する要素数を返す（0 or 1）
   * @param key 検索キー
   */
  static constexpr auto count(std::string_view key) noexcept -> size_type {
    return lookup_::find(key) != size() ? 1uz : 0uz;
  }

  /**
   * @brief キーに対応するイテレータを取得
   * @param key 検索キー
   */
  static constexpr auto find(std::string_view key) noexcept -> iterator {
    auto const i = lookup_::find(key);
    return i != size() ? iterator{i} : end();
  }

  static constexpr auto begin()  noexcept -> iterator { return iterator{0}; }
  static constexpr auto end()    noexcept -> iterator { return iterator{size()}; }
  static constexpr auto cbegin() noexcept -> iterator { return begin(); }
  static constexpr auto cend()   noexcept -> iterator { return end(); }

  /**
   * @brief 全キーのビューを取得
   */
  static constexpr auto keys() noexcept -> std::span<const std::string_view, size()> {
    return lookup_::k_key_views_;
  }

private:
  using lookup_ = frozen_trie_index<Keys...>;  ///< キー検索に使うトライ木インデックス
};

} // namespace frozenchars
