#include "catch2/catch_all.hpp"

#include <array>
#include <concepts>
#include <optional>
#include <string_view>
#include <utility>

#include "frozenchars/bimap.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("frozen_bimap inherits frozen_map lookup", "[frozen_bimap]") {
  frozen_bimap<int, "status_ok"_fs, "status_err"_fs, "status_retry"_fs> map{
    std::array<int, 3>{200, 500, 429}
  };

  REQUIRE(map.at("status_ok")->get() == 200);
  REQUIRE(map.contains("status_err"));
  REQUIRE_FALSE(map.contains("status_unknown"));
  REQUIRE(map.get("status_retry").has_value());
}

TEST_CASE("frozen_bimap by_value reverse lookup", "[frozen_bimap]") {
  frozen_bimap<int, "status_ok"_fs, "status_err"_fs, "status_retry"_fs> map{
    std::array<int, 3>{200, 500, 429}
  };

  REQUIRE(map.by_value(200) == "status_ok");
  REQUIRE(map.by_value(500) == "status_err");
  REQUIRE(map.by_value(404) == std::nullopt);
  REQUIRE(map.contains_value(429));
  REQUIRE_FALSE(map.contains_value(999));
}

TEST_CASE("frozen_bimap by_value returns first match", "[frozen_bimap]") {
  frozen_bimap<int, "first"_fs, "second"_fs> map{std::array<int, 2>{7, 7}};

  REQUIRE(map.by_value(7) == "first");
}

TEST_CASE("frozen_bimap by_value reflects mutable values", "[frozen_bimap]") {
  auto map = make_frozen_bimap<int, "a"_fs, "b"_fs>(std::array<int, 2>{1, 2});

  map["a"] = 42;
  REQUIRE(map.by_value(42) == "a");
  REQUIRE(map.by_value(1) == std::nullopt);
}

TEST_CASE("frozen_bimap make_frozen_bimap with pair entries", "[frozen_bimap]") {
  constexpr auto map = make_frozen_bimap<int, "x"_fs, "y"_fs>(
    std::pair{"x", 10},
    std::pair{"y", 20}
  );

  STATIC_CHECK(map.by_value(20) == "y");
  STATIC_CHECK(map.by_value(10) == "x");
  STATIC_CHECK(!map.contains_value(999));
}

TEST_CASE("frozen_bimap make_frozen_bimap_kv builds from compile-time kv entries", "[frozen_bimap]") {
  auto map = make_frozen_bimap_kv<int,
    kv{"ok", 200},
    kv{"error", 500}
  >();

  static_assert(std::same_as<
    decltype(map),
    frozen_bimap<int, "ok"_fs, "error"_fs>>);
  REQUIRE(map["ok"] == 200);
  REQUIRE(map.by_value(500) == "error");
}

TEST_CASE("frozen_bimap contains_all inherited from frozen_map", "[frozen_bimap]") {
  constexpr auto map = make_frozen_bimap<int, "a"_fs, "b"_fs, "c"_fs>(
    std::array<int, 3>{1, 2, 3});

  STATIC_CHECK(map.contains_all<"a"_fs, "c"_fs>());
  STATIC_CHECK(map.contains_all<>());
  STATIC_CHECK(!map.contains_all<"a"_fs, "z"_fs>());
}
