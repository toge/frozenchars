#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("complex string") {
  // repeat<回数>(FrozenString) で結合
  auto constexpr banner = "!"_ss + repeat<5>("=-"_ss) + "!";

  static_assert(banner.sv() == "!=-=-=-=-=-!");
  REQUIRE(banner.sv() == "!=-=-=-=-=-!");
}

TEST_CASE("complex string2") {
  // repeat<回数>(FrozenString) で結合
  auto constexpr banner = "!" + repeat<5>("=-"_ss) + "!";

  static_assert(banner.sv() == "!=-=-=-=-=-!");
  REQUIRE(banner.sv() == "!=-=-=-=-=-!");
}

TEST_CASE("complex string3") {
  // repeat<回数>(FrozenString) で結合
  auto constexpr banner = "!" + repeat<5>("=-") + "!";

  static_assert(banner.sv() == "!=-=-=-=-=-!");
  REQUIRE(banner.sv() == "!=-=-=-=-=-!");
}

TEST_CASE("concat with mixed types") {
  auto constexpr mixed = concat("The answer is: ", 42);
  static_assert(mixed.sv() == "The answer is: 42");
  REQUIRE(mixed.sv() == "The answer is: 42");

  auto constexpr hex_msg = concat("0x", Hex(255), " in decimal");
  static_assert(hex_msg.sv() == "0xff in decimal");
  REQUIRE(hex_msg.sv() == "0xff in decimal");

  auto constexpr bin_msg = concat("bin=", Bin(10));
  static_assert(bin_msg.sv() == "bin=1010");
  REQUIRE(bin_msg.sv() == "bin=1010");

  auto constexpr oct_msg = concat("oct=", Oct(8));
  static_assert(oct_msg.sv() == "oct=10");
  REQUIRE(oct_msg.sv() == "oct=10");

  auto constexpr pi_msg = concat("Pi approx: ", Precision(3.14159265, 2));
  static_assert(pi_msg.sv() == "Pi approx: 3.14");
  REQUIRE(pi_msg.sv() == "Pi approx: 3.14");
}

TEST_CASE("concat named constexpr strings") {
  auto constexpr hello = "Hello, "_ss;
  auto constexpr world = "world!"_ss;
  auto constexpr message = concat(hello, world);

  static_assert(message.sv() == "Hello, world!");
  REQUIRE(message.sv() == "Hello, world!");
}
