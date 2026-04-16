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

  SECTION("reserved characters") {
    auto constexpr input = ":/?#[]@!$&'()*+,;="_fs;
    auto constexpr encoded = url_encode(input);
    static_assert(encoded.sv() == "%3A%2F%3F%23%5B%5D%40%21%24%26%27%28%29%2A%2B%2C%3B%3D");
    REQUIRE(encoded.sv() == "%3A%2F%3F%23%5B%5D%40%21%24%26%27%28%29%2A%2B%2C%3B%3D");
  }

  SECTION("UTF-8 characters") {
    auto constexpr input = "こんにちは"_fs;
    auto constexpr encoded = url_encode(input);
    static_assert(encoded.sv() == "%E3%81%93%E3%82%93%E3%81%AB%E3%81%A1%E3%81%AF");
    REQUIRE(encoded.sv() == "%E3%81%93%E3%82%93%E3%81%AB%E3%81%A1%E3%81%AF");
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

  SECTION("mixed case hex") {
    auto constexpr input = "%3a%2F%3f%23"_fs;
    auto constexpr decoded = url_decode(input);
    static_assert(decoded.sv() == ":/?#");
    REQUIRE(decoded.sv() == ":/?#");
  }

  SECTION("invalid percent sequences") {
    auto constexpr input = "%G1%1G%1%%"_fs;
    auto constexpr decoded = url_decode(input);
    static_assert(decoded.sv() == "%G1%1G%1%%");
    REQUIRE(decoded.sv() == "%G1%1G%1%%");
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

TEST_CASE("URL Query String") {
  auto constexpr query_string = make_querystring(
    "key1", "value1",
    "key2", 1,
    "key3", 3.54
  );
  static_assert(query_string.sv() == "?key1=value1&key2=1&key3=3.54");
  REQUIRE(query_string.sv() == "?key1=value1&key2=1&key3=3.54");
}

TEST_CASE("URL Query String Tuples") {
  auto constexpr query_string = make_querystring(
    std::pair{"key1", "value1"},
    std::pair{"key2", "value2"},
    std::pair{"key3", "value3"}
  );
  static_assert(query_string.sv() == "?key1=value1&key2=value2&key3=value3");
  REQUIRE(query_string.sv() == "?key1=value1&key2=value2&key3=value3");
}

TEST_CASE("URL Query String with URL encoded") {
  auto constexpr query_string = make_querystring(
    "key1", "a b:c",
    "key2", "value2",
    "key3", "value3"
  );
  static_assert(query_string.sv() == "?key1=a%20b%3Ac&key2=value2&key3=value3");
  REQUIRE(query_string.sv() == "?key1=a%20b%3Ac&key2=value2&key3=value3");
}

