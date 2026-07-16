/// @file test_minify_lua.cpp
/// @brief Lua/Luau minify の統合テスト

#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"
#include "frozenchars/minify.hpp"

using frozenchars::minify_lua;
using frozenchars::minify_lua_opt;
using namespace frozenchars::literals;
namespace fops = frozenchars::ops;

// ═════════════════════════════════════════════════════════════════════════════
// Lua/Luau minify — コンパイル時検証（static_assert）
// ═════════════════════════════════════════════════════════════════════════════

namespace {

// 1. 行コメント除去
static_assert(minify_lua("local x = 1 -- c\nlocal y = 2").sv() == "local x=1 local y=2");

// 2. 長括弧コメント L0
static_assert(minify_lua("--[[ block ]]local x = 1").sv() == "local x=1");

// 3. 長括弧コメント L2
static_assert(minify_lua("--[==[ nested ]] ]==]local x").sv() == "local x");

// 4. 長文字列内の -- は保持
static_assert(minify_lua("s = [[ -- not a comment ]]").sv() == "s=[[ -- not a comment ]]");

// 5. 長文字列 L1: 分離された ]=] を認識
static_assert(minify_lua("s=[=[ a ]=]").sv() == "s=[=[ a ]=]");

// 6. 短文字列内の --
static_assert(minify_lua("s = '-- not a comment'").sv() == "s='-- not a comment'");

// 7. トークン境界（ident-ident）で空白を保持
static_assert(minify_lua("local x").sv() == "local x");

// 8. 不要な空白の除去
static_assert(minify_lua("local   x   =   1").sv() == "local x=1");

// 9. 空行の除去（改行は単一空白に畳まれる）
static_assert(minify_lua("a\n\n\nb").sv() == "a b");

// 10. keep_directives で --!strict を保持
static_assert(minify_lua("--!strict\nlocal x = 1", minify_lua_opt::keep_directives).sv() ==
              "--!strict\nlocal x=1");

// 11. none では --!strict も削除
static_assert(minify_lua("--!strict\nlocal x").sv() == "local x");

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// 実行時検証（TEST_CASE）
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("minify_lua - コメント・空白の除去", "[minifier]")
{
  SECTION("行コメント除去")
  {
    auto constexpr result = minify_lua("local x = 1 -- c\nlocal y = 2");
    static_assert(result.sv() == "local x=1 local y=2");
    REQUIRE(result.sv() == "local x=1 local y=2");
  }

  SECTION("長括弧コメント L0")
  {
    auto constexpr result = minify_lua("--[[ block ]]local x = 1");
    REQUIRE(result.sv() == "local x=1");
  }

  SECTION("長括弧コメント L2")
  {
    auto constexpr result = minify_lua("--[==[ nested ]] ]==]local x");
    REQUIRE(result.sv() == "local x");
  }

  SECTION("長文字列内の -- は保持")
  {
    auto constexpr result = minify_lua("s = [[ -- not a comment ]]");
    REQUIRE(result.sv() == "s=[[ -- not a comment ]]");
  }

  SECTION("短文字列内の -- は保持")
  {
    auto constexpr result = minify_lua("s = '-- not a comment'");
    REQUIRE(result.sv() == "s='-- not a comment'");
  }

  SECTION("不要な空白の除去")
  {
    auto constexpr result = minify_lua("local   x   =   1");
    REQUIRE(result.sv() == "local x=1");
  }

  SECTION("空行の除去")
  {
    auto constexpr result = minify_lua("a\n\n\nb");
    REQUIRE(result.sv() == "a b");
  }
}

TEST_CASE("minify_lua - keep_directives オプション", "[minifier]")
{
  SECTION("ディレクティブを保持")
  {
    auto constexpr result = minify_lua("--!strict\nlocal x = 1",
                                       minify_lua_opt::keep_directives);
    REQUIRE(result.sv() == "--!strict\nlocal x=1");
  }

  SECTION("デフォルトではディレクティブも削除")
  {
    auto constexpr result = minify_lua("--!strict\nlocal x");
    REQUIRE(result.sv() == "local x");
  }
}

TEST_CASE("minify_lua - ops パイプ演算子", "[minifier]")
{
  SECTION("FrozenString からパイプ")
  {
    auto constexpr result = "local x = 1 -- c\nlocal y = 2"_fs
      | fops::minify_lua;
    static_assert(result.sv() == "local x=1 local y=2");
    REQUIRE(result.sv() == "local x=1 local y=2");
  }

  SECTION("オプション付きパイプ")
  {
    auto constexpr result = "--!strict\nlocal x = 1"_fs
      | fops::minify_lua(minify_lua_opt::keep_directives);
    REQUIRE(result.sv() == "--!strict\nlocal x=1");
  }
}

// バッファ末尾はゼロ埋めされ、size() は実長を返す
namespace {
template <std::size_t N>
constexpr bool buffer_trailing_zero(frozenchars::FrozenString<N> const& s) {
  for (std::size_t i = s.size(); i + 1 < N; ++i) {
    if (s.buffer[i] != '\0') {
      return false;
    }
  }
  return true;
}
} // namespace

TEST_CASE("minify_lua - size() は実長を返しバッファ末尾はゼロ", "[minifier][regression]")
{
  auto constexpr c = "local x = 1 -- c\nlocal y = 2"_fs | fops::minify_lua;
  static_assert(c.size() == 19); // "local x=1 local y=2"
  static_assert(c.size() < c.buffer.size());
  static_assert(buffer_trailing_zero(c));
  REQUIRE(c.size() == 19);
  REQUIRE(buffer_trailing_zero(c));
}
