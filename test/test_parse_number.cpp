#include "catch2/catch_all.hpp"

#include <expected>
#include <limits>
#include <system_error>

#include "frozenchars/split.hpp"
#include "frozenchars/string_ops.hpp"

/** @brief parse_number による整数・浮動小数点数のパースと境界値・エラーケースのテスト。 */

using namespace frozenchars;
using namespace frozenchars::literals;

static_assert(*parse_number<int>("42"_fs) == 42);
static_assert(*parse_number<int>("-100"_fs) == -100);
static_assert(*parse_number<int>("0xff"_fs) == 255);
static_assert(*parse_number<int>("0b1010"_fs) == 10);
static_assert(*parse_number<int>("077"_fs) == 63);
static_assert(*parse_number<int>("-0xff"_fs) == -255);
static_assert(*parse_number<int>("+077"_fs) == 63);
static_assert(*parse_number<int>("-0b10"_fs) == -2);
static_assert(*parse_number<double>("3.14"_fs) == 3.14);
static_assert(*parse_number<float>("-1.5"_fs) == -1.5f);
static_assert(*parse_number<double>("1e2"_fs) == 100.0);
static_assert(*parse_number<double>("-2.5e1"_fs) == -25.0);
static_assert(!parse_number<int>("abc"_fs).has_value());
static_assert(parse_number<int>("abc"_fs).error() == std::errc::invalid_argument);

TEST_CASE("parse_number handles integer boundaries", "[parse_number]") {
  auto constexpr int_max = "2147483647"_fs;
  auto constexpr int_min = "-2147483648"_fs;
  auto constexpr long_long_max = "9223372036854775807"_fs;
  auto constexpr long_long_min = "-9223372036854775808"_fs;
  auto constexpr ulong_long_max = "18446744073709551615"_fs;

  REQUIRE(*parse_number<int>(int_max) == std::numeric_limits<int>::max());
  REQUIRE(*parse_number<int>(int_min) == std::numeric_limits<int>::min());
  REQUIRE(*parse_number<long long>(long_long_max) == std::numeric_limits<long long>::max());
  REQUIRE(*parse_number<long long>(long_long_min) == std::numeric_limits<long long>::min());
  REQUIRE(*parse_number<unsigned long long>(ulong_long_max)
          == std::numeric_limits<unsigned long long>::max());
}

TEST_CASE("parse_number reports runtime errors via expected", "[parse_number]") {
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

  auto const check_invalid = [](auto const& v) {
    REQUIRE_FALSE(v.has_value());
    REQUIRE(v.error() == std::errc::invalid_argument);
  };
  auto const check_range = [](auto const& v) {
    REQUIRE_FALSE(v.has_value());
    REQUIRE(v.error() == std::errc::result_out_of_range);
  };

  check_invalid(parse_number<int>(alpha));
  check_invalid(parse_number<int>(missing_hex));
  check_invalid(parse_number<int>(invalid_binary));
  check_invalid(parse_number<int>(invalid_octal));
  check_invalid(parse_number<unsigned>(leading_space));
  check_invalid(parse_number<unsigned>(negative_unsigned));
  check_invalid(parse_number<float>(malformed_float));
  check_invalid(parse_number<double>(invalid_exponent));
  check_range(parse_number<double>(float_overflow));
  check_range(parse_number<int>(int_overflow));
}
