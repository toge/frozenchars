#include "frozenchars/inja_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

TEST_CASE("value_view: holds string_view without allocation", "[inja_types]") {
  std::string source = "hello world";
  frozenchars::inja::value_view v{std::string_view{source}};
  REQUIRE(std::holds_alternative<std::string_view>(v.storage));
  REQUIRE(std::get<std::string_view>(v.storage) == "hello world");
  REQUIRE(std::get<std::string_view>(v.storage).data() == source.data());
}

TEST_CASE("temp_value: stores owned string for function returns", "[inja_types]") {
  frozenchars::inja::temp_value v{std::string{"ALPHA"}};
  REQUIRE(std::holds_alternative<std::string>(v.storage));
  REQUIRE(std::get<std::string>(v.storage) == "ALPHA");
}

TEST_CASE("temp_value: variant types", "[inja_types]") {
  frozenchars::inja::temp_value a{std::int64_t{42}};
  frozenchars::inja::temp_value b{double{3.14}};
  frozenchars::inja::temp_value c{true};
  REQUIRE(std::get<std::int64_t>(a.storage) == 42);
  REQUIRE(std::get<double>(b.storage) == 3.14);
  REQUIRE(std::get<bool>(c.storage) == true);
}
