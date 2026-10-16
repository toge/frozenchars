#include "catch2/catch_all.hpp"
#include "frozenchars/ops.hpp"

/** @brief Base64 エンコード/デコードのテスト。 */

using namespace frozenchars;
using namespace frozenchars::literals;
namespace fops = frozenchars::ops;

TEST_CASE("Base64 encode") {
  SECTION("basic") {
    auto constexpr input = "Hello"_fs;
    auto constexpr encoded = base64_encode(input);
    static_assert(encoded.sv() == "SGVsbG8=");
    REQUIRE(encoded.sv() == "SGVsbG8=");
  }

  SECTION("padding 0") {
    auto constexpr input = "any carnal pleasure"_fs;
    auto constexpr encoded = base64_encode(input);
    static_assert(encoded.sv() == "YW55IGNhcm5hbCBwbGVhc3VyZQ==");
    REQUIRE(encoded.sv() == "YW55IGNhcm5hbCBwbGVhc3VyZQ==");
  }

  SECTION("padding 1") {
    auto constexpr input = "any carnal pleasur"_fs;
    auto constexpr encoded = base64_encode(input);
    static_assert(encoded.sv() == "YW55IGNhcm5hbCBwbGVhc3Vy");
    REQUIRE(encoded.sv() == "YW55IGNhcm5hbCBwbGVhc3Vy");
  }

  SECTION("NTTP exact size") {
    auto constexpr encoded = base64_encode<"f"_fs>();
    static_assert(encoded.size() == 4);
    static_assert(encoded.sv() == "Zg==");
    REQUIRE(encoded.sv() == "Zg==");
  }
}

TEST_CASE("Base64 decode") {
  SECTION("basic") {
    auto constexpr input = "SGVsbG8="_fs;
    auto constexpr decoded = base64_decode(input);
    static_assert(decoded.sv() == "Hello");
    REQUIRE(decoded.sv() == "Hello");
  }

  SECTION("with padding") {
    auto constexpr input = "YW55IGNhcm5hbCBwbGVhc3VyZQ=="_fs;
    auto constexpr decoded = base64_decode(input);
    static_assert(decoded.sv() == "any carnal pleasure");
    REQUIRE(decoded.sv() == "any carnal pleasure");
  }

  SECTION("NTTP exact size") {
    auto constexpr decoded = base64_decode<"Zg=="_fs>();
    static_assert(decoded.size() == 1);
    static_assert(decoded.sv() == "f");
    REQUIRE(decoded.sv() == "f");
  }

  SECTION("NTTP unpadded tail of 2 chars") {
    auto constexpr decoded = base64_decode<"Zg"_fs>();
    static_assert(decoded.sv() == "f");
    REQUIRE(decoded.sv() == "f");
  }

  SECTION("NTTP tail of 3 chars with single padding") {
    auto constexpr decoded = base64_decode<"Zg="_fs>();
    static_assert(decoded.sv() == "f");
    REQUIRE(decoded.sv() == "f");
  }

  SECTION("NTTP tail of 3 chars without padding") {
    auto constexpr decoded = base64_decode<"Zm8"_fs>();
    static_assert(decoded.sv() == "fo");
    REQUIRE(decoded.sv() == "fo");
  }

  SECTION("NTTP length not multiple of 4") {
    auto constexpr decoded = base64_decode<"QUJDRA"_fs>();
    static_assert(decoded.size() == 4);
    static_assert(decoded.sv() == "ABCD");
    REQUIRE(decoded.sv() == "ABCD");
  }

  SECTION("NTTP length not multiple of 4 with single padding") {
    auto constexpr decoded = base64_decode<"QUJDRA=="_fs>();
    static_assert(decoded.sv() == "ABCD");
    REQUIRE(decoded.sv() == "ABCD");
  }

  SECTION("NTTP leftover single char is ignored") {
    auto constexpr decoded = base64_decode<"QUJDR"_fs>();
    static_assert(decoded.sv() == "ABC");
    REQUIRE(decoded.sv() == "ABC");
  }
}

TEST_CASE("Base64 pipe adaptors") {
  auto constexpr res = "Hello World!"_fs | fops::base64_encode | fops::base64_decode;
  static_assert(res.sv() == "Hello World!");
  REQUIRE(res.sv() == "Hello World!");
}
