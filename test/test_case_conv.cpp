#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

/** @brief ケース変換（キャピタライズ、スネークケース、キャメルケース、パスカルケース）のテスト。 */

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("case_conv: capitalize") {
  STATIC_CHECK(capitalize("hello").sv() == "Hello");
  STATIC_CHECK(capitalize("HELLO").sv() == "Hello");
  STATIC_CHECK(capitalize("hELLO").sv() == "Hello");
  STATIC_CHECK(capitalize("h").sv() == "H");
  STATIC_CHECK(capitalize("").sv() == "");
  STATIC_CHECK(capitalize("123abc").sv() == "123abc");

  constexpr auto str = "world"_fs;
  STATIC_CHECK(capitalize(str).sv() == "World");
}

TEST_CASE("case_conv: to_snake_case - char[] version") {
  STATIC_CHECK(to_snake_case("helloWorld").sv() == "hello_world");
  STATIC_CHECK(to_snake_case("HelloWorld").sv() == "hello_world");
  STATIC_CHECK(to_snake_case("hello").sv() == "hello");
  STATIC_CHECK(to_snake_case("ABC").sv() == "a_b_c");
  STATIC_CHECK(to_snake_case("").sv() == "");
  STATIC_CHECK(to_snake_case("aB").sv() == "a_b");
}

TEST_CASE("case_conv: to_snake_case - FrozenString version") {
  constexpr auto str = "camelCaseName"_fs;
  STATIC_CHECK(to_snake_case(str).sv() == "camel_case_name");
}

TEST_CASE("case_conv: to_snake_case - NTTP version") {
  STATIC_CHECK(to_snake_case<"helloWorld"_fs>().sv() == "hello_world");
  STATIC_CHECK(to_snake_case<"XmlHttpRequest"_fs>().sv() == "xml_http_request");
  STATIC_CHECK(to_snake_case<"plain"_fs>().sv() == "plain");
}

TEST_CASE("case_conv: to_camel_case") {
  STATIC_CHECK(to_camel_case("hello_world").sv() == "helloWorld");
  STATIC_CHECK(to_camel_case("a_b_c").sv() == "aBC");
  STATIC_CHECK(to_camel_case("hello").sv() == "hello");
  STATIC_CHECK(to_camel_case("").sv() == "");
  STATIC_CHECK(to_camel_case("_leading").sv() == "Leading");

  constexpr auto str = "snake_case_value"_fs;
  STATIC_CHECK(to_camel_case(str).sv() == "snakeCaseValue");
}

TEST_CASE("case_conv: to_pascal_case") {
  STATIC_CHECK(to_pascal_case("hello_world").sv() == "HelloWorld");
  STATIC_CHECK(to_pascal_case("a_b_c").sv() == "ABC");
  STATIC_CHECK(to_pascal_case("hello").sv() == "Hello");
  STATIC_CHECK(to_pascal_case("").sv() == "");

  constexpr auto str = "snake_case_value"_fs;
  STATIC_CHECK(to_pascal_case(str).sv() == "SnakeCaseValue");
}

TEST_CASE("case_conv: round-trip") {
  STATIC_CHECK(to_snake_case(to_camel_case("hello_world")).sv() == "hello_world");
  STATIC_CHECK(to_camel_case(to_snake_case("helloWorld")).sv() == "helloWorld");
}
