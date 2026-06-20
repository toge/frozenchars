#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("contains<Substr>(str) - NTTP version") {
  STATIC_CHECK("hello world"_fs | ops::contains<"world">);
  STATIC_CHECK(!("hello"_fs | ops::contains<"xyz">));
  STATIC_CHECK("abc"_fs | ops::contains<"abc">);
  STATIC_CHECK(!("abc"_fs | ops::contains<"abcd">));
}

TEST_CASE("contains<Substr>(str) - FrozenString version") {
  constexpr auto str = "hello world"_fs;
  constexpr auto found = "world"_fs;
  constexpr auto not_found = "xyz"_fs;
  STATIC_CHECK(contains<found>(str));
  STATIC_CHECK(!contains<not_found>(str));
}

TEST_CASE("contains<Substr>(str) - char[] version") {
  STATIC_CHECK(contains<"world">("hello world"));
  STATIC_CHECK(!contains<"xyz">("hello"));
}
