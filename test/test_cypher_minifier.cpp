/// @file test_cypher_minifier.cpp
/// @brief CypherQL ミニファイヤのテスト
///
/// コンパイル時検証（static_assert）と実行時検証（Catch2）の両方を含む。

#include "catch2/catch_all.hpp"

#include "frozenchars/minify.hpp"

using frozenchars::minify;
using frozenchars::minify_cypher;

// ─── コンパイル時検証（static_assert）────────────────────────────────────────

namespace {

// 1. 基本: 括弧・スペース除去、識別子間スペース保持
static_assert(minify("MATCH (n) RETURN n") == "MATCH(n)RETURN n");

// 2a. 行コメント除去
static_assert(minify("MATCH (n) // comment\nRETURN n") == "MATCH(n)RETURN n");

// 2b. ブロックコメント除去
static_assert(minify("MATCH /* block */ (n) RETURN n") == "MATCH(n)RETURN n");

// 2c. 複数行ブロックコメント除去
static_assert(minify("MATCH /*\n  multi\n  line\n*/ (n) RETURN n") ==
              "MATCH(n)RETURN n");

// 3. 文字列内のコメント記号は除去しない
static_assert(minify("RETURN '//not a comment'") == "RETURN'//not a comment'");
static_assert(minify("RETURN '/* also not */removed'") ==
              "RETURN'/* also not */removed'");

// 4. バッククォート識別子（空白・改行も含めてそのまま保存）
static_assert(minify("MATCH (`my node`) RETURN `my node`") ==
              "MATCH(`my node`)RETURN`my node`");

// 5. 識別子間スペース挿入の各ケース
static_assert(minify("WHERE x = 1") == "WHERE x=1"); // x と WHERE の間: 必要
static_assert(minify("RETURN 1") == "RETURN 1");     // R と 1 の間: 必要

// 6. 末尾セミコロン除去
static_assert(minify("RETURN 1;") == "RETURN 1"); // スペース挿入 + セミコロン除去

// 7. エスケープシーケンス: \' はそのまま維持
static_assert(minify("RETURN 'It\\'s fine'") == "RETURN'It\\'s fine'");

// 8. 複数文: 中間セミコロンは保持、末尾には ; なし
static_assert(minify("MATCH (n) RETURN n; MATCH (m) RETURN m") ==
              "MATCH(n)RETURN n;MATCH(m)RETURN m");

// 9. 複数文末尾セミコロン: 最後の1つのみ除去
static_assert(minify("MATCH (n) RETURN n; MATCH (m) RETURN m;") ==
              "MATCH(n)RETURN n;MATCH(m)RETURN m");

// 10. 連続スペース・タブ・改行は1スペースに集約（識別子間のみ）
static_assert(minify("MATCH   \t  (n)") == "MATCH(n)");
static_assert(minify("RETURN   \n\n  n") == "RETURN n");

// 11. 行末のコメント後に識別子が続く場合
static_assert(minify("WHERE // eq\nn = 1") == "WHERE n=1");

// 12. ダブルクォート文字列も保存
static_assert(minify("RETURN \"hello world\"") == "RETURN\"hello world\"");

// 13. 識別子同士が記号で区切られる場合はスペース不要
static_assert(minify("n.name = m.name") == "n.name=m.name");
static_assert(minify("n:Person") == "n:Person");

// 14. 先頭・末尾の空白は除去
static_assert(minify("  MATCH (n)  ") == "MATCH(n)");

// 15. ブロックコメントが識別子間に挟まれる場合はスペース補完
static_assert(minify("MATCH/*comment*/n") == "MATCH n");
static_assert(minify("WHERE/*comment*/n=1") == "WHERE n=1");

// 16. 空文字列
static_assert(minify("") == "");

// 17. 空白のみ
static_assert(minify("   \t\n  ") == "");

// 18. バッククォート識別子内のコメント記号も保存
static_assert(minify("`foo // bar`") == "`foo // bar`");

} // namespace

// ─── 実行時検証（Catch2）────────────────────────────────────────────────────

TEST_CASE("minify_cypher - 基本的なスペース・括弧の除去", "[cypher_minifier]")
{
  SECTION("MATCH/RETURN の基本形")
  {
    auto constexpr result = minify("MATCH (n) RETURN n");
    REQUIRE(result == "MATCH(n)RETURN n");
  }

  SECTION("WHERE 句のスペース保持とイコール前後の除去")
  {
    auto constexpr result = minify("WHERE x = 1");
    REQUIRE(result == "WHERE x=1");
  }

  SECTION("n.name = m.name: ドット区切りはスペース不要")
  {
    auto constexpr result = minify("n.name = m.name");
    REQUIRE(result == "n.name=m.name");
  }
}

TEST_CASE("minify_cypher - コメント除去", "[cypher_minifier]")
{
  SECTION("行コメント")
  {
    auto constexpr result = minify("MATCH (n) // comment\nRETURN n");
    REQUIRE(result == "MATCH(n)RETURN n");
  }

  SECTION("ブロックコメント")
  {
    auto constexpr result = minify("MATCH /* block */ (n) RETURN n");
    REQUIRE(result == "MATCH(n)RETURN n");
  }

  SECTION("複数行ブロックコメント")
  {
    auto constexpr result = minify("MATCH /*\n  multi\n  line\n*/ (n) RETURN n");
    REQUIRE(result == "MATCH(n)RETURN n");
  }

  SECTION("ブロックコメントが識別子間: スペース補完")
  {
    auto constexpr result = minify("MATCH/*comment*/n");
    REQUIRE(result == "MATCH n");
  }

  SECTION("ブロックコメントが記号間: スペース不要")
  {
    auto constexpr result = minify("(/*comment*/n)");
    REQUIRE(result == "(n)");
  }
}

TEST_CASE("minify_cypher - 文字列リテラル保存", "[cypher_minifier]")
{
  SECTION("文字列内の行コメント記号は除去しない")
  {
    auto constexpr result = minify("RETURN '//not a comment'");
    REQUIRE(result == "RETURN'//not a comment'");
  }

  SECTION("文字列内のブロックコメント記号は除去しない")
  {
    auto constexpr result = minify("RETURN '/* also not */removed'");
    REQUIRE(result == "RETURN'/* also not */removed'");
  }

  SECTION("エスケープされたシングルクォート")
  {
    auto constexpr result = minify("RETURN 'It\\'s fine'");
    REQUIRE(result == "RETURN'It\\'s fine'");
  }

  SECTION("ダブルクォート文字列")
  {
    auto constexpr result = minify("RETURN \"hello world\"");
    REQUIRE(result == "RETURN\"hello world\"");
  }

  SECTION("文字列内の空白・改行はそのまま保存")
  {
    auto constexpr result = minify("RETURN 'line1\nline2'");
    REQUIRE(result == "RETURN'line1\nline2'");
  }
}

TEST_CASE("minify_cypher - バッククォート識別子保存", "[cypher_minifier]")
{
  SECTION("空白を含む識別子")
  {
    auto constexpr result =
        minify("MATCH (`my node`) RETURN `my node`");
    REQUIRE(result == "MATCH(`my node`)RETURN`my node`");
  }

  SECTION("バッククォート識別子内のコメント記号は除去しない")
  {
    auto constexpr result = minify("`foo // bar`");
    REQUIRE(result == "`foo // bar`");
  }
}

TEST_CASE("minify_cypher - セミコロン処理", "[cypher_minifier]")
{
  SECTION("末尾セミコロン除去")
  {
    auto constexpr result = minify("RETURN 1;");
    REQUIRE(result == "RETURN 1");
  }

  SECTION("複数文: 中間セミコロンは保持")
  {
    auto constexpr result =
        minify("MATCH (n) RETURN n; MATCH (m) RETURN m");
    REQUIRE(result == "MATCH(n)RETURN n;MATCH(m)RETURN m");
  }

  SECTION("複数文: 末尾セミコロンのみ除去")
  {
    auto constexpr result =
        minify("MATCH (n) RETURN n; MATCH (m) RETURN m;");
    REQUIRE(result == "MATCH(n)RETURN n;MATCH(m)RETURN m");
  }
}

TEST_CASE("minify_cypher - エッジケース", "[cypher_minifier]")
{
  SECTION("空文字列")
  {
    auto constexpr result = minify("");
    REQUIRE(result == "");
  }

  SECTION("空白のみ")
  {
    auto constexpr result = minify("   \t\n  ");
    REQUIRE(result == "");
  }

  SECTION("先頭・末尾の空白は除去")
  {
    auto constexpr result = minify("  MATCH (n)  ");
    REQUIRE(result == "MATCH(n)");
  }

  SECTION("連続する空白は1スペースに集約（識別子間のみ）")
  {
    auto constexpr result = minify("RETURN   \n\n  n");
    REQUIRE(result == "RETURN n");
  }
}

TEST_CASE("minify_cypher - 実行時バッファ版", "[cypher_minifier]")
{
  SECTION("通常の変換")
  {
    char const *input = "MATCH (n) RETURN n";
    std::array<char, 64> buf{};
    auto const len = minify_cypher(input, buf.data(), buf.size());
    REQUIRE(std::string_view(buf.data(), len) == "MATCH(n)RETURN n");
  }

  SECTION("容量 0 は 0 を返す")
  {
    char const *input = "MATCH (n)";
    auto const len = minify_cypher(input, nullptr, 0);
    REQUIRE(len == 0);
  }

  SECTION("容量ちょうど 1 のとき空文字列が出力される")
  {
    char const *input = "MATCH (n)";
    char buf[1] = {'\xff'};
    auto const len = minify_cypher(input, buf, 1);
    REQUIRE(len == 0);
    REQUIRE(buf[0] == '\0');
  }
}
