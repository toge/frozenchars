#include "catch2/catch_all.hpp"

#include <iterator>
#include <string>
#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/trie_index.hpp"
#include "frozenchars/trie_set.hpp"
#include "frozenchars/trie_map.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

/**
 * @brief frozen_trie_index / frozen_trie_set / frozen_trie_map のテスト。
 *   トライ木による完全一致検索、共通プレフィックス、深い分岐、単一キー、
 *   セットのメンバーシップ・イテレーション、マップの基本操作・イテレーションを検証する。
 */

TEST_CASE("frozen_trie_index finds exact keys", "[frozen_trie]") {
  using Idx = frozen_trie_index<"cat"_fs, "car"_fs, "dog"_fs, "door"_fs, "dove"_fs>;

  CHECK(Idx::find("cat") == 0);
  CHECK(Idx::find("car") == 1);
  CHECK(Idx::find("dog") == 2);
  CHECK(Idx::find("door") == 3);
  CHECK(Idx::find("dove") == 4);

  CHECK(Idx::find("c") == Idx::NPOS);
  CHECK(Idx::find("ca") == Idx::NPOS);
  CHECK(Idx::find("cats") == Idx::NPOS);
  CHECK(Idx::find("dove_") == Idx::NPOS);
  CHECK(Idx::find("zebra") == Idx::NPOS);
  CHECK(Idx::find("") == Idx::NPOS);
}

TEST_CASE("frozen_trie_index handles common prefix keys", "[frozen_trie]") {
  using Idx = frozen_trie_index<"timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>;

  CHECK(Idx::find("timeout") == 0);
  CHECK(Idx::find("timeout_ms") == 1);
  CHECK(Idx::find("timeout_us") == 2);
  CHECK(Idx::find("timeout_ns") == 3);

  CHECK(Idx::find("timeout_") == Idx::NPOS);
  CHECK(Idx::find("timeout_abc") == Idx::NPOS);
}

TEST_CASE("frozen_trie_index handles short unique keys", "[frozen_trie]") {
  using Idx = frozen_trie_index<"a"_fs, "b"_fs, "c"_fs>;

  CHECK(Idx::find("a") == 0);
  CHECK(Idx::find("b") == 1);
  CHECK(Idx::find("c") == 2);

  CHECK(Idx::find("ab") == Idx::NPOS);
  CHECK(Idx::find("") == Idx::NPOS);
}

TEST_CASE("frozen_trie_index handles key that is prefix of another", "[frozen_trie]") {
  using Idx = frozen_trie_index<"a"_fs, "ab"_fs, "abc"_fs>;

  CHECK(Idx::find("a") == 0);
  CHECK(Idx::find("ab") == 1);
  CHECK(Idx::find("abc") == 2);

  CHECK(Idx::find("abcd") == Idx::NPOS);
  CHECK(Idx::find("ac") == Idx::NPOS);
  CHECK(Idx::find("b") == Idx::NPOS);
}

TEST_CASE("frozen_trie_index handles deep branching", "[frozen_trie]") {
  using Idx = frozen_trie_index<
    "pre"_fs, "prefix"_fs, "preprocess"_fs, "preparation"_fs, "premium"_fs>;

  CHECK(Idx::find("pre") == 0);
  CHECK(Idx::find("prefix") == 1);
  CHECK(Idx::find("preprocess") == 2);
  CHECK(Idx::find("preparation") == 3);
  CHECK(Idx::find("premium") == 4);

  CHECK(Idx::find("prex") == Idx::NPOS);
  CHECK(Idx::find("prefi") == Idx::NPOS);
  CHECK(Idx::find("prepar") == Idx::NPOS);
}

TEST_CASE("frozen_trie_index handles more than 8 children (LUT path)", "[frozen_trie]") {
  // 子ノード数 > 8 で k_use_child_lut が有効になる経路の回帰テスト
  using Idx = frozen_trie_index<
    "alpha"_fs, "bravo"_fs, "charlie"_fs, "delta"_fs, "echo"_fs,
    "foxtrot"_fs, "golf"_fs, "hotel"_fs, "india"_fs, "juliet"_fs>;

  STATIC_CHECK(Idx::k_use_child_lut);
  CHECK(Idx::find("alpha") == 0);
  CHECK(Idx::find("charlie") == 2);
  CHECK(Idx::find("juliet") == 9);
  CHECK(Idx::find("alph") == Idx::NPOS);
  CHECK(Idx::find("zulu") == Idx::NPOS);
}

TEST_CASE("frozen_trie_index rejects short input in long label comparison", "[frozen_trie]") {
  using Idx = frozen_trie_index<"0123456789abcdefXYZ"_fs>;

  CHECK_FALSE(Idx::compare_label("x", 0, Idx::k_nodes[0].label_offset, Idx::k_nodes[0].label_length));
  CHECK(Idx::find("0123456789abcdefXYZ") == 0);
}

TEST_CASE("frozen_trie_index handles single key", "[frozen_trie]") {
  using Idx = frozen_trie_index<"onlykey"_fs>;

  CHECK(Idx::find("onlykey") == 0);
  CHECK(Idx::find("only") == Idx::NPOS);
  CHECK(Idx::find("onlykey_extra") == Idx::NPOS);
}

TEST_CASE("frozen_trie_set basic membership", "[frozen_trie]") {
  using Set = frozen_trie_set<"apple"_fs, "banana"_fs, "cherry"_fs>;

  CHECK(Set::contains("apple"));
  CHECK(Set::contains("banana"));
  CHECK(Set::contains("cherry"));
  CHECK_FALSE(Set::contains("apricot"));
  CHECK_FALSE(Set::contains("APPLE"));
  CHECK(Set::count("apple") == 1);
  CHECK(Set::count("apricot") == 0);

  CHECK(Set::find("banana") != Set::end());
  CHECK(Set::find("apricot") == Set::end());

  auto const k = Set::keys();
  CHECK(k.size() == 3);
  CHECK(k[0] == "apple");
  CHECK(k[1] == "banana");
  CHECK(k[2] == "cherry");
}

TEST_CASE("frozen_trie_map basic operations", "[frozen_trie]") {
  frozen_trie_map<int, "x"_fs, "y"_fs, "z"_fs> map{std::array<int, 3>{10, 20, 30}};

  CHECK(map["x"] == 10);
  CHECK(map["y"] == 20);
  CHECK(map["z"] == 30);
  CHECK_THROWS_AS(map["w"], std::out_of_range);

  CHECK(map.contains("x"));
  CHECK_FALSE(map.contains("w"));

  map["x"] = 42;
  CHECK(map["x"] == 42);
}

TEST_CASE("frozen_trie_map iteration", "[frozen_trie]") {
  frozen_trie_map<int, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
    std::array<int, 3>{30, 7, 2}
  };

  auto count = 0;
  for (auto&& [key, value] : map) {
    count++;
    CHECK_FALSE(key.empty());
  }
  CHECK(count == 3);
}

TEST_CASE("frozen_trie_map make_frozen_trie_map", "[frozen_trie]") {
  auto map = make_frozen_trie_map<int, "timeout"_fs, "retry"_fs>(
    std::pair{"timeout", 30},
    std::pair{"retry", 5}
  );

  CHECK(map["timeout"] == 30);
  CHECK(map["retry"] == 5);
  CHECK(map.size() == 2);
}

TEST_CASE("frozen_trie_set iteration", "[frozen_trie]") {
  using Set = frozen_trie_set<"alpha"_fs, "beta"_fs, "gamma"_fs>;

  std::array<std::string_view, 3> seen{};
  auto idx = 0uz;
  for (auto const& key : Set{}) {
    seen[idx++] = key;
  }
  CHECK(seen[0] == "alpha");
  CHECK(seen[1] == "beta");
  CHECK(seen[2] == "gamma");

  CHECK(Set::keys().size() == 3);
}

TEST_CASE("frozen_trie iterators model random_access_iterator", "[frozen_trie]") {
  using Map = frozen_trie_map<int, "x"_fs, "y"_fs, "z"_fs>;
  using Set = frozen_trie_set<"alpha"_fs, "beta"_fs, "gamma"_fs>;
  static_assert(std::random_access_iterator<Map::iterator>);
  static_assert(std::random_access_iterator<Map::const_iterator>);
  static_assert(std::random_access_iterator<Set::iterator>);
  auto map = Map{std::array<int, 3>{10, 20, 30}};
  CHECK(map.begin()[2].first == "z");
  CHECK(map.begin()[2].second == 30);
  CHECK(Set::begin()[1] == "beta");
  CHECK(Set::end()[-1] == "gamma");
}
