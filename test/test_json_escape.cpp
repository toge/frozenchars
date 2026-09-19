#include "catch2/catch_all.hpp"
#include "frozenchars/encoding.hpp"
#include "frozenchars/literals.hpp"
#include "frozenchars/ops.hpp"

/** @brief JSON 文字列エスケープ（json_escape / json_quoted）のテスト。 */

using namespace frozenchars;
using namespace frozenchars::literals;
namespace fops = frozenchars::ops;

TEST_CASE("json_escape") {
  SECTION("quotes and backslashes") {
    auto constexpr input = "say \"hi\""_fs;
    auto constexpr escaped = json_escape(input);
    static_assert(escaped.sv() == R"(say \"hi\")");
    REQUIRE(escaped.sv() == R"(say \"hi\")");
  }

  SECTION("short escapes") {
    auto constexpr input = "a\nb\tc\rd\fe\bf"_fs;
    auto constexpr escaped = json_escape(input);
    static_assert(escaped.sv() == "a\\nb\\tc\\rd\\fe\\bf");
    REQUIRE(escaped.sv() == "a\\nb\\tc\\rd\\fe\\bf");
  }

  SECTION("control characters become \\u00XX") {
    auto constexpr input = "x\x01y"_fs;
    auto constexpr escaped = json_escape(input);
    static_assert(escaped.sv() == "x\\u0001y");
    REQUIRE(escaped.sv() == "x\\u0001y");
  }

  SECTION("UTF-8 bytes pass through unchanged") {
    auto constexpr input = "日本語"_fs;
    auto constexpr escaped = json_escape(input);
    static_assert(escaped.sv() == "日本語");
    REQUIRE(escaped.sv() == "日本語");
  }

  SECTION("NTTP exact size") {
    auto constexpr escaped = json_escape<"a\nb"_fs>();
    static_assert(escaped.size() == 4);
    static_assert(escaped.sv() == "a\\nb");
    REQUIRE(escaped.sv() == "a\\nb");
  }
}

TEST_CASE("json_quoted") {
  SECTION("wraps in quotes") {
    static_assert(json_quoted("hi"_fs).sv() == "\"hi\"");
    REQUIRE(json_quoted("hi"_fs).sv() == "\"hi\"");
  }

  SECTION("escapes inside the quotes") {
    auto constexpr input = "say \"hi\""_fs;
    auto constexpr quoted = json_quoted(input);
    static_assert(quoted.sv() == R"("say \"hi\"")");
    REQUIRE(quoted.sv() == R"("say \"hi\"")");
  }

  SECTION("NTTP exact size") {
    auto constexpr quoted = json_quoted<"hi"_fs>();
    static_assert(quoted.size() == 4);
    static_assert(quoted.sv() == "\"hi\"");
    REQUIRE(quoted.sv() == "\"hi\"");
  }
}

TEST_CASE("json escape pipe adaptor") {
  SECTION("FrozenString") {
    auto constexpr escaped = "say \"hi\""_fs | fops::json_escape;
    static_assert(escaped.sv() == R"(say \"hi\")");
    REQUIRE(escaped.sv() == R"(say \"hi\")");
  }

  SECTION("bare string literal") {
    auto constexpr escaped = "x\x01y" | fops::json_escape;
    static_assert(escaped.sv() == "x\\u0001y");
    REQUIRE(escaped.sv() == "x\\u0001y");
  }

  SECTION("json_quoted") {
    auto constexpr quoted = "hi"_fs | fops::json_quoted;
    static_assert(quoted.sv() == "\"hi\"");
    REQUIRE(quoted.sv() == "\"hi\"");
  }
}
