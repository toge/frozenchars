#include "frozenchars/inja_access.hpp"
#include <glaze/glaze.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

struct simple_struct {
  std::string name;
  std::int64_t value;
};
template <> struct glz::meta<simple_struct> {
  using T = simple_struct;
  static constexpr auto value = object("name", &T::name, "value", &T::value);
};

TEST_CASE("accessor: direct string field", "[inja_access]") {
  simple_struct s{.name = "alpha", .value = 1};
  auto const& v = frozenchars::inja::accessor<simple_struct, "name">::resolve(s);
  REQUIRE(v == "alpha");
}

TEST_CASE("accessor: direct int field", "[inja_access]") {
  simple_struct s{.name = "alpha", .value = 42};
  auto const& v = frozenchars::inja::accessor<simple_struct, "value">::resolve(s);
  REQUIRE(v == 42);
}
