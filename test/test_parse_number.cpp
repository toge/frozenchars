#include "catch2/catch_all.hpp"

#include <limits>

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

static_assert(parse_number<int>("42"_fs) == 42);
static_assert(parse_number<int>("-100"_fs) == -100);
static_assert(parse_number<int>("0xff"_fs) == 255);
static_assert(parse_number<int>("0b1010"_fs) == 10);
static_assert(parse_number<int>("077"_fs) == 63);
static_assert(parse_number<int>("-0xff"_fs) == -255);
static_assert(parse_number<int>("+077"_fs) == 63);
static_assert(parse_number<int>("-0b10"_fs) == -2);
static_assert(parse_number<double>("3.14"_fs) == 3.14);
static_assert(parse_number<float>("-1.5"_fs) == -1.5f);
static_assert(parse_number<double>("1e2"_fs) == 100.0);
static_assert(parse_number<double>("-2.5e1"_fs) == -25.0);

TEST_CASE("parse_number handles integer boundaries", "[parse_number]") {
  auto constexpr int_max = "2147483647"_fs;
  auto constexpr int_min = "-2147483648"_fs;
  auto constexpr long_long_max = "9223372036854775807"_fs;
  auto constexpr ulong_long_max = "18446744073709551615"_fs;

  REQUIRE(parse_number<int>(int_max) == std::numeric_limits<int>::max());
  REQUIRE(parse_number<int>(int_min) == std::numeric_limits<int>::min());
  REQUIRE(parse_number<long long>(long_long_max) == std::numeric_limits<long long>::max());
  REQUIRE(parse_number<unsigned long long>(ulong_long_max)
          == std::numeric_limits<unsigned long long>::max());
}

TEST_CASE("parse_number throws runtime exceptions", "[parse_number]") {
  auto constexpr alpha = "abc"_fs;
  auto constexpr missing_hex = "0x"_fs;
  auto constexpr invalid_binary = "0b102"_fs;
  auto constexpr invalid_octal = "08"_fs;
  auto constexpr leading_space = " -1"_fs;
  auto constexpr negative_unsigned = "-1"_fs;
  auto constexpr malformed_float = "1.0.0"_fs;
  auto constexpr invalid_exponent = "1e"_fs;
  auto constexpr float_overflow = "1e999"_fs;
  auto constexpr int_overflow = "2147483648"_fs;

  REQUIRE_THROWS_AS(parse_number<int>(alpha), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_number<int>(missing_hex), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_number<int>(invalid_binary), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_number<int>(invalid_octal), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_number<unsigned>(leading_space), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_number<unsigned>(negative_unsigned), std::out_of_range);
  REQUIRE_THROWS_AS(parse_number<float>(malformed_float), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_number<double>(invalid_exponent), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_number<double>(float_overflow), std::out_of_range);
  REQUIRE_THROWS_AS(parse_number<int>(int_overflow), std::out_of_range);
}
