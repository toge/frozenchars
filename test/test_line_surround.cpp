#include "catch2/catch_all.hpp"
#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("prefix_lines") {
  auto constexpr src = "line1\nline2\n"_fs;
  auto constexpr res = prefix_lines(src, "> "_fs);
  static_assert(res.sv() == "> line1\n> line2\n");
  REQUIRE(res.sv() == "> line1\n> line2\n");

  // Include empty lines
  auto constexpr src_with_empty = "line1\n\nline2"_fs;
  auto constexpr res_with_empty = prefix_lines(src_with_empty, "#"_fs);
  static_assert(res_with_empty.sv() == "#line1\n#\n#line2");
  REQUIRE(res_with_empty.sv() == "#line1\n#\n#line2");

  // Pipe adaptor
  namespace fops = frozenchars::ops;
  auto constexpr res_pipe = src | fops::prefix_lines("> "_fs);
  static_assert(res_pipe.sv() == "> line1\n> line2\n");
  REQUIRE(res_pipe.sv() == "> line1\n> line2\n");
}

TEST_CASE("postfix_lines") {
  auto constexpr src = "line1\nline2\n"_fs;
  auto constexpr res = postfix_lines(src, " <"_fs);
  static_assert(res.sv() == "line1 <\nline2 <\n");
  REQUIRE(res.sv() == "line1 <\nline2 <\n");

  // Include empty lines
  auto constexpr src_with_empty = "line1\n\nline2"_fs;
  auto constexpr res_with_empty = postfix_lines(src_with_empty, "#"_fs);
  static_assert(res_with_empty.sv() == "line1#\n#\nline2#");
  REQUIRE(res_with_empty.sv() == "line1#\n#\nline2#");

  // Pipe adaptor
  namespace fops = frozenchars::ops;
  auto constexpr res_pipe = src | fops::postfix_lines(" <"_fs);
  static_assert(res_pipe.sv() == "line1 <\nline2 <\n");
  REQUIRE(res_pipe.sv() == "line1 <\nline2 <\n");
}

TEST_CASE("surround_lines") {
  auto constexpr src = "line1\nline2\n"_fs;
  
  // Two arguments
  auto constexpr res1 = surround_lines(src, "[ "_fs, " ]"_fs);
  static_assert(res1.sv() == "[ line1 ]\n[ line2 ]\n");
  REQUIRE(res1.sv() == "[ line1 ]\n[ line2 ]\n");

  // One argument (same for both)
  auto constexpr res2 = surround_lines(src, "*"_fs);
  static_assert(res2.sv() == "*line1*\n*line2*\n");
  REQUIRE(res2.sv() == "*line1*\n*line2*\n");

  // Include empty lines
  auto constexpr src_with_empty = "line1\n\nline2"_fs;
  auto constexpr res_with_empty = surround_lines(src_with_empty, "\""_fs);
  static_assert(res_with_empty.sv() == "\"line1\"\n\"\"\n\"line2\"");
  REQUIRE(res_with_empty.sv() == "\"line1\"\n\"\"\n\"line2\"");

  // Pipe adaptor
  namespace fops = frozenchars::ops;
  auto constexpr res_pipe1 = src | fops::surround_lines("[ "_fs, " ]"_fs);
  static_assert(res_pipe1.sv() == "[ line1 ]\n[ line2 ]\n");
  REQUIRE(res_pipe1.sv() == "[ line1 ]\n[ line2 ]\n");

  auto constexpr res_pipe2 = src | fops::surround_lines("*"_fs);
  static_assert(res_pipe2.sv() == "*line1*\n*line2*\n");
  REQUIRE(res_pipe2.sv() == "*line1*\n*line2*\n");
}
