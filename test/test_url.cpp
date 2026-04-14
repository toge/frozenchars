#include "catch2/catch_all.hpp"
#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;
namespace fops = frozenchars::ops;

TEST_CASE("URL encode") {
  SECTION("unreserved characters") {
    auto constexpr input = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~"_fs;
    auto constexpr encoded = url_encode(input);
    static_assert(encoded.sv() == "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~");
    REQUIRE(encoded.sv() == "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-._~");
  }

  SECTION("special characters") {
    auto constexpr input = "Hello World!"_fs;
    auto constexpr encoded = url_encode(input);
    static_assert(encoded.sv() == "Hello%20World%21");
    REQUIRE(encoded.sv() == "Hello%20World%21");
  }

  SECTION("NTTP exact size") {
    auto constexpr encoded = url_encode<"a b c"_fs>();
    static_assert(encoded.size() == 9);
    static_assert(encoded.sv() == "a%20b%20c");
    REQUIRE(encoded.sv() == "a%20b%20c");
  }
}

TEST_CASE("URL decode") {
  SECTION("basic") {
    auto constexpr input = "Hello%20World%21"_fs;
    auto constexpr decoded = url_decode(input);
    static_assert(decoded.sv() == "Hello World!");
    REQUIRE(decoded.sv() == "Hello World!");
  }

  SECTION("plus to space") {
    auto constexpr input = "a+b+c"_fs;
    auto constexpr decoded = url_decode(input);
    static_assert(decoded.sv() == "a b c");
    REQUIRE(decoded.sv() == "a b c");
  }

  SECTION("NTTP exact size") {
    auto constexpr decoded = url_decode<"a%20b%20c"_fs>();
    static_assert(decoded.size() == 5);
    static_assert(decoded.sv() == "a b c");
    REQUIRE(decoded.sv() == "a b c");
  }
}

TEST_CASE("URL pipe adaptors") {
  auto constexpr res = "Hello World!"_fs | fops::url_encode | fops::url_decode;
  static_assert(res.sv() == "Hello World!");
  REQUIRE(res.sv() == "Hello World!");
}
