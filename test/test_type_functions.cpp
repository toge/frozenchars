#include "catch2/catch_all.hpp"
#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("fn_int - string to int", "[type_conversion][int]") {
  constexpr auto src = "{{ int(\"42\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "42");
}

TEST_CASE("fn_int - float to int truncate", "[type_conversion][int]") {
  constexpr auto src = "{{ int(3.14) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("fn_int - bool true to int", "[type_conversion][int]") {
  constexpr auto src = "{{ int(true) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "1");
}

TEST_CASE("fn_int - bool false to int", "[type_conversion][int]") {
  constexpr auto src = "{{ int(false) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "0");
}

TEST_CASE("fn_int - null to int", "[type_conversion][int]") {
  constexpr auto src = "{{ int(null) }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE(render_template<src>(ctx) == "0");
}

TEST_CASE("fn_int - array length", "[type_conversion][int]") {
  constexpr auto src = "{{ int(arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  REQUIRE(render_template<src>(ctx) == "3");
}

TEST_CASE("fn_int - empty array to int", "[type_conversion][int]") {
  constexpr auto src = "{{ int(arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({})},
  });
  REQUIRE(render_template<src>(ctx) == "0");
}

TEST_CASE("fn_int - invalid string error", "[type_conversion][int][error]") {
  constexpr auto src = "{{ int(\"not a number\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("fn_int - object error", "[type_conversion][int][error]") {
  constexpr auto src = "{{ int(obj) }}"_fs;
  auto const ctx = make_template_object({
    {"obj", make_template_object({{"key", "value"}})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("fn_float - string to float", "[type_conversion][float]") {
  constexpr auto src = "{{ float(\"3.14\") }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result.find("3.1") != std::string::npos) || (result.find("3.14") != std::string::npos);
  REQUIRE(valid);
}

TEST_CASE("fn_float - int to float", "[type_conversion][float]") {
  constexpr auto src = "{{ float(42) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  REQUIRE(result.find("42") != std::string::npos);
}

TEST_CASE("fn_float - bool true to float", "[type_conversion][float]") {
  constexpr auto src = "{{ float(true) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  REQUIRE(result.find("1") != std::string::npos);
}

TEST_CASE("fn_float - bool false to float", "[type_conversion][float]") {
  constexpr auto src = "{{ float(false) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  REQUIRE(result.find("0") != std::string::npos);
}

TEST_CASE("fn_float - null to float", "[type_conversion][float]") {
  constexpr auto src = "{{ float(null) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  REQUIRE(result.find("0") != std::string::npos);
}

TEST_CASE("fn_float - array length", "[type_conversion][float]") {
  constexpr auto src = "{{ float(arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2})},
  });
  auto const result = render_template<src>(ctx);
  REQUIRE(result.find("2") != std::string::npos);
}

TEST_CASE("fn_float - empty array to float", "[type_conversion][float]") {
  constexpr auto src = "{{ float(arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({})},
  });
  auto const result = render_template<src>(ctx);
  REQUIRE(result.find("0") != std::string::npos);
}

TEST_CASE("fn_float - invalid string error", "[type_conversion][float][error]") {
  constexpr auto src = "{{ float(\"not a number\") }}"_fs;
  auto const ctx = make_template_object({});
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("fn_float - object error", "[type_conversion][float][error]") {
  constexpr auto src = "{{ float(obj) }}"_fs;
  auto const ctx = make_template_object({
    {"obj", make_template_object({{"key", "value"}})},
  });
  REQUIRE_THROWS_AS(render_template<src>(ctx), template_render_error);
}

TEST_CASE("fn_isString - string literal", "[type_check][isString]") {
  constexpr auto src = "{{ isString(\"hello\") }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isString - int", "[type_check][isString]") {
  constexpr auto src = "{{ isString(42) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isString - null", "[type_check][isString]") {
  constexpr auto src = "{{ isString(null) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isArray - array from context", "[type_check][isArray]") {
  constexpr auto src = "{{ isArray(arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isArray - empty array", "[type_check][isArray]") {
  constexpr auto src = "{{ isArray(arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({})},
  });
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isArray - string", "[type_check][isArray]") {
  constexpr auto src = "{{ isArray(\"string\") }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isNumber - int", "[type_check][isNumber]") {
  constexpr auto src = "{{ isNumber(42) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isNumber - float", "[type_check][isNumber]") {
  constexpr auto src = "{{ isNumber(3.14) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isNumber - string", "[type_check][isNumber]") {
  constexpr auto src = "{{ isNumber(\"42\") }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isObject - object from context", "[type_check][isObject]") {
  constexpr auto src = "{{ isObject(obj) }}"_fs;
  auto const ctx = make_template_object({
    {"obj", make_template_object({{"key", "value"}})},
  });
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isObject - array from context", "[type_check][isObject]") {
  constexpr auto src = "{{ isObject(arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({1, 2, 3})},
  });
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isBoolean - true", "[type_check][isBoolean]") {
  constexpr auto src = "{{ isBoolean(true) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isBoolean - false", "[type_check][isBoolean]") {
  constexpr auto src = "{{ isBoolean(false) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isBoolean - int", "[type_check][isBoolean]") {
  constexpr auto src = "{{ isBoolean(1) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isFloat - float literal", "[type_check][isFloat]") {
  constexpr auto src = "{{ isFloat(3.14) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isFloat - int", "[type_check][isFloat]") {
  constexpr auto src = "{{ isFloat(42) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isFloat - string", "[type_check][isFloat]") {
  constexpr auto src = "{{ isFloat(\"3.14\") }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isInteger - int literal", "[type_check][isInteger]") {
  constexpr auto src = "{{ isInteger(42) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isInteger - float", "[type_check][isInteger]") {
  constexpr auto src = "{{ isInteger(3.14) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isInteger - string", "[type_check][isInteger]") {
  constexpr auto src = "{{ isInteger(\"42\") }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isNone - null", "[type_check][isNone]") {
  constexpr auto src = "{{ isNone(null) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isNone - zero", "[type_check][isNone]") {
  constexpr auto src = "{{ isNone(0) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isNone - empty string", "[type_check][isNone]") {
  constexpr auto src = "{{ isNone(\"\") }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isEmpty - empty string", "[type_check][isEmpty]") {
  constexpr auto src = "{{ isEmpty(\"\") }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isEmpty - empty array", "[type_check][isEmpty]") {
  constexpr auto src = "{{ isEmpty(arr) }}"_fs;
  auto const ctx = make_template_object({
    {"arr", make_template_array({})},
  });
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isEmpty - empty object", "[type_check][isEmpty]") {
  constexpr auto src = "{{ isEmpty(obj) }}"_fs;
  auto const ctx = make_template_object({
    {"obj", make_template_object({})},
  });
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isEmpty - null", "[type_check][isEmpty]") {
  constexpr auto src = "{{ isEmpty(null) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "true") || (result == "1");
  REQUIRE(valid);
}

TEST_CASE("fn_isEmpty - zero", "[type_check][isEmpty]") {
  constexpr auto src = "{{ isEmpty(0) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}

TEST_CASE("fn_isEmpty - false", "[type_check][isEmpty]") {
  constexpr auto src = "{{ isEmpty(false) }}"_fs;
  auto const ctx = make_template_object({});
  auto const result = render_template<src>(ctx);
  bool valid = (result == "false") || (result == "0");
  REQUIRE(valid);
}
