#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

TEST_CASE("simple string") {
    auto constexpr star = "*"_ss;

    static_assert(star.sv() == "*");
    REQUIRE(star.sv() == "*");
}

TEST_CASE("simple repeat") {
    auto constexpr starline = repeat<10>("*"_ss);
    static_assert(starline.sv() == "**********");
    REQUIRE(starline.sv() == "**********");
}

TEST_CASE("simple concat") {
    auto constexpr hello = "Hello, "_ss;
    auto constexpr world = "world!"_ss;
    auto constexpr message = hello + world;

    static_assert(message.sv() == "Hello, world!");
    REQUIRE(message.sv() == "Hello, world!");
}

TEST_CASE("simple concat with literal rhs") {
    auto constexpr hello = "Hello, "_ss;
    auto constexpr message = hello + "world!";

    static_assert(message.sv() == "Hello, world!");
    REQUIRE(message.sv() == "Hello, world!");
}

TEST_CASE("make_static with integers") {
  auto constexpr num42 = make_static(42);
  static_assert(num42.sv() == "42");
  REQUIRE(num42.sv() == "42");

  auto constexpr num0 = make_static(0);
  static_assert(num0.sv() == "0");
  REQUIRE(num0.sv() == "0");

  auto constexpr numneg = make_static(-123);
  static_assert(numneg.sv() == "-123");
  REQUIRE(numneg.sv() == "-123");
}

TEST_CASE("make_static with Hex") {
  auto constexpr hex255 = make_static(Hex(255));
  static_assert(hex255.sv() == "0xff");
  REQUIRE(hex255.sv() == "0xff");

  auto constexpr hex0 = make_static(Hex(0));
  static_assert(hex0.sv() == "0x0");
  REQUIRE(hex0.sv() == "0x0");
}

TEST_CASE("make_static with Precision") {
  auto constexpr pi_2 = make_static(Precision(3.14159265, 2));
  static_assert(pi_2.sv() == "3.14");
  REQUIRE(pi_2.sv() == "3.14");

  auto constexpr pi_4 = make_static(Precision(3.14159265, 4));
  static_assert(pi_4.sv() == "3.1415");
  REQUIRE(pi_4.sv() == "3.1415");

  auto constexpr frac = make_static(Precision(0.5, 1));
  static_assert(frac.sv() == "0.5");
  REQUIRE(frac.sv() == "0.5");
}
