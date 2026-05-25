#include "catch2/catch_all.hpp"

#include <type_traits>

#include "frozenchars.hpp"

using builtin_fn = frozenchars::inja::detail::builtin_fn;

using namespace frozenchars;
using namespace frozenchars::inja;
using namespace frozenchars::literals;

static_assert(std::is_same_v<builtin_fn, inja_value (*)(std::vector<inja_value> const&)>);

TEST_CASE("template functions - upper", "[template_functions][upper]") {
  constexpr auto src = "{{ upper(name) }}"_fs;
  auto const ctx = make_object({
    {"name", "hello"},
  });
  REQUIRE(render<src>(ctx) == "HELLO");
}

TEST_CASE("template functions - lower", "[template_functions][lower]") {
  constexpr auto src = "{{ lower(name) }}"_fs;
  auto const ctx = make_object({
    {"name", "WORLD"},
  });
  REQUIRE(render<src>(ctx) == "world");
}

TEST_CASE("template functions - capitalize", "[template_functions][capitalize]") {
  constexpr auto src = "{{ capitalize(phrase) }}"_fs;
  auto const ctx = make_object({
    {"phrase", "hello world"},
  });
  REQUIRE(render<src>(ctx) == "Hello world");
}

TEST_CASE("template functions - capitalize empty", "[template_functions][capitalize]") {
  constexpr auto src = "{{ capitalize(text) }}"_fs;
  auto const ctx = make_object({
    {"text", ""},
  });
  REQUIRE(render<src>(ctx) == "");
}

TEST_CASE("template functions - replace", "[template_functions][replace]") {
  constexpr auto src = "{{ replace(text, old, new) }}"_fs;
  auto const ctx = make_object({
    {"text", "hello"},
    {"old", "l"},
    {"new", "L"},
  });
  REQUIRE(render<src>(ctx) == "heLLo");
}

TEST_CASE("template functions - replace not found", "[template_functions][replace]") {
  constexpr auto src = "{{ replace(text, old, new) }}"_fs;
  auto const ctx = make_object({
    {"text", "hello"},
    {"old", "x"},
    {"new", "y"},
  });
  REQUIRE(render<src>(ctx) == "hello");
}

TEST_CASE("template functions - upper with string literal", "[template_functions][upper]") {
  constexpr auto src = "{{ upper(\"hello\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "HELLO");
}

TEST_CASE("template functions - lower with string literal", "[template_functions][lower]") {
  constexpr auto src = "{{ lower(\"WORLD\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "world");
}

TEST_CASE("template functions - capitalize with string literal", "[template_functions][capitalize]") {
  constexpr auto src = "{{ capitalize(\"hello world\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "Hello world");
}

TEST_CASE("template functions - replace with string literals", "[template_functions][replace]") {
  constexpr auto src = "{{ replace(\"hello\", \"l\", \"L\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "heLLo");
}

TEST_CASE("template functions - type error upper on number", "[template_functions][type_error]") {
  constexpr auto src = "{{ upper(42) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - type error lower on number", "[template_functions][type_error]") {
  constexpr auto src = "{{ lower(123) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - wrong arg count upper", "[template_functions][arg_error]") {
  constexpr auto src = "{{ upper(\"hello\", \"world\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - wrong arg count lower", "[template_functions][arg_error]") {
  constexpr auto src = "{{ lower() }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - wrong arg count replace", "[template_functions][arg_error]") {
  constexpr auto src = "{{ replace(\"hello\", \"l\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - unknown function", "[template_functions][unknown]") {
  constexpr auto src = "{{ unknown_func(\"test\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - nested in if", "[template_functions][nested]") {
  constexpr auto src = "{% if text %}{{ upper(text) }}{% endif %}"_fs;
  auto const ctx = make_object({
    {"text", "hello"},
  });
  REQUIRE(render<src>(ctx) == "HELLO");
}

TEST_CASE("template functions - in for loop", "[template_functions][loop]") {
  constexpr auto src = "{% for item in items %}{{ upper(item) }};{% endfor %}"_fs;
  auto const ctx = make_object({
    {"items", make_array({"hello", "world"})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out.find("HELLO;") != std::string::npos);
  REQUIRE(out.find("WORLD;") != std::string::npos);
}

TEST_CASE("function calls in short-circuit or context", "[functions][short_circuit]") {
  constexpr auto src = "{{ true or upper(name) }}"_fs;
  auto const ctx = make_object({{"name", "hello"}});
  auto const out = render<src>(ctx);
  REQUIRE(out == "true");
}

TEST_CASE("function calls in short-circuit and context", "[functions][short_circuit]") {
  constexpr auto src = "{{ false and upper(name) }}"_fs;
  auto const ctx = make_object({{"name", "hello"}});
  auto const out = render<src>(ctx);
  REQUIRE(out == "false");
}

TEST_CASE("function calls evaluated when not short-circuited in and", "[functions][short_circuit]") {
  constexpr auto src = "{{ true and upper(name) }}"_fs;
  auto const ctx = make_object({{"name", "hello"}});
  auto const out = render<src>(ctx);
  REQUIRE(out == "true");
}

// ============ List functions tests ============

// length() tests
TEST_CASE("template functions - length", "[template_functions][length]") {
  constexpr auto src = "{{ length(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("template functions - length empty", "[template_functions][length]") {
  constexpr auto src = "{{ length(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({})},
  });
  REQUIRE(render<src>(ctx) == "0");
}

TEST_CASE("template functions - length single element", "[template_functions][length]") {
  constexpr auto src = "{{ length(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({"hello"})},
  });
  REQUIRE(render<src>(ctx) == "1");
}

// first() tests
TEST_CASE("template functions - first", "[template_functions][first]") {
  constexpr auto src = "{{ first(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "1");
}

TEST_CASE("template functions - first single element", "[template_functions][first]") {
  constexpr auto src = "{{ first(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({"hello"})},
  });
  REQUIRE(render<src>(ctx) == "hello");
}

TEST_CASE("template functions - first empty throws", "[template_functions][first]") {
  constexpr auto src = "{{ first(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// last() tests
TEST_CASE("template functions - last", "[template_functions][last]") {
  constexpr auto src = "{{ last(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("template functions - last single element", "[template_functions][last]") {
  constexpr auto src = "{{ last(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({"world"})},
  });
  REQUIRE(render<src>(ctx) == "world");
}

TEST_CASE("template functions - last empty throws", "[template_functions][last]") {
  constexpr auto src = "{{ last(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// join() tests
TEST_CASE("template functions - join basic", "[template_functions][join]") {
  constexpr auto src = "{{ join(items, \", \") }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "1, 2, 3");
}

TEST_CASE("template functions - join with strings", "[template_functions][join]") {
  constexpr auto src = "{{ join(items, \",\") }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({"a", "b", "c"})},
  });
  REQUIRE(render<src>(ctx) == "a,b,c");
}

TEST_CASE("template functions - join empty", "[template_functions][join]") {
  constexpr auto src = "{{ join(items, \", \") }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({})},
  });
  REQUIRE(render<src>(ctx) == "");
}

TEST_CASE("template functions - join single element", "[template_functions][join]") {
  constexpr auto src = "{{ join(items, \",\") }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({"hello"})},
  });
  REQUIRE(render<src>(ctx) == "hello");
}

TEST_CASE("template functions - join mixed types", "[template_functions][join]") {
  constexpr auto src = "{{ join(items, \"-\") }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, "hello", true})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out.find("1") != std::string::npos);
  REQUIRE(out.find("hello") != std::string::npos);
  REQUIRE(out.find("true") != std::string::npos);
}

TEST_CASE("template functions - sort integers", "[template_functions][sort]") {
  constexpr auto src = "{{ sort(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({3, 1, 2})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out.find("[array]") != std::string::npos);
}

TEST_CASE("template functions - sort strings", "[template_functions][sort]") {
  constexpr auto src = "{{ sort(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({"zebra", "apple", "mango"})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out.find("[array]") != std::string::npos);
}

TEST_CASE("template functions - sort empty", "[template_functions][sort]") {
  constexpr auto src = "{{ sort(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out.find("[array]") != std::string::npos);
}

TEST_CASE("template functions - sort single element", "[template_functions][sort]") {
  constexpr auto src = "{{ sort(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({42})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out.find("[array]") != std::string::npos);
}

TEST_CASE("template functions - sort preserves original", "[template_functions][sort]") {
  // Test by verifying first and last elements of sorted vs original
  constexpr auto src = "{{ first(items) }}-{{ first(sort(items)) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({3, 1, 2})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out == "3-1");
}

TEST_CASE("template functions - render object", "[template_functions]") {
  constexpr auto src = "{{ items }}"_fs;
  auto const ctx = make_object({
    {"items", make_object({{"a", 1}})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out.find("{object}") != std::string::npos);
}

// range() tests
TEST_CASE("template functions - range single arg", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(5)) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("template functions - range two args", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(2, 5)) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("template functions - range three args", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(0, 10, 2)) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("template functions - range three args negative step", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(10, 0, step)) }}"_fs;
  auto const ctx = make_object({
    {"step", std::int64_t{-2}},
  });
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("template functions - range zero", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(0)) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "0");
}

TEST_CASE("template functions - range large", "[template_functions][range]") {
  constexpr auto src = "{{ length(range(100)) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "100");
}

// Error cases for list functions
TEST_CASE("template functions - type error length on string", "[template_functions][type_error]") {
  constexpr auto src = "{{ length(\"hello\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - type error first on string", "[template_functions][type_error]") {
  constexpr auto src = "{{ first(\"hello\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - type error join second arg", "[template_functions][type_error]") {
  constexpr auto src = "{{ join(items, 42) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - wrong arg count length", "[template_functions][arg_error]") {
  constexpr auto src = "{{ length() }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - wrong arg count join", "[template_functions][arg_error]") {
  constexpr auto src = "{{ join(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - wrong arg count range", "[template_functions][arg_error]") {
  constexpr auto src = "{{ range(1, 2, 3, 4) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// Numeric functions - abs tests
TEST_CASE("template functions - abs positive integer", "[template_functions][abs]") {
  constexpr auto src = "{{ abs(3) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("template functions - abs negative integer", "[template_functions][abs]") {
  constexpr auto src = "{{ abs(-5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("template functions - abs zero", "[template_functions][abs]") {
  constexpr auto src = "{{ abs(0) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "0");
}

TEST_CASE("template functions - abs positive double", "[template_functions][abs]") {
  constexpr auto src = "{{ abs(3.14) }}"_fs;
  auto const ctx = make_object({});
  auto const result = render<src>(ctx);
  REQUIRE(result == "3.14");
}

TEST_CASE("template functions - abs negative double", "[template_functions][abs]") {
  constexpr auto src = "{{ abs(-3.14) }}"_fs;
  auto const ctx = make_object({});
  auto const result = render<src>(ctx);
  REQUIRE(result == "3.14");
}

TEST_CASE("template functions - abs type error string", "[template_functions][type_error]") {
  constexpr auto src = "{{ abs(\"hello\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - abs arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ abs(1, 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// Numeric functions - round tests
TEST_CASE("template functions - round default places", "[template_functions][round]") {
  constexpr auto src = "{{ round(3.14159) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("template functions - round 2 places", "[template_functions][round]") {
  constexpr auto src = "{{ round(3.14159, 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "3.14");
}

TEST_CASE("template functions - round negative 2 places", "[template_functions][round]") {
  constexpr auto src = "{{ round(-3.14159, 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "-3.14");
}

TEST_CASE("template functions - round 3 places", "[template_functions][round]") {
  constexpr auto src = "{{ round(3.14159, 3) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "3.142");
}

TEST_CASE("template functions - round 0 places explicit", "[template_functions][round]") {
  constexpr auto src = "{{ round(5.5, 0) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "6");
}

TEST_CASE("template functions - round integer input", "[template_functions][round]") {
  constexpr auto src = "{{ round(5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("template functions - round type error string", "[template_functions][type_error]") {
  constexpr auto src = "{{ round(\"text\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - round type error non-integer places", "[template_functions][type_error]") {
  constexpr auto src = "{{ round(3.14, \"places\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - round arg count error 3 args", "[template_functions][arg_error]") {
  constexpr auto src = "{{ round(1, 2, 3) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// Numeric functions - max tests
TEST_CASE("template functions - max integers", "[template_functions][max]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 5, 3})},
  });
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("template functions - max doubles", "[template_functions][max]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1.5, 2.0})},
  });
  REQUIRE(render<src>(ctx) == "2");
}

TEST_CASE("template functions - max negative integers", "[template_functions][max]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({-10, -5, -20})},
  });
  REQUIRE(render<src>(ctx) == "-5");
}

TEST_CASE("template functions - max mixed types", "[template_functions][max]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 3.5, 2})},
  });
  auto const result = render<src>(ctx);
  REQUIRE(result == "3.5");
}

TEST_CASE("template functions - max empty array", "[template_functions][type_error]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - max non-array", "[template_functions][type_error]") {
  constexpr auto src = "{{ max(\"not array\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - max array with string", "[template_functions][type_error]") {
  constexpr auto src = "{{ max(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, "string", 3})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - max arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ max(1, 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// Numeric functions - min tests
TEST_CASE("template functions - min integers", "[template_functions][min]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 5, 3})},
  });
  REQUIRE(render<src>(ctx) == "1");
}

TEST_CASE("template functions - min doubles", "[template_functions][min]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1.5, 2.0})},
  });
  REQUIRE(render<src>(ctx) == "1.5");
}

TEST_CASE("template functions - min negative integers", "[template_functions][min]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({-10, -5, -20})},
  });
  REQUIRE(render<src>(ctx) == "-20");
}

TEST_CASE("template functions - min mixed types", "[template_functions][min]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({3, 1.5, 2})},
  });
  REQUIRE(render<src>(ctx) == "1.5");
}

TEST_CASE("template functions - min empty array", "[template_functions][type_error]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - min non-array", "[template_functions][type_error]") {
  constexpr auto src = "{{ min(\"not array\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - min array with string", "[template_functions][type_error]") {
  constexpr auto src = "{{ min(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, "string", 3})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - min arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ min(1, 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// Numeric functions - even tests
TEST_CASE("template functions - even positive", "[template_functions][even]") {
  constexpr auto src = "{{ even(4) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - even odd positive", "[template_functions][even]") {
  constexpr auto src = "{{ even(5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - even zero", "[template_functions][even]") {
  constexpr auto src = "{{ even(0) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - even negative", "[template_functions][even]") {
  constexpr auto src = "{{ even(-4) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - even odd negative", "[template_functions][even]") {
  constexpr auto src = "{{ even(-5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - even type error float", "[template_functions][type_error]") {
  constexpr auto src = "{{ even(3.5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - even type error string", "[template_functions][type_error]") {
  constexpr auto src = "{{ even(\"4\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - even arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ even(1, 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// Numeric functions - odd tests
TEST_CASE("template functions - odd positive", "[template_functions][odd]") {
  constexpr auto src = "{{ odd(5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - odd even positive", "[template_functions][odd]") {
  constexpr auto src = "{{ odd(4) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - odd zero", "[template_functions][odd]") {
  constexpr auto src = "{{ odd(0) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - odd negative", "[template_functions][odd]") {
  constexpr auto src = "{{ odd(-5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - odd even negative", "[template_functions][odd]") {
  constexpr auto src = "{{ odd(-4) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - odd type error float", "[template_functions][type_error]") {
  constexpr auto src = "{{ odd(3.5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - odd type error string", "[template_functions][type_error]") {
  constexpr auto src = "{{ odd(\"5\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - odd arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ odd(1, 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// Numeric functions - divisibleBy tests
TEST_CASE("template functions - divisibleBy true", "[template_functions][divisibleBy]") {
  constexpr auto src = "{{ divisibleBy(10, 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - divisibleBy false", "[template_functions][divisibleBy]") {
  constexpr auto src = "{{ divisibleBy(10, 3) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - divisibleBy exact", "[template_functions][divisibleBy]") {
  constexpr auto src = "{{ divisibleBy(15, 5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - divisibleBy negative dividend", "[template_functions][divisibleBy]") {
  constexpr auto src = "{{ divisibleBy(-10, 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - divisibleBy zero", "[template_functions][divisibleBy]") {
  constexpr auto src = "{{ divisibleBy(0, 5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - divisibleBy divide by zero", "[template_functions][type_error]") {
  constexpr auto src = "{{ divisibleBy(10, 0) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - divisibleBy type error float dividend", "[template_functions][type_error]") {
  constexpr auto src = "{{ divisibleBy(10.5, 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - divisibleBy type error string dividend", "[template_functions][type_error]") {
  constexpr auto src = "{{ divisibleBy(\"10\", 2) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - divisibleBy type error float divisor", "[template_functions][type_error]") {
  constexpr auto src = "{{ divisibleBy(10, 2.5) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - divisibleBy arg count error", "[template_functions][arg_error]") {
  constexpr auto src = "{{ divisibleBy(10) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// ============ Utility Functions Tests ============

// default() tests
TEST_CASE("template functions - default with non-empty string", "[template_functions][default]") {
  constexpr auto src = "{{ default(text, \"fallback\") }}"_fs;
  auto const ctx = make_object({
    {"text", "hello"},
  });
  REQUIRE(render<src>(ctx) == "hello");
}

TEST_CASE("template functions - default with empty string", "[template_functions][default]") {
  constexpr auto src = "{{ default(text, \"fallback\") }}"_fs;
  auto const ctx = make_object({
    {"text", ""},
  });
  REQUIRE(render<src>(ctx) == "fallback");
}

TEST_CASE("template functions - default with null", "[template_functions][default]") {
  constexpr auto src = "{{ default(val, 42) }}"_fs;
  auto const ctx = make_object({
    {"val", nullptr},
  });
  REQUIRE(render<src>(ctx) == "42");
}

TEST_CASE("template functions - default with zero", "[template_functions][default]") {
  constexpr auto src = "{{ default(num, 10) }}"_fs;
  auto const ctx = make_object({
    {"num", 0},
  });
  REQUIRE(render<src>(ctx) == "0");
}

TEST_CASE("template functions - default with false", "[template_functions][default]") {
  constexpr auto src = "{{ default(flag, true) }}"_fs;
  auto const ctx = make_object({
    {"flag", false},
  });
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - default with empty array", "[template_functions][default]") {
  constexpr auto src = "{{ default(arr, fallback_val) }}"_fs;
  auto const ctx = make_object({
    {"arr", inja_array{}},
    {"fallback_val", "empty"},
  });
  REQUIRE(render<src>(ctx) == "empty");
}

TEST_CASE("template functions - default with empty object", "[template_functions][default]") {
  constexpr auto src = "{{ default(obj, fallback_val) }}"_fs;
  auto const ctx = make_object({
    {"obj", inja_object{}},
    {"fallback_val", "empty"},
  });
  REQUIRE(render<src>(ctx) == "empty");
}

TEST_CASE("template functions - default with string literal", "[template_functions][default]") {
  constexpr auto src = "{{ default(\"hello\", \"world\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "hello");
}

TEST_CASE("template functions - default with empty string literal", "[template_functions][default]") {
  constexpr auto src = "{{ default(\"\", \"fallback\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "fallback");
}

// at() tests
TEST_CASE("template functions - at array index 0", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, 0) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "1");
}

TEST_CASE("template functions - at array index 1", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, 1) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "2");
}

TEST_CASE("template functions - at array index 2", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, 2) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("template functions - at array negative index -1", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, -1) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("template functions - at array negative index -2", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, -2) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "2");
}

TEST_CASE("template functions - at array negative index -3", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, -3) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "1");
}

TEST_CASE("template functions - at string array", "[template_functions][at]") {
  constexpr auto src = "{{ at(arr, 1) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({"a", "b", "c"})},
  });
  REQUIRE(render<src>(ctx) == "b");
}

TEST_CASE("template functions - at empty array error", "[template_functions][at_error]") {
  constexpr auto src = "{{ at(arr, 0) }}"_fs;
  auto const ctx = make_object({
    {"arr", inja_array{}},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - at out of bounds error", "[template_functions][at_error]") {
  constexpr auto src = "{{ at(arr, 5) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - at negative out of bounds error", "[template_functions][at_error]") {
  constexpr auto src = "{{ at(arr, -10) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - at non-integer index error", "[template_functions][at_error]") {
  constexpr auto src = "{{ at(arr, 1.5) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - at non-array error", "[template_functions][at_error]") {
  constexpr auto src = "{{ at(\"string\", 0) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// exists() tests
TEST_CASE("template functions - exists non-empty array", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(arr) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - exists empty array", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(arr) }}"_fs;
  auto const ctx = make_object({
    {"arr", inja_array{}},
  });
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - exists non-empty object", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(obj) }}"_fs;
  auto const ctx = make_object({
    {"obj", make_object({{"key", "value"}})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - exists empty object", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(obj) }}"_fs;
  auto const ctx = make_object({
    {"obj", inja_object{}},
  });
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - exists string returns false", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(text) }}"_fs;
  auto const ctx = make_object({
    {"text", "string"},
  });
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - exists null returns false", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(val) }}"_fs;
  auto const ctx = make_object({
    {"val", nullptr},
  });
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - exists number returns false", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(num) }}"_fs;
  auto const ctx = make_object({
    {"num", 42},
  });
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - exists bool returns false", "[template_functions][exists]") {
  constexpr auto src = "{{ exists(flag) }}"_fs;
  auto const ctx = make_object({
    {"flag", true},
  });
  REQUIRE(render<src>(ctx) == "false");
}

// existsIn() tests
TEST_CASE("template functions - existsIn array found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(2, arr) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn array not found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(5, arr) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - existsIn object value found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(\"b\", obj) }}"_fs;
  auto const ctx = make_object({
    {"obj", make_object({{"a", "b"}, {"c", "d"}})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn object value not found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(\"c\", obj) }}"_fs;
  auto const ctx = make_object({
    {"obj", make_object({{"a", "b"}})},
  });
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("template functions - existsIn int matches double", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(2, arr) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({2.0, 3.0})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn double matches int", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(2.0, arr) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn bool found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(true, arr) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({false, true})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn string found", "[template_functions][existsIn]") {
  constexpr auto src = "{{ existsIn(\"hello\", arr) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({"world", "hello"})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("template functions - existsIn non-array/object error", "[template_functions][existsIn_error]") {
  constexpr auto src = "{{ existsIn(2, \"string\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

TEST_CASE("template functions - existsIn number container error", "[template_functions][existsIn_error]") {
  constexpr auto src = "{{ existsIn(2, 42) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// Integration tests
TEST_CASE("template functions - default with conditional", "[template_functions][integration]") {
  constexpr auto src = "{% if exists(arr) %}{{ at(arr, 0) }}{% endif %}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({42})},
  });
  REQUIRE(render<src>(ctx) == "42");
}

TEST_CASE("template functions - at with mixed types", "[template_functions][integration]") {
  constexpr auto src = "{{ at(arr, 1) }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, "string", 3.14})},
  });
  REQUIRE(render<src>(ctx) == "string");
}

TEST_CASE("template functions - default with function call", "[template_functions][integration]") {
  constexpr auto src = "{{ default(\"\", upper(fallback)) }}"_fs;
  auto const ctx = make_object({
    {"fallback", "hello"},
  });
  REQUIRE(render<src>(ctx) == "HELLO");
}

// ============ Comprehensive Integration Tests ============

// 1. Data Processing Pipeline
TEST_CASE("integration - data pipeline with length and int", "[integration][data_processing]") {
  constexpr auto src = "{{ items | length | int }} people"_fs;
  auto const ctx = make_object({
    {"items", make_array({make_object({{"name", "Alice"}, {"age", 25}}),
                                   make_object({{"name", "Bob"}, {"age", 30}})})},
  });
  REQUIRE(render<src>(ctx) == "2 people");
}

TEST_CASE("integration - array first with exists", "[integration][data_processing]") {
  constexpr auto src = "{{ items | first | exists }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "false");
}

// 2. String Transformation Chains
TEST_CASE("integration - string chain upper replace lower", "[integration][string_chain]") {
  constexpr auto src = "{{ \"hello world\" | upper | replace(\"WORLD\", \"THERE\") | lower }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "hello there");
}

TEST_CASE("integration - string chain capitalize upper", "[integration][string_chain]") {
  constexpr auto src = "{{ \"hello\" | capitalize | upper }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "HELLO");
}

TEST_CASE("integration - complex string transform", "[integration][string_chain]") {
  constexpr auto src = "{{ text | upper | replace(\"HELLO\", \"HI\") | lower | capitalize }}"_fs;
  auto const ctx = make_object({
    {"text", "hello world"},
  });
  REQUIRE(render<src>(ctx) == "Hi world");
}

// 3. Array Operations
TEST_CASE("integration - array sort and join", "[integration][array_ops]") {
  constexpr auto src = "{{ arr | sort | join(\",\") }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({3, 1, 4, 1, 5, 9, 2, 6})},
  });
  REQUIRE(render<src>(ctx) == "1,1,2,3,4,5,6,9");
}

TEST_CASE("integration - array first with int and abs", "[integration][array_ops]") {
  constexpr auto src = "{{ arr | first | int | abs }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({10, 20, 30})},
  });
  REQUIRE(render<src>(ctx) == "10");
}

TEST_CASE("integration - array operations pipeline", "[integration][array_ops]") {
  constexpr auto src = "{{ nums | sort | last | int }}"_fs;
  auto const ctx = make_object({
    {"nums", make_array({5, 2, 8, 1, 9})},
  });
  REQUIRE(render<src>(ctx) == "9");
}

// 4. Type Checking and Conversion
TEST_CASE("integration - type check with conditional", "[integration][type_conversion]") {
  constexpr auto src = "{% if value | isNumber %}{{ value | int }}{% else %}N/A{% endif %}"_fs;
  auto const ctx = make_object({
    {"value", 42},
  });
  REQUIRE(render<src>(ctx) == "42");
}

TEST_CASE("integration - type check with conditional false", "[integration][type_conversion]") {
  constexpr auto src = "{% if value | isNumber %}{{ value | int }}{% else %}N/A{% endif %}"_fs;
  auto const ctx = make_object({
    {"value", "hello"},
  });
  REQUIRE(render<src>(ctx) == "N/A");
}

TEST_CASE("integration - array type checking", "[integration][type_conversion]") {
  constexpr auto src = "{{ arr | first | isInteger }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, "two", 3})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

// 5. Conditional Logic with Functions
TEST_CASE("integration - conditional with array operations", "[integration][conditional]") {
  constexpr auto src = "{% if items | length > 0 %}{% for item in items %}{{ item | upper }} {% endfor %}{% else %}No items{% endif %}"_fs;
  auto const ctx = make_object({
    {"items", make_array({"alice", "bob"})},
  });
  REQUIRE(render<src>(ctx) == "ALICE BOB ");
}

TEST_CASE("integration - conditional empty array", "[integration][conditional]") {
  constexpr auto src = "{% if items | length > 0 %}Has items{% else %}No items{% endif %}"_fs;
  auto const ctx = make_object({
    {"items", make_array({})},
  });
  REQUIRE(render<src>(ctx) == "No items");
}

TEST_CASE("integration - nested conditional with functions", "[integration][conditional]") {
  constexpr auto src = "{% if items | exists %}{% if items | length > 1 %}Multiple{% else %}Single{% endif %}{% else %}None{% endif %}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2})},
  });
  REQUIRE(render<src>(ctx) == "Multiple");
}

// 6. Default Values and Fallbacks
TEST_CASE("integration - default with null value", "[integration][default]") {
  constexpr auto src = "{{ null | default(\"N/A\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "N/A");
}

TEST_CASE("integration - default with empty string", "[integration][default]") {
  constexpr auto src = "{{ \"\" | default(\"empty\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "empty");
}

TEST_CASE("integration - default with empty array length", "[integration][default]") {
  constexpr auto src = "{{ arr | default(arr_default) | length }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({})},
    {"arr_default", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "3");
}

// 7. Complex Data Extraction
TEST_CASE("integration - extract first element exists", "[integration][data_extraction]") {
  constexpr auto src = "{{ users | first | exists }}"_fs;
  auto const ctx = make_object({
    {"users", make_array({make_object({{"name", "Alice"}})})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("integration - array at with isArray", "[integration][data_extraction]") {
  constexpr auto src = "{{ data | at(0) | isArray }}"_fs;
  auto const ctx = make_object({
    {"data", make_array({make_array({1, 2, 3})})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

// 8. Chained Function Calls
TEST_CASE("integration - string to int to abs to float", "[integration][chained]") {
  constexpr auto src = "{{ \"42\" | int | abs | float }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "42");
}

TEST_CASE("integration - array length with default and conversion", "[integration][chained]") {
  constexpr auto src = "{{ items | length | default(0) | int | even }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "false");
}

TEST_CASE("integration - complex multi-step conversion", "[integration][chained]") {
  constexpr auto src = "{{ value | abs | float | int }}"_fs;
  auto const ctx = make_object({
    {"value", -42},
  });
  REQUIRE(render<src>(ctx) == "42");
}

// 9. Nested Operations
TEST_CASE("integration - nested string operations in for loop", "[integration][nested]") {
  constexpr auto src = "{% for item in list | sort %}{{ item | capitalize | upper }} {% endfor %}"_fs;
  auto const ctx = make_object({
    {"list", make_array({"charlie", "alice", "bob"})},
  });
  REQUIRE(render<src>(ctx) == "ALICE BOB CHARLIE ");
}

TEST_CASE("integration - nested array operations", "[integration][nested]") {
  constexpr auto src = "{{ items | first | at(0) | upper }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({make_array({"hello", "world"})})},
  });
  REQUIRE(render<src>(ctx) == "HELLO");
}

// 10. Real-world Complete Template
TEST_CASE("integration - user summary template", "[integration][realistic]") {
  constexpr auto src = "{% if users | exists %}Total: {{ users | length }}{% for user in users | sort %}Name: {{ user | upper }}{% endfor %}{% else %}Empty{% endif %}"_fs;
  auto const ctx = make_object({
    {"users", make_array({"charlie", "alice", "bob"})},
  });
  REQUIRE(render<src>(ctx) == "Total: 3Name: ALICEName: BOBName: CHARLIE");
}

TEST_CASE("integration - data processing pipeline", "[integration][realistic]") {
  constexpr auto src = "Numbers: {{ nums | sort | join(\",\") }} Min: {{ nums | min }} Max: {{ nums | max }}"_fs;
  auto const ctx = make_object({
    {"nums", make_array({5, 2, 8, 1, 9})},
  });
  REQUIRE(render<src>(ctx) == "Numbers: 1,2,5,8,9 Min: 1 Max: 9");
}

// 11. Performance Test - No exponential complexity
TEST_CASE("integration - large array sort performance", "[integration][performance]") {
  constexpr auto src = "{{ arr | sort | length }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({100, 99, 98, 97, 96, 95, 94, 93, 92, 91,
                                 90, 89, 88, 87, 86, 85, 84, 83, 82, 81,
                                 80, 79, 78, 77, 76, 75, 74, 73, 72, 71,
                                 70, 69, 68, 67, 66, 65, 64, 63, 62, 61,
                                 60, 59, 58, 57, 56, 55, 54, 53, 52, 51,
                                 50, 49, 48, 47, 46, 45, 44, 43, 42, 41,
                                 40, 39, 38, 37, 36, 35, 34, 33, 32, 31,
                                 30, 29, 28, 27, 26, 25, 24, 23, 22, 21,
                                 20, 19, 18, 17, 16, 15, 14, 13, 12, 11,
                                 10, 9, 8, 7, 6, 5, 4, 3, 2, 1})},
  });
  REQUIRE(render<src>(ctx) == "100");
}

TEST_CASE("integration - long string chain performance", "[integration][performance]") {
  constexpr auto src = "{{ text | upper | lower | upper | lower | upper | lower | upper }}"_fs;
  auto const ctx = make_object({
    {"text", "hello"},
  });
  REQUIRE(render<src>(ctx) == "HELLO");
}

// 12. Error Handling in Context
TEST_CASE("integration - error propagation in conditional", "[integration][error_handling]") {
  constexpr auto src = "{% if arr | exists %}{{ arr | first }}{% endif %}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({42})},
  });
  REQUIRE(render<src>(ctx) == "42");
}

// 13. Multi-step Transformations
TEST_CASE("integration - transform pipeline with multiple stages", "[integration][transformation]") {
  constexpr auto src = "{{ text | upper | replace(\"HELLO\", \"HI\") | lower | capitalize }}"_fs;
  auto const ctx = make_object({
    {"text", "hello world"},
  });
  REQUIRE(render<src>(ctx) == "Hi world");
}

TEST_CASE("integration - numeric transformation pipeline", "[integration][transformation]") {
  constexpr auto src = "{{ value | abs | round(2) | float }}"_fs;
  auto const ctx = make_object({
    {"value", -3.14159},
  });
  REQUIRE(render<src>(ctx) == "3.14");
}

// 14. Array and Conditional Combinations
TEST_CASE("integration - conditional array processing", "[integration][array_conditional]") {
  constexpr auto src = "{% if items | length > 1 %}{{ items | sort | join(\"-\") }}{% else %}Single{% endif %}"_fs;
  auto const ctx = make_object({
    {"items", make_array({3, 1, 2})},
  });
  REQUIRE(render<src>(ctx) == "1-2-3");
}

TEST_CASE("integration - filter with default fallback", "[integration][array_conditional]") {
  constexpr auto src = "{{ empty_list | length | default(5) | int }}"_fs;
  auto const ctx = make_object({
    {"empty_list", make_array({})},
  });
  REQUIRE(render<src>(ctx) == "0");
}

// 15. String and Type Operations
TEST_CASE("integration - string to array operations", "[integration][string_type]") {
  constexpr auto src = "{{ items | length | float }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3, 4})},
  });
  REQUIRE(render<src>(ctx) == "4");
}

TEST_CASE("integration - numeric string conversion", "[integration][string_type]") {
  constexpr auto src = "{{ \"123\" | int | even }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "false");
}

// 16. For Loop with Function Chains
TEST_CASE("integration - for loop with nested functions", "[integration][for_loop]") {
  constexpr auto src = "{% for n in nums | sort %}{{ n | abs | even }}, {% endfor %}"_fs;
  auto const ctx = make_object({
    {"nums", make_array({-4, -1, 2})},
  });
  REQUIRE(render<src>(ctx) == "true, false, true, ");
}

TEST_CASE("integration - for loop with string transformation", "[integration][for_loop]") {
  constexpr auto src = "{% for word in words %}{{ word | upper | capitalize }},{% endfor %}"_fs;
  auto const ctx = make_object({
    {"words", make_array({"apple", "banana", "cherry"})},
  });
  REQUIRE(render<src>(ctx) == "APPLE,BANANA,CHERRY,");
}

// 17. Complex Mixed Operations
TEST_CASE("integration - max with conditional", "[integration][mixed]") {
  constexpr auto src = "{% if nums | max | even %}Even{% else %}Odd{% endif %}"_fs;
  auto const ctx = make_object({
    {"nums", make_array({1, 2, 3, 4, 5})},
  });
  REQUIRE(render<src>(ctx) == "Odd");
}

TEST_CASE("integration - min with transformation", "[integration][mixed]") {
  constexpr auto src = "{{ nums | min | abs | float }}"_fs;
  auto const ctx = make_object({
    {"nums", make_array({-10, 5, -3})},
  });
  REQUIRE(render<src>(ctx) == "10");
}

// 18. Default with Complex Chains
TEST_CASE("integration - default in chain", "[integration][default_chain]") {
  constexpr auto src = "{{ arr | first | default(0) | int }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}

// 19. Multiple Array Operations
TEST_CASE("integration - sort and reverse via last", "[integration][array_multi]") {
  constexpr auto src = "{{ nums | sort | last }}"_fs;
  auto const ctx = make_object({
    {"nums", make_array({3, 1, 4, 1, 5})},
  });
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("integration - first and last extraction", "[integration][array_multi]") {
  constexpr auto src = "First: {{ arr | first }} Last: {{ arr | last }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({10, 20, 30})},
  });
  REQUIRE(render<src>(ctx) == "First: 10 Last: 30");
}

// 20. Range with Other Operations
TEST_CASE("integration - range with join", "[integration][range]") {
  constexpr auto src = "{{ range(1, 4) | join(\"-\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "1-2-3");
}

TEST_CASE("integration - range with sort", "[integration][range]") {
  constexpr auto src = "{{ range(1, 4) | sort | join(\",\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "1,2,3");
}

// 21. Exists Checks with Operations
TEST_CASE("integration - exists with nested access", "[integration][exists]") {
  constexpr auto src = "{{ items | first | exists }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({make_object({{"x", 1}})})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("integration - at with exists check", "[integration][exists]") {
  constexpr auto src = "{% if arr | exists %}{{ arr | at(0) }}{% else %}No{% endif %}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({42, 99})},
  });
  REQUIRE(render<src>(ctx) == "42");
}

// 22. Divisibility with Control Flow
TEST_CASE("integration - divisibility in conditional", "[integration][divisibility]") {
  constexpr auto src = "{% if divisibleBy(num, 3) %}Divisible{% else %}Not{% endif %}"_fs;
  auto const ctx = make_object({
    {"num", 15},
  });
  REQUIRE(render<src>(ctx) == "Divisible");
}

// 23. Odd/Even in Processing
TEST_CASE("integration - odd even in array filter via loop", "[integration][odd_even]") {
  constexpr auto src = "{% for n in nums %}{% if even(n) %}{{ n }},{% endif %}{% endfor %}"_fs;
  auto const ctx = make_object({
    {"nums", make_array({1, 2, 3, 4, 5})},
  });
  REQUIRE(render<src>(ctx) == "2,4,");
}

// 24. Replace in Loops
TEST_CASE("integration - replace in array iteration", "[integration][replace_loop]") {
  constexpr auto src = "{% for word in words %}{{ replace(word, \"a\", \"@\") }}, {% endfor %}"_fs;
  auto const ctx = make_object({
    {"words", make_array({"apple", "banana", "apricot"})},
  });
  REQUIRE(render<src>(ctx) == "@pple, b@n@n@, @pricot, ");
}

// 25. Complex Nested Pipes
TEST_CASE("integration - pipe in array operations", "[integration][pipe_complex]") {
  constexpr auto src = "{{ arr | sort | first | upper }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({"zebra", "apple", "mango"})},
  });
  REQUIRE(render<src>(ctx) == "APPLE");
}

// ==================== Pipe Syntax Tests ====================

// Basic pipe tests
TEST_CASE("pipe - basic upper", "[pipe][basic]") {
  constexpr auto src = "{{ \"hello\" | upper }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "HELLO");
}

TEST_CASE("pipe - basic lower", "[pipe][basic]") {
  constexpr auto src = "{{ \"HELLO\" | lower }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "hello");
}

TEST_CASE("pipe - basic capitalize", "[pipe][basic]") {
  constexpr auto src = "{{ \"hello\" | capitalize }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "Hello");
}

TEST_CASE("pipe - basic with variable", "[pipe][basic]") {
  constexpr auto src = "{{ name | upper }}"_fs;
  auto const ctx = make_object({
    {"name", "world"},
  });
  REQUIRE(render<src>(ctx) == "WORLD");
}

// Pipe with arrays
TEST_CASE("pipe - array length", "[pipe][array]") {
  constexpr auto src = "{{ arr | length }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("pipe - array first", "[pipe][array]") {
  constexpr auto src = "{{ arr | first }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "1");
}

TEST_CASE("pipe - array last", "[pipe][array]") {
  constexpr auto src = "{{ arr | last }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("pipe - array sort and first", "[pipe][array]") {
  constexpr auto src = "{{ arr | sort | first }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({3, 1, 2})},
  });
  REQUIRE(render<src>(ctx) == "1");
}

// Pipe chains (multiple pipes)
TEST_CASE("pipe chain - upper then lower", "[pipe][chain]") {
  constexpr auto src = "{{ \"hello\" | upper | lower }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "hello");
}

TEST_CASE("pipe chain - upper then capitalize", "[pipe][chain]") {
  constexpr auto src = "{{ \"hello\" | upper | capitalize }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "HELLO");
}

TEST_CASE("pipe chain - triple pipe", "[pipe][chain]") {
  constexpr auto src = "{{ \"hello\" | upper | lower | upper }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "HELLO");
}

TEST_CASE("pipe chain - sort join", "[pipe][chain]") {
  constexpr auto src = "{{ arr | sort | join(\",\") }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({3, 1, 2})},
  });
  REQUIRE(render<src>(ctx) == "1,2,3");
}

// Pipes with function arguments
TEST_CASE("pipe - replace with arguments", "[pipe][function_args]") {
  constexpr auto src = "{{ \"hello world\" | replace(\"world\", \"there\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "hello there");
}

TEST_CASE("pipe - replace multiple t", "[pipe][function_args]") {
  constexpr auto src = "{{ \"test\" | replace(\"t\", \"T\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "TesT");
}

TEST_CASE("pipe - join with separator", "[pipe][function_args]") {
  constexpr auto src = "{{ arr | join(\", \") }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "1, 2, 3");
}

// Pipes with type conversions
TEST_CASE("pipe - string to int", "[pipe][type_conversion]") {
  constexpr auto src = "{{ \"42\" | int }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "42");
}

TEST_CASE("pipe - float to int", "[pipe][type_conversion]") {
  constexpr auto src = "{{ 3.14 | int }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("pipe - null to default string", "[pipe][type_conversion]") {
  constexpr auto src = "{{ value | default(\"fallback\") }}"_fs;
  auto const ctx = make_object({
    {"value", inja_value{}},
  });
  REQUIRE(render<src>(ctx) == "fallback");
}

// Complex chains
TEST_CASE("pipe chain - sort join with bar separator", "[pipe][complex]") {
  constexpr auto src = "{{ arr | sort | join(\"|\") }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({5, 1, 3, 1, 4})},
  });
  REQUIRE(render<src>(ctx) == "1|1|3|4|5");
}

TEST_CASE("pipe chain - upper replace upper", "[pipe][complex]") {
  constexpr auto src = "{{ \"hello world\" | upper | replace(\"WORLD\", \"THERE\") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "HELLO THERE");
}

TEST_CASE("pipe chain - sort length int", "[pipe][complex]") {
  constexpr auto src = "{{ arr | sort | length | int }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "3");
}

// Edge cases
TEST_CASE("pipe - first of array with single element", "[pipe][edge]") {
  constexpr auto src = "{{ arr | first }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({42})},
  });
  REQUIRE(render<src>(ctx) == "42");
}

TEST_CASE("pipe - isString after pipe", "[pipe][type_check]") {
  constexpr auto src = "{{ \"test\" | isString }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("pipe - isArray after pipe", "[pipe][type_check]") {
  constexpr auto src = "{{ arr | isArray }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "true");
}

TEST_CASE("pipe - even after sort", "[pipe][edge]") {
  constexpr auto src = "{{ arr | sort | first | even }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({3, 2, 1})},
  });
  REQUIRE(render<src>(ctx) == "false");
}

// Pipes in conditionals
TEST_CASE("pipe in if condition - length check", "[pipe][conditional]") {
  constexpr auto src = "{% if arr | length > 0 %}yes{% endif %}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 2})},
  });
  REQUIRE(render<src>(ctx) == "yes");
}

TEST_CASE("pipe in if condition - empty array", "[pipe][conditional]") {
  constexpr auto src = "{% if arr | length > 0 %}yes{% else %}no{% endif %}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({})},
  });
  REQUIRE(render<src>(ctx) == "no");
}

TEST_CASE("pipe in if condition - isString check", "[pipe][conditional]") {
  constexpr auto src = "{% if val | isString %}string{% else %}not string{% endif %}"_fs;
  auto const ctx = make_object({
    {"val", "hello"},
  });
  REQUIRE(render<src>(ctx) == "string");
}

// Pipes with abs and round
TEST_CASE("pipe - abs negative number", "[pipe][math]") {
  constexpr auto src = "{{ -42 | abs }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "42");
}

TEST_CASE("pipe - round float", "[pipe][math]") {
  constexpr auto src = "{{ 3.7 | round }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "4");
}

// Pipes with max and min
TEST_CASE("pipe - max of array", "[pipe][array_math]") {
  constexpr auto src = "{{ arr | max }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({1, 5, 3, 2})},
  });
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("pipe - min then int", "[pipe][array_math]") {
  constexpr auto src = "{{ arr | min | int }}"_fs;
  auto const ctx = make_object({
    {"arr", make_array({5, 1, 3})},
  });
  REQUIRE(render<src>(ctx) == "1");
}

// Complex mixed operations
TEST_CASE("pipe with variable and function args", "[pipe][mixed]") {
  constexpr auto src = "{{ text | replace(old, new) }}"_fs;
  auto const ctx = make_object({
    {"text", "hello"},
    {"old", "l"},
    {"new", "L"},
  });
  REQUIRE(render<src>(ctx) == "heLLo");
}

TEST_CASE("pipe chain with literal and variable", "[pipe][mixed]") {
  constexpr auto src = "{{ \"HELLO\" | lower | replace(pattern, repl) }}"_fs;
  auto const ctx = make_object({
    {"pattern", "l"},
    {"repl", "L"},
  });
  REQUIRE(render<src>(ctx) == "heLLo");
}

// Operator + Pipe precedence tests
TEST_CASE("multiplication with pipe", "[pipe][operator_precedence]") {
  constexpr auto src = "{{ 2 * 3 | abs }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "6");
}

TEST_CASE("division with pipe", "[pipe][operator_precedence]") {
  constexpr auto src = "{{ 6 / 2 | abs }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("subtraction with pipe on right operand", "[pipe][operator_precedence]") {
  constexpr auto src = "{{ 10 - -5 | abs }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("addition with pipe on right operand", "[pipe][operator_precedence]") {
  constexpr auto src = "{{ 2 + 3 | int }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("modulo with pipe on right operand", "[pipe][operator_precedence]") {
  constexpr auto src = "{{ 100 % 30 | int }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "10");
}

TEST_CASE("chained pipes on right operand", "[pipe][operator_precedence]") {
  constexpr auto src = "{{ 10 - -3 | int | abs }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "7");
}

TEST_CASE("multiplication with negative right operand pipe", "[pipe][operator_precedence]") {
  constexpr auto src = "{{ 2 * -3 | abs }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "6");
}

TEST_CASE("division with chained pipes on right operand", "[pipe][operator_precedence]") {
  constexpr auto src = "{{ 6 / -2 | int | abs }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "3");
}

TEST_CASE("complex arithmetic with pipes on operand", "[pipe][operator_precedence]") {
  constexpr auto src = "{{ 5 * -2 | abs }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "10");
}

TEST_CASE("pipe on left side of addition", "[pipe][operator_precedence]") {
  constexpr auto src = "{{ 5 | abs + 3 }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "8");
}

TEST_CASE("make_object variadic syntax", "[inja][make_object]") {
  SECTION("single pair") {
    auto obj = make_object("a", 1);
    REQUIRE(std::holds_alternative<inja_object>(obj.storage));
    auto const& map = std::get<inja_object>(obj.storage);
    REQUIRE(map.size() == 1);
    REQUIRE(as_int(map.at("a")) == 1);
  }

  SECTION("multiple pairs") {
    auto obj = make_object("a", 1, "b", "hello");
    auto const& map = std::get<inja_object>(obj.storage);
    REQUIRE(map.size() == 2);
    REQUIRE(as_int(map.at("a")) == 1);
    REQUIRE(as_string(map.at("b")) == "hello");
  }

  SECTION("nested objects") {
    auto obj = make_object("aaa", make_object("bbb", "ccc"));
    auto const& map = std::get<inja_object>(obj.storage);
    auto const& inner = as_object(map.at("aaa"));
    REQUIRE(as_string(inner.at("bbb")) == "ccc");
  }

  SECTION("object() helper variadic") {
    auto obj = object("key", "value");
    REQUIRE(as_string(as_object(obj).at("key")) == "value");
  }
}

TEST_CASE("make_array variadic syntax", "[inja][make_array]") {
  SECTION("single element") {
    auto arr = make_array(1);
    REQUIRE(std::holds_alternative<inja_array>(arr.storage));
    auto const& vec = std::get<std::vector<inja_value>>(std::get<inja_array>(arr.storage));
    REQUIRE(vec.size() == 1);
    REQUIRE(as_int(vec[0]) == 1);
  }

  SECTION("multiple elements") {
    auto arr = make_array(1, "hello", true);
    auto const& vec = std::get<std::vector<inja_value>>(std::get<inja_array>(arr.storage));
    REQUIRE(vec.size() == 3);
    REQUIRE(as_int(vec[0]) == 1);
    REQUIRE(as_string(vec[1]) == "hello");
    REQUIRE(as_bool(vec[2]) == true);
  }

  SECTION("nested arrays") {
    auto arr = make_array(make_array(1, 2), 3);
    auto const& vec = std::get<std::vector<inja_value>>(std::get<inja_array>(arr.storage));
    REQUIRE(vec.size() == 2);
    auto const& inner = std::get<std::vector<inja_value>>(as_array(vec[0]));
    REQUIRE(inner.size() == 2);
    REQUIRE(as_int(inner[0]) == 1);
    REQUIRE(as_int(inner[1]) == 2);
    REQUIRE(as_int(vec[1]) == 3);
  }

  SECTION("array() helper variadic") {
    auto arr = array(1, 2, 3);
    auto const& vec = std::get<std::vector<inja_value>>(std::get<inja_array>(arr.storage));
    REQUIRE(vec.size() == 3);
  }
}
