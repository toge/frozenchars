#include "catch2/catch_all.hpp"

#include <array>
#include <concepts>
#include <iterator>
#include <map>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_map.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("frozen_map basic shape", "[frozen_map]") {
  frozen_map<int, "timeout"_fs, "retry"_fs> map{};

  static_assert(frozen_map<int, "timeout"_fs, "retry"_fs>::size() == 2);
  REQUIRE(map.contains("timeout"));
  REQUIRE(map.contains("retry"));
  REQUIRE_FALSE(map.contains("other"));
}

TEST_CASE("frozen_map converts to requested STL containers by explicit result type",
    "[frozen_map]") {
  frozen_map<int, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array<int, 3>{30, 7, 2}
  };

  auto const ordered = map.to<std::map<std::string, int>>();
  auto const hashed = map.to<std::unordered_map<std::string_view, int>>();
  auto const array =
      map.to<std::array<std::pair<std::string_view, int>, 3>>();

  REQUIRE(ordered == std::map<std::string, int>{
    {"backoff", 2},
    {"retry", 7},
    {"timeout", 30},
  });
  REQUIRE(hashed == std::unordered_map<std::string_view, int>{
    {"timeout", 30},
    {"retry", 7},
    {"backoff", 2},
  });
  REQUIRE(array == std::array<std::pair<std::string_view, int>, 3>{
    std::pair<std::string_view, int>{"timeout", 30},
    std::pair<std::string_view, int>{"backoff", 2},
    std::pair<std::string_view, int>{"retry", 7},
  });
}

TEST_CASE("frozen_map exposes container-like type aliases and size helpers", "[frozen_map]") {
  using Map = frozen_map<int, "timeout"_fs, "retry"_fs>;

  static_assert(std::same_as<Map::key_type, std::string_view>);
  static_assert(std::same_as<Map::mapped_type, int>);
  static_assert(std::same_as<Map::size_type, std::size_t>);
  static_assert(std::same_as<Map::difference_type, std::ptrdiff_t>);
  static_assert(Map::size() == 2);
  static_assert(Map::max_size() == 2);
  static_assert(!Map::empty());
}

TEST_CASE("frozen_map derives compile-time seed metadata", "[frozen_map]") {
  static_assert(detail::fnv1a_hash("timeout", 0) == 6954259676504937608ull);
  static_assert(detail::find_seed<1'000'001, "timeout"_fs, "retry"_fs, "backoff"_fs>() == 13u);
  static_assert(detail::fnv1a_hash("timeout", 13) % 3 == 0);
  static_assert(detail::fnv1a_hash("retry", 13) % 3 == 2);
  static_assert(detail::fnv1a_hash("backoff", 13) % 3 == 1);
}

TEST_CASE("frozen_map lookup and miss handling", "[frozen_map]") {
  frozen_map<int, "timeout"_fs, "retry"_fs> map{};

  map["timeout"] = 30;
  map["retry"] = 5;

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
  REQUIRE(map.at("timeout") == 30);
  REQUIRE(map.get("timeout")->get() == 30);
  REQUIRE_FALSE(map.get("missing").has_value());
  REQUIRE_THROWS_AS(map.at("missing"), std::out_of_range);
  REQUIRE_THROWS_AS(map["missing"], std::out_of_range);
}

TEST_CASE("frozen_map supports find and count", "[frozen_map]") {
  frozen_map<int, "timeout"_fs, "retry"_fs> map{};
  map["timeout"] = 30;

  auto const found = map.find("timeout");
  auto const missing = map.find("missing");

  REQUIRE(found != map.end());
  REQUIRE(missing == map.end());
  REQUIRE(map.count("timeout") == 1);
  REQUIRE(map.count("missing") == 0);
}

static_assert(std::default_initializable<
  frozen_map<int, "timeout"_fs, "retry"_fs>::iterator>);
static_assert(std::default_initializable<
  frozen_map<int, "timeout"_fs, "retry"_fs>::const_iterator>);
static_assert(std::same_as<
  decltype(std::declval<frozen_map<int, "timeout"_fs, "retry"_fs>::iterator const&>().operator->()),
  frozen_map<int, "timeout"_fs, "retry"_fs>::iterator::arrow_proxy>);
static_assert(std::same_as<
  decltype(std::declval<frozen_map<int, "timeout"_fs, "retry"_fs>::const_iterator const&>().operator->()),
  frozen_map<int, "timeout"_fs, "retry"_fs>::const_iterator::arrow_proxy>);
using IntPerfectMap = frozen_map<int, "timeout"_fs, "retry"_fs>;
static_assert(requires { { IntPerfectMap::size() } -> std::same_as<IntPerfectMap::size_type>; });
static_assert(requires(IntPerfectMap& map, IntPerfectMap const& cmap) {
  { map.find("timeout") };
  { cmap.find("timeout") };
  { map.at("timeout") } -> std::same_as<int&>;
  { cmap.at("timeout") } -> std::same_as<int const&>;
  { map.get("timeout") } -> std::same_as<std::optional<std::reference_wrapper<int>>>;
  { cmap.get("timeout") } -> std::same_as<std::optional<std::reference_wrapper<int const>>>;
  { cmap.contains("timeout") } -> std::same_as<bool>;
  { cmap.count("timeout") } -> std::same_as<IntPerfectMap::size_type>;
});

TEST_CASE("frozen_map iterators model forward_iterator", "[frozen_map]") {
  using Map = frozen_map<int, "timeout"_fs, "retry"_fs>;
  static_assert(std::forward_iterator<Map::iterator>);
  static_assert(std::forward_iterator<Map::const_iterator>);
}

TEST_CASE("frozen_map default-constructed iterators compare equal by type", "[frozen_map]") {
  using Map = frozen_map<int, "timeout"_fs, "retry"_fs>;
  REQUIRE(Map::iterator{} == Map::iterator{});
  REQUIRE(Map::const_iterator{} == Map::const_iterator{});
}

TEST_CASE("frozen_map iterator operator-> exposes key and value", "[frozen_map]") {
  auto map = make_frozen_map<int, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", 5},
    std::pair{"timeout", 30}
  );

  auto it = map.find("timeout");
  REQUIRE(it != map.end());
  REQUIRE(it->key == "timeout");
  REQUIRE(it->value == 30);
}

TEST_CASE("frozen_map const_iterator operator-> exposes key and value", "[frozen_map]") {
  auto const map = make_frozen_map<int, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", 5},
    std::pair{"timeout", 30}
  );

  auto it = map.find("timeout");
  REQUIRE(it != map.end());
  REQUIRE(it->key == "timeout");
  REQUIRE(it->value == 30);
}

TEST_CASE("frozen_map iterator operator-> preserves writable mapped access", "[frozen_map]") {
  auto map = make_frozen_map<int, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", 5},
    std::pair{"timeout", 30}
  );

  auto it = map.find("timeout");
  REQUIRE(it != map.end());
  it->value += 7;
  REQUIRE(map["timeout"] == 37);
}

namespace {

struct NoDefault {
  explicit constexpr NoDefault(int initial_value) noexcept
  : value{initial_value} {
  }

  int value;
};

struct MoveOnly {
  explicit MoveOnly(int initial_value) noexcept
  : value{initial_value} {
  }

  MoveOnly(MoveOnly&&) noexcept = default;
  auto operator=(MoveOnly&&) noexcept -> MoveOnly& = default;
  MoveOnly(MoveOnly const&) = delete;
  auto operator=(MoveOnly const&) -> MoveOnly& = delete;

  int value;
};

TEST_CASE("frozen_map moves values when converting rvalues", "[frozen_map]") {
  auto map = make_frozen_map<MoveOnly, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", MoveOnly{5}},
    std::pair{"timeout", MoveOnly{30}}
  );

  auto moved = std::move(map).to<std::unordered_map<std::string, MoveOnly>>();

  REQUIRE(moved.at("timeout").value == 30);
  REQUIRE(moved.at("retry").value == 5);
}

template <typename T>
struct PairLikeEntry {
  std::string_view key;
  T value;
};

template <std::size_t I, typename T>
constexpr decltype(auto) get(PairLikeEntry<T>& entry) noexcept {
  static_assert(I < 2);
  if constexpr (I == 0) {
    return (entry.key);
  } else {
    return (entry.value);
  }
}

template <std::size_t I, typename T>
constexpr decltype(auto) get(PairLikeEntry<T> const& entry) noexcept {
  static_assert(I < 2);
  if constexpr (I == 0) {
    return entry.key;
  } else {
    return (entry.value);
  }
}

template <std::size_t I, typename T>
constexpr decltype(auto) get(PairLikeEntry<T>&& entry) noexcept {
  static_assert(I < 2);
  if constexpr (I == 0) {
    return std::move(entry.key);
  } else {
    return std::move(entry.value);
  }
}

} // namespace

namespace std {

template <typename T>
struct tuple_size<PairLikeEntry<T>>
  : integral_constant<std::size_t, 2> {
};

template <typename T>
struct tuple_element<0, PairLikeEntry<T>> {
  using type = std::string_view;
};

template <typename T>
struct tuple_element<1, PairLikeEntry<T>> {
  using type = T;
};

} // namespace std

TEST_CASE("frozen_map supports declaration-order initialization", "[frozen_map]") {
  frozen_map<int, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array<int, 3>{30, 7, 2}
  };

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 7);
  REQUIRE(map["backoff"] == 2);
}

TEST_CASE("frozen_map supports keyed entry initialization", "[frozen_map]") {
  frozen_map<int, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array{
      frozen_map_entry<int>{"retry", 7},
      frozen_map_entry<int>{"backoff", 2},
      frozen_map_entry<int>{"timeout", 30},
    }
  };

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 7);
  REQUIRE(map["backoff"] == 2);
}

TEST_CASE("frozen_map make_frozen_map builds from pair-like entries", "[frozen_map]") {
  auto map = make_frozen_map<int, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", 5},
    std::pair{"timeout", 30}
  );

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
}

TEST_CASE("frozen_map keyed initialization rejects unknown keys", "[frozen_map]") {
  REQUIRE_THROWS_AS(
    (frozen_map<int, "timeout"_fs, "retry"_fs>{
      std::array{
        frozen_map_entry<int>{"timeout", 30},
        frozen_map_entry<int>{"other", 5},
      }
    }),
    std::invalid_argument
  );
}

TEST_CASE("frozen_map keyed initialization rejects duplicate keys", "[frozen_map]") {
  REQUIRE_THROWS_AS(
    (frozen_map<int, "timeout"_fs, "retry"_fs>{
      std::array{
        frozen_map_entry<int>{"timeout", 30},
        frozen_map_entry<int>{"timeout", 5},
      }
    }),
    std::invalid_argument
  );
}

TEST_CASE("frozen_map keyed initialization supports non-default-constructible values",
    "[frozen_map]") {
  auto map = make_frozen_map<NoDefault, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", NoDefault{5}},
    std::pair{"timeout", NoDefault{30}}
  );

  REQUIRE(map["timeout"].value == 30);
  REQUIRE(map["retry"].value == 5);
}

TEST_CASE("frozen_map keyed initialization supports move-only values", "[frozen_map]") {
  auto map = make_frozen_map<MoveOnly, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", MoveOnly{5}},
    std::pair{"timeout", MoveOnly{30}}
  );

  REQUIRE(map["timeout"].value == 30);
  REQUIRE(map["retry"].value == 5);
}

TEST_CASE("frozen_map make_frozen_map accepts pair-like entries", "[frozen_map]") {
  auto map = make_frozen_map<int, "timeout"_fs, "retry"_fs>(
    PairLikeEntry<int>{"retry", 5},
    PairLikeEntry<int>{"timeout", 30}
  );

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
}

TEST_CASE("frozen_map make_frozen_map accepts std::tuple entries", "[frozen_map]") {
  auto map = make_frozen_map<int, "timeout"_fs, "retry"_fs>(
    std::tuple{"retry", 5},
    std::tuple{"timeout", 30}
  );

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
}

TEST_CASE("frozen_map make_frozen_map_kv builds from compile-time kv entries", "[frozen_map]") {
  auto map = make_frozen_map_kv<int,
    kv{"retry", 5},
    kv{"timeout", 30},
    kv{"backoff", 2}
  >();

  static_assert(std::same_as<
    decltype(map),
    frozen_map<int, "retry"_fs, "timeout"_fs, "backoff"_fs>>);
  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
  REQUIRE(map["backoff"] == 2);
}

TEST_CASE("frozen_map example supports make_frozen_map", "[frozen_map]") {
  auto map = make_frozen_map<int, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", 5},
    std::pair{"timeout", 30}
  );

  REQUIRE(map.find("timeout") != map.end());
  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
}

TEST_CASE("frozen_map const access returns const references", "[frozen_map]") {
  auto const map = frozen_map<int, "timeout"_fs, "retry"_fs, "backoff"_fs>{
    std::array<int, 3>{30, 7, 2}
  };

  static_assert(std::same_as<decltype(map.at("timeout")), int const&>);
  REQUIRE(map.at("timeout") == 30);
  REQUIRE(map.get("timeout")->get() == 30);
  REQUIRE(map["timeout"] == 30);
}

static_assert(!std::default_initializable<frozen_map<NoDefault, "timeout"_fs>>);

TEST_CASE("frozen_map example flow", "[frozen_map]") {
  frozen_map<int, "timeout"_fs, "retry"_fs> map{};
  map["timeout"] = 30;
  map["retry"] = 5;

  auto const pair = std::tuple{map["timeout"], map["retry"]};
  auto const timeout = std::get<0>(pair);

  REQUIRE(timeout == 30);
}

TEST_CASE("frozen_map iterates with key and value access", "[frozen_map]") {
  frozen_map<int, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array<int, 3>{30, 7, 2}
  };

  std::array<std::string_view, 3> keys{};
  std::array<int, 3> values{};
  auto index = 0uz;

  for (auto&& [key, value] : map) {
    keys[index] = key;
    values[index] = value;
    ++index;
  }

  REQUIRE(index == 3);
  REQUIRE(std::ranges::find(values, 30) != values.end());
  REQUIRE(keys == std::array<std::string_view, 3>{"timeout", "backoff", "retry"});
}

TEST_CASE("frozen_map iterates with key and value access (string)", "[frozen_map]") {
  frozen_map<std::string_view, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array<std::string_view, 3>{"100", "200", "300"}
  };

  std::array<std::string_view, 3> keys{};
  std::array<std::string_view, 3> values{};
  auto index = 0uz;

  for (auto&& [key, value] : map) {
    keys[index] = key;
    values[index] = value;
    ++index;
  }

  REQUIRE(index == 3);
  REQUIRE(std::ranges::find(values, std::string_view{"300"}) != values.end());
  REQUIRE(keys == std::array<std::string_view, 3>{"timeout", "backoff", "retry"});
}

TEST_CASE("frozen_map supports std::string declaration-order initialization", "[frozen_map]") {
  frozen_map<std::string, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array<std::string, 3>{"100", "200", "300"}
  };

  REQUIRE(map["timeout"] == "100");
  REQUIRE(map["retry"] == "200");
  REQUIRE(map["backoff"] == "300");
}

TEST_CASE("frozen_map supports std::string initializer_list initialization", "[frozen_map]") {
  frozen_map<std::string, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    "100", "200", "300"
  };

  REQUIRE(map["timeout"] == "100");
  REQUIRE(map["retry"] == "200");
  REQUIRE(map["backoff"] == "300");
}

TEST_CASE("frozen_map initializer_list initialization rejects wrong size", "[frozen_map]") {
  REQUIRE_THROWS_WITH(
    (frozen_map<std::string, "timeout"_fs, "retry"_fs, "backoff"_fs>{
      "100", "200"
    }),
    Catch::Matchers::ContainsSubstring(
      "expected 3 values (one per key), got 2"
    )
  );
}

TEST_CASE("frozen_map structured bindings mutate mapped value", "[frozen_map]") {
  frozen_map<int, "timeout"_fs, "retry"_fs> map{};

  for (auto&& [key, value] : map) {
    if (key == "timeout") {
      value = 42;
    }
  }

  REQUIRE(map["timeout"] == 42);
}

TEST_CASE("frozen_map const iteration uses cbegin and cend", "[frozen_map]") {
  auto const map = frozen_map<int, "timeout"_fs, "retry"_fs, "backoff"_fs>{
    std::array<int, 3>{30, 7, 2}
  };

  auto it = map.cbegin();
  REQUIRE(it != map.cend());

  auto const [key, value] = *it;
  REQUIRE_FALSE(key.empty());
  REQUIRE((value == 30 || value == 7 || value == 2));
}

TEST_CASE("frozen_map example supports find and iterator loops", "[frozen_map]") {
  frozen_map<int, "timeout"_fs, "retry"_fs> map{};
  map["timeout"] = 30;
  map["retry"] = 5;

  auto const found = map.find("timeout");
  REQUIRE(found != map.end());

  auto total = 0;
  for (auto it = map.begin(); it != map.end(); it++) {
    auto const [key, value] = *it;
    std::ignore = key;
    total += value;
  }

  REQUIRE(total == 35);
}
