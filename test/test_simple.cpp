#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("simple string") {
  auto constexpr star = "*"_fs;

  static_assert(star.sv() == "*");
  REQUIRE(star.sv() == "*");
}

TEST_CASE("simple repeat") {
  auto constexpr starline = repeat<10>("*"_fs);
  static_assert(starline.sv() == "**********");
  REQUIRE(starline.sv() == "**********");
}

TEST_CASE("simple concat") {
  auto constexpr hello   = "Hello, "_fs;
  auto constexpr world   = "world!"_fs;
  auto constexpr message = hello + world;

  static_assert(message.sv() == "Hello, world!");
  REQUIRE(message.sv() == "Hello, world!");
}

TEST_CASE("simple concat with literal rhs") {
  auto constexpr hello   = "Hello, "_fs;
  auto constexpr message = hello + "world!";

  static_assert(message.sv() == "Hello, world!");
  REQUIRE(message.sv() == "Hello, world!");
}

TEST_CASE("simple right align") {
  auto constexpr s1 = right<5>("abc"_fs);
  static_assert(s1.sv() == "  abc");
  REQUIRE(s1.sv() == "  abc");

  auto constexpr s2 = right<6, '.'>("abc"_fs);
  static_assert(s2.sv() == "...abc");
  REQUIRE(s2.sv() == "...abc");

  auto constexpr s3 = right<3>("abcdef"_fs);
  static_assert(s3.sv() == "abcdef");
  REQUIRE(s3.sv() == "abcdef");

  auto constexpr s4 = right<5>("abc"_fs);
  static_assert(s4.sv() == "  abc");
  REQUIRE(s4.sv() == "  abc");
}

TEST_CASE("simple center align") {
  auto constexpr s1 = center<7>("abc"_fs);
  static_assert(s1.sv() == "  abc  ");
  REQUIRE(s1.sv() == "  abc  ");

  auto constexpr s2 = center<8, '-'>("abc"_fs);
  static_assert(s2.sv() == "--abc---");
  REQUIRE(s2.sv() == "--abc---");

  auto constexpr s3 = center<3>("abcdef"_fs);
  static_assert(s3.sv() == "abcdef");
  REQUIRE(s3.sv() == "abcdef");

  auto constexpr s4 = center<7>("abc"_fs);
  static_assert(s4.sv() == "  abc  ");
  REQUIRE(s4.sv() == "  abc  ");
}

TEST_CASE("freeze with integers") {
  auto constexpr num42 = freeze(42);
  static_assert(num42.sv() == "42");
  REQUIRE(num42.sv() == "42");

  auto constexpr num0 = freeze(0);
  static_assert(num0.sv() == "0");
  REQUIRE(num0.sv() == "0");

  auto constexpr numneg = freeze(-123);
  static_assert(numneg.sv() == "-123");
  REQUIRE(numneg.sv() == "-123");
}

TEST_CASE("freeze with Hex") {
  auto constexpr hex255 = freeze(Hex(255));
  static_assert(hex255.sv() == "ff");
  REQUIRE(hex255.sv() == "ff");

  auto constexpr hex0 = freeze(Hex(0));
  static_assert(hex0.sv() == "0");
  REQUIRE(hex0.sv() == "0");
}

TEST_CASE("freeze with Bin") {
  auto constexpr bin255 = freeze(Bin(255));
  static_assert(bin255.sv() == "11111111");
  REQUIRE(bin255.sv() == "11111111");

  auto constexpr bin0 = freeze(Bin(0));
  static_assert(bin0.sv() == "0");
  REQUIRE(bin0.sv() == "0");
}

TEST_CASE("freeze with Oct") {
  auto constexpr oct255 = freeze(Oct(255));
  static_assert(oct255.sv() == "377");
  REQUIRE(oct255.sv() == "377");

  auto constexpr oct0 = freeze(Oct(0));
  static_assert(oct0.sv() == "0");
  REQUIRE(oct0.sv() == "0");
}

TEST_CASE("freeze with Precision") {
  auto constexpr pi_2 = freeze(Precision(3.14159265, 2));
  static_assert(pi_2.sv() == "3.14");
  REQUIRE(pi_2.sv() == "3.14");

  auto constexpr pi_4 = freeze(Precision(3.14159265, 4));
  static_assert(pi_4.sv() == "3.1415");
  REQUIRE(pi_4.sv() == "3.1415");

  auto constexpr frac = freeze(Precision(0.5, 1));
  static_assert(frac.sv() == "0.5");
  REQUIRE(frac.sv() == "0.5");
}

TEST_CASE("freeze with constexpr std::array<char, N>") {
  auto constexpr arr1 = std::array<char, 8>{'a', 'r', 'r', 'a', 'y', '\0', 'x', 'x'};
  auto constexpr s1   = freeze(arr1);
  static_assert(s1.sv() == "array");
  REQUIRE(s1.sv() == "array");

  auto constexpr arr2 = std::array<char, 4>{'a', 'b', 'c', 'd'};
  auto constexpr s2   = freeze(arr2);
  static_assert(s2.sv() == "abcd");
  REQUIRE(s2.sv() == "abcd");
}

TEST_CASE("freeze with constexpr std::span<char>") {
  auto constexpr buf1 = std::array<char, 8>{'s', 'p', 'a', 'n', '\0', 'x', 'x', 'x'};
  auto constexpr s1   = freeze(std::span<char const>{buf1});
  static_assert(s1.sv() == "span");
  REQUIRE(s1.sv() == "span");

  auto constexpr buf2 = std::array<char, 5>{'h', 'e', 'l', 'l', 'o'};
  auto constexpr s2   = freeze(std::span<char const>{buf2});
  static_assert(s2.sv() == "hello");
  REQUIRE(s2.sv() == "hello");
}

TEST_CASE("freeze with constexpr signed char buffers") {
  auto constexpr arr = std::array<signed char, 6>{'s', '8', '\0', 'x', 'x', 'x'};
  auto constexpr s1  = freeze(arr);
  static_assert(s1.sv() == "s8");
  REQUIRE(s1.sv() == "s8");

  auto constexpr s3 = freeze(std::span<signed char const>{arr});
  static_assert(s3.sv() == "s8");
  REQUIRE(s3.sv() == "s8");
}

TEST_CASE("freeze with constexpr unsigned char buffers") {
  auto constexpr arr = std::array<unsigned char, 6>{'u', '8', '\0', 'x', 'x', 'x'};
  auto constexpr s1  = freeze(arr);
  static_assert(s1.sv() == "u8");
  REQUIRE(s1.sv() == "u8");

  auto constexpr s3 = freeze(std::span<unsigned char const>{arr});
  static_assert(s3.sv() == "u8");
  REQUIRE(s3.sv() == "u8");
}

TEST_CASE("freeze with constexpr std::byte buffers") {
  auto const to_b = [](char c) { return std::byte{static_cast<unsigned char>(c)}; };

  auto constexpr arr = std::array<std::byte, 7>{to_b('b'), to_b('y'), to_b('t'), to_b('e'), std::byte{0}, to_b('x'), to_b('x')};
  auto constexpr s1  = freeze(arr);
  REQUIRE(s1.sv() == "byte");

  auto constexpr s3 = freeze(std::span<std::byte const>{arr});
  static_assert(s3.sv() == "byte");
  REQUIRE(s3.sv() == "byte");
}

TEST_CASE("toupper") {
  auto constexpr s1 = toupper("hello"_fs);
  static_assert(s1.sv() == "HELLO");
  REQUIRE(s1.sv() == "HELLO");

  auto constexpr s2 = toupper("Hello, World!");
  static_assert(s2.sv() == "HELLO, WORLD!");
  REQUIRE(s2.sv() == "HELLO, WORLD!");

  auto constexpr s3 = toupper("ALREADY"_fs);
  static_assert(s3.sv() == "ALREADY");
  REQUIRE(s3.sv() == "ALREADY");

  auto constexpr s4 = toupper("abc123xyz"_fs);
  static_assert(s4.sv() == "ABC123XYZ");
  REQUIRE(s4.sv() == "ABC123XYZ");
}

TEST_CASE("tolower") {
  auto constexpr s1 = tolower("HELLO"_fs);
  static_assert(s1.sv() == "hello");
  REQUIRE(s1.sv() == "hello");

  auto constexpr s2 = tolower("Hello, World!");
  static_assert(s2.sv() == "hello, world!");
  REQUIRE(s2.sv() == "hello, world!");

  auto constexpr s3 = tolower("already"_fs);
  static_assert(s3.sv() == "already");
  REQUIRE(s3.sv() == "already");

  auto constexpr s4 = tolower("ABC123XYZ"_fs);
  static_assert(s4.sv() == "abc123xyz");
  REQUIRE(s4.sv() == "abc123xyz");
}

TEST_CASE("substr") {
  auto constexpr s1 = substr<0, 5>("Hello, World!"_fs);
  static_assert(s1.sv() == "Hello");
  REQUIRE(s1.sv() == "Hello");

  auto constexpr s2 = substr<7, 5>("Hello, World!"_fs);
  static_assert(s2.sv() == "World");
  REQUIRE(s2.sv() == "World");

  auto constexpr s3 = substr<0, 5>("Hello, World!");
  static_assert(s3.sv() == "Hello");
  REQUIRE(s3.sv() == "Hello");

  // Pos beyond length returns empty
  auto constexpr s4 = substr<20, 5>("Hello"_fs);
  static_assert(s4.sv() == "");
  REQUIRE(s4.sv() == "");

  // Len extends beyond end: truncate to available
  auto constexpr s5 = substr<3, 10>("Hello"_fs);
  static_assert(s5.sv() == "lo");
  REQUIRE(s5.sv() == "lo");
}
