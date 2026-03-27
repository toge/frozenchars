#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

#include <tuple>

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("freeze with char pointers") {
  char const* cstr = "runtime";
  auto const  s1   = freeze(cstr);
  REQUIRE(s1.sv() == "runtime");

  char       mutable_buf[] = "mutable";
  char*      ptr           = mutable_buf;
  auto const s2            = freeze(ptr);
  REQUIRE(s2.sv() == "mutable");

  char const* nullp = nullptr;
  auto const  s3    = freeze(nullp);
  REQUIRE(s3.sv().empty());
}

TEST_CASE("trim helpers with char pointers") {
  char const* padded = "  runtime  ";
  auto const  s1     = trim(padded);
  REQUIRE(s1.sv() == "runtime");

  auto const s2 = ltrim(padded);
  REQUIRE(s2.sv() == "runtime  ");

  auto const s3 = rtrim(padded);
  REQUIRE(s3.sv() == "  runtime");

  char const* dashed = "---runtime---";
  auto const  s4     = trim<'-'>(dashed);
  REQUIRE(s4.sv() == "runtime");

  char const* nullp = nullptr;
  auto const  s5    = trim(nullp);
  REQUIRE(s5.sv().empty());
}

TEST_CASE("freeze truncates long char pointer") {
  auto big = std::array<char, 300>{};
  for (auto i = 0uz; i < big.size() - 1; ++i) {
    big[i] = 'a';
  }
  big.back() = '\0';

  auto const s = freeze(big.data());
  REQUIRE(s.sv().size() == 256);
  REQUIRE(s.sv() == std::string_view{big.data(), 256});
}

TEST_CASE("freeze with std::array<char, N>") {
  auto const arr1 = std::array<char, 8>{'a', 'r', 'r', 'a', 'y', '\0', 'x', 'x'};
  auto const s1   = freeze(arr1);
  REQUIRE(s1.sv() == "array");

  auto const arr2 = std::array<char, 4>{'a', 'b', 'c', 'd'};
  auto const s2   = freeze(arr2);
  REQUIRE(s2.sv() == "abcd");
}

TEST_CASE("freeze with std::span<char>") {
  auto       buf1 = std::array<char, 8>{'s', 'p', 'a', 'n', '\0', 'x', 'x', 'x'};
  auto const s1   = freeze(std::span<char>{buf1});
  REQUIRE(s1.sv() == "span");

  auto const buf2 = std::array<char, 5>{'h', 'e', 'l', 'l', 'o'};
  auto const s2   = freeze(std::span<char const>{buf2});
  REQUIRE(s2.sv() == "hello");
}

TEST_CASE("freeze with std::vector<char>") {
  auto       v1 = std::vector<char>{'v', 'e', 'c', '\0', 'x'};
  auto const s1 = freeze(v1);
  REQUIRE(s1.sv() == "vec");

  auto const v2 = std::vector<char>{'n', 'o', 'n', 'u', 'l', 'l'};
  auto const s2 = freeze(v2);
  REQUIRE(s2.sv() == "nonull");

  auto const empty = std::vector<char>{};
  auto const s3    = freeze(empty);
  REQUIRE(s3.sv().empty());
}

TEST_CASE("freeze truncates long std::vector<char>") {
  auto big   = std::vector<char>(300, 'b');
  big.back() = '\0';

  auto const s = freeze(big);
  REQUIRE(s.sv().size() == 256);
  REQUIRE(s.sv() == std::string_view{big.data(), 256});
}

TEST_CASE("freeze with signed char buffers") {
  auto const arr = std::array<signed char, 6>{'s', '8', '\0', 'x', 'x', 'x'};
  auto const s1  = freeze(arr);
  REQUIRE(s1.sv() == "s8");

  auto       vec = std::vector<signed char>{'n', 'e', 'g'};
  auto const s2  = freeze(vec);
  REQUIRE(s2.sv() == "neg");

  auto const s3 = freeze(std::span<signed char const>{arr});
  REQUIRE(s3.sv() == "s8");
}

TEST_CASE("freeze truncates long std::vector<signed char>") {
  auto big   = std::vector<signed char>(300, static_cast<signed char>('q'));
  big.back() = static_cast<signed char>(0);

  auto const s = freeze(big);
  REQUIRE(s.sv().size() == 256);
  REQUIRE(std::all_of(s.sv().begin(), s.sv().end(), [](char c) { return c == 'q'; }));
}

TEST_CASE("freeze with unsigned char buffers") {
  auto const arr = std::array<unsigned char, 6>{'u', '8', '\0', 'x', 'x', 'x'};
  auto const s1  = freeze(arr);
  REQUIRE(s1.sv() == "u8");

  auto       vec = std::vector<unsigned char>{'v', 'e', 'c'};
  auto const s2  = freeze(vec);
  REQUIRE(s2.sv() == "vec");

  auto const s3 = freeze(std::span<unsigned char const>{arr});
  REQUIRE(s3.sv() == "u8");
}

TEST_CASE("freeze with std::byte buffers") {
  auto const to_b = [](char c) { return std::byte{static_cast<unsigned char>(c)}; };

  auto const arr = std::array<std::byte, 7>{to_b('b'), to_b('y'), to_b('t'), to_b('e'), std::byte{0}, to_b('x'), to_b('x')};
  auto const s1  = freeze(arr);
  REQUIRE(s1.sv() == "byte");

  auto       vec = std::vector<std::byte>{to_b('d'), to_b('a'), to_b('t'), to_b('a')};
  auto const s2  = freeze(vec);
  REQUIRE(s2.sv() == "data");

  auto const s3 = freeze(std::span<std::byte const>{arr});
  REQUIRE(s3.sv() == "byte");
}

TEST_CASE("freeze truncates long std::vector<std::byte>") {
  auto big   = std::vector<std::byte>(300, std::byte{static_cast<unsigned char>('z')});
  big.back() = std::byte{0};

  auto const s = freeze(big);
  REQUIRE(s.sv().size() == 256);
  REQUIRE(std::all_of(s.sv().begin(), s.sv().end(), [](char c) { return c == 'z'; }));
}

TEST_CASE("parse_hex_rgb parses runtime values") {
  auto const rgb = parse_hex_rgb("#335577");
  REQUIRE(std::get<0>(rgb) == 0x33);
  REQUIRE(std::get<1>(rgb) == 0x55);
  REQUIRE(std::get<2>(rgb) == 0x77);

  auto const short_rgb = parse_hex_rgb("#532");
  REQUIRE(std::get<0>(short_rgb) == 0x55);
  REQUIRE(std::get<1>(short_rgb) == 0x33);
  REQUIRE(std::get<2>(short_rgb) == 0x22);
}

TEST_CASE("parse_hex_rgb rejects invalid length at runtime") {
  REQUIRE_THROWS_AS(parse_hex_rgb("#33557"), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_hex_rgb("335577"), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_hex_rgb("#33557cc"), std::invalid_argument);
}

TEST_CASE("parse_hex_rgb rejects invalid digits at runtime") {
  REQUIRE_THROWS_AS(parse_hex_rgb("#33GG77"), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_hex_rgb("#5g2"), std::invalid_argument);
}

TEST_CASE("parse_hex_rgba parses runtime values") {
  auto const rgba = parse_hex_rgba("#335577cc");
  REQUIRE(std::get<0>(rgba) == 0x33);
  REQUIRE(std::get<1>(rgba) == 0x55);
  REQUIRE(std::get<2>(rgba) == 0x77);
  REQUIRE(std::get<3>(rgba) == 0xcc);

  auto const short_rgba = parse_hex_rgba("#5a3c");
  REQUIRE(std::get<0>(short_rgba) == 0x55);
  REQUIRE(std::get<1>(short_rgba) == 0xaa);
  REQUIRE(std::get<2>(short_rgba) == 0x33);
  REQUIRE(std::get<3>(short_rgba) == 0xcc);
}

TEST_CASE("parse_hex_rgba rejects invalid length at runtime") {
  REQUIRE_THROWS_AS(parse_hex_rgba("#335577"), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_hex_rgba("#335577c"), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_hex_rgba("335577cc"), std::invalid_argument);
}

TEST_CASE("parse_hex_rgba rejects invalid digits at runtime") {
  REQUIRE_THROWS_AS(parse_hex_rgba("#33GG77cc"), std::invalid_argument);
  REQUIRE_THROWS_AS(parse_hex_rgba("#5a3z"), std::invalid_argument);
}

TEST_CASE("tuple channel reorder helpers reorder runtime tuples") {
  auto const bgr = to_bgr(parse_hex_rgb("#123456"));
  REQUIRE(std::get<0>(bgr) == 0x56);
  REQUIRE(std::get<1>(bgr) == 0x34);
  REQUIRE(std::get<2>(bgr) == 0x12);

  auto const bgra = to_bgra(parse_hex_rgba("#12345678"));
  REQUIRE(std::get<0>(bgra) == 0x56);
  REQUIRE(std::get<1>(bgra) == 0x34);
  REQUIRE(std::get<2>(bgra) == 0x12);
  REQUIRE(std::get<3>(bgra) == 0x78);

  auto const abgr = to_abgr(parse_hex_rgba("#12345678"));
  REQUIRE(std::get<0>(abgr) == 0x78);
  REQUIRE(std::get<1>(abgr) == 0x56);
  REQUIRE(std::get<2>(abgr) == 0x34);
  REQUIRE(std::get<3>(abgr) == 0x12);
}
