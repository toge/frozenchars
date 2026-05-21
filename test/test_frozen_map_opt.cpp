#include "catch2/catch_all.hpp"
#include "frozenchars/frozen_map.hpp"
#include "frozenchars/literals.hpp"
#include <string_view>

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("frozen_map Optimization A: Length filtering", "[frozen_map][opt]") {
  using Map = frozen_map<int, "foo"_fs, "bar"_fs>;
  Map map{std::array{1, 2}};

  // Length 3 is valid, but length 4 is not.
  REQUIRE(map.contains("foo"));
  REQUIRE(map.contains("bar"));
  REQUIRE_FALSE(map.contains("quxx")); // length 4, should early exit
  REQUIRE_FALSE(map.contains("f"));    // length 1, should early exit
}

TEST_CASE("frozen_map Optimization B: Hashless lookup", "[frozen_map][opt]") {
  SECTION("Unique lengths") {
    using Map = frozen_map<int, "a"_fs, "bb"_fs, "ccc"_fs>;

    // Check all_lengths_unique_ internal flag via a helper or just behavior
    // We can't access private members directly, but we can verify behavior.
    Map map{std::array{1, 2, 3}};
    REQUIRE(map.at("a") == 1);
    REQUIRE(map.at("bb") == 2);
    REQUIRE(map.at("ccc") == 3);
    REQUIRE_FALSE(map.contains("b"));
    REQUIRE_FALSE(map.contains("aa"));
    REQUIRE_FALSE(map.contains("cccc"));
  }

  SECTION("Duplicate lengths") {
    using Map = frozen_map<int, "foo"_fs, "bar"_fs, "baz"_fs>;
    Map map{std::array{1, 2, 3}};
    REQUIRE(map.at("foo") == 1);
    REQUIRE(map.at("bar") == 2);
    REQUIRE(map.at("baz") == 3);
    REQUIRE_FALSE(map.contains("qux"));
  }
}

TEST_CASE("frozen_map Optimization C: find_index_raw constexpr", "[frozen_map][opt]") {
  constexpr auto result = [] {
    auto m = frozen_map<int, "foo"_fs, "bar"_fs, "baz"_fs>{std::array{1, 2, 3}};
    return m.find("bar") != m.end();
  }();
  static_assert(result);
  REQUIRE(result);
}

TEST_CASE("frozen_map unique lengths detection", "[frozen_map][opt]") {
  // We can't easily test private static members.
  // But we can verify that O(1) path works by testing all keys.
  auto map = make_frozen_map<int, "a"_fs, "bb"_fs, "ccc"_fs>(
    std::pair{"a", 1},
    std::pair{"bb", 2},
    std::pair{"ccc", 3}
  );
  REQUIRE(map.at("a") == 1);
  REQUIRE(map.at("bb") == 2);
  REQUIRE(map.at("ccc") == 3);
}
