/// @file test_smoke_concepts_freeze_literals.cpp
/// @brief 6 ヘッダ (concepts / freeze / literals / number_conv / trie_set / glaze_frozen_map) の最小スモークテスト

#include "catch2/catch_all.hpp"

#include "frozenchars/concepts.hpp"
#include "frozenchars/freeze.hpp"
#include "frozenchars/literals.hpp"
#include "frozenchars/number_conv.hpp"
#include "frozenchars/trie_set.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

// AGENTS.md 規約: "_fs" リテラル戻り値型は FrozenString<文字数+1> (終端 '\0' を含む)
TEST_CASE("_fs literal returns FrozenString<length+1>", "[literals][concepts]") {
  STATIC_CHECK(std::same_as<decltype("abc"_fs), FrozenString<4>>);
  STATIC_CHECK(std::same_as<decltype("a"_fs), FrozenString<2>>);
  STATIC_CHECK(std::same_as<decltype(""_fs), FrozenString<1>>);
  STATIC_CHECK("abc"_fs.sv() == "abc");
}

// concepts.hpp のタグ型が freeze で文字列化できることの最小確認
TEST_CASE("Hex/Bin/Oct/Precision freeze yields expected string", "[freeze][concepts]") {
  STATIC_CHECK(freeze(Hex{0xABCD}).sv() == "abcd");
  STATIC_CHECK(freeze(Bin{5}).sv() == "101");
  STATIC_CHECK(freeze(Oct{8}).sv() == "10");
  STATIC_CHECK(freeze(Precision{3.14, 2}).sv() == "3.14");
  STATIC_CHECK(freeze(42).sv() == "42");
  STATIC_CHECK(freeze(-42).sv() == "-42");
  STATIC_CHECK(freeze(2.5).sv() == "2.50");
  STATIC_CHECK(freeze("hello"_fs).sv() == "hello");
}

// number_conv のランタイム/コンパイル時 parse_number スモーク
TEST_CASE("parse_number integer and float", "[number_conv]") {
  STATIC_CHECK(*parse_number<int>("42"_fs) == 42);
  STATIC_CHECK(*parse_number<int>("-7"_fs) == -7);
  STATIC_CHECK(*parse_number<unsigned>("0x1A"_fs) == 0x1Au);
  REQUIRE(*parse_number<double>("3.14"_fs) == 3.14);
  REQUIRE(*parse_number<float>("1.5"_fs) == 1.5f);
}

// trie_set: 1 キーと複数キーのスモーク
TEST_CASE("frozen_trie_set basic find/contains", "[trie_set]") {
  using single = frozen_trie_set<"only"_fs>;
  STATIC_CHECK(single::contains("only"));
  STATIC_CHECK_FALSE(single::contains("other"));

  using multi = frozen_trie_set<"alpha"_fs, "beta"_fs, "gamma"_fs>;
  STATIC_CHECK(multi::contains("alpha"));
  STATIC_CHECK(multi::contains("beta"));
  STATIC_CHECK(multi::contains("gamma"));
  STATIC_CHECK_FALSE(multi::contains("delta"));
  STATIC_CHECK(multi::size() == 3);
  STATIC_CHECK(multi::count("alpha") == 1);
  STATIC_CHECK(multi::count("delta") == 0);
}
