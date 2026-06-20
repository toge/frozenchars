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

TEST_CASE("starts_with<Prefix>(str) - NTTP version") {
  STATIC_CHECK("hello"_fs | ops::starts_with<"hel">);
  STATIC_CHECK(!("hello"_fs | ops::starts_with<"llo">));
  STATIC_CHECK("abc"_fs | ops::starts_with<"">);
  STATIC_CHECK(!("abc"_fs | ops::starts_with<"abcd">));
}

TEST_CASE("starts_with<Prefix>(str) - FrozenString version") {
  constexpr auto str = "hello"_fs;
  constexpr auto prefix = "hel"_fs;
  constexpr auto not_prefix = "xyz"_fs;
  STATIC_CHECK(starts_with<prefix>(str));
  STATIC_CHECK(!starts_with<not_prefix>(str));
}

TEST_CASE("starts_with<Prefix>(str) - char[] version") {
  STATIC_CHECK(starts_with<"hel">("hello"));
  STATIC_CHECK(!starts_with<"xyz">("hello"));
}

TEST_CASE("ends_with<Suffix>(str) - NTTP version") {
  STATIC_CHECK("hello"_fs | ops::ends_with<"llo">);
  STATIC_CHECK(!("hello"_fs | ops::ends_with<"hel">));
  STATIC_CHECK("abc"_fs | ops::ends_with<"">);
  STATIC_CHECK(!("abc"_fs | ops::ends_with<"abcd">));
}

TEST_CASE("ends_with<Suffix>(str) - FrozenString version") {
  constexpr auto str = "hello"_fs;
  constexpr auto suffix = "llo"_fs;
  constexpr auto not_suffix = "xyz"_fs;
  STATIC_CHECK(ends_with<suffix>(str));
  STATIC_CHECK(!ends_with<not_suffix>(str));
}

TEST_CASE("ends_with<Suffix>(str) - char[] version") {
  STATIC_CHECK(ends_with<"llo">("hello"));
  STATIC_CHECK(!ends_with<"xyz">("hello"));
}

TEST_CASE("partition<Delim>(str) - NTTP version") {
  constexpr auto [before, delim, after] = "key=value"_fs | ops::partition<"=">;
  STATIC_CHECK(before.sv() == "key");
  STATIC_CHECK(delim.sv() == "=");
  STATIC_CHECK(after.sv() == "value");
}

TEST_CASE("partition<Delim>(str) - no match") {
  constexpr auto [before, delim, after] = "hello"_fs | ops::partition<"=">;
  STATIC_CHECK(before.sv() == "hello");
  STATIC_CHECK(delim.sv() == "");
  STATIC_CHECK(after.sv() == "");
}

TEST_CASE("partition<Delim>(str) - multiple delimiters") {
  constexpr auto [before, delim, after] = "a=b=c"_fs | ops::partition<"=">;
  STATIC_CHECK(before.sv() == "a");
  STATIC_CHECK(delim.sv() == "=");
  STATIC_CHECK(after.sv() == "b=c");
}

TEST_CASE("partition<Delim>(str) - char[] version") {
  constexpr auto [before, delim, after] = partition<"=">("key=value");
  STATIC_CHECK(before.sv() == "key");
  STATIC_CHECK(delim.sv() == "=");
  STATIC_CHECK(after.sv() == "value");
}
