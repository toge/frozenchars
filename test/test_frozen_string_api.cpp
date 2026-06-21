#include "catch2/catch_all.hpp"

#include <unordered_map>

#include "frozenchars/string.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("FrozenString::empty") {
  static_assert(FrozenString<1>{}.empty());
  static_assert(!"abc"_fs.empty());
  REQUIRE(FrozenString<1>{}.empty());
  REQUIRE_FALSE("abc"_fs.empty());
}

TEST_CASE("FrozenString::operator[]") {
  static_assert("abc"_fs[0] == 'a');
  static_assert("abc"_fs[1] == 'b');
  static_assert("abc"_fs[2] == 'c');
  REQUIRE("abc"_fs[0] == 'a');
  REQUIRE("abc"_fs[1] == 'b');
  REQUIRE("abc"_fs[2] == 'c');
}

TEST_CASE("FrozenString::front and back") {
  static_assert("abc"_fs.front() == 'a');
  static_assert("abc"_fs.back() == 'c');
  REQUIRE("abc"_fs.front() == 'a');
  REQUIRE("abc"_fs.back() == 'c');
}

TEST_CASE("FrozenString range-based for") {
  // begin/end によるイテレーション
  auto const fs = "hello"_fs;
  std::string collected;
  for (auto const c : fs) {
    collected += c;
  }
  REQUIRE(collected == "hello");

  // cbegin/cend も同等
  std::string collected2;
  for (auto it = fs.cbegin(); it != fs.cend(); ++it) {
    collected2 += *it;
  }
  REQUIRE(collected2 == "hello");
}

TEST_CASE("FrozenString comparison operators") {
  // FrozenString vs FrozenString
  static_assert("abc"_fs == "abc"_fs);
  static_assert("abc"_fs != "abd"_fs);
  static_assert("abc"_fs < "abd"_fs);
  static_assert("abd"_fs > "abc"_fs);
  static_assert("abc"_fs <= "abc"_fs);
  static_assert("abc"_fs >= "abc"_fs);
  REQUIRE("abc"_fs == "abc"_fs);
  REQUIRE("abc"_fs != "abd"_fs);
  REQUIRE("abc"_fs < "abd"_fs);

  // FrozenString vs string_view
  static_assert("abc"_fs == std::string_view{"abc"});
  static_assert("abc"_fs != std::string_view{"xyz"});
  REQUIRE("abc"_fs == std::string_view{"abc"});
  REQUIRE("abc"_fs != std::string_view{"xyz"});

  // FrozenString vs 文字列リテラル
  static_assert("abc"_fs == "abc");
  static_assert("abc"_fs != "xyz");
  REQUIRE("abc"_fs == "abc");
  REQUIRE("abc"_fs != "xyz");
}

TEST_CASE("FrozenString std::hash") {
  // std::unordered_map のキーとして使用できることを確認
  std::unordered_map<FrozenString<4>, int> m;
  m["abc"_fs] = 1;
  m["xyz"_fs] = 2;
  REQUIRE(m["abc"_fs] == 1);
  REQUIRE(m["xyz"_fs] == 2);
  REQUIRE(m.contains("abc"_fs));
  REQUIRE_FALSE(m.contains("zzz"_fs));

  // 同じ文字列は同じハッシュ値
  auto const h = std::hash<FrozenString<4>>{};
  REQUIRE(h("abc"_fs) == h("abc"_fs));
  // std::string_view との整合性
  REQUIRE(h("abc"_fs) == std::hash<std::string_view>{}("abc"));
}
