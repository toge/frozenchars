#include "catch2/catch_all.hpp"
#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::inja;
using namespace frozenchars::literals;

TEST_CASE("specialized arrays - range", "[specialized_arrays]") {
  constexpr auto src = "{{ join(range(5), \", \") }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "0, 1, 2, 3, 4");
}

TEST_CASE("specialized arrays - sort int", "[specialized_arrays]") {
  constexpr auto src = "{{ join(sort(as_int_array(items)), \"-\") }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({3, 1, 4, 2})},
  });
  REQUIRE(render<src>(ctx) == "1-2-3-4");
}

TEST_CASE("specialized arrays - sort double", "[specialized_arrays]") {
  constexpr auto src = "{{ join(sort(as_double_array(items)), \"/\") }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({3.5, 1.2, 4.8, 2.1})},
  });
  // Note: join rendering of double might have many trailing zeros if not handled
  // But fn_join uses std::to_string(double) currently.
  auto const out = render<src>(ctx);
  REQUIRE(out.find("1.2") != std::string::npos);
  REQUIRE(out.find("2.1") != std::string::npos);
  REQUIRE(out.find("3.5") != std::string::npos);
  REQUIRE(out.find("4.8") != std::string::npos);
}

TEST_CASE("specialized_arrays - sort string", "[specialized_arrays]") {
  constexpr auto src = "{{ join(sort(as_string_array(items)), \" \") }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({"banana", "apple", "cherry"})},
  });
  REQUIRE(render<src>(ctx) == "apple banana cherry");
}

TEST_CASE("specialized arrays - for loop specialized", "[specialized_arrays]") {
  constexpr auto src = "{% for i in range(3) %}{{ i }}{% endfor %}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src>(ctx) == "012");
}

TEST_CASE("specialized arrays - nested specialized", "[specialized_arrays]") {
  // Test if first() on specialized array works
  constexpr auto src = "{{ first(sort(as_int_array(items))) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({10, 5, 20})},
  });
  REQUIRE(render<src>(ctx) == "5");
}

TEST_CASE("specialized arrays - type error", "[specialized_arrays]") {
  constexpr auto src = "{{ as_int_array(items) }}"_fs;
  auto const ctx = make_object({
    {"items", make_array({"not an int"})},
  });
  REQUIRE_THROWS_AS(render<src>(ctx), render_error);
}
