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

    constexpr auto operator*()  const noexcept -> reference { return key_views_[index_]; }
    constexpr auto operator->() const noexcept -> pointer   { return &key_views_[index_]; }
    constexpr auto operator[](difference_type n) const noexcept -> reference { return key_views_[index_ + n]; }

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
    return find_index_raw(key) != size();
  }

  /**
   * @brief 指定されたキーの出現回数を返す
   * @param key 検索するキー
   * @return キーが存在すれば 1、存在しなければ 0
   */
  static constexpr auto count(std::string_view key) noexcept -> size_type {
    return find_index_raw(key) != size() ? 1uz : 0uz;
  }

  /**
   * @brief 指定されたキーを検索する
   * @param key 検索するキー
   * @return キーを指すイテレータ。見つからない場合は end()
   */
  static constexpr auto find(std::string_view key) noexcept -> iterator {
    auto const i = find_index_raw(key);
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
    return sorted_key_views_;
  }

  /**
   * @brief 宣言順のキー配列を取得する
   * @return 宣言順キーの std::span
   */
  static constexpr auto keys_in_declaration_order() noexcept -> std::span<const std::string_view, size()> {
    return key_views_;
  }

  /**
   * @brief この frozen_set と同じキー集合を持つ frozen_map の型エイリアス
   * @tparam T マップの値型
   */
  template <typename T>
  using map_type = frozen_map<T, Keys...>;

private:
  static constexpr auto k_total_chars_ = (Keys.length + ... + 0);

  static constexpr auto k_packed_data_ = [] {
    std::array<char, k_total_chars_> arr{};
    std::size_t pos = 0;
    ((void)(std::copy_n(Keys.buffer.data(), Keys.length, arr.data() + pos), pos += Keys.length), ...);
    return arr;
  }();

  static constexpr auto k_lengths_ = std::array<std::size_t, size()>{Keys.length...};

  static constexpr auto k_offsets_ = [] {
    std::array<std::size_t, size()> off{};
    std::size_t pos = 0;
    for (auto i = 0uz; i < size(); ++i) {
      off[i] = pos;
      pos += k_lengths_[i];
    }
    return off;
  }();

  static constexpr auto k_max_key_len_ = std::max({Keys.length...});

  static constexpr std::array<std::string_view, size()> key_views_ = [] {
    std::array<std::string_view, size()> arr{};
    for (auto i = 0uz; i < size(); ++i)
      arr[i] = std::string_view{k_packed_data_.data() + k_offsets_[i], k_lengths_[i]};
    return arr;
  }();

  static constexpr auto valid_lengths_ = [] {
    std::array<bool, k_max_key_len_ + 1> table{};
    ((table[Keys.length] = true), ...);
    return table;
  }();

  static constexpr auto all_lengths_unique_ = [] {
    auto lens = k_lengths_;
    std::ranges::sort(lens);
    return std::ranges::adjacent_find(lens) == lens.end();
  }();

  static constexpr auto length_to_index_ = [] {
    using index_t = detail::index_type_t<size()>;
    std::array<index_t, k_max_key_len_ + 1> table{};
    table.fill(static_cast<index_t>(-1));
    std::size_t idx = 0;
    ((table[Keys.length] = static_cast<index_t>(idx++)), ...);
    return table;
  }();

  static constexpr auto sorted_key_views_ = [] {
    auto views = key_views_;
    std::ranges::sort(views);
    return views;
  }();

  static constexpr auto k_lookup_threshold = 64uz;
  static constexpr auto use_lookup_table_ = (size() <= k_lookup_threshold);
  static constexpr auto table_size_ = use_lookup_table_ ? detail::next_pow2(size() * 4) : 1uz;
  static constexpr auto mask_ = table_size_ - 1;

  static consteval auto make_lookup_metadata() {
    if constexpr (use_lookup_table_) {
      return detail::find_lookup_seed<table_size_, Keys...>();
    } else {
      using index_t = detail::index_type_t<size()>;
      return detail::lookup_seed_result<table_size_, size()>{0, {static_cast<index_t>(-1)}};
    }
  }

  static constexpr auto metadata_ = make_lookup_metadata();
  static constexpr auto k_seed = metadata_.seed;
  static constexpr auto lookup_table_ = metadata_.table;

  static constexpr bool all_keys_short = ((Keys.length <= 16) && ...);

  struct alignas(16) PaddedKey {
    std::uint64_t data[2]{0, 0};
    std::uint8_t len = 0;
  };

  static consteval auto make_padded_keys() {
    std::array<PaddedKey, size()> res{};
    std::size_t idx = 0;
    ([&] {
      res[idx].len = static_cast<std::uint8_t>(Keys.length);
      for (std::size_t i = 0; i < Keys.length; ++i) {
        res[idx].data[i / 8] |=
          (static_cast<std::uint64_t>(static_cast<unsigned char>(Keys.buffer[i])) << ((i % 8) * 8));
      }
      idx++;
    }(), ...);
    return res;
  }

  static constexpr auto padded_keys_ = [] {
    if constexpr (all_keys_short) {
      return make_padded_keys();
    } else {
      return std::array<PaddedKey, size()>{};
    }
  }();

  static constexpr auto key_equals(std::string_view key, size_type index) noexcept -> bool {
    if constexpr (all_keys_short) {
      if (key.size() > 16) return false;
      auto const& pk = padded_keys_[index];
      if (key.size() != pk.len) return false;
      std::uint64_t low = 0, high = 0;
      auto const d = key.data();
      auto const n = key.size();
      for (std::size_t i = 0; i < n; ++i) {
        if (i < 8) low |= (static_cast<std::uint64_t>(static_cast<unsigned char>(d[i])) << (i * 8));
        else high |= (static_cast<std::uint64_t>(static_cast<unsigned char>(d[i])) << ((i - 8) * 8));
      }
      return (low == pk.data[0] && high == pk.data[1]);
    }
    return key_views_[index] == key;
  }

  static constexpr auto find_index_raw(std::string_view key) noexcept -> size_type {
    if constexpr (use_lookup_table_) {
      auto const len = key.size();
      if constexpr (all_lengths_unique_) {
        if (len > k_max_key_len_) return size();
        auto const index = length_to_index_[len];
        if (index == static_cast<decltype(index)>(-1)) return size();
        if (key_equals(key, static_cast<size_type>(index))) [[likely]]
          return static_cast<size_type>(index);
        return size();
      }
      if (len > k_max_key_len_ || !valid_lengths_[len]) return size();
      auto const h = detail::hash_impl(key, k_seed);
      auto const slot = static_cast<std::size_t>(h & mask_);
      auto const index = lookup_table_[slot];
      if (index == static_cast<decltype(index)>(-1)) return size();
      if (key_equals(key, static_cast<size_type>(index))) [[likely]]
        return static_cast<size_type>(index);
      return size();
    } else {
      for (auto const i : std::views::iota(0uz, size())) {
        if (key_equals(key, i)) return i;
      }
      return size();
    }
  }
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
