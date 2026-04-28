#include "catch2/catch_all.hpp"

#include <regex>

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("remove_regex_comment removes extended-mode whitespace and comments") {
  auto constexpr pattern = remove_regex_comment(
    "(?x)^ a\\ b\\# [ # ] c # trailing comment\n $"_fs
  );

  static_assert(pattern.sv() == "^a\\ b\\#[ # ]c$");
  REQUIRE(pattern.sv() == "^a\\ b\\#[ # ]c$");

  auto const re = std::regex{std::string{pattern.sv()}};
  REQUIRE(std::regex_match("a b# c", re));
  REQUIRE_FALSE(std::regex_match("abc", re));
}

TEST_CASE("remove_regex_comment NTTP shrinks the normalized pattern") {
  static constexpr auto source = "(?x)a  # comment\n b"_fs;
  auto constexpr pattern = remove_regex_comment<source>();

  static_assert(std::same_as<std::remove_cvref_t<decltype(pattern)>, FrozenString<3>>);
  static_assert(pattern.sv() == "ab");
  REQUIRE(pattern.sv() == "ab");
}

TEST_CASE("remove_regex_comment strips the full required whitespace set without a prefix") {
  auto constexpr pattern = remove_regex_comment("a \t\r\f\n b # comment\n c"_fs);

  static_assert(pattern.sv() == "abc");
  REQUIRE(pattern.sv() == "abc");
}

TEST_CASE("remove_regex_comment strips an end-of-pattern comment without a trailing newline") {
  auto constexpr pattern = remove_regex_comment("(?x)foo # trailing comment"_fs);

  static_assert(pattern.sv() == "foo");
  REQUIRE(pattern.sv() == "foo");
}
