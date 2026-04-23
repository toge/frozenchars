#pragma once

#include "frozen_string.hpp"

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace frozenchars {

template <typename T, FrozenString... Keys>
class StaticPerfectMap {
 public:
  static_assert(sizeof...(Keys) > 0, "StaticPerfectMap requires at least one key");

  static constexpr auto size() noexcept -> std::size_t {
    return sizeof...(Keys);
  }

  constexpr StaticPerfectMap() noexcept = default;

  constexpr auto contains(std::string_view key) const noexcept -> bool {
    return find_linear(key).has_value();
  }

  constexpr auto at(std::string_view key) noexcept
      -> std::optional<std::reference_wrapper<T>> {
    if (auto const index = find_linear(key); index.has_value()) {
      return values_[*index];
    }
    return std::nullopt;
  }

  constexpr auto at(std::string_view key) const noexcept
      -> std::optional<std::reference_wrapper<T const>> {
    if (auto const index = find_linear(key); index.has_value()) {
      return values_[*index];
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

  static constexpr auto find_linear(std::string_view key) noexcept
      -> std::optional<std::size_t> {
    for (auto i = 0uz; i < key_views_.size(); ++i) {
      if (key_views_[i] == key) {
        return i;
      }
    }
    return std::nullopt;
  }

  std::array<T, size()> values_{};
};

} // namespace frozenchars
