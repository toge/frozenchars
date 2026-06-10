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

TEST_CASE("wildcard: escape sequences") {
  REQUIRE(wildcard_match<"a\\*b">("a*b"));
  REQUIRE_FALSE(wildcard_match<"a\\*b">("axb"));
  REQUIRE(wildcard_match<"a\\?b">("a?b"));
  REQUIRE_FALSE(wildcard_match<"a\\?b">("aab"));
  REQUIRE(wildcard_match<"\\\\">("\\"));
  REQUIRE(wildcard_match<"a\\[b">("a[b"));
}

TEST_CASE("wildcard: character sets") {
  REQUIRE(wildcard_match<"[abc]">("a"));
  REQUIRE(wildcard_match<"[abc]">("b"));
  REQUIRE(wildcard_match<"[abc]">("c"));
  REQUIRE_FALSE(wildcard_match<"[abc]">("d"));
  REQUIRE_FALSE(wildcard_match<"[abc]">("ab"));
  REQUIRE_FALSE(wildcard_match<"[abc]">(""));
}

TEST_CASE("wildcard: negated character sets") {
  REQUIRE(wildcard_match<"[!abc]">("d"));
  REQUIRE(wildcard_match<"[!abc]">("x"));
  REQUIRE_FALSE(wildcard_match<"[!abc]">("a"));
  REQUIRE_FALSE(wildcard_match<"[!abc]">("b"));
  REQUIRE_FALSE(wildcard_match<"[!abc]">("c"));
}

TEST_CASE("wildcard: sets with * and ?") {
  REQUIRE(wildcard_match<"file[0-9].txt">("file0.txt"));
  REQUIRE(wildcard_match<"file[0-9].txt">("file9.txt"));
  REQUIRE(wildcard_match<"a[xyz]c">("axc"));
  REQUIRE(wildcard_match<"a[xyz]c">("azc"));
  REQUIRE_FALSE(wildcard_match<"a[xyz]c">("abc"));
  REQUIRE(wildcard_match<"*[xyz]*">("prefix_z_suffix"));
  REQUIRE_FALSE(wildcard_match<"*[xyz]*">("abc_def_ghi"));
}

TEST_CASE("wildcard: alternatives") {
  REQUIRE(wildcard_match<"(ab|cd)">("ab"));
  REQUIRE(wildcard_match<"(ab|cd)">("cd"));
  REQUIRE_FALSE(wildcard_match<"(ab|cd)">("ac"));
  REQUIRE_FALSE(wildcard_match<"(ab|cd)">("abc"));
}

TEST_CASE("wildcard: alternatives with more branches") {
  REQUIRE(wildcard_match<"(a|b|c)">("a"));
  REQUIRE(wildcard_match<"(a|b|c)">("b"));
  REQUIRE(wildcard_match<"(a|b|c)">("c"));
  REQUIRE_FALSE(wildcard_match<"(a|b|c)">("d"));
}

TEST_CASE("wildcard: alternatives with surrounding pattern") {
  REQUIRE(wildcard_match<"prefix_(ab|cd)_suffix">("prefix_ab_suffix"));
  REQUIRE(wildcard_match<"prefix_(ab|cd)_suffix">("prefix_cd_suffix"));
  REQUIRE_FALSE(wildcard_match<"prefix_(ab|cd)_suffix">("prefix_xy_suffix"));
  REQUIRE(wildcard_match<"*(ab|cd)*">("xyz_ab_xyz"));
  REQUIRE(wildcard_match<"*(ab|cd)*">("cd"));
  REQUIRE_FALSE(wildcard_match<"*(ab|cd)*">("xy"));
}

TEST_CASE("wildcard: alternatives with * and ?") {
  REQUIRE(wildcard_match<"(ab|cd)e?">("abex"));
  REQUIRE(wildcard_match<"(ab|cd)e?">("cdey"));
  REQUIRE_FALSE(wildcard_match<"(ab|cd)e?">("abe"));
  REQUIRE_FALSE(wildcard_match<"(ab|cd)e?">("cdexy"));
}

TEST_CASE("wildcard: alternatives with sets") {
  REQUIRE(wildcard_match<"([abc]|xyz)">("a"));
  REQUIRE(wildcard_match<"([abc]|xyz)">("xyz"));
  REQUIRE_FALSE(wildcard_match<"([abc]|xyz)">("d"));
}

TEST_CASE("wildcard: alternatives with nested alternatives") {
  REQUIRE(wildcard_match<"(a|(bc|de))">("a"));
  REQUIRE(wildcard_match<"(a|(bc|de))">("bc"));
  REQUIRE(wildcard_match<"(a|(bc|de))">("de"));
  REQUIRE_FALSE(wildcard_match<"(a|(bc|de))">("bd"));
}

TEST_CASE("wildcard: unbalanced alternatives") {
  // Unbalanced '(' should be treated as literal '('
  REQUIRE(wildcard_match<"(ab">("(ab"));
  REQUIRE_FALSE(wildcard_match<"(ab">("ab"));
  
  // Nested unbalanced '('
  REQUIRE(wildcard_match<"((ab">("((ab"));
  REQUIRE_FALSE(wildcard_match<"((ab">("(ab"));
}
