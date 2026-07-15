#include "catch2/catch_all.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <ranges>

#include "frozenchars/literals.hpp"
#include "frozenchars/set.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;
using namespace std::string_literals;

/**
 * @brief frozen_set の基本機能（contains / count / find / イテレータ / キー取得）のテスト。
 *   単一キー・同長キー・長キー（>16 バイト）・65 キー以上の線形探索フォールバックを網羅する。
 */

TEST_CASE("frozen_set basic shape", "[frozen_set]") {
  static_assert(frozen_set<"timeout"_fs, "retry"_fs>::size() == 2);
  static_assert(!frozen_set<"timeout"_fs, "retry"_fs>::empty());
  REQUIRE(frozen_set<"timeout"_fs, "retry"_fs>{}.contains("timeout"));
  REQUIRE(frozen_set<"timeout"_fs, "retry"_fs>{}.contains("retry"));
  REQUIRE_FALSE(frozen_set<"timeout"_fs, "retry"_fs>{}.contains("other"));
}

TEST_CASE("frozen_set contains", "[frozen_set]") {
  using Set = frozen_set<"alpha"_fs, "beta"_fs, "gamma"_fs>;
  REQUIRE(Set{}.contains("alpha"));
  REQUIRE(Set{}.contains("beta"));
  REQUIRE(Set{}.contains("gamma"));
  REQUIRE_FALSE(Set{}.contains("delta"));
  REQUIRE_FALSE(Set{}.contains(""));
  REQUIRE_FALSE(Set{}.contains("alphabet"));
}

TEST_CASE("frozen_set count", "[frozen_set]") {
  using Set = frozen_set<"x"_fs, "y"_fs, "z"_fs>;
  REQUIRE(Set{}.count("x") == 1uz);
  REQUIRE(Set{}.count("y") == 1uz);
  REQUIRE(Set{}.count("w") == 0uz);
}

TEST_CASE("frozen_set find", "[frozen_set]") {
  using Set = frozen_set<"a"_fs, "b"_fs, "c"_fs>;
  auto const s = Set{};
  REQUIRE(s.find("a") == s.begin());
  REQUIRE(s.find("b") != s.end());
  REQUIRE(s.find("b") - s.begin() == 1);
  REQUIRE(s.find("z") == s.end());
}

TEST_CASE("frozen_set iterator traversal", "[frozen_set]") {
  using Set = frozen_set<"x"_fs, "y"_fs, "z"_fs>;
  auto const s = Set{};
  auto count = 0uz;
  for ([[maybe_unused]] auto const& k : s) {
    ++count;
  }
  REQUIRE(count == 3uz);
  REQUIRE(std::distance(s.begin(), s.end()) == 3);
}

TEST_CASE("frozen_set iterator random access", "[frozen_set]") {
  using Set = frozen_set<"a"_fs, "b"_fs, "c"_fs>;
  auto const s = Set{};
  auto const v0 = s.begin()[0];
  auto const v1 = s.begin()[1];
  auto const v2 = s.begin()[2];
  REQUIRE(v0.size() == 1);
  REQUIRE(v1.size() == 1);
  REQUIRE(v2.size() == 1);
  REQUIRE(s.begin() + 3 == s.end());
  REQUIRE(s.end() - s.begin() == 3);
  REQUIRE(s.begin() < s.end());
}

TEST_CASE("frozen_set keys sorted", "[frozen_set]") {
  using Set = frozen_set<"z"_fs, "a"_fs, "m"_fs>;
  auto const keys = Set::keys();
  REQUIRE(keys.size() == 3);
  REQUIRE(keys[0] == "a");
  REQUIRE(keys[1] == "m");
  REQUIRE(keys[2] == "z");
}

TEST_CASE("frozen_set keys_in_declaration_order", "[frozen_set]") {
  using Set = frozen_set<"z"_fs, "a"_fs, "m"_fs>;
  auto const keys = Set::keys_in_declaration_order();
  REQUIRE(keys.size() == 3);
  REQUIRE(keys[0] == "z");
  REQUIRE(keys[1] == "a");
  REQUIRE(keys[2] == "m");
}

TEST_CASE("frozen_set map_type alias", "[frozen_set]") {
  using Set = frozen_set<"x"_fs, "y"_fs>;
  using Map = Set::map_type<int>;
  static_assert(Map::size() == 2);
  Map map{std::array<int, 2>{10, 20}};
  REQUIRE(map.at("x") == 10);
  REQUIRE(map.at("y") == 20);
}

TEST_CASE("frozen_set to_frozen_map", "[frozen_set]") {
  using Set = frozen_set<"key1"_fs, "key2"_fs>;
  auto map = to_frozen_map<std::string>(Set{}, std::array{"val1"s, "val2"s});
  REQUIRE(map.at("key1") == "val1");
  REQUIRE(map.at("key2") == "val2");
}

TEST_CASE("frozen_set single key", "[frozen_set]") {
  using Set = frozen_set<"only"_fs>;
  REQUIRE(Set{}.contains("only"));
  REQUIRE_FALSE(Set{}.contains("other"));
  REQUIRE(Set{}.find("only") == Set{}.begin());
  REQUIRE(Set{}.find("other") == Set{}.end());
}

TEST_CASE("frozen_set same-length keys", "[frozen_set][no_hash]") {
  using Set = frozen_set<"abc"_fs, "def"_fs, "ghi"_fs>;
  REQUIRE(Set{}.contains("abc"));
  REQUIRE(Set{}.contains("def"));
  REQUIRE(Set{}.contains("ghi"));
  REQUIRE_FALSE(Set{}.contains("xyz"));
}

TEST_CASE("frozen_set long keys (>16 bytes)", "[frozen_set][long_key]") {
  using Set = frozen_set<
    "this-is-a-long-key-01"_fs,
    "this-is-a-long-key-02"_fs,
    "this-is-a-long-key-03"_fs
  >;
  REQUIRE(Set{}.contains("this-is-a-long-key-01"));
  REQUIRE(Set{}.contains("this-is-a-long-key-02"));
  REQUIRE(Set{}.contains("this-is-a-long-key-03"));
  REQUIRE_FALSE(Set{}.contains("this-is-a-long-key-99"));
}

TEST_CASE("frozen_set linear search fallback with 65 keys", "[frozen_set][linear]") {
  using Set = frozen_set<
    "k00"_fs, "k01"_fs, "k02"_fs, "k03"_fs, "k04"_fs,
    "k05"_fs, "k06"_fs, "k07"_fs, "k08"_fs, "k09"_fs,
    "k10"_fs, "k11"_fs, "k12"_fs, "k13"_fs, "k14"_fs,
    "k15"_fs, "k16"_fs, "k17"_fs, "k18"_fs, "k19"_fs,
    "k20"_fs, "k21"_fs, "k22"_fs, "k23"_fs, "k24"_fs,
    "k25"_fs, "k26"_fs, "k27"_fs, "k28"_fs, "k29"_fs,
    "k30"_fs, "k31"_fs, "k32"_fs, "k33"_fs, "k34"_fs,
    "k35"_fs, "k36"_fs, "k37"_fs, "k38"_fs, "k39"_fs,
    "k40"_fs, "k41"_fs, "k42"_fs, "k43"_fs, "k44"_fs,
    "k45"_fs, "k46"_fs, "k47"_fs, "k48"_fs, "k49"_fs,
    "k50"_fs, "k51"_fs, "k52"_fs, "k53"_fs, "k54"_fs,
    "k55"_fs, "k56"_fs, "k57"_fs, "k58"_fs, "k59"_fs,
    "k60"_fs, "k61"_fs, "k62"_fs, "k63"_fs, "k64"_fs
  >;
  REQUIRE(Set{}.contains("k00"));
  REQUIRE(Set{}.contains("k32"));
  REQUIRE(Set{}.contains("k64"));
  REQUIRE_FALSE(Set{}.contains("k99"));
}
