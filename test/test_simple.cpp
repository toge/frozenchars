#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

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
  auto constexpr hello   = "Hello, "_ss;
  auto constexpr world   = "world!"_ss;
  auto constexpr message = hello + world;

  static_assert(message.sv() == "Hello, world!");
  REQUIRE(message.sv() == "Hello, world!");
}

TEST_CASE("simple concat with literal rhs") {
  auto constexpr hello   = "Hello, "_ss;
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

TEST_CASE("make_static with constexpr std::array<char, N>") {
  auto constexpr arr1 = std::array<char, 8>{'a', 'r', 'r', 'a', 'y', '\0', 'x', 'x'};
  auto constexpr s1   = make_static(arr1);
  static_assert(s1.sv() == "array");
  REQUIRE(s1.sv() == "array");

  auto constexpr arr2 = std::array<char, 4>{'a', 'b', 'c', 'd'};
  auto constexpr s2   = make_static(arr2);
  static_assert(s2.sv() == "abcd");
  REQUIRE(s2.sv() == "abcd");
}

TEST_CASE("make_static with constexpr std::span<char>") {
  auto constexpr buf1 = std::array<char, 8>{'s', 'p', 'a', 'n', '\0', 'x', 'x', 'x'};
  auto constexpr s1   = make_static(std::span<char const>{buf1});
  static_assert(s1.sv() == "span");
  REQUIRE(s1.sv() == "span");

  auto constexpr buf2 = std::array<char, 5>{'h', 'e', 'l', 'l', 'o'};
  auto constexpr s2   = make_static(std::span<char const>{buf2});
  static_assert(s2.sv() == "hello");
  REQUIRE(s2.sv() == "hello");
}

TEST_CASE("make_static with constexpr signed char buffers") {
  auto constexpr arr = std::array<signed char, 6>{'s', '8', '\0', 'x', 'x', 'x'};
  auto constexpr s1  = make_static(arr);
  static_assert(s1.sv() == "s8");
  REQUIRE(s1.sv() == "s8");

  auto constexpr s3 = make_static(std::span<signed char const>{arr});
  static_assert(s3.sv() == "s8");
  REQUIRE(s3.sv() == "s8");
}

TEST_CASE("make_static with constexpr unsigned char buffers") {
  auto constexpr arr = std::array<unsigned char, 6>{'u', '8', '\0', 'x', 'x', 'x'};
  auto constexpr s1  = make_static(arr);
  static_assert(s1.sv() == "u8");
  REQUIRE(s1.sv() == "u8");

  auto constexpr s3 = make_static(std::span<unsigned char const>{arr});
  static_assert(s3.sv() == "u8");
  REQUIRE(s3.sv() == "u8");
}

TEST_CASE("make_static with constexpr std::byte buffers") {
  auto const to_b = [](char c) { return std::byte{static_cast<unsigned char>(c)}; };

  auto constexpr arr = std::array<std::byte, 7>{to_b('b'), to_b('y'), to_b('t'), to_b('e'), std::byte{0}, to_b('x'), to_b('x')};
  auto constexpr s1  = make_static(arr);
  REQUIRE(s1.sv() == "byte");

  auto constexpr s3 = make_static(std::span<std::byte const>{arr});
  static_assert(s3.sv() == "byte");
  REQUIRE(s3.sv() == "byte");
}
