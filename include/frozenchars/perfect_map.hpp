#pragma once

#include "frozen_string.hpp"

#include <array>
#include <cstddef>
#include <concepts>
#include <cstdint>
#include <functional>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace frozenchars {

namespace detail {

inline constexpr auto k_fnv_offset_basis = 14695981039346656037ull;
inline constexpr auto k_fnv_prime = 1099511628211ull;

constexpr auto fnv1a_hash(std::string_view key, std::uint32_t seed) noexcept
    -> std::uint64_t {
  auto hash = k_fnv_offset_basis ^ static_cast<std::uint64_t>(seed);
  for (auto const ch : key) {
    hash ^= static_cast<unsigned char>(ch);
    hash *= k_fnv_prime;
  }
  return hash;
}

template <FrozenString... Keys>
consteval auto has_duplicate_keys() -> bool {
  constexpr std::array key_views{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };

  for (auto i = 0uz; i < key_views.size(); ++i) {
    for (auto j = i + 1; j < key_views.size(); ++j) {
      if (key_views[i] == key_views[j]) {
        return true;
      }
    }
  }
  return false;
}

template <std::uint32_t MaxSeedExclusive, FrozenString... Keys>
consteval auto find_seed() -> std::uint32_t {
  static_assert(!has_duplicate_keys<Keys...>(),
    "PerfectMap keys must be unique");
  static_assert(MaxSeedExclusive > 0,
    "PerfectMap seed search exhausted");

  constexpr std::array key_views{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };
  constexpr auto key_count = key_views.size();

  for (auto seed = 0u; seed < MaxSeedExclusive; ++seed) {
    std::array<bool, key_count> used_slots{};
    auto collision = false;

    for (auto const key_view : key_views) {
      auto const slot = fnv1a_hash(key_view, seed) % key_count;
      if (used_slots[slot]) {
        collision = true;
        break;
      }
      used_slots[slot] = true;
    }

    if (!collision) {
      return seed;
    }
  }

  throw "PerfectMap seed search exhausted";
}

} // namespace detail

template <typename T>
struct PerfectMapReference {
  std::string_view key;
  T& value;
};

template <typename T>
struct PerfectMapConstReference {
  std::string_view key;
  T const& value;
};

template <std::size_t I, typename T>
constexpr decltype(auto) get(PerfectMapReference<T> ref) noexcept {
  static_assert(I < 2);
  if constexpr (I == 0) {
    return ref.key;
  } else {
    return (ref.value);
  }
}

template <std::size_t I, typename T>
constexpr decltype(auto) get(PerfectMapConstReference<T> ref) noexcept {
  static_assert(I < 2);
  if constexpr (I == 0) {
    return ref.key;
  } else {
    return (ref.value);
  }
}

template <typename T, FrozenString... Keys>
class PerfectMap {
 public:
  using key_type = std::string_view;
  using mapped_type = T;
  using value_type = std::pair<key_type, mapped_type>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = PerfectMapReference<mapped_type>;
  using const_reference = PerfectMapConstReference<mapped_type>;

  class const_iterator;

  class iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = PerfectMap::value_type;
    using difference_type = PerfectMap::difference_type;
    using reference = PerfectMap::reference;

    constexpr auto operator==(iterator const&) const noexcept -> bool = default;
    constexpr operator const_iterator() const noexcept;
    constexpr auto operator*() const noexcept -> reference {
      return reference{owner_->slot_key_views_[index_], owner_->values_[index_]};
    }
    constexpr auto operator++() noexcept -> iterator& {
      ++index_;
      return *this;
    }

   private:
    friend class PerfectMap;
    constexpr iterator(PerfectMap* owner, size_type index) noexcept
    : owner_{owner}, index_{index} {
    }

    PerfectMap* owner_{nullptr};
    size_type index_{0};
  };

  class const_iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = PerfectMap::value_type;
    using difference_type = PerfectMap::difference_type;
    using reference = PerfectMap::const_reference;

    constexpr auto operator==(const_iterator const&) const noexcept -> bool = default;
    constexpr auto operator*() const noexcept -> reference {
      return reference{owner_->slot_key_views_[index_], owner_->values_[index_]};
    }
    constexpr auto operator++() noexcept -> const_iterator& {
      ++index_;
      return *this;
    }

   private:
    friend class PerfectMap;
    friend class iterator;
    constexpr const_iterator(PerfectMap const* owner, size_type index) noexcept
    : owner_{owner}, index_{index} {
    }

    PerfectMap const* owner_{nullptr};
    size_type index_{0};
  };

  /**
   * @brief 固定キー集合の要素数を返す
   * @return std::size_t キー数
   */
  static_assert(sizeof...(Keys) > 0, "PerfectMap requires at least one key");

  static constexpr auto size() noexcept -> size_type {
    return sizeof...(Keys);
  }

  static constexpr auto max_size() noexcept -> size_type {
    return size();
  }

  static constexpr auto empty() noexcept -> bool {
    return false;
  }

  constexpr auto find(std::string_view key) noexcept -> iterator {
    if (auto const slot = find_slot(key); slot.has_value()) {
      return iterator{this, *slot};
    }
    return end();
  }

  constexpr auto find(std::string_view key) const noexcept -> const_iterator {
    if (auto const slot = find_slot(key); slot.has_value()) {
      return const_iterator{this, *slot};
    }
    return end();
  }

  constexpr auto count(std::string_view key) const noexcept -> size_type {
    return find_slot(key).has_value() ? 1uz : 0uz;
  }

  constexpr auto begin() noexcept -> iterator {
    return iterator{this, 0};
  }

  constexpr auto end() noexcept -> iterator {
    return iterator{this, size()};
  }

  constexpr auto begin() const noexcept -> const_iterator {
    return const_iterator{this, 0};
  }

  constexpr auto end() const noexcept -> const_iterator {
    return const_iterator{this, size()};
  }

  constexpr auto cbegin() const noexcept -> const_iterator {
    return begin();
  }

  constexpr auto cend() const noexcept -> const_iterator {
    return end();
  }

  /**
   * @brief 値をデフォルト構築してマップを初期化する
   */
  constexpr PerfectMap() noexcept
    requires std::default_initializable<T> = default;

  /**
   * @brief 宣言順の値配列からマップを初期化する
   * @param values キー宣言順に並んだ値配列
   */
  constexpr explicit PerfectMap(std::array<T, size()> values) noexcept(
      std::is_nothrow_move_constructible_v<T>)
  : values_{reorder_to_slots(std::move(values), std::make_index_sequence<size()>{})} {
  }

  /**
   * @brief キーが存在するか判定する
   * @param key 検索対象キー
   * @return bool 存在する場合 true
   */
  constexpr auto contains(std::string_view key) const noexcept -> bool {
    return at(key).has_value();
  }

  /**
   * @brief キーに対応する値を取得する
   * @param key 検索対象キー
   * @return std::optional<std::reference_wrapper<T>> 値参照。未存在なら空
   */
  constexpr auto at(std::string_view key) noexcept
      -> std::optional<std::reference_wrapper<T>> {
    auto const slot = slot_for(key);
    if (slot_key_views_[slot] == key) [[likely]] {
      return values_[slot];
    }
    return std::nullopt;
  }

  /**
   * @brief キーに対応する値を取得する const 版
   * @param key 検索対象キー
   * @return std::optional<std::reference_wrapper<T const>> 値参照。未存在なら空
   */
  constexpr auto at(std::string_view key) const noexcept
      -> std::optional<std::reference_wrapper<T const>> {
    auto const slot = slot_for(key);
    if (slot_key_views_[slot] == key) [[likely]] {
      return values_[slot];
    }
    return std::nullopt;
  }

  /**
   * @brief キーに対応する値を参照する
   * @param key 検索対象キー
   * @return T& 値参照
   */
  constexpr auto operator[](std::string_view key) -> T& {
    if (auto value = at(key); value.has_value()) [[likely]] {
      return value->get();
    }
    throw std::out_of_range("PerfectMap key not found");
  }

  /**
   * @brief キーに対応する値を参照する const 版
   * @param key 検索対象キー
   * @return T const& 値参照
   */
  constexpr auto operator[](std::string_view key) const -> T const& {
    if (auto value = at(key); value.has_value()) [[likely]] {
      return value->get();
    }
    throw std::out_of_range("PerfectMap key not found");
  }

 private:
  static constexpr std::array<std::string_view, size()> key_views_{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };

  static constexpr auto k_seed = detail::find_seed<1'000'001, Keys...>();

  static constexpr auto slot_for(std::string_view key) noexcept -> std::size_t {
    return detail::fnv1a_hash(key, k_seed) % size();
  }

  static constexpr auto find_slot(std::string_view key) noexcept
      -> std::optional<size_type> {
    auto const slot = slot_for(key);
    if (slot_key_views_[slot] == key) [[likely]] {
      return slot;
    }
    return std::nullopt;
  }

  static consteval auto make_slot_key_views() -> std::array<std::string_view, size()> {
    std::array<std::string_view, size()> slot_key_views{};
    for (auto i = 0uz; i < size(); ++i) {
      slot_key_views[slot_for(key_views_[i])] = key_views_[i];
    }
    return slot_key_views;
  }

  static constexpr auto slot_key_views_ = make_slot_key_views();

  static consteval auto make_key_order_to_slot() -> std::array<std::size_t, size()> {
    std::array<std::size_t, size()> key_order_to_slot{};
    for (auto i = 0uz; i < size(); ++i) {
      key_order_to_slot[i] = slot_for(key_views_[i]);
    }
    return key_order_to_slot;
  }

  static consteval auto make_slot_to_key_order() -> std::array<std::size_t, size()> {
    auto const key_order_to_slot = make_key_order_to_slot();
    std::array<std::size_t, size()> slot_to_key_order{};
    for (auto i = 0uz; i < size(); ++i) {
      slot_to_key_order[key_order_to_slot[i]] = i;
    }
    return slot_to_key_order;
  }

  template <std::size_t... SlotIndex>
  static constexpr auto reorder_to_slots(
      std::array<T, size()> values,
      std::index_sequence<SlotIndex...>) -> std::array<T, size()> {
    constexpr auto slot_to_key_order = make_slot_to_key_order();
    return {std::move(values[slot_to_key_order[SlotIndex]])...};
  }

  std::array<T, size()> values_{};
};

template <typename T, FrozenString... Keys>
constexpr PerfectMap<T, Keys...>::iterator::operator
    PerfectMap<T, Keys...>::const_iterator() const noexcept {
  return typename PerfectMap<T, Keys...>::const_iterator{owner_, index_};
}

} // namespace frozenchars

namespace std {

template <typename T>
struct tuple_size<frozenchars::PerfectMapReference<T>>
  : integral_constant<std::size_t, 2> {
};

template <typename T>
struct tuple_size<frozenchars::PerfectMapConstReference<T>>
  : integral_constant<std::size_t, 2> {
};

template <typename T>
struct tuple_element<0, frozenchars::PerfectMapReference<T>> {
  using type = std::string_view;
};

template <typename T>
struct tuple_element<1, frozenchars::PerfectMapReference<T>> {
  using type = T&;
};

template <typename T>
struct tuple_element<0, frozenchars::PerfectMapConstReference<T>> {
  using type = std::string_view;
};

template <typename T>
struct tuple_element<1, frozenchars::PerfectMapConstReference<T>> {
  using type = T const&;
};

} // namespace std
