#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstring>
#include "frozenchars.hpp"

/** @brief frozen_format によるコンパイル時書式整形のテスト */

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("frozen_format basic {} replacement", "[frozen_format]") {
  auto r1 = frozen_format<"value={}"_fs>(42);
  REQUIRE(strcmp(r1.sv().data(), "value=42") == 0);

  auto r2 = frozen_format<"hello {}!"_fs>("world");
  REQUIRE(strcmp(r2.sv().data(), "hello world!") == 0);

  auto r3 = frozen_format<"{}+{}={}"_fs>(1, 2, 3);
  REQUIRE(strcmp(r3.sv().data(), "1+2=3") == 0);

  auto r4 = frozen_format<"{{hello}}"_fs>();
  REQUIRE(strcmp(r4.sv().data(), "{hello}") == 0);

  auto r5 = frozen_format<"no placeholders"_fs>();
  REQUIRE(strcmp(r5.sv().data(), "no placeholders") == 0);
}

TEST_CASE("frozen_format negative numbers", "[frozen_format]") {
  auto r = frozen_format<"{}"_fs>(-42);
  REQUIRE(strcmp(r.sv().data(), "-42") == 0);
}

TEST_CASE("frozen_format float", "[frozen_format]") {
  auto r = frozen_format<"{:.2f}"_fs>(3.14159);
  REQUIRE(strcmp(r.sv().data(), "3.14") == 0);
}

TEST_CASE("frozen_format bool", "[frozen_format]") {
  auto r1 = frozen_format<"{}"_fs>(true);
  REQUIRE(strcmp(r1.sv().data(), "true") == 0);
  auto r2 = frozen_format<"{}"_fs>(false);
  REQUIRE(strcmp(r2.sv().data(), "false") == 0);
}

TEST_CASE("frozen_format char", "[frozen_format]") {
  auto r = frozen_format<"{}"_fs>('A');
  REQUIRE(strcmp(r.sv().data(), "A") == 0);
}

TEST_CASE("frozen_format integer types", "[frozen_format]") {
  SECTION("decimal") {
    auto r = frozen_format<"{:d}"_fs>(255);
    REQUIRE(strcmp(r.sv().data(), "255") == 0);
  }
  SECTION("hex lowercase") {
    auto r = frozen_format<"{:x}"_fs>(255);
    REQUIRE(strcmp(r.sv().data(), "ff") == 0);
  }
  SECTION("hex uppercase") {
    auto r = frozen_format<"{:X}"_fs>(255);
    REQUIRE(strcmp(r.sv().data(), "FF") == 0);
  }
  SECTION("octal") {
    auto r = frozen_format<"{:o}"_fs>(255);
    REQUIRE(strcmp(r.sv().data(), "377") == 0);
  }
  SECTION("binary") {
    auto r = frozen_format<"{:b}"_fs>(255);
    REQUIRE(strcmp(r.sv().data(), "11111111") == 0);
  }
  SECTION("zero") {
    auto r = frozen_format<"{:d}"_fs>(0);
    REQUIRE(strcmp(r.sv().data(), "0") == 0);
  }
}

TEST_CASE("frozen_format alignment", "[frozen_format]") {
  SECTION("right align (default)") {
    auto r = frozen_format<"{:10}"_fs>(42);
    REQUIRE(strcmp(r.sv().data(), "        42") == 0);
  }
  SECTION("left align") {
    auto r = frozen_format<"{:<10}"_fs>(42);
    REQUIRE(strcmp(r.sv().data(), "42        ") == 0);
  }
  SECTION("center") {
    auto r = frozen_format<"{:^10}"_fs>(42);
    REQUIRE(strcmp(r.sv().data(), "    42    ") == 0);
  }
  SECTION("custom fill") {
    auto r = frozen_format<"{:*<10}"_fs>(42);
    REQUIRE(strcmp(r.sv().data(), "42********") == 0);
  }
}

TEST_CASE("frozen_format sign", "[frozen_format]") {
  SECTION("always sign") {
    auto r1 = frozen_format<"{:+}"_fs>(42);
    REQUIRE(strcmp(r1.sv().data(), "+42") == 0);
    auto r2 = frozen_format<"{:+}"_fs>(-42);
    REQUIRE(strcmp(r2.sv().data(), "-42") == 0);
  }
  SECTION("space for positive") {
    auto r = frozen_format<"{: }"_fs>(42);
    REQUIRE(strcmp(r.sv().data(), " 42") == 0);
  }
}

TEST_CASE("frozen_format zero-pad", "[frozen_format]") {
  SECTION("basic zero pad") {
    auto r = frozen_format<"{:010}"_fs>(42);
    REQUIRE(strcmp(r.sv().data(), "0000000042") == 0);
  }
  SECTION("zero pad hex") {
    auto r = frozen_format<"{:08x}"_fs>(255);
    REQUIRE(strcmp(r.sv().data(), "000000ff") == 0);
  }
}

TEST_CASE("frozen_format float types", "[frozen_format]") {
  SECTION("fixed default precision") {
    auto r = frozen_format<"{:f}"_fs>(3.14);
    REQUIRE(strcmp(r.sv().data(), "3.140000") == 0);
  }
  SECTION("fixed with precision") {
    auto r = frozen_format<"{:.2f}"_fs>(3.14159);
    REQUIRE(strcmp(r.sv().data(), "3.14") == 0);
  }
}

TEST_CASE("frozen_format edge cases", "[frozen_format]") {
  SECTION("empty string arg") {
    auto r = frozen_format<"[{}]"_fs>("");
    REQUIRE(strcmp(r.sv().data(), "[]") == 0);
  }
  SECTION("long literal") {
    auto r = frozen_format<"Hello, {}! Welcome to {}."_fs>("world", "frozenchars");
    REQUIRE(strcmp(r.sv().data(), "Hello, world! Welcome to frozenchars.") == 0);
  }
}

TEST_CASE("frozen_format Capacity", "[frozen_format]") {
  SECTION("default capacity 4096") {
    auto r = frozen_format<"value={}"_fs>(42);
    REQUIRE(r.length == 8);
    REQUIRE(strcmp(r.sv().data(), "value=42") == 0);
  }
  SECTION("large capacity exceeds 4096") {
    // string_view リテラルはコンパイル時評価可能なので、consteval な frozen_format に渡せる
    constexpr std::string_view big = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx";
    auto r = frozen_format<"{}"_fs, 9000>(big);
    REQUIRE(r.length == 5000);
    REQUIRE(r.sv().size() == 5000);
  }
  SECTION("small capacity truncates mid-string") {
    // Capacity=5 の場合、"big hello" は途中で切り捨てられ "big " (4文字) になる
    auto r = frozen_format<"big {}"_fs, 5>("hello");
    REQUIRE(r.length == 4);
    REQUIRE(strcmp(r.sv().data(), "big ") == 0);
  }
  SECTION("small capacity truncates numeric field") {
    // "{:08X}" と 0xABCD は "0000ABCD" (8文字)。Capacity=4 で "000" (3文字) に切り捨て
    auto r = frozen_format<"{:08X}"_fs, 4>(0xABCD);
    REQUIRE(r.length == 3);
    REQUIRE(strcmp(r.sv().data(), "000") == 0);
  }
  SECTION("small capacity truncates literal") {
    // リテラル "abcdef" は 6文字。Capacity=3 で "ab" (2文字) に切り捨て
    auto r = frozen_format<"abcdef"_fs, 3>();
    REQUIRE(r.length == 2);
    REQUIRE(strcmp(r.sv().data(), "ab") == 0);
  }
}
