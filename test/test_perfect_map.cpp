#include "catch2/catch_all.hpp"

#include <concepts>
#include <ranges>
#include <tuple>
#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/perfect_map.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("PerfectMap basic shape", "[perfect_map]") {
  PerfectMap<int, "timeout"_fs, "retry"_fs> map{};

  static_assert(PerfectMap<int, "timeout"_fs, "retry"_fs>::size() == 2);
  REQUIRE(map.contains("timeout"));
  REQUIRE(map.contains("retry"));
  REQUIRE_FALSE(map.contains("other"));
}

TEST_CASE("PerfectMap exposes container-like type aliases and size helpers", "[perfect_map]") {
  using Map = PerfectMap<int, "timeout"_fs, "retry"_fs>;

  static_assert(std::same_as<Map::key_type, std::string_view>);
  static_assert(std::same_as<Map::mapped_type, int>);
  static_assert(std::same_as<Map::size_type, std::size_t>);
  static_assert(std::same_as<Map::difference_type, std::ptrdiff_t>);
  static_assert(Map::size() == 2);
  static_assert(Map::max_size() == 2);
  static_assert(!Map::empty());
}

TEST_CASE("PerfectMap derives compile-time seed metadata", "[perfect_map]") {
  static_assert(detail::fnv1a_hash("timeout", 0) == 6954259676504937608ull);
  static_assert(detail::find_seed<1'000'001, "timeout"_fs, "retry"_fs, "backoff"_fs>() == 13u);
  static_assert(detail::fnv1a_hash("timeout", 13) % 3 == 0);
  static_assert(detail::fnv1a_hash("retry", 13) % 3 == 2);
  static_assert(detail::fnv1a_hash("backoff", 13) % 3 == 1);
}

TEST_CASE("PerfectMap lookup and miss handling", "[perfect_map]") {
  PerfectMap<int, "timeout"_fs, "retry"_fs> map{};

  map["timeout"] = 30;
  map["retry"] = 5;

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
  REQUIRE(map.at("timeout")->get() == 30);
  REQUIRE_FALSE(map.at("missing").has_value());
  REQUIRE_THROWS_AS(map["missing"], std::out_of_range);
}

TEST_CASE("PerfectMap supports find and count", "[perfect_map]") {
  PerfectMap<int, "timeout"_fs, "retry"_fs> map{};
  map["timeout"] = 30;

  auto const found = map.find("timeout");
  auto const missing = map.find("missing");

  REQUIRE(found != map.end());
  REQUIRE(missing == map.end());
  REQUIRE(map.count("timeout") == 1);
  REQUIRE(map.count("missing") == 0);
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

TEST_CASE("PerfectMap supports declaration-order initialization", "[perfect_map]") {
  PerfectMap<int, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array<int, 3>{30, 7, 2}
  };

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 7);
  REQUIRE(map["backoff"] == 2);
}

TEST_CASE("PerfectMap supports keyed entry initialization", "[perfect_map]") {
  PerfectMap<int, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array{
      PerfectMapEntry<int>{"retry", 7},
      PerfectMapEntry<int>{"backoff", 2},
      PerfectMapEntry<int>{"timeout", 30},
    }
  };

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 7);
  REQUIRE(map["backoff"] == 2);
}

TEST_CASE("PerfectMap make_perfect_map builds from pair-like entries", "[perfect_map]") {
  auto map = make_perfect_map<int, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", 5},
    std::pair{"timeout", 30}
  );

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
}

TEST_CASE("PerfectMap keyed initialization rejects unknown keys", "[perfect_map]") {
  REQUIRE_THROWS_AS(
    (PerfectMap<int, "timeout"_fs, "retry"_fs>{
      std::array{
        PerfectMapEntry<int>{"timeout", 30},
        PerfectMapEntry<int>{"other", 5},
      }
    }),
    std::invalid_argument
  );
}

TEST_CASE("PerfectMap keyed initialization rejects duplicate keys", "[perfect_map]") {
  REQUIRE_THROWS_AS(
    (PerfectMap<int, "timeout"_fs, "retry"_fs>{
      std::array{
        PerfectMapEntry<int>{"timeout", 30},
        PerfectMapEntry<int>{"timeout", 5},
      }
    }),
    std::invalid_argument
  );
}

TEST_CASE("PerfectMap keyed initialization supports non-default-constructible values",
    "[perfect_map]") {
  auto map = make_perfect_map<NoDefault, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", NoDefault{5}},
    std::pair{"timeout", NoDefault{30}}
  );

  REQUIRE(map["timeout"].value == 30);
  REQUIRE(map["retry"].value == 5);
}

TEST_CASE("PerfectMap keyed initialization supports move-only values", "[perfect_map]") {
  auto map = make_perfect_map<MoveOnly, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", MoveOnly{5}},
    std::pair{"timeout", MoveOnly{30}}
  );

  REQUIRE(map["timeout"].value == 30);
  REQUIRE(map["retry"].value == 5);
}

TEST_CASE("PerfectMap make_perfect_map accepts pair-like entries", "[perfect_map]") {
  auto map = make_perfect_map<int, "timeout"_fs, "retry"_fs>(
    PairLikeEntry<int>{"retry", 5},
    PairLikeEntry<int>{"timeout", 30}
  );

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
}

TEST_CASE("PerfectMap example supports make_perfect_map", "[perfect_map]") {
  auto map = make_perfect_map<int, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", 5},
    std::pair{"timeout", 30}
  );

  REQUIRE(map.find("timeout") != map.end());
  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
}

TEST_CASE("PerfectMap const access returns const references", "[perfect_map]") {
  auto const map = PerfectMap<int, "timeout"_fs, "retry"_fs, "backoff"_fs>{
    std::array<int, 3>{30, 7, 2}
  };

  static_assert(std::same_as<decltype(map.at("timeout")->get()), int const&>);
  REQUIRE(map.at("timeout")->get() == 30);
  REQUIRE(map["timeout"] == 30);
}

static_assert(!std::default_initializable<PerfectMap<NoDefault, "timeout"_fs>>);

TEST_CASE("PerfectMap example flow", "[perfect_map]") {
  PerfectMap<int, "timeout"_fs, "retry"_fs> map{};
  map["timeout"] = 30;
  map["retry"] = 5;

  auto const pair = std::tuple{map["timeout"], map["retry"]};
  auto const timeout = std::get<0>(pair);

  REQUIRE(timeout == 30);
}

TEST_CASE("PerfectMap iterates with key and value access", "[perfect_map]") {
  PerfectMap<int, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
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

TEST_CASE("PerfectMap structured bindings mutate mapped value", "[perfect_map]") {
  PerfectMap<int, "timeout"_fs, "retry"_fs> map{};

  for (auto&& [key, value] : map) {
    if (key == "timeout") {
      value = 42;
    }
  }

  REQUIRE(map["timeout"] == 42);
}

TEST_CASE("PerfectMap const iteration uses cbegin and cend", "[perfect_map]") {
  auto const map = PerfectMap<int, "timeout"_fs, "retry"_fs, "backoff"_fs>{
    std::array<int, 3>{30, 7, 2}
  };

  auto it = map.cbegin();
  REQUIRE(it != map.cend());

  auto const [key, value] = *it;
  REQUIRE_FALSE(key.empty());
  REQUIRE((value == 30 || value == 7 || value == 2));
}

TEST_CASE("PerfectMap example supports find and iterator loops", "[perfect_map]") {
  PerfectMap<int, "timeout"_fs, "retry"_fs> map{};
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
