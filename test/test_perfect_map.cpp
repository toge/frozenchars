#include "catch2/catch_all.hpp"

#include <concepts>
#include <tuple>

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

} // namespace

TEST_CASE("PerfectMap supports declaration-order initialization", "[perfect_map]") {
  PerfectMap<int, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array<int, 3>{30, 7, 2}
  };

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 7);
  REQUIRE(map["backoff"] == 2);
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
