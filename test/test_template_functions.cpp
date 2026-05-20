#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("template functions - upper", "[template_functions][upper]") {
  constexpr auto src = "{{ upper(name) }}"_fs;
  auto const ctx = make_template_object({
    {"name", "hello"},
  });
  REQUIRE(render_template<src>(ctx) == "HELLO");
}

TEST_CASE("template functions - lower", "[template_functions][lower]") {
  constexpr auto src = "{{ lower(name) }}"_fs;
  auto const ctx = make_template_object({
    {"name", "WORLD"},
  });
  REQUIRE(render_template<src>(ctx) == "world");
}

TEST_CASE("template functions - capitalize", "[template_functions][capitalize]") {
  constexpr auto src = "{{ capitalize(phrase) }}"_fs;
  auto const ctx = make_template_object({
    {"phrase", "hello world"},
  });
  REQUIRE(render_template<src>(ctx) == "Hello world");
}

TEST_CASE("template functions - capitalize empty", "[template_functions][capitalize]") {
  constexpr auto src = "{{ capitalize(text) }}"_fs;
  auto const ctx = make_template_object({
    {"text", ""},
  });
  REQUIRE(render_template<src>(ctx) == "");
}

TEST_CASE("template functions - replace", "[template_functions][replace]") {
  constexpr auto src = "{{ replace(text, old, new) }}"_fs;
  auto const ctx = make_template_object({
    {"text", "hello"},
    {"old", "l"},
    {"new", "L"},
  });
  REQUIRE(render_template<src>(ctx) == "heLlo");
}

TEST_CASE("template functions - replace not found", "[template_functions][replace]") {
  constexpr auto src = "{{ replace(text, old, new) }}"_fs;
  auto const ctx = make_template_object({
    {"text", "hello"},
    {"old", "x"},
    {"new", "y"},
  });
  REQUIRE(render_template<src>(ctx) == "hello");
}

TEST_CASE("template functions - upper with string literal", "[template_functions][upper]") {
  constexpr auto src = "{{ upper(\"hello\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "HELLO");
}

TEST_CASE("template functions - lower with string literal", "[template_functions][lower]") {
  constexpr auto src = "{{ lower(\"WORLD\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "world");
}

TEST_CASE("template functions - capitalize with string literal", "[template_functions][capitalize]") {
  constexpr auto src = "{{ capitalize(\"hello world\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "Hello world");
}

TEST_CASE("template functions - replace with string literals", "[template_functions][replace]") {
  constexpr auto src = "{{ replace(\"hello\", \"l\", \"L\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "heLlo");
}

TEST_CASE("template functions - type error upper on number", "[template_functions][type_error]") {
  constexpr auto src = "{{ upper(42) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - type error lower on number", "[template_functions][type_error]") {
  constexpr auto src = "{{ lower(123) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - wrong arg count upper", "[template_functions][arg_error]") {
  constexpr auto src = "{{ upper(\"hello\", \"world\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - wrong arg count lower", "[template_functions][arg_error]") {
  constexpr auto src = "{{ lower() }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - wrong arg count replace", "[template_functions][arg_error]") {
  constexpr auto src = "{{ replace(\"hello\", \"l\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - unknown function", "[template_functions][unknown]") {
  constexpr auto src = "{{ unknown_func(\"test\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - nested in if", "[template_functions][nested]") {
  constexpr auto src = "{% if text %}{{ upper(text) }}{% endif %}"_fs;
  auto const ctx = make_template_object({
    {"text", "hello"},
  });
  REQUIRE(render_template<src>(ctx) == "HELLO");
}

TEST_CASE("template functions - in for loop", "[template_functions][loop]") {
  constexpr auto src = "{% for item in items %}{{ upper(item) }};{% endfor %}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({"hello", "world"})},
  });
  auto const out = render_template<src>(ctx);
  REQUIRE(out.find("HELLO;") != std::string::npos);
  REQUIRE(out.find("WORLD;") != std::string::npos);
}
