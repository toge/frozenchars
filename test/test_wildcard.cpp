#include "catch2/catch_all.hpp"

#include "frozenchars/string.hpp"
#include "frozenchars/literals.hpp"
#include "frozenchars/wildcard.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("wildcard: basic * matching") {
  REQUIRE(wildcard_match<"a*c">("abc"));
  REQUIRE(wildcard_match<"a*c">("axc"));
  REQUIRE(wildcard_match<"a*c">("ac"));
  REQUIRE(wildcard_match<"a*c">("abxc"));
  REQUIRE(wildcard_match<"a*c">("aabbbbbc"));
}

TEST_CASE("wildcard: * non-matching") {
  REQUIRE_FALSE(wildcard_match<"a*c">("ab"));
  REQUIRE_FALSE(wildcard_match<"a*c">("abxd"));
  REQUIRE_FALSE(wildcard_match<"a*c">("xab"));
}

TEST_CASE("wildcard: basic ? matching") {
  REQUIRE(wildcard_match<"a?c">("abc"));
  REQUIRE(wildcard_match<"a?c">("axc"));
}

TEST_CASE("wildcard: ? non-matching") {
  REQUIRE_FALSE(wildcard_match<"a?c">("ac"));
  REQUIRE_FALSE(wildcard_match<"a?c">("abxc"));
}

TEST_CASE("wildcard: edge cases") {
  REQUIRE(wildcard_match<"*">("anything"));
  REQUIRE(wildcard_match<"*">(""));

  REQUIRE(wildcard_match<"">(""));
  REQUIRE_FALSE(wildcard_match<"">("a"));

  REQUIRE(wildcard_match<"a">("a"));
  REQUIRE_FALSE(wildcard_match<"a">("b"));
}

TEST_CASE("wildcard: combined * and ?") {
  REQUIRE(wildcard_match<"a*b?c">("axbyc"));
  REQUIRE_FALSE(wildcard_match<"a*b?c">("axbc"));
  REQUIRE_FALSE(wildcard_match<"a*b?c">("axby"));
}

TEST_CASE("wildcard: multiple *") {
  REQUIRE(wildcard_match<"**a">("ba"));
  REQUIRE(wildcard_match<"*a*">("xyzaxyz"));
  REQUIRE_FALSE(wildcard_match<"*a*">("xyzbyxyz"));
}

TEST_CASE("wildcard: runtime text") {
  auto text = std::string("hello world");

  REQUIRE(wildcard_match<"hello*">(text));
  REQUIRE(wildcard_match<"*world">(text));
  REQUIRE(wildcard_match<"hello*world">(text));
  REQUIRE_FALSE(wildcard_match<"goodbye*">(text));
  REQUIRE_FALSE(wildcard_match<"hello*earth">(text));
}

TEST_CASE("wildcard: runtime text with ?") {
  auto text = std::string("hello");

  REQUIRE(wildcard_match<"hell?">(text));
  REQUIRE_FALSE(wildcard_match<"hell?o">(text));
  REQUIRE_FALSE(wildcard_match<"hell">(text));
}

TEST_CASE("wildcard: longer patterns") {
  REQUIRE(wildcard_match<"*.txt">("file.txt"));
  REQUIRE_FALSE(wildcard_match<"*.txt">("file.pdf"));
  REQUIRE(wildcard_match<"file.*">("file.txt"));
  REQUIRE(wildcard_match<"f*.*">("file.txt"));
}
