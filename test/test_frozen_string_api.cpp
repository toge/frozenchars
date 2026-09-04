#include "catch2/catch_all.hpp"

#include <span>
#include <type_traits>
#include <unordered_map>

#include "frozenchars/string.hpp"
#include "frozenchars/literals.hpp"

/** @brief FrozenString 基本 API（empty, operator[], front/back, range-for, 比較, hash）のテスト */

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

  // begin/end の const 版も同等
  std::string collected2;
  for (auto it = fs.begin(); it != fs.end(); ++it) {
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

TEST_CASE("FrozenString operator+") {
  // 戻り値型は N1+N2-1（終端 '\0' を二重に含めない）
  static_assert(std::is_same_v<decltype("foo"_fs + "bar"_fs), FrozenString<7>>);

  // FrozenString 同士
  auto constexpr concat1 = "foo"_fs + "bar"_fs;
  static_assert(concat1.sv() == "foobar");
  static_assert(concat1.length == 6);
  REQUIRE(concat1.sv() == "foobar");

  // FrozenString + 文字列リテラル
  auto constexpr concat2 = "foo"_fs + "bar";
  static_assert(concat2.sv() == "foobar");
  REQUIRE(concat2.sv() == "foobar");

  // 文字列リテラル + FrozenString
  auto constexpr concat3 = "foo" + "bar"_fs;
  static_assert(concat3.sv() == "foobar");
  REQUIRE(concat3.sv() == "foobar");

  // 連結の連鎖
  auto constexpr concat4 = "a"_fs + "b"_fs + "c"_fs;
  static_assert(concat4.sv() == "abc");
  REQUIRE(concat4.sv() == "abc");

  // 空文字列との連結
  auto constexpr concat5 = FrozenString<1>{} + "abc"_fs;
  static_assert(concat5.sv() == "abc");
  REQUIRE(concat5.sv() == "abc");

  // ランタイムでも動作
  auto const runtime = FrozenString{"hello"} + FrozenString{" world"};
  REQUIRE(runtime.sv() == "hello world");
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

TEST_CASE("FrozenString span conversion") {
  auto const fs = "abc"_fs;
  auto const view = std::span<char const>{fs};
  REQUIRE(view.size() == 3);
  REQUIRE(view[0] == 'a');
  REQUIRE(view[1] == 'b');
  REQUIRE(view[2] == 'c');
  REQUIRE(std::string_view{view.data(), view.size()} == "abc");
}
