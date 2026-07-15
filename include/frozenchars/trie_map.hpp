#pragma once

#include <array>
#include <concepts>
#include <cstddef>
#include <iterator>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "string.hpp"
#include "trie_index.hpp"

namespace frozenchars {

/**
 * @brief トライ木構造を利用したコンパイル時キー・値マップ
 * @tparam T 値の型
 * @tparam Keys キー文字列の NTTP パック
 */
template <typename T, FrozenString... Keys>
class frozen_trie_map {
  static_assert(sizeof...(Keys) > 0, "frozen_trie_map requires at least one key");

public:
  using key_type        = std::string_view;
  using mapped_type     = T;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  using value_type      = std::pair<std::string_view const, T>;
  using reference       = std::pair<std::string_view, T&>;
  using const_reference = std::pair<std::string_view, T const&>;

  /**
   * @brief operator-> のプロキシ型（参照ペアのポインタ演算をエミュレート）
   */
  template <typename Ref>
  struct arrow_proxy {
    Ref ref_v;
    std::string_view& key;
    T& value;
    constexpr arrow_proxy(Ref r) : ref_v(r), key(ref_v.first), value(ref_v.second) {}
    constexpr auto operator->() noexcept -> arrow_proxy* { return this; }
    constexpr auto operator->() const noexcept -> const arrow_proxy* { return this; }
  };

  /**
   * @brief frozen_trie_map の内部イテレータ実装
   * @tparam Owner 所有者型（const 修飾で const/非 const を切り替え）
   * @tparam Ref 参照型
   */
  template <typename Owner, typename Ref>
  class iterator_base {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = frozen_trie_map::value_type;
    using difference_type = std::ptrdiff_t;

    struct arrow_proxy_impl {
      Ref ref_v;
      std::string_view& key;
      decltype(std::declval<Ref>().second)& value;
      constexpr arrow_proxy_impl(Ref r) : ref_v(r), key(ref_v.first), value(ref_v.second) {}
      constexpr auto operator->() noexcept -> arrow_proxy_impl* { return this; }
      constexpr auto operator->() const noexcept -> const arrow_proxy_impl* { return this; }
    };
    using arrow_proxy = arrow_proxy_impl;
    using pointer = arrow_proxy;
    using reference = Ref;

    constexpr iterator_base() noexcept = default;
    /**
     * @param owner 所属コンテナへのポインタ
     * @param index キーインデックス
     */
    constexpr iterator_base(Owner* owner, size_type index) noexcept : owner_{owner}, index_{index} {}
    constexpr auto operator*() const noexcept -> reference {
      return reference{frozen_trie_index<Keys...>::k_key_views_[index_], owner_->values_[index_]};
    }
    constexpr auto operator->() const noexcept -> pointer { return pointer{operator*()}; }
    constexpr auto operator++() noexcept -> iterator_base& { ++index_; return *this; }
    constexpr auto operator++(int) noexcept -> iterator_base { auto tmp = *this; ++index_; return tmp; }
    constexpr auto operator--() noexcept -> iterator_base& { --index_; return *this; }
    constexpr auto operator--(int) noexcept -> iterator_base { auto tmp = *this; --index_; return tmp; }
    constexpr auto operator+=(difference_type n) noexcept -> iterator_base& { index_ += n; return *this; }
    constexpr auto operator-=(difference_type n) noexcept -> iterator_base& { index_ -= n; return *this; }
    friend constexpr auto operator+(iterator_base a, difference_type n) noexcept -> iterator_base { return a += n; }
    friend constexpr auto operator+(difference_type n, iterator_base a) noexcept -> iterator_base { return a += n; }
    friend constexpr auto operator-(iterator_base a, difference_type n) noexcept -> iterator_base { return a -= n; }
    friend constexpr auto operator-(iterator_base a, iterator_base b) noexcept -> difference_type {
      return static_cast<difference_type>(a.index_) - static_cast<difference_type>(b.index_);
    }
    friend constexpr bool operator==(iterator_base const& a, iterator_base const& b) noexcept { return a.index_ == b.index_; }
    friend constexpr auto operator<=>(iterator_base const& a, iterator_base const& b) noexcept { return a.index_ <=> b.index_; }

  private:
    Owner* owner_{nullptr};  ///< 所属コンテナへのポインタ
    size_type index_{0};     ///< 現在のキーインデックス
  };

  using iterator = iterator_base<frozen_trie_map, reference>;
  using const_iterator = iterator_base<frozen_trie_map const, const_reference>;

  static constexpr auto size() noexcept -> size_type { return sizeof...(Keys); }
  static constexpr auto max_size() noexcept -> size_type { return size(); }
  static constexpr auto empty() noexcept -> bool { return false; }

  constexpr frozen_trie_map() noexcept requires std::default_initializable<T> = default;
  /**
   * @brief 値の配列から構築
   * @param values キー順に対応する値の配列
   */
  constexpr explicit frozen_trie_map(std::array<T, size()> values) noexcept(std::is_nothrow_move_constructible_v<T>)
    : values_{std::move(values)} {}

  /**
   * @brief キーに対応するイテレータを取得
   * @param key 検索キー
   */
  constexpr auto find(std::string_view key) noexcept -> iterator {
    auto const i = lookup_::find(key);
    return i != size() ? iterator{this, i} : end();
  }
  /** @copydoc find */
  constexpr auto find(std::string_view key) const noexcept -> const_iterator {
    auto const i = lookup_::find(key);
    return i != size() ? const_iterator{this, i} : end();
  }
  /**
   * @brief キーの存在確認
   * @param key 検索キー
   */
  constexpr auto contains(std::string_view key) const noexcept -> bool {
    return lookup_::find(key) != size();
  }
  /**
   * @brief キーに対応する要素数を返す（0 or 1）
   * @param key 検索キー
   */
  constexpr auto count(std::string_view key) const noexcept -> size_type {
    return lookup_::find(key) != size() ? 1uz : 0uz;
  }
  /**
   * @brief キーに対応する値にアクセス（なければ例外）
   * @param key 検索キー
   * @throws std::out_of_range キーが見つからない場合
   */
  constexpr auto at(std::string_view key) -> T& {
    auto const i = lookup_::find(key);
    if (i != size()) [[likely]] return values_[i];
    throw std::out_of_range(std::string{"frozen_trie_map key not found: "} + std::string{key});
  }
  /** @copydoc at */
  constexpr auto at(std::string_view key) const -> T const& {
    auto const i = lookup_::find(key);
    if (i != size()) [[likely]] return values_[i];
    throw std::out_of_range(std::string{"frozen_trie_map key not found: "} + std::string{key});
  }
  constexpr auto operator[](std::string_view key) -> T& { return at(key); }
  constexpr auto operator[](std::string_view key) const -> T const& { return at(key); }

  constexpr auto begin() noexcept -> iterator { return iterator{this, 0}; }
  constexpr auto end() noexcept -> iterator { return iterator{this, size()}; }
  constexpr auto begin() const noexcept -> const_iterator { return const_iterator{this, 0}; }
  constexpr auto end() const noexcept -> const_iterator { return const_iterator{this, size()}; }
  constexpr auto cbegin() const noexcept -> const_iterator { return begin(); }
  constexpr auto cend() const noexcept -> const_iterator { return end(); }

  /**
   * @brief 全キーのビューを取得
   */
  static constexpr auto keys() noexcept -> std::span<const std::string_view, size()> {
    return lookup_::k_key_views_;
  }

private:
  using lookup_ = frozen_trie_index<Keys...>;  ///< キー検索に使うトライ木インデックス
  std::array<T, size()> values_{};             ///< キー順に対応する値の配列
};

/**
 * @brief frozen_trie_map をエントリ対から構築するヘルパー関数
 * @tparam T 値の型
 * @tparam Keys キー文字列の NTTP パック
 * @tparam Entries エントリの型パック
 * @param entries キー・値のペア
 */
template <typename T, FrozenString... Keys, typename... Entries>
  requires((std::convertible_to<Entries, std::pair<std::string_view, T>> && ...))
constexpr auto make_frozen_trie_map(Entries&&... entries) -> frozen_trie_map<T, Keys...> {
  static_assert(sizeof...(Keys) == sizeof...(Entries), "make_frozen_trie_map requires exactly one entry per key");
  auto arr = std::array<T, sizeof...(Keys)>{
    static_cast<T>(std::forward<Entries>(entries).second)...
  };
  return frozen_trie_map<T, Keys...>{std::move(arr)};
}

} // namespace frozenchars
