#include "catch2/catch_all.hpp"

#include <array>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/map.hpp"
#include "frozenchars/multimap.hpp"
#include "frozenchars/trie_map.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

namespace {

struct throwing_mapped_value_error : std::runtime_error {
  using std::runtime_error::runtime_error;
};

struct throwing_mapped_value {
  constexpr explicit throwing_mapped_value(int value) noexcept : value_{value} {}
  constexpr throwing_mapped_value() { throw throwing_mapped_value_error{"default construction"}; }
  constexpr throwing_mapped_value(throwing_mapped_value const&) { throw throwing_mapped_value_error{"copy construction"}; }
  constexpr throwing_mapped_value(throwing_mapped_value&&) noexcept = default;

  constexpr auto operator=(throwing_mapped_value const&) -> throwing_mapped_value& = delete;
  constexpr auto operator=(throwing_mapped_value&&) noexcept -> throwing_mapped_value& = default;

  friend constexpr auto operator==(throwing_mapped_value const&, throwing_mapped_value const&) -> bool {
    throw throwing_mapped_value_error{"equality"};
  }

  int value_{};
};

using IntMap = frozen_map<int, "key"_fs>;
using ThrowingMap = frozen_map<throwing_mapped_value, "key"_fs>;
using IntMultimap = frozen_multimap<int, "key"_fs>;
using ThrowingMultimap = frozen_multimap<throwing_mapped_value, "key"_fs>;
using IntTrieMap = frozen_trie_map<int, "key"_fs>;
using ThrowingTrieMap = frozen_trie_map<throwing_mapped_value, "key"_fs>;

static_assert(noexcept(IntMap{}));
static_assert(!noexcept(ThrowingMap{}));
static_assert(noexcept(std::declval<IntMap const&>().get_value_or("key", std::declval<int const&>())));
static_assert(!noexcept(std::declval<ThrowingMap const&>().get_value_or("key", std::declval<throwing_mapped_value const&>())));
static_assert(noexcept(std::declval<IntMap const&>() == std::declval<IntMap const&>()));
static_assert(!noexcept(std::declval<ThrowingMap const&>() == std::declval<ThrowingMap const&>()));
static_assert(noexcept(IntMultimap{}));
static_assert(!noexcept(ThrowingMultimap{}));
static_assert(noexcept(IntTrieMap{}));
static_assert(!noexcept(ThrowingTrieMap{}));

} // namespace

TEST_CASE("mapped value exceptions propagate from conditional noexcept map operations", "[frozen_map][noexcept]") {
  REQUIRE_THROWS_AS(ThrowingMap{}, throwing_mapped_value_error);
  REQUIRE_THROWS_AS(ThrowingMultimap{}, throwing_mapped_value_error);
  REQUIRE_THROWS_AS(ThrowingTrieMap{}, throwing_mapped_value_error);

  auto map = ThrowingMap{std::array<throwing_mapped_value, 1>{throwing_mapped_value{1}}};
  auto const fallback = throwing_mapped_value{2};
  REQUIRE_THROWS_AS(map.get_value_or("key", fallback), throwing_mapped_value_error);

  auto const lhs = ThrowingMap{std::array<throwing_mapped_value, 1>{throwing_mapped_value{3}}};
  auto const rhs = ThrowingMap{std::array<throwing_mapped_value, 1>{throwing_mapped_value{3}}};
  REQUIRE_THROWS_AS(lhs == rhs, throwing_mapped_value_error);
}
