#include "catch2/catch_all.hpp"
#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("remove_leading_spaces") {
  auto constexpr src = "  line1\n    line2\n  line3"_fs;
  auto constexpr res = remove_leading_spaces(src, 2);
  static_assert(res.sv() == "line1\n  line2\nline3");
  REQUIRE(res.sv() == "line1\n  line2\nline3");

  // Test n=0 (should remove all spaces)
  auto constexpr res_all = remove_leading_spaces(src, 0);
  static_assert(res_all.sv() == "line1\nline2\nline3");
  REQUIRE(res_all.sv() == "line1\nline2\nline3");

  // Pipe adaptor
  namespace fops = frozenchars::ops;
  auto constexpr res_pipe = src | fops::remove_leading_spaces(2);
  static_assert(res_pipe.sv() == "line1\n  line2\nline3");
  REQUIRE(res_pipe.sv() == "line1\n  line2\nline3");

  auto constexpr res_pipe_all = src | fops::remove_leading_spaces;
  static_assert(res_pipe_all.sv() == "line1\nline2\nline3");
  REQUIRE(res_pipe_all.sv() == "line1\nline2\nline3");
}

TEST_CASE("remove_comment_lines") {
  auto constexpr src = "line1\n##comment\nline2\n;comment2\nline3"_fs;

  // 文字列での指定 (##)
  auto constexpr res1 = remove_comment_lines(src, "##");
  static_assert(res1.sv() == "line1\nline2\n;comment2\nline3");
  REQUIRE(res1.sv() == "line1\nline2\n;comment2\nline3");

  // 文字での指定 (;)
  auto constexpr res2 = remove_comment_lines(src, ";");
  static_assert(res2.sv() == "line1\n##comment\nline2\nline3");
  REQUIRE(res2.sv() == "line1\n##comment\nline2\nline3");

  // Pipe adaptor
  namespace fops = frozenchars::ops;
  auto constexpr res_pipe = src | fops::remove_comment_lines("##");
  static_assert(res_pipe.sv() == "line1\nline2\n;comment2\nline3");
  REQUIRE(res_pipe.sv() == "line1\nline2\n;comment2\nline3");
}

TEST_CASE("remove_comments") {
  auto constexpr src = "code1 # comment\ncode2    ## long comment\ncode3;comment\ncode4"_fs;

  // # 以降を削除（直前の空白は残す）
  auto constexpr res1 = remove_comments(src, "#");
  static_assert(res1.sv() == "code1 \ncode2    \ncode3;comment\ncode4");
  REQUIRE(res1.sv() == "code1 \ncode2    \ncode3;comment\ncode4");

  // ## 以降を削除（直前の空白は残す）
  auto constexpr res2 = remove_comments(src, "##");
  static_assert(res2.sv() == "code1 # comment\ncode2    \ncode3;comment\ncode4");
  REQUIRE(res2.sv() == "code1 # comment\ncode2    \ncode3;comment\ncode4");

  // ; 以降を削除 (直前に空白なし)
  auto constexpr res3 = remove_comments(src, ";");
  static_assert(res3.sv() == "code1 # comment\ncode2    ## long comment\ncode3\ncode4");
  REQUIRE(res3.sv() == "code1 # comment\ncode2    ## long comment\ncode3\ncode4");

  // Pipe adaptor
  namespace fops = frozenchars::ops;
  auto constexpr res_pipe = src | fops::remove_comments("##");
  static_assert(res_pipe.sv() == "code1 # comment\ncode2    \ncode3;comment\ncode4");
  REQUIRE(res_pipe.sv() == "code1 # comment\ncode2    \ncode3;comment\ncode4");

  auto constexpr res_clean = src | fops::remove_comments("#") | fops::remove_trailing_spaces;
  static_assert(res_clean.sv() == "code1\ncode2\ncode3;comment\ncode4");
  REQUIRE(res_clean.sv() == "code1\ncode2\ncode3;comment\ncode4");
}

TEST_CASE("remove_trailing_spaces") {
  auto constexpr src = "line1  \nline2\t \nline3 "_fs;
  auto constexpr res = remove_trailing_spaces(src);
  static_assert(res.sv() == "line1\nline2\t\nline3");
  REQUIRE(res.sv() == "line1\nline2\t\nline3");

  namespace fops = frozenchars::ops;
  auto constexpr res_pipe = src | fops::remove_trailing_spaces;
  static_assert(res_pipe.sv() == "line1\nline2\t\nline3");
  REQUIRE(res_pipe.sv() == "line1\nline2\t\nline3");
}

TEST_CASE("remove_range_comments") {
  auto constexpr src = "a /* block\ncomment */ b /* c */ d"_fs;

  auto constexpr res = remove_range_comments(src, "/*", "*/");
  static_assert(res.sv() == "a  b  d");
  REQUIRE(res.sv() == "a  b  d");

  namespace fops = frozenchars::ops;
  auto constexpr res_pipe = src | fops::remove_range_comments("/*", "*/");
  static_assert(res_pipe.sv() == "a  b  d");
  REQUIRE(res_pipe.sv() == "a  b  d");
}

TEST_CASE("join_lines") {
  auto constexpr src = "line1\nline2 \n line3\nline4"_fs;

  // デフォルト (sep="")
  auto constexpr res = join_lines(src);
  static_assert(res.sv() == "line1line2  line3line4");
  REQUIRE(res.sv() == "line1line2  line3line4");

  // セパレータ指定 (" ")
  auto constexpr res_space = join_lines(src, " ");
  static_assert(res_space.sv() == "line1 line2   line3 line4");
  REQUIRE(res_space.sv() == "line1 line2   line3 line4");

  // セパレータ指定 (",")
  auto constexpr res_comma = join_lines(src, ",");
  static_assert(res_comma.sv() == "line1,line2 , line3,line4");
  REQUIRE(res_comma.sv() == "line1,line2 , line3,line4");

  auto constexpr src2 = "line1\n"_fs;
  auto constexpr res2 = join_lines(src2);
  static_assert(res2.sv() == "line1");
  REQUIRE(res2.sv() == "line1");

  auto constexpr src3 = "line1\n\n"_fs;
  auto constexpr res3 = join_lines(src3);
  static_assert(res3.sv() == "line1");
  REQUIRE(res3.sv() == "line1");

  // Pipe adaptor
  namespace fops = frozenchars::ops;
  auto constexpr res_pipe = src | fops::join_lines;
  static_assert(res_pipe.sv() == "line1line2  line3line4");
  REQUIRE(res_pipe.sv() == "line1line2  line3line4");

  auto constexpr res_pipe_sep = src | fops::join_lines(" ");
  static_assert(res_pipe_sep.sv() == "line1 line2   line3 line4");
  REQUIRE(res_pipe_sep.sv() == "line1 line2   line3 line4");
}

TEST_CASE("trim_trailing_spaces") {
  auto constexpr src = "line1  \nline2\t\nline3 "_fs;
  auto constexpr res = trim_trailing_spaces(src);
  static_assert(res.sv() == "line1\nline2\nline3");
  REQUIRE(res.sv() == "line1\nline2\nline3");

  // Pipe adaptor
  namespace fops = frozenchars::ops;
  auto constexpr res_pipe = src | fops::trim_trailing_spaces;
  static_assert(res_pipe.sv() == "line1\nline2\nline3");
  REQUIRE(res_pipe.sv() == "line1\nline2\nline3");
}

TEST_CASE("remove_empty_lines") {
  auto constexpr src = "line1\n\nline2\n\n\nline3\n"_fs;
  auto constexpr res = remove_empty_lines(src);
  static_assert(res.sv() == "line1\nline2\nline3\n");
  REQUIRE(res.sv() == "line1\nline2\nline3\n");

  // Pipe adaptor
  namespace fops = frozenchars::ops;
  auto constexpr res_pipe = src | fops::remove_empty_lines;
  static_assert(res_pipe.sv() == "line1\nline2\nline3\n");
  REQUIRE(res_pipe.sv() == "line1\nline2\nline3\n");
}
