#pragma once

#include "frozen_string.hpp"

#include <array>
#include <cstddef>
#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string_view>
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
    "StaticPerfectMap keys must be unique");

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

  throw "StaticPerfectMap seed search exhausted";
}

} // namespace detail

template <typename T, FrozenString... Keys>
class StaticPerfectMap {
 public:
  static_assert(sizeof...(Keys) > 0, "StaticPerfectMap requires at least one key");

  static constexpr auto size() noexcept -> std::size_t {
    return sizeof...(Keys);
  }

  constexpr StaticPerfectMap() noexcept
    requires std::default_initializable<T> = default;

  constexpr explicit StaticPerfectMap(std::array<T, size()> values) noexcept(
      std::is_nothrow_move_constructible_v<T>)
  : values_{reorder_to_slots(std::move(values), std::make_index_sequence<size()>{})} {
  }

  constexpr auto contains(std::string_view key) const noexcept -> bool {
    return at(key).has_value();
  }

  constexpr auto at(std::string_view key) noexcept
      -> std::optional<std::reference_wrapper<T>> {
    auto const slot = slot_for(key);
    if (slot_key_views_[slot] == key) {
      return values_[slot];
    }
    return std::nullopt;
  }

  constexpr auto at(std::string_view key) const noexcept
      -> std::optional<std::reference_wrapper<T const>> {
    auto const slot = slot_for(key);
    if (slot_key_views_[slot] == key) {
      return values_[slot];
    }
    return std::nullopt;
  }

  constexpr auto operator[](std::string_view key) -> T& {
    if (auto value = at(key); value.has_value()) {
      return value->get();
    }
    throw std::out_of_range("StaticPerfectMap key not found");
  }

  constexpr auto operator[](std::string_view key) const -> T const& {
    if (auto value = at(key); value.has_value()) {
      return value->get();
    }
    throw std::out_of_range("StaticPerfectMap key not found");
  }

 private:
  static constexpr std::array<std::string_view, size()> key_views_{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };

  static constexpr auto k_seed = detail::find_seed<1'000'001, Keys...>();

  static constexpr auto slot_for(std::string_view key) noexcept -> std::size_t {
    return detail::fnv1a_hash(key, k_seed) % size();
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

} // namespace frozenchars
