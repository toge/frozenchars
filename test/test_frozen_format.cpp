#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstring>
#include "frozenchars.hpp"

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
