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
  REQUIRE(render_template<src>(ctx) == "heLLo");
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
  REQUIRE(render_template<src>(ctx) == "heLLo");
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

// Numeric functions - abs tests
TEST_CASE("template functions - abs positive integer", "[template_functions][abs]") {
  constexpr auto src = "{{ abs(3) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("template functions - abs negative integer", "[template_functions][abs]") {
  constexpr auto src = "{{ abs(-5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "5");
}

TEST_CASE("template functions - abs zero", "[template_functions][abs]") {
  constexpr auto src = "{{ abs(0) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "0");
}

TEST_CASE("template functions - abs positive double", "[template_functions][abs]") {
  constexpr auto src = "{{ abs(3.14) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  REQUIRE(result == "3.14");
}

TEST_CASE("template functions - abs negative double", "[template_functions][abs]") {
  constexpr auto src = "{{ abs(-3.14) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  REQUIRE(result == "3.14");
}

TEST_CASE("template functions - abs type error string", "[template_functions][type_error]") {
  constexpr auto src = "{{ abs(\"hello\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - abs arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ abs(1, 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// Numeric functions - round tests
TEST_CASE("template functions - round default places", "[template_functions][round]") {
  constexpr auto src = "{{ round(3.14159) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("template functions - round 2 places", "[template_functions][round]") {
  constexpr auto src = "{{ round(3.14159, 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "3.14");
}

TEST_CASE("template functions - round negative 2 places", "[template_functions][round]") {
  constexpr auto src = "{{ round(-3.14159, 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "-3.14");
}

TEST_CASE("template functions - round 3 places", "[template_functions][round]") {
  constexpr auto src = "{{ round(3.14159, 3) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "3.142");
}

TEST_CASE("template functions - round 0 places explicit", "[template_functions][round]") {
  constexpr auto src = "{{ round(5.5, 0) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "6");
}

TEST_CASE("template functions - round integer input", "[template_functions][round]") {
  constexpr auto src = "{{ round(5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "5");
}

TEST_CASE("template functions - round type error string", "[template_functions][type_error]") {
  constexpr auto src = "{{ round(\"text\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - round type error non-integer places", "[template_functions][type_error]") {
  constexpr auto src = "{{ round(3.14, \"places\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - round arg count error 3 args", "[template_functions][arg_error]") {
  constexpr auto src = "{{ round(1, 2, 3) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// Numeric functions - max tests
TEST_CASE("template functions - max integers", "[template_functions][max]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, 5, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "5");
}

TEST_CASE("template functions - max doubles", "[template_functions][max]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1.5, 2.0})},
  });
  REQUIRE(render_template<src>(ctx) == "2");
}

TEST_CASE("template functions - max negative integers", "[template_functions][max]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({-10, -5, -20})},
  });
  REQUIRE(render_template<src>(ctx) == "-5");
}

TEST_CASE("template functions - max mixed types", "[template_functions][max]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, 3.5, 2})},
  });
  auto const result = render_template<src>(ctx);
  REQUIRE(result == "3.5");
}

TEST_CASE("template functions - max empty array", "[template_functions][type_error]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - max non-array", "[template_functions][type_error]") {
  constexpr auto src = "{{ max(\"not array\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - max array with string", "[template_functions][type_error]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, "string", 3})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - max arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ max(1, 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// Numeric functions - min tests
TEST_CASE("template functions - min integers", "[template_functions][min]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, 5, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "1");
}

TEST_CASE("template functions - min doubles", "[template_functions][min]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1.5, 2.0})},
  });
  REQUIRE(render_template<src>(ctx) == "1.5");
}

TEST_CASE("template functions - min negative integers", "[template_functions][min]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({-10, -5, -20})},
  });
  REQUIRE(render_template<src>(ctx) == "-20");
}

TEST_CASE("template functions - min mixed types", "[template_functions][min]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({3, 1.5, 2})},
  });
  REQUIRE(render_template<src>(ctx) == "1.5");
}

TEST_CASE("template functions - min empty array", "[template_functions][type_error]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - min non-array", "[template_functions][type_error]") {
  constexpr auto src = "{{ min(\"not array\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - min array with string", "[template_functions][type_error]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_template_object({
    {"items", make_template_array({1, "string", 3})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - min arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ min(1, 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// Numeric functions - even tests
TEST_CASE("template functions - even positive", "[template_functions][even]") {
  constexpr auto src = "{{ even(4) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - even odd positive", "[template_functions][even]") {
  constexpr auto src = "{{ even(5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - even zero", "[template_functions][even]") {
  constexpr auto src = "{{ even(0) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - even negative", "[template_functions][even]") {
  constexpr auto src = "{{ even(-4) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - even odd negative", "[template_functions][even]") {
  constexpr auto src = "{{ even(-5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - even type error float", "[template_functions][type_error]") {
  constexpr auto src = "{{ even(3.5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - even type error string", "[template_functions][type_error]") {
  constexpr auto src = "{{ even(\"4\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - even arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ even(1, 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// Numeric functions - odd tests
TEST_CASE("template functions - odd positive", "[template_functions][odd]") {
  constexpr auto src = "{{ odd(5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - odd even positive", "[template_functions][odd]") {
  constexpr auto src = "{{ odd(4) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - odd zero", "[template_functions][odd]") {
  constexpr auto src = "{{ odd(0) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - odd negative", "[template_functions][odd]") {
  constexpr auto src = "{{ odd(-5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - odd even negative", "[template_functions][odd]") {
  constexpr auto src = "{{ odd(-4) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - odd type error float", "[template_functions][type_error]") {
  constexpr auto src = "{{ odd(3.5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - odd type error string", "[template_functions][type_error]") {
  constexpr auto src = "{{ odd(\"5\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - odd arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ odd(1, 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// Numeric functions - divisibleBy tests
TEST_CASE("template functions - divisibleBy true", "[template_functions][divisibleBy]") {
  constexpr auto src = "{{ divisibleBy(10, 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - divisibleBy false", "[template_functions][divisibleBy]") {
  constexpr auto src = "{{ divisibleBy(10, 3) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - divisibleBy exact", "[template_functions][divisibleBy]") {
  constexpr auto src = "{{ divisibleBy(15, 5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - divisibleBy negative dividend", "[template_functions][divisibleBy]") {
  constexpr auto src = "{{ divisibleBy(-10, 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - divisibleBy zero", "[template_functions][divisibleBy]") {
  constexpr auto src = "{{ divisibleBy(0, 5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - divisibleBy divide by zero", "[template_functions][type_error]") {
  constexpr auto src = "{{ divisibleBy(10, 0) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - divisibleBy type error float dividend", "[template_functions][type_error]") {
  constexpr auto src = "{{ divisibleBy(10.5, 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - divisibleBy type error string dividend", "[template_functions][type_error]") {
  constexpr auto src = "{{ divisibleBy(\"10\", 2) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - divisibleBy type error float divisor", "[template_functions][type_error]") {
  constexpr auto src = "{{ divisibleBy(10, 2.5) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - divisibleBy arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ divisibleBy(10) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// ============ Utility Functions Tests ============

// default() tests
TEST_CASE("template functions - default with non-empty string", "[template_functions][default]") {
  constexpr auto src = "{{ default(text, \"fallback\") }}"_fs;
  auto const ctx = make_template_object({
    {"text", "hello"},
  });
  REQUIRE(render_template<src>(ctx) == "hello");
}

TEST_CASE("template functions - default with empty string", "[template_functions][default]") {
  constexpr auto src = "{{ default(text, \"fallback\") }}"_fs;
  auto const ctx = make_template_object({
    {"text", ""},
  });
  REQUIRE(render_template<src>(ctx) == "fallback");
}

TEST_CASE("template functions - default with null", "[template_functions][default]") {
  constexpr auto src = "{{ default(val, 42) }}"_fs;
  auto const ctx = make_template_object({
    {"val", nullptr},
  });
  REQUIRE(render_template<src>(ctx) == "42");
}

TEST_CASE("template functions - default with zero", "[template_functions][default]") {
  constexpr auto src = "{{ default(num, 10) }}"_fs;
  auto const ctx = make_template_object({
    {"num", 0},
  });
  REQUIRE(render_template<src>(ctx) == "0");
}

TEST_CASE("template functions - default with false", "[template_functions][default]") {
  constexpr auto src = "{{ default(flag, true) }}"_fs;
  auto const ctx = make_template_object({
    {"flag", false},
  });
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - default with empty array", "[template_functions][default]") {
  constexpr auto src = "{{ default(arr, fallback_val) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", template_array{}},
    {"fallback_val", "empty"},
  });
  REQUIRE(render_template<src>(ctx) == "empty");
}

TEST_CASE("template functions - default with empty object", "[template_functions][default]") {
  constexpr auto src = "{{ default(obj, fallback_val) }}"_fs;
  auto const ctx = make_template_object({
    {"obj", template_object{}},
    {"fallback_val", "empty"},
  });
  REQUIRE(render_template<src>(ctx) == "empty");
}

TEST_CASE("template functions - default with string literal", "[template_functions][default]") {
  constexpr auto src = "{{ default(\"hello\", \"world\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "hello");
}

TEST_CASE("template functions - default with empty string literal", "[template_functions][default]") {
  constexpr auto src = "{{ default(\"\", \"fallback\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "fallback");
}

// at() tests
TEST_CASE("template functions - at array index 0", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, 0) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "1");
}

TEST_CASE("template functions - at array index 1", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, 1) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "2");
}

TEST_CASE("template functions - at array index 2", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, 2) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("template functions - at array negative index -1", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, -1) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("template functions - at array negative index -2", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, -2) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "2");
}

TEST_CASE("template functions - at array negative index -3", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, -3) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "1");
}

TEST_CASE("template functions - at string array", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, 1) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({"a", "b", "c"})},
  });
  REQUIRE(render_template<src>(ctx) == "b");
}

TEST_CASE("template functions - at empty array error", "[template_functions][at_error]") {
  constexpr auto src = "{{ at(arr, 0) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", template_array{}},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - at out of bounds error", "[template_functions][at_error]") {
  constexpr auto src = "{{ at(arr, 5) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - at negative out of bounds error", "[template_functions][at_error]") {
  constexpr auto src = "{{ at(arr, -10) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - at non-integer index error", "[template_functions][at_error]") {
  constexpr auto src = "{{ at(arr, 1.5) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - at non-array error", "[template_functions][at_error]") {
  constexpr auto src = "{{ at(\"string\", 0) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// exists() tests
TEST_CASE("template functions - exists non-empty array", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - exists empty array", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", template_array{}},
  });
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - exists non-empty object", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(obj) }}"_fs;
  auto const ctx = make_template_object({
    {"obj", make_template_object({{"key", "value"}})},
  });
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - exists empty object", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(obj) }}"_fs;
  auto const ctx = make_template_object({
    {"obj", template_object{}},
  });
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - exists string returns false", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(text) }}"_fs;
  auto const ctx = make_template_object({
    {"text", "string"},
  });
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - exists null returns false", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(val) }}"_fs;
  auto const ctx = make_template_object({
    {"val", nullptr},
  });
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - exists number returns false", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(num) }}"_fs;
  auto const ctx = make_template_object({
    {"num", 42},
  });
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - exists bool returns false", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(flag) }}"_fs;
  auto const ctx = make_template_object({
    {"flag", true},
  });
  REQUIRE(render_template<src>(ctx) == "false");
}

// existsIn() tests
TEST_CASE("template functions - existsIn array found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(2, arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn array not found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(5, arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - existsIn object value found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(\"b\", obj) }}"_fs;
  auto const ctx = make_template_object({
    {"obj", make_template_object({{"a", "b"}, {"c", "d"}})},
  });
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn object value not found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(\"c\", obj) }}"_fs;
  auto const ctx = make_template_object({
    {"obj", make_template_object({{"a", "b"}})},
  });
  REQUIRE(render_template<src>(ctx) == "false");
}

TEST_CASE("template functions - existsIn int matches double", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(2, arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({2.0, 3.0})},
  });
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn double matches int", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(2.0, arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn bool found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(true, arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({false, true})},
  });
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn string found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(\"hello\", arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({"world", "hello"})},
  });
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn non-array/object error", "[template_functions][existsIn_error]") {
  constexpr auto src = "{{ existsIn(2, \"string\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("template functions - existsIn number container error", "[template_functions][existsIn_error]") {
  constexpr auto src = "{{ existsIn(2, 42) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

// Integration tests
TEST_CASE("template functions - default with conditional", "[template_functions][integration]") {
  constexpr auto src = "{% if exists(arr) %}{{ at(arr, 0) }}{% endif %}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({42})},
  });
  REQUIRE(render_template<src>(ctx) == "42");
}

TEST_CASE("template functions - at with mixed types", "[template_functions][integration]") {
  constexpr auto src = "{{ at(arr, 1) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, "string", 3.14})},
  });
  REQUIRE(render_template<src>(ctx) == "string");
}

TEST_CASE("template functions - default with function call", "[template_functions][integration]") {
  constexpr auto src = "{{ default(\"\", upper(fallback)) }}"_fs;
  auto const ctx = make_template_object({
    {"fallback", "hello"},
  });
  REQUIRE(render_template<src>(ctx) == "HELLO");
}

// ==================== Pipe Syntax Tests ====================

// Basic pipe tests
TEST_CASE("pipe - basic upper", "[pipe][basic]") {
  constexpr auto src = "{{ \"hello\" | upper }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "HELLO");
}

TEST_CASE("pipe - basic lower", "[pipe][basic]") {
  constexpr auto src = "{{ \"HELLO\" | lower }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "hello");
}

TEST_CASE("pipe - basic capitalize", "[pipe][basic]") {
  constexpr auto src = "{{ \"hello\" | capitalize }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "Hello");
}

TEST_CASE("pipe - basic with variable", "[pipe][basic]") {
  constexpr auto src = "{{ name | upper }}"_fs;
  auto const ctx = make_template_object({
    {"name", "world"},
  });
  REQUIRE(render_template<src>(ctx) == "WORLD");
}

// Pipe with arrays
TEST_CASE("pipe - array length", "[pipe][array]") {
  constexpr auto src = "{{ arr | length }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("pipe - array first", "[pipe][array]") {
  constexpr auto src = "{{ arr | first }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "1");
}

TEST_CASE("pipe - array last", "[pipe][array]") {
  constexpr auto src = "{{ arr | last }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("pipe - array sort and first", "[pipe][array]") {
  constexpr auto src = "{{ arr | sort | first }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({3, 1, 2})},
  });
  REQUIRE(render_template<src>(ctx) == "1");
}

// Pipe chains (multiple pipes)
TEST_CASE("pipe chain - upper then lower", "[pipe][chain]") {
  constexpr auto src = "{{ \"hello\" | upper | lower }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "hello");
}

TEST_CASE("pipe chain - upper then capitalize", "[pipe][chain]") {
  constexpr auto src = "{{ \"hello\" | upper | capitalize }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "HELLO");
}

TEST_CASE("pipe chain - triple pipe", "[pipe][chain]") {
  constexpr auto src = "{{ \"hello\" | upper | lower | upper }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "HELLO");
}

TEST_CASE("pipe chain - sort join", "[pipe][chain]") {
  constexpr auto src = "{{ arr | sort | join(\",\") }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({3, 1, 2})},
  });
  REQUIRE(render_template<src>(ctx) == "1,2,3");
}

// Pipes with function arguments
TEST_CASE("pipe - replace with arguments", "[pipe][function_args]") {
  constexpr auto src = "{{ \"hello world\" | replace(\"world\", \"there\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "hello there");
}

TEST_CASE("pipe - replace multiple t", "[pipe][function_args]") {
  constexpr auto src = "{{ \"test\" | replace(\"t\", \"T\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "TesT");
}

TEST_CASE("pipe - join with separator", "[pipe][function_args]") {
  constexpr auto src = "{{ arr | join(\", \") }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "1, 2, 3");
}

// Pipes with type conversions
TEST_CASE("pipe - string to int", "[pipe][type_conversion]") {
  constexpr auto src = "{{ \"42\" | int }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "42");
}

TEST_CASE("pipe - float to int", "[pipe][type_conversion]") {
  constexpr auto src = "{{ 3.14 | int }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("pipe - null to default string", "[pipe][type_conversion]") {
  constexpr auto src = "{{ value | default(\"fallback\") }}"_fs;
  auto const ctx = make_template_object({
    {"value", template_value{}},
  });
  REQUIRE(render_template<src>(ctx) == "fallback");
}

// Complex chains
TEST_CASE("pipe chain - sort join with bar separator", "[pipe][complex]") {
  constexpr auto src = "{{ arr | sort | join(\"|\") }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({5, 1, 3, 1, 4})},
  });
  REQUIRE(render_template<src>(ctx) == "1|1|3|4|5");
}

TEST_CASE("pipe chain - upper replace upper", "[pipe][complex]") {
  constexpr auto src = "{{ \"hello world\" | upper | replace(\"WORLD\", \"THERE\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "HELLO THERE");
}

TEST_CASE("pipe chain - sort length int", "[pipe][complex]") {
  constexpr auto src = "{{ arr | sort | length | int }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "3");
}

// Edge cases
TEST_CASE("pipe - first of array with single element", "[pipe][edge]") {
  constexpr auto src = "{{ arr | first }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({42})},
  });
  REQUIRE(render_template<src>(ctx) == "42");
}

TEST_CASE("pipe - isString after pipe", "[pipe][type_check]") {
  constexpr auto src = "{{ \"test\" | isString }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("pipe - isArray after pipe", "[pipe][type_check]") {
  constexpr auto src = "{{ arr | isArray }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "true");
}

TEST_CASE("pipe - even after sort", "[pipe][edge]") {
  constexpr auto src = "{{ arr | sort | first | even }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({3, 2, 1})},
  });
  REQUIRE(render_template<src>(ctx) == "false");
}

// Pipes in conditionals
TEST_CASE("pipe in if condition - length check", "[pipe][conditional]") {
  constexpr auto src = "{% if arr | length > 0 %}yes{% endif %}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2})},
  });
  REQUIRE(render_template<src>(ctx) == "yes");
}

TEST_CASE("pipe in if condition - empty array", "[pipe][conditional]") {
  constexpr auto src = "{% if arr | length > 0 %}yes{% else %}no{% endif %}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({})},
  });
  REQUIRE(render_template<src>(ctx) == "no");
}

TEST_CASE("pipe in if condition - isString check", "[pipe][conditional]") {
  constexpr auto src = "{% if val | isString %}string{% else %}not string{% endif %}"_fs;
  auto const ctx = make_template_object({
    {"val", "hello"},
  });
  REQUIRE(render_template<src>(ctx) == "string");
}

// Pipes with abs and round
TEST_CASE("pipe - abs negative number", "[pipe][math]") {
  constexpr auto src = "{{ -42 | abs }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "42");
}

TEST_CASE("pipe - round float", "[pipe][math]") {
  constexpr auto src = "{{ 3.7 | round }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "4");
}

// Pipes with max and min
TEST_CASE("pipe - max of array", "[pipe][array_math]") {
  constexpr auto src = "{{ arr | max }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 5, 3, 2})},
  });
  REQUIRE(render_template<src>(ctx) == "5");
}

TEST_CASE("pipe - min then int", "[pipe][array_math]") {
  constexpr auto src = "{{ arr | min | int }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({5, 1, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "1");
}

// Complex mixed operations
TEST_CASE("pipe with variable and function args", "[pipe][mixed]") {
  constexpr auto src = "{{ text | replace(old, new) }}"_fs;
  auto const ctx = make_template_object({
    {"text", "hello"},
    {"old", "l"},
    {"new", "L"},
  });
  REQUIRE(render_template<src>(ctx) == "heLLo");
}

TEST_CASE("pipe chain with literal and variable", "[pipe][mixed]") {
  constexpr auto src = "{{ \"HELLO\" | lower | replace(pattern, repl) }}"_fs;
  auto const ctx = make_template_object({
    {"pattern", "l"},
    {"repl", "L"},
  });
  REQUIRE(render_template<src>(ctx) == "heLLo");
}
