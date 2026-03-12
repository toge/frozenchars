#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("make_static with char pointers") {
  char const* cstr = "runtime";
  auto const  s1   = make_static(cstr);
  REQUIRE(s1.sv() == "runtime");

  char       mutable_buf[] = "mutable";
  char*      ptr           = mutable_buf;
  auto const s2            = make_static(ptr);
  REQUIRE(s2.sv() == "mutable");

  char const* nullp = nullptr;
  auto const  s3    = make_static(nullp);
  REQUIRE(s3.sv().empty());
}

TEST_CASE("make_static truncates long char pointer") {
  auto big = std::array<char, 300>{};
  for (auto i = 0uz; i < big.size() - 1; ++i) {
    big[i] = 'a';
  }
  big.back() = '\0';

  auto const s = make_static(big.data());
  REQUIRE(s.sv().size() == 256);
  REQUIRE(s.sv() == std::string_view{big.data(), 256});
}

TEST_CASE("make_static with std::array<char, N>") {
  auto const arr1 = std::array<char, 8>{'a', 'r', 'r', 'a', 'y', '\0', 'x', 'x'};
  auto const s1   = make_static(arr1);
  REQUIRE(s1.sv() == "array");

  auto const arr2 = std::array<char, 4>{'a', 'b', 'c', 'd'};
  auto const s2   = make_static(arr2);
  REQUIRE(s2.sv() == "abcd");
}

TEST_CASE("make_static with std::span<char>") {
  auto       buf1 = std::array<char, 8>{'s', 'p', 'a', 'n', '\0', 'x', 'x', 'x'};
  auto const s1   = make_static(std::span<char>{buf1});
  REQUIRE(s1.sv() == "span");

  auto const buf2 = std::array<char, 5>{'h', 'e', 'l', 'l', 'o'};
  auto const s2   = make_static(std::span<char const>{buf2});
  REQUIRE(s2.sv() == "hello");
}

TEST_CASE("make_static with std::vector<char>") {
  auto       v1 = std::vector<char>{'v', 'e', 'c', '\0', 'x'};
  auto const s1 = make_static(v1);
  REQUIRE(s1.sv() == "vec");

  auto const v2 = std::vector<char>{'n', 'o', 'n', 'u', 'l', 'l'};
  auto const s2 = make_static(v2);
  REQUIRE(s2.sv() == "nonull");

  auto const empty = std::vector<char>{};
  auto const s3    = make_static(empty);
  REQUIRE(s3.sv().empty());
}

TEST_CASE("make_static truncates long std::vector<char>") {
  auto big   = std::vector<char>(300, 'b');
  big.back() = '\0';

  auto const s = make_static(big);
  REQUIRE(s.sv().size() == 256);
  REQUIRE(s.sv() == std::string_view{big.data(), 256});
}

TEST_CASE("make_static with signed char buffers") {
  auto const arr = std::array<signed char, 6>{'s', '8', '\0', 'x', 'x', 'x'};
  auto const s1  = make_static(arr);
  REQUIRE(s1.sv() == "s8");

  auto       vec = std::vector<signed char>{'n', 'e', 'g'};
  auto const s2  = make_static(vec);
  REQUIRE(s2.sv() == "neg");

  auto const s3 = make_static(std::span<signed char const>{arr});
  REQUIRE(s3.sv() == "s8");
}

TEST_CASE("make_static truncates long std::vector<signed char>") {
  auto big   = std::vector<signed char>(300, static_cast<signed char>('q'));
  big.back() = static_cast<signed char>(0);

  auto const s = make_static(big);
  REQUIRE(s.sv().size() == 256);
  REQUIRE(std::all_of(s.sv().begin(), s.sv().end(), [](char c) { return c == 'q'; }));
}

TEST_CASE("make_static with unsigned char buffers") {
  auto const arr = std::array<unsigned char, 6>{'u', '8', '\0', 'x', 'x', 'x'};
  auto const s1  = make_static(arr);
  REQUIRE(s1.sv() == "u8");

  auto       vec = std::vector<unsigned char>{'v', 'e', 'c'};
  auto const s2  = make_static(vec);
  REQUIRE(s2.sv() == "vec");

  auto const s3 = make_static(std::span<unsigned char const>{arr});
  REQUIRE(s3.sv() == "u8");
}

TEST_CASE("make_static with std::byte buffers") {
  auto const to_b = [](char c) { return std::byte{static_cast<unsigned char>(c)}; };

  auto const arr = std::array<std::byte, 7>{to_b('b'), to_b('y'), to_b('t'), to_b('e'), std::byte{0}, to_b('x'), to_b('x')};
  auto const s1  = make_static(arr);
  REQUIRE(s1.sv() == "byte");

  auto       vec = std::vector<std::byte>{to_b('d'), to_b('a'), to_b('t'), to_b('a')};
  auto const s2  = make_static(vec);
  REQUIRE(s2.sv() == "data");

  auto const s3 = make_static(std::span<std::byte const>{arr});
  REQUIRE(s3.sv() == "byte");
}

TEST_CASE("make_static truncates long std::vector<std::byte>") {
  auto big   = std::vector<std::byte>(300, std::byte{static_cast<unsigned char>('z')});
  big.back() = std::byte{0};

  auto const s = make_static(big);
  REQUIRE(s.sv().size() == 256);
  REQUIRE(std::all_of(s.sv().begin(), s.sv().end(), [](char c) { return c == 'z'; }));
}
