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

TEST_CASE("function calls in short-circuit or context", "[functions][short_circuit]") {
  constexpr auto src = "{{ true or upper(name) }}"_fs;
  auto const ctx = make_template_object({{"name", "hello"}});
  auto const out = render_template<src>(ctx);
  REQUIRE(out == "true");
}

TEST_CASE("function calls in short-circuit and context", "[functions][short_circuit]") {
  constexpr auto src = "{{ false and upper(name) }}"_fs;
  auto const ctx = make_template_object({{"name", "hello"}});
  auto const out = render_template<src>(ctx);
  REQUIRE(out == "false");
}

TEST_CASE("function calls evaluated when not short-circuited in and", "[functions][short_circuit]") {
  constexpr auto src = "{{ true and upper(name) }}"_fs;
  auto const ctx = make_template_object({{"name", "hello"}});
  auto const out = render_template<src>(ctx);
  REQUIRE(out == "true");
}

// ============ List functions tests ============

// length() tests
TEST_CASE("template functions - length", "[template_functions][length]") {
  constexpr auto src = "{{ length(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("template functions - length empty", "[template_functions][length]") {
  constexpr auto src = "{{ length(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({})},
  });
  REQUIRE(render_template<src>(ctx) == "0");
}

TEST_CASE("template functions - length single element", "[template_functions][length]") {
  constexpr auto src = "{{ length(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({"hello"})},
  });
  REQUIRE(render_template<src>(ctx) == "1");
}

// first() tests
TEST_CASE("template functions - first", "[template_functions][first]") {
  constexpr auto src = "{{ first(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "1");
}

TEST_CASE("template functions - first single element", "[template_functions][first]") {
  constexpr auto src = "{{ first(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({"hello"})},
  });
  REQUIRE(render_template<src>(ctx) == "hello");
}

TEST_CASE("template functions - first empty throws", "[template_functions][first]") {
  constexpr auto src = "{{ first(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// last() tests
TEST_CASE("template functions - last", "[template_functions][last]") {
  constexpr auto src = "{{ last(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("template functions - last single element", "[template_functions][last]") {
  constexpr auto src = "{{ last(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({"world"})},
  });
  REQUIRE(render_template<src>(ctx) == "world");
}

TEST_CASE("template functions - last empty throws", "[template_functions][last]") {
  constexpr auto src = "{{ last(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// join() tests
TEST_CASE("template functions - join basic", "[template_functions][join]") {
  constexpr auto src = "{{ join(items, \", \") }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "1, 2, 3");
}

TEST_CASE("template functions - join with strings", "[template_functions][join]") {
  constexpr auto src = "{{ join(items, \",\") }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({"a", "b", "c"})},
  });
  REQUIRE(render_template<src>(ctx) == "a,b,c");
}

TEST_CASE("template functions - join empty", "[template_functions][join]") {
  constexpr auto src = "{{ join(items, \", \") }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({})},
  });
  REQUIRE(render_template<src>(ctx) == "");
}

TEST_CASE("template functions - join single element", "[template_functions][join]") {
  constexpr auto src = "{{ join(items, \",\") }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({"hello"})},
  });
  REQUIRE(render_template<src>(ctx) == "hello");
}

TEST_CASE("template functions - join mixed types", "[template_functions][join]") {
  constexpr auto src = "{{ join(items, \"-\") }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, "hello", true})},
  });
  auto const out = render_template<src>(ctx);
  REQUIRE(out.find("1") != std::string::npos);
  REQUIRE(out.find("hello") != std::string::npos);
  REQUIRE(out.find("true") != std::string::npos);
}

TEST_CASE("template functions - sort integers", "[template_functions][sort]") {
  constexpr auto src = "{{ sort(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({3, 1, 2})},
  });
  auto const out = render_template<src>(ctx);
  REQUIRE(out.find("[array]") != std::string::npos);
}

TEST_CASE("template functions - sort strings", "[template_functions][sort]") {
  constexpr auto src = "{{ sort(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({"zebra", "apple", "mango"})},
  });
  auto const out = render_template<src>(ctx);
  REQUIRE(out.find("[array]") != std::string::npos);
}

TEST_CASE("template functions - sort empty", "[template_functions][sort]") {
  constexpr auto src = "{{ sort(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({})},
  });
  auto const out = render_template<src>(ctx);
  REQUIRE(out.find("[array]") != std::string::npos);
}

TEST_CASE("template functions - sort single element", "[template_functions][sort]") {
  constexpr auto src = "{{ sort(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({42})},
  });
  auto const out = render_template<src>(ctx);
  REQUIRE(out.find("[array]") != std::string::npos);
}

TEST_CASE("template functions - sort preserves original", "[template_functions][sort]") {
  // Test by verifying first and last elements of sorted vs original
  constexpr auto src = "{{ first(items) }}-{{ first(sort(items)) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({3, 1, 2})},
  });
  auto const out = render_template<src>(ctx);
  REQUIRE(out == "3-1");
}

// range() tests
TEST_CASE("template functions - range single arg", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(5)) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "5");
}

TEST_CASE("template functions - range two args", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(2, 5)) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("template functions - range three args", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(0, 10, 2)) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "5");
}

TEST_CASE("template functions - range three args negative step", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(10, 0, step)) }}"_fs;
  auto const ctx = make_template_object({
    {"step", std::int64_t{-2}},
  });
  REQUIRE(render_template<src>(ctx) == "5");
}

TEST_CASE("template functions - range zero", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(0)) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "0");
}

TEST_CASE("template functions - range large", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(100)) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "100");
}

// Error cases for list functions
TEST_CASE("template functions - type error length on string", "[template_functions][type_error]") {
  constexpr auto src = "{{ length(\"hello\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - type error first on string", "[template_functions][type_error]") {
  constexpr auto src = "{{ first(\"hello\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - type error join second arg", "[template_functions][type_error]") {
  constexpr auto src = "{{ join(items, 42) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, 2, 3})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - wrong arg count length", "[template_functions][arg_error]") {
  constexpr auto src = "{{ length() }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - wrong arg count join", "[template_functions][arg_error]") {
  constexpr auto src = "{{ join(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, 2, 3})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - wrong arg count range", "[template_functions][arg_error]") {
  constexpr auto src = "{{ range(1, 2, 3, 4) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}
