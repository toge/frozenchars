#include "catch2/catch_all.hpp"

#include <concepts>

#include "frozenchars/literals.hpp"
#include "frozenchars/static_perfect_map.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("StaticPerfectMap basic shape", "[static_perfect_map]") {
  StaticPerfectMap<int, "timeout"_fs, "retry"_fs> map{};

  static_assert(StaticPerfectMap<int, "timeout"_fs, "retry"_fs>::size() == 2);
  REQUIRE(map.contains("timeout"));
  REQUIRE(map.contains("retry"));
  REQUIRE_FALSE(map.contains("other"));
}

TEST_CASE("StaticPerfectMap derives compile-time seed metadata", "[static_perfect_map]") {
  static_assert(detail::fnv1a_hash("timeout", 0) == 6954259676504937608ull);
  static_assert(detail::find_seed<1'000'001, "timeout"_fs, "retry"_fs, "backoff"_fs>() == 13u);
  static_assert(detail::fnv1a_hash("timeout", 13) % 3 == 0);
  static_assert(detail::fnv1a_hash("retry", 13) % 3 == 2);
  static_assert(detail::fnv1a_hash("backoff", 13) % 3 == 1);
}

TEST_CASE("StaticPerfectMap lookup and miss handling", "[static_perfect_map]") {
  StaticPerfectMap<int, "timeout"_fs, "retry"_fs> map{};

  map["timeout"] = 30;
  map["retry"] = 5;

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 5);
  REQUIRE(map.at("timeout")->get() == 30);
  REQUIRE_FALSE(map.at("missing").has_value());
  REQUIRE_THROWS_AS(map["missing"], std::out_of_range);
}

namespace {

struct NoDefault {
  explicit constexpr NoDefault(int initial_value) noexcept
  : value{initial_value} {
  }

  int value;
};

} // namespace

TEST_CASE("StaticPerfectMap supports declaration-order initialization", "[static_perfect_map]") {
  StaticPerfectMap<int, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array<int, 3>{30, 7, 2}
  };

  REQUIRE(map["timeout"] == 30);
  REQUIRE(map["retry"] == 7);
  REQUIRE(map["backoff"] == 2);
}

TEST_CASE("StaticPerfectMap const access returns const references", "[static_perfect_map]") {
  auto const map = StaticPerfectMap<int, "timeout"_fs, "retry"_fs, "backoff"_fs>{
    std::array<int, 3>{30, 7, 2}
  };

  static_assert(std::same_as<decltype(map.at("timeout")->get()), int const&>);
  REQUIRE(map.at("timeout")->get() == 30);
  REQUIRE(map["timeout"] == 30);
}

static_assert(!std::default_initializable<StaticPerfectMap<NoDefault, "timeout"_fs>>);
