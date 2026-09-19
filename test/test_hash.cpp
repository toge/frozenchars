#include "catch2/catch_all.hpp"
#include "frozenchars/hash.hpp"
#include "frozenchars/literals.hpp"

#include <cstdint>
#include <string_view>

/** @brief FNV-1a ハッシュ、hash_v、_hash リテラル、hash_combine のテスト。 */

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("fnv1a hashing") {
  SECTION("compile-time equals run-time") {
    static_assert(fnv1a("hello") == fnv1a(std::string_view{"hello"}));
    static_assert(fnv1a("hello") != fnv1a("world"));
    REQUIRE(fnv1a("hello") == fnv1a(std::string_view{"hello"}));
  }

  SECTION("known 64-bit value") {
    static_assert(fnv1a("hello") == 0xa430d84680aabd0buz);
    REQUIRE(fnv1a("hello") == 0xa430d84680aabd0buz);
  }

  SECTION("known 32-bit value") {
    static_assert(fnv1a<std::uint32_t>("hello") == 0x4f9f2cabu);
    REQUIRE(fnv1a<std::uint32_t>("hello") == 0x4f9f2cabu);
  }

  SECTION("FrozenString overload") {
    auto constexpr input = "hello"_fs;
    static_assert(fnv1a(input) == fnv1a("hello"));
    REQUIRE(fnv1a(input) == fnv1a("hello"));
  }
}

TEST_CASE("hash_v and _hash literal") {
  SECTION("hash_v") {
    static_assert(hash_v<"hello"> == fnv1a("hello"));
    static_assert(hash_v<"hello"> != hash_v<"world">);
    REQUIRE(hash_v<"hello"> == fnv1a("hello"));
  }

  SECTION("_hash literal") {
    static_assert("hello"_hash == fnv1a("hello"));
    static_assert("hello"_hash != "world"_hash);
    REQUIRE("hello"_hash == fnv1a("hello"));
  }
}

TEST_CASE("string switch dispatch") {
  auto constexpr status_for = [](std::string_view const method) constexpr {
    switch (fnv1a(method)) {
    case "GET"_hash: return 200;
    case "POST"_hash: return 201;
    case "DELETE"_hash: return 204;
    default: return 405;
    }
  };

  static_assert(status_for("GET") == 200);
  static_assert(status_for("POST") == 201);
  static_assert(status_for("DELETE") == 204);
  static_assert(status_for("BREW") == 405);
  REQUIRE(status_for("GET") == 200);
  REQUIRE(status_for("BREW") == 405);
}

TEST_CASE("hash_combine") {
  static_assert(hash_combine(1uz, 2uz) != 1uz);
  static_assert(hash_combine(1uz, 2uz) == hash_combine(1uz, 2uz));

  // 可変長版は左から順に畳み込む
  static_assert(hash_combine(1uz, 2uz, 3uz) == hash_combine(hash_combine(1uz, 2uz), 3uz));
  REQUIRE(hash_combine(1uz, 2uz, 3uz) == hash_combine(hash_combine(1uz, 2uz), 3uz));
}

TEST_CASE("constexpr_hash functor") {
  auto constexpr hasher = constexpr_hash<std::string_view>{};
  static_assert(hasher(std::string_view{"hello"}) == fnv1a("hello"));
  REQUIRE(hasher(std::string_view{"hello"}) == fnv1a("hello"));
}
