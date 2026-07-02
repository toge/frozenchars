#include "catch2/catch_all.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <string>
#include <string_view>

#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("frozen_regex literal match", "[frozen_regex]") {
  using R = frozen_regex<"a"_fs>;
  static_assert(R::count_v == 1);
  static_assert(R::max_length_v == 1);
  REQUIRE(R::contains("a"));
  REQUIRE_FALSE(R::contains("b"));
  REQUIRE_FALSE(R::contains(""));
  REQUIRE_FALSE(R::contains("aa"));
}

TEST_CASE("frozen_regex concatenation", "[frozen_regex]") {
  using R = frozen_regex<"abc"_fs>;
  static_assert(R::count_v == 1);
  static_assert(R::max_length_v == 3);
  REQUIRE(R::contains("abc"));
  REQUIRE_FALSE(R::contains("ab"));
  REQUIRE_FALSE(R::contains("abcd"));
  REQUIRE_FALSE(R::contains(""));
}

TEST_CASE("frozen_regex alternation", "[frozen_regex]") {
  using R = frozen_regex<R"(GET|POST)"_fs>;
  static_assert(R::count_v == 2);
  REQUIRE(R::contains("GET"));
  REQUIRE(R::contains("POST"));
  REQUIRE_FALSE(R::contains("PUT"));
  REQUIRE_FALSE(R::contains(""));
}

TEST_CASE("frozen_regex alternation dedup", "[frozen_regex]") {
  using R = frozen_regex<R"(a|a)"_fs>;
  static_assert(R::count_v == 1);
  REQUIRE(R::contains("a"));
}

TEST_CASE("frozen_regex char class explicit", "[frozen_regex]") {
  using R = frozen_regex<"[abc]"_fs>;
  static_assert(R::count_v == 3);
  REQUIRE(R::contains("a"));
  REQUIRE(R::contains("b"));
  REQUIRE(R::contains("c"));
  REQUIRE_FALSE(R::contains("d"));
}

TEST_CASE("frozen_regex char class range", "[frozen_regex]") {
  using R = frozen_regex<"[a-z]"_fs>;
  static_assert(R::count_v == 26);
  REQUIRE(R::contains("a"));
  REQUIRE(R::contains("m"));
  REQUIRE(R::contains("z"));
  REQUIRE_FALSE(R::contains("A"));
  REQUIRE_FALSE(R::contains("0"));
}

TEST_CASE("frozen_regex char class negate", "[frozen_regex]") {
  using R = frozen_regex<"[^a]"_fs>;
  static_assert(R::count_v == 62);
  REQUIRE_FALSE(R::contains("a"));
  REQUIRE(R::contains("b"));
  REQUIRE(R::contains("A"));
  REQUIRE(R::contains("0"));
  REQUIRE(R::contains("_"));
  REQUIRE_FALSE(R::contains("-"));
}

TEST_CASE("frozen_regex dot default charset", "[frozen_regex]") {
  using R = frozen_regex<"."_fs>;
  static_assert(R::count_v == 63);
  REQUIRE(R::contains("a"));
  REQUIRE(R::contains("Z"));
  REQUIRE(R::contains("0"));
  REQUIRE(R::contains("_"));
  REQUIRE_FALSE(R::contains("-"));
  REQUIRE_FALSE(R::contains("/"));
  REQUIRE_FALSE(R::contains("."));
}

TEST_CASE("frozen_regex group", "[frozen_regex]") {
  using R = frozen_regex<R"((ab|cd))"_fs>;
  static_assert(R::count_v == 2);
  REQUIRE(R::contains("ab"));
  REQUIRE(R::contains("cd"));
  REQUIRE_FALSE(R::contains("abc"));
}

TEST_CASE("frozen_regex concat with group", "[frozen_regex]") {
  using R = frozen_regex<R"(a(b|c))"_fs>;
  static_assert(R::count_v == 2);
  REQUIRE(R::contains("ab"));
  REQUIRE(R::contains("ac"));
  REQUIRE_FALSE(R::contains("a"));
  REQUIRE_FALSE(R::contains("abc"));
}

TEST_CASE("frozen_regex escape dot", "[frozen_regex]") {
  using R = frozen_regex<R"(a\.\.b)"_fs>;
  static_assert(R::count_v == 1);
  REQUIRE(R::contains("a..b"));
  REQUIRE_FALSE(R::contains("a.b"));
}

TEST_CASE("frozen_regex escape pipe", "[frozen_regex]") {
  using R = frozen_regex<R"(a\|b)"_fs>;
  static_assert(R::count_v == 1);
  REQUIRE(R::contains("a|b"));
  REQUIRE_FALSE(R::contains("a"));
  REQUIRE_FALSE(R::contains("b"));
}

TEST_CASE("frozen_regex escape paren", "[frozen_regex]") {
  using R = frozen_regex<R"(\(a\))"_fs>;
  static_assert(R::count_v == 1);
  REQUIRE(R::contains("(a)"));
}

TEST_CASE("frozen_regex enumerate returns sorted", "[frozen_regex]") {
  using R = frozen_regex<R"(POST|GET|PUT)"_fs>;
  auto const arr = R::enumerate();
  static_assert(decltype(R::enumerate())::extent == 3);
  REQUIRE(arr.size() == 3);
  REQUIRE(arr[0].sv() == "GET");
  REQUIRE(arr[1].sv() == "POST");
  REQUIRE(arr[2].sv() == "PUT");
}

TEST_CASE("frozen_regex keys returns sorted span", "[frozen_regex]") {
  using R = frozen_regex<R"(POST|GET|PUT)"_fs>;
  auto const views = R::keys();
  REQUIRE(views.size() == 3);
  REQUIRE(views[0] == "GET");
  REQUIRE(views[1] == "POST");
  REQUIRE(views[2] == "PUT");
}

TEST_CASE("frozen_regex to_frozen_map int values", "[frozen_regex]") {
  using R = frozen_regex<R"(GET|POST|PUT)"_fs>;
  auto const map = R::template to_frozen_map<int, 200, 201, 204>();
  static_assert(map.size() == 3);
  REQUIRE(map.at("GET") == 200);
  REQUIRE(map.at("POST") == 201);
  REQUIRE(map.at("PUT") == 204);
}

TEST_CASE("frozen_regex to_frozen_map double values", "[frozen_regex]") {
  using R = frozen_regex<R"(dev|prd|stg)"_fs>;
  auto const map = R::template to_frozen_map<double, 0.5, 1.0, 1.5>();
  static_assert(map.size() == 3);
  REQUIRE(map.at("dev") == 0.5);
  REQUIRE(map.at("prd") == 1.0);
  REQUIRE(map.at("stg") == 1.5);
}

TEST_CASE("frozen_regex complex pattern 7 HTTP methods", "[frozen_regex]") {
  using R = frozen_regex<R"(GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS)"_fs>;
  static_assert(R::count_v == 7);
  REQUIRE(R::contains("GET"));
  REQUIRE(R::contains("POST"));
  REQUIRE(R::contains("PUT"));
  REQUIRE(R::contains("DELETE"));
  REQUIRE(R::contains("PATCH"));
  REQUIRE(R::contains("HEAD"));
  REQUIRE(R::contains("OPTIONS"));
  REQUIRE_FALSE(R::contains("TRACE"));
}

TEST_CASE("frozen_regex compile-time static_assert only", "[frozen_regex]") {
  using R = frozen_regex<R"(a(b|c)d)"_fs>;
  static_assert(R::count_v == 2);
  static_assert(R::max_length_v == 3);
  static_assert(R::contains("abd"));
  static_assert(R::contains("acd"));
  static_assert(!R::contains("a"));
  static_assert(!R::contains(""));
  static_assert(!R::contains("abcd"));
}

TEST_CASE("frozen_regex single space", "[frozen_regex]") {
  using R = frozen_regex<" "_fs>;
  static_assert(R::count_v == 1);
  static_assert(R::max_length_v == 1);
  REQUIRE(R::contains(" "));
  REQUIRE_FALSE(R::contains(""));
  REQUIRE_FALSE(R::contains("  "));
}

TEST_CASE("frozen_regex to_frozen_set alternation", "[frozen_regex][frozen_set]") {
  constexpr auto set =
    frozen_regex<"GET|POST|PUT|DELETE"_fs>::to_frozen_set();
  static_assert(set.contains("GET"));
  static_assert(set.contains("DELETE"));
  static_assert(set.contains("POST"));
  static_assert(set.contains("PUT"));
  static_assert(!set.contains("PATCH"));
  static_assert(set.size() == 4);
}

TEST_CASE("frozen_regex to_frozen_set char class", "[frozen_regex][frozen_set]") {
  constexpr auto set = frozen_regex<"[abc]"_fs>::to_frozen_set();
  static_assert(set.contains("a"));
  static_assert(set.contains("b"));
  static_assert(set.contains("c"));
  static_assert(!set.contains("d"));
  static_assert(set.size() == 3);
}

TEST_CASE("frozen_regex regex_map string length", "[frozen_regex][frozen_map]") {
  constexpr auto map =
    frozen_regex<"cat|dog|elephant|hi"_fs>::regex_map<[](std::string_view s) {
      return s.size();
    }>();
  static_assert(map.at("cat") == 3);
  static_assert(map.at("dog") == 3);
  static_assert(map.at("elephant") == 8);
  static_assert(map.at("hi") == 2);
  REQUIRE(map.at("cat") == 3);
  REQUIRE(map.at("hi") == 2);
  REQUIRE(map.find("bird") == map.end());
}

TEST_CASE("frozen_regex regex_map parse int", "[frozen_regex][frozen_map]") {
  constexpr auto map =
    frozen_regex<"100|200|300|400"_fs>::regex_map<[](std::string_view s) {
      int v{};
      std::from_chars(s.data(), s.data() + s.size(), v);
      return v;
    }>();
  static_assert(map.at("100") == 100);
  static_assert(map.at("200") == 200);
  static_assert(map.at("300") == 300);
  static_assert(map.at("400") == 400);
  REQUIRE(map.at("100") == 100);
  REQUIRE(map.at("400") == 400);
  REQUIRE(map.find("500") == map.end());
}

TEST_CASE("frozen_regex regex_map custom type", "[frozen_regex][frozen_map]") {
  struct wrapper { int value; };
  constexpr auto map =
    frozen_regex<"a|b|c"_fs>::regex_map<[](std::string_view) {
      return wrapper{42};
    }>();
  static_assert(map.at("a").value == 42);
  static_assert(map.at("b").value == 42);
  static_assert(map.at("c").value == 42);
  REQUIRE(map.at("a").value == 42);
}

TEST_CASE("frozen_regex regex_map asserts count_v", "[frozen_regex][frozen_map]") {
  // to_frozen_map with wrong value count should fail (compile-time test via pattern match)
  using R = frozen_regex<"x|y"_fs>;
  static_assert(R::count_v == 2);
  // regex_map deduces count_v automatically from enumeration — no manual count needed
  constexpr auto map = R::regex_map<[](std::string_view) { return 1; }>();
  static_assert(map.at("x") == 1);
  static_assert(map.at("y") == 1);
}
