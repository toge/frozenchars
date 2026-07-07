/// @file test_minifier.cpp
/// @brief minify 系関数の統合テスト
///
/// Cypher / HTML / XML / JSON / YAML / SQL の minify と
/// ops パイプアダプタの検証を含む。

#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"
#include "frozenchars/minify.hpp"

using frozenchars::minify;
using frozenchars::minify_cypher;
using frozenchars::minify_markup_opt;
using frozenchars::minify_sql_opt;
using namespace frozenchars::literals;

// ═════════════════════════════════════════════════════════════════════════════
// Cypher minify — コンパイル時検証（static_assert）
// ═════════════════════════════════════════════════════════════════════════════

namespace {

// 1. 基本: 括弧・スペース除去、識別子間スペース保持
static_assert(minify("MATCH (n) RETURN n") == "MATCH(n) RETURN n");

// 2a. 行コメント除去
static_assert(minify("MATCH (n) // comment\nRETURN n") == "MATCH(n) RETURN n");

// 2b. ブロックコメント除去
static_assert(minify("MATCH /* block */ (n) RETURN n") == "MATCH(n) RETURN n");

// 2c. 複数行ブロックコメント除去
static_assert(minify("MATCH /*\n  multi\n  line\n*/ (n) RETURN n") ==
              "MATCH(n) RETURN n");

// 3. 文字列内のコメント記号は除去しない
static_assert(minify("RETURN '//not a comment'") == "RETURN '//not a comment'");
static_assert(minify("RETURN '/* also not */removed'") ==
              "RETURN '/* also not */removed'");

// 4. バッククォート識別子（空白・改行も含めてそのまま保存）
static_assert(minify("MATCH (`my node`) RETURN `my node`") ==
              "MATCH(`my node`) RETURN `my node`");

// 5. 識別子間スペース挿入の各ケース
static_assert(minify("WHERE x = 1") == "WHERE x=1");
static_assert(minify("RETURN 1") == "RETURN 1");

// 6. 末尾セミコロン除去
static_assert(minify("RETURN 1;") == "RETURN 1");

// 7. エスケープシーケンス: \' はそのまま維持
static_assert(minify("RETURN 'It\\'s fine'") == "RETURN 'It\\'s fine'");

// 8. 複数文: 中間セミコロンは保持、末尾には ; なし
static_assert(minify("MATCH (n) RETURN n; MATCH (m) RETURN m") ==
              "MATCH(n) RETURN n;MATCH(m) RETURN m");

// 9. 複数文末尾セミコロン: 最後の1つのみ除去
static_assert(minify("MATCH (n) RETURN n; MATCH (m) RETURN m;") ==
              "MATCH(n) RETURN n;MATCH(m) RETURN m");

// 10. 連続スペース・タブ・改行は1スペースに集約（識別子間のみ）
static_assert(minify("MATCH   \t  (n)") == "MATCH(n)");
static_assert(minify("RETURN   \n\n  n") == "RETURN n");

// 11. 行末のコメント後に識別子が続く場合
static_assert(minify("WHERE // eq\nn = 1") == "WHERE n=1");

// 12. ダブルクォート文字列も保存
static_assert(minify("RETURN \"hello world\"") == "RETURN \"hello world\"");

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

// 19. 文字列と数値の混在 RETURN（識別子→文字列、文字列→識別子などのスペース保持）
static_assert(minify("RETURN 'Hello' + ' ' + 'World' AS greeting, 42 AS answer, 3.14 AS pi") ==
              "RETURN 'Hello'+' '+'World' AS greeting,42 AS answer,3.14 AS pi");

// 20. 様々な型の式を RETURN で並べる（閉じ括弧→識別子のスペース保持）
static_assert(minify(
  "RETURN true AS bool_val, 42 AS int_val, 3.14159 AS float_val,"
  " 'text' AS str_val, [1,2,3] AS list_val, date('2024-01-15') AS date_val") ==
  "RETURN true AS bool_val,42 AS int_val,3.14159 AS float_val,"
  "'text' AS str_val,[1,2,3] AS list_val,"
  "date('2024-01-15') AS date_val");

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// Cypher minify — 実行時検証（Catch2）
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("minify_cypher - 基本的なスペース・括弧の除去", "[minifier]")
{
  SECTION("MATCH/RETURN の基本形")
  {
    auto constexpr result = minify("MATCH (n) RETURN n");
    REQUIRE(result == "MATCH(n) RETURN n");
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

TEST_CASE("minify_cypher - コメント除去", "[minifier]")
{
  SECTION("行コメント")
  {
    auto constexpr result = minify("MATCH (n) // comment\nRETURN n");
    REQUIRE(result == "MATCH(n) RETURN n");
  }

  SECTION("ブロックコメント")
  {
    auto constexpr result = minify("MATCH /* block */ (n) RETURN n");
    REQUIRE(result == "MATCH(n) RETURN n");
  }

  SECTION("複数行ブロックコメント")
  {
    auto constexpr result = minify("MATCH /*\n  multi\n  line\n*/ (n) RETURN n");
    REQUIRE(result == "MATCH(n) RETURN n");
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

TEST_CASE("minify_cypher - 文字列リテラル保存", "[minifier]")
{
  SECTION("文字列内の行コメント記号は除去しない")
  {
    auto constexpr result = minify("RETURN '//not a comment'");
    REQUIRE(result == "RETURN '//not a comment'");
  }

  SECTION("文字列内のブロックコメント記号は除去しない")
  {
    auto constexpr result = minify("RETURN '/* also not */removed'");
    REQUIRE(result == "RETURN '/* also not */removed'");
  }

  SECTION("エスケープされたシングルクォート")
  {
    auto constexpr result = minify("RETURN 'It\\'s fine'");
    REQUIRE(result == "RETURN 'It\\'s fine'");
  }

  SECTION("ダブルクォート文字列")
  {
    auto constexpr result = minify("RETURN \"hello world\"");
    REQUIRE(result == "RETURN \"hello world\"");
  }

  SECTION("文字列内の空白・改行はそのまま保存")
  {
    auto constexpr result = minify("RETURN 'line1\nline2'");
    REQUIRE(result == "RETURN 'line1\nline2'");
  }
}

TEST_CASE("minify_cypher - バッククォート識別子保存", "[minifier]")
{
  SECTION("空白を含む識別子")
  {
    auto constexpr result =
        minify("MATCH (`my node`) RETURN `my node`");
    REQUIRE(result == "MATCH(`my node`) RETURN `my node`");
  }

  SECTION("バッククォート識別子内のコメント記号は除去しない")
  {
    auto constexpr result = minify("`foo // bar`");
    REQUIRE(result == "`foo // bar`");
  }
}

TEST_CASE("minify_cypher - セミコロン処理", "[minifier]")
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
    REQUIRE(result == "MATCH(n) RETURN n;MATCH(m) RETURN m");
  }

  SECTION("複数文: 末尾セミコロンのみ除去")
  {
    auto constexpr result =
        minify("MATCH (n) RETURN n; MATCH (m) RETURN m;");
    REQUIRE(result == "MATCH(n) RETURN n;MATCH(m) RETURN m");
  }
}

TEST_CASE("minify_cypher - エッジケース", "[minifier]")
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

  SECTION("RETURN a.* の後ろはスペース保持")
  {
    auto constexpr result = minify("RETURN a.* ORDER BY a.id");
    REQUIRE(result == "RETURN a.* ORDER BY a.id");
  }

  SECTION("RETURN a.* の後ろに改行がある場合もスペース保持")
  {
    auto constexpr result = minify("  RETURN\n    a.*\n  ORDER BY\n    a.id");
    REQUIRE(result == "RETURN a.* ORDER BY a.id");
  }

  SECTION("RETURN * の識別子と * の間にスペース保持")
  {
    auto constexpr result = minify("CALL\n    show_tables()\n  RETURN\n    * ");
    REQUIRE(result == "CALL show_tables() RETURN *");
  }
}

TEST_CASE("minify_cypher - 文字列と数値の混在 RETURN", "[minifier]")
{
  SECTION("文字列の連結と数値")
  {
    // 複数の式を RETURN で並べる: 文字列連結 + 数値
    auto constexpr result = minify(
      "RETURN 'Hello' + ' ' + 'World' AS greeting\n"
      ", 42 AS answer\n"
      ", 3.14 AS pi");
    REQUIRE(result == "RETURN 'Hello'+' '+'World' AS greeting,42 AS answer,3.14 AS pi");
  }

  SECTION("様々な型の式を RETURN で並べる")
  {
    auto constexpr result = minify(
      "RETURN true AS bool_val\n"
      ", 42 AS int_val\n"
      ", 3.14159 AS float_val\n"
      ", 'text' AS str_val\n"
      ", [1,2,3] AS list_val\n"
      ", date('2024-01-15') AS date_val");
    REQUIRE(result ==
            "RETURN true AS bool_val,42 AS int_val,3.14159 AS float_val,"
            "'text' AS str_val,[1,2,3] AS list_val,"
            "date('2024-01-15') AS date_val");
  }
}

TEST_CASE("minify_cypher - 実行時バッファ版", "[minifier]")
{
  SECTION("通常の変換")
  {
    char const *input = "MATCH (n) RETURN n";
    std::array<char, 64> buf{};
    auto const len = minify_cypher(input, buf.data(), buf.size());
    REQUIRE(std::string_view(buf.data(), len) == "MATCH(n) RETURN n");
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

// ═════════════════════════════════════════════════════════════════════════════
// HTML / XML minify
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("minify_html - 基本", "[minifier]")
{
  auto constexpr html = minify_html(
    "<div class = \"x\"  >\n"
    "  <span>  hi  </span>\n"
    "  <!-- ignore -->\n"
    "</div>"_fs);
  static_assert(html.sv() == "<div class=x><span>hi</span></div>");
  REQUIRE(html.sv() == "<div class=x><span>hi</span></div>");
}

TEST_CASE("minify_html - 複数要素", "[minifier]")
{
  auto constexpr html2 = minify_html(
    "<div>  hello  </div>\n"
    "<span>  world  </span>"_fs);
  static_assert(html2.sv() == "<div>hello</div><span>world</span>");
  REQUIRE(html2.sv() == "<div>hello</div><span>world</span>");
}

TEST_CASE("minify_html - 自閉じタグ", "[minifier]")
{
  auto constexpr html3 = minify_html(
    "<div class = \"x\" >\n"
    "  <input type = \"text\" />\n"
    "</div>"_fs);
  static_assert(html3.sv() == "<div class=x><input /></div>");
  REQUIRE(html3.sv() == "<div class=x><input /></div>");
}

TEST_CASE("minify_html - boolean 属性", "[minifier]")
{
  auto constexpr html4 = minify_html("<video autoplay=\"autoplay\"></video>"_fs);
  static_assert(html4.sv() == "<video autoplay></video>");
  REQUIRE(html4.sv() == "<video autoplay></video>");

  auto constexpr html5 = minify_html("<img ismap=\"ismap\" />"_fs);
  static_assert(html5.sv() == "<img ismap />");
  REQUIRE(html5.sv() == "<img ismap />");

  auto constexpr html6 = minify_html("<div itemscope=\"itemscope\">x</div>"_fs);
  static_assert(html6.sv() == "<div itemscope>x</div>");
  REQUIRE(html6.sv() == "<div itemscope>x</div>");
}

TEST_CASE("minify_html - 省略可能な終了タグ", "[minifier]")
{
  auto constexpr html7 = minify_html(
    "<table><caption>T</caption>"
    "<thead><tr><th>H</th></tr></thead>"
    "<tbody><tr><td>D</td></tr></tbody>"
    "</table>"_fs);
  static_assert(html7.sv() == "<table><caption>T<thead><tr><th>H<tbody><tr><td>D</table>");
  REQUIRE(html7.sv() == "<table><caption>T<thead><tr><th>H<tbody><tr><td>D</table>");
}

TEST_CASE("minify_html - オプション", "[minifier]")
{
  auto constexpr html_no_quote_removal = minify_html(
    "<div class=\"x\" id='y'>text</div>"_fs, minify_markup_opt::remove_end_tags);
  static_assert(html_no_quote_removal.sv() == "<div class=\"x\" id='y'>text</div>");
  REQUIRE(html_no_quote_removal.sv() == "<div class=\"x\" id='y'>text</div>");

  auto constexpr html_no_end_tag_removal = minify_html(
    "<table><caption>T</caption>"
    "<thead><tr><th>H</th></tr></thead>"
    "<tbody><tr><td>D</td></tr></tbody>"
    "</table>"_fs, minify_markup_opt::remove_quotes);
  static_assert(html_no_end_tag_removal.sv() ==
    "<table><caption>T</caption><thead><tr><th>H</th></tr></thead>"
    "<tbody><tr><td>D</td></tr></tbody></table>");
  REQUIRE(html_no_end_tag_removal.sv() ==
    "<table><caption>T</caption><thead><tr><th>H</th></tr></thead>"
    "<tbody><tr><td>D</td></tr></tbody></table>");

  auto constexpr html_no_both = minify_html(
    "<div class=\"x\"></div>"_fs, minify_markup_opt::none);
  static_assert(html_no_both.sv() == "<div class=\"x\"></div>");
  REQUIRE(html_no_both.sv() == "<div class=\"x\"></div>");
}

TEST_CASE("minify_html - void 要素", "[minifier]")
{
  auto constexpr html_void = minify_html(
    "<div><br></br><hr></hr></div>"_fs);
  static_assert(html_void.sv() == "<div><br><hr></div>");
  REQUIRE(html_void.sv() == "<div><br><hr></div>");
}

TEST_CASE("minify_html - 空属性値", "[minifier]")
{
  auto constexpr html_empty_attr = minify_html(
    "<div class=\"\" id=\"x\">text</div>"_fs);
  static_assert(html_empty_attr.sv() == "<div class id=x>text</div>");
  REQUIRE(html_empty_attr.sv() == "<div class id=x>text</div>");
}

// ═════════════════════════════════════════════════════════════════════════════
// JSON minify
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("minify_json - 基本", "[minifier]")
{
  auto constexpr json = minify_json(
    "{\n"
    "  \"k\": \"a b\",\n"
    "  \"n\": 1, // comment\n"
    "  \"x\": true\n"
    "}"_fs);
  static_assert(json.sv() == "{\"k\":\"a b\",\"n\":1,\"x\":true}");
  REQUIRE(json.sv() == "{\"k\":\"a b\",\"n\":1,\"x\":true}");
}

// ═════════════════════════════════════════════════════════════════════════════
// YAML minify
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("minify_yaml - 基本", "[minifier]")
{
  auto constexpr yaml = minify_yaml(
    "key: value   # comment\n"
    "list:\n"
    "  - one  \n"
    "  - \"two # keep\"   # drop\n"
    "\n"_fs);
  static_assert(yaml.sv() == "key: value\nlist:\n  - one\n  - \"two # keep\"\n");
  REQUIRE(yaml.sv() == "key: value\nlist:\n  - one\n  - \"two # keep\"\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// SQL minify
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("minify_sql - 基本", "[minifier]")
{
  auto constexpr sql = minify_sql(
    "SELECT  a,  b  -- comment\n"
    "FROM  tbl\n"
    "WHERE  c = 'x  y'  AND d = 1 ;"_fs);
  static_assert(sql.sv() == "SELECT a,b FROM tbl WHERE c='x  y' AND d=1;");
  REQUIRE(sql.sv() == "SELECT a,b FROM tbl WHERE c='x  y' AND d=1;");
}

TEST_CASE("minify_sql - 型短縮", "[minifier]")
{
  auto constexpr sql_type = minify_sql(
    "SELECT  INTEGER,  BOOLEAN  FROM  tbl\n"
    "WHERE  col :: text = 'x'"_fs);
  static_assert(sql_type.sv() == "SELECT INT,BOOL FROM tbl WHERE col::text='x'");
  REQUIRE(sql_type.sv() == "SELECT INT,BOOL FROM tbl WHERE col::text='x'");
}

TEST_CASE("minify_sql - none オプション", "[minifier]")
{
  auto constexpr sql_no_shorten = minify_sql(
    "SELECT INTEGER FROM tbl"_fs, minify_sql_opt::none);
  static_assert(sql_no_shorten.sv() == "SELECT INTEGER FROM tbl");
  REQUIRE(sql_no_shorten.sv() == "SELECT INTEGER FROM tbl");
}

TEST_CASE("minify_sql - CHARACTER VARYING 短縮", "[minifier]")
{
  auto constexpr sql_charvar = minify_sql(
    "SELECT  CHARACTER  VARYING(100)  FROM  tbl"_fs);
  static_assert(sql_charvar.sv() == "SELECT VARCHAR(100) FROM tbl");
  REQUIRE(sql_charvar.sv() == "SELECT VARCHAR(100) FROM tbl");
}

TEST_CASE("minify_sql - 演算子", "[minifier]")
{
  auto constexpr sql_operators = minify_sql(
    "a  ||  b  ->  'key'  @>  '{1}'"_fs);
  static_assert(sql_operators.sv() == "a||b->'key'@>'{1}'");
  REQUIRE(sql_operators.sv() == "a||b->'key'@>'{1}'");
}

TEST_CASE("minify_sql - AS キーワード除去", "[minifier]")
{
  auto constexpr sql_remove_as = minify_sql(
    "SELECT col AS alias FROM tbl"_fs,
    minify_sql_opt::shorten_types | minify_sql_opt::remove_as);
  static_assert(sql_remove_as.sv() == "SELECT col alias FROM tbl");
  REQUIRE(sql_remove_as.sv() == "SELECT col alias FROM tbl");
}

TEST_CASE("minify_sql - INNER JOIN 簡略化", "[minifier]")
{
  auto constexpr sql_simplify_join = minify_sql(
    "SELECT * FROM t1 INNER JOIN t2 ON t1.id = t2.id"_fs,
    minify_sql_opt::shorten_types | minify_sql_opt::simplify_join);
  static_assert(sql_simplify_join.sv() == "SELECT * FROM t1 JOIN t2 ON t1.id=t2.id");
  REQUIRE(sql_simplify_join.sv() == "SELECT * FROM t1 JOIN t2 ON t1.id=t2.id");
}

TEST_CASE("minify_sql - 回帰: simplify_join 単独では AS を除去しない", "[minifier]")
{
  auto constexpr sql_join_keeps_as = minify_sql(
    "SELECT col AS alias FROM t1 INNER JOIN t2 ON t1.id = t2.id"_fs,
    minify_sql_opt::simplify_join);
  static_assert(sql_join_keeps_as.sv() ==
    "SELECT col AS alias FROM t1 JOIN t2 ON t1.id=t2.id");
  REQUIRE(sql_join_keeps_as.sv() ==
    "SELECT col AS alias FROM t1 JOIN t2 ON t1.id=t2.id");
}

TEST_CASE("minify_sql - 回帰: CAST の AS は除去しない", "[minifier]")
{
  auto constexpr sql_cast_keeps_as = minify_sql(
    "SELECT CAST(val AS INTEGER) FROM tbl INNER JOIN t2 ON tbl.id = t2.id"_fs,
    minify_sql_opt::simplify_join);
  static_assert(sql_cast_keeps_as.sv() ==
    "SELECT CAST(val AS INTEGER) FROM tbl JOIN t2 ON tbl.id=t2.id");
  REQUIRE(sql_cast_keeps_as.sv() ==
    "SELECT CAST(val AS INTEGER) FROM tbl JOIN t2 ON tbl.id=t2.id");
}

TEST_CASE("minify_sql - 回帰: shorten_types + simplify_join", "[minifier]")
{
  auto constexpr sql_shorten_with_join = minify_sql(
    "SELECT INTEGER, BOOLEAN FROM t1 INNER JOIN t2 ON t1.id = t2.id"_fs,
    minify_sql_opt::shorten_types | minify_sql_opt::simplify_join);
  static_assert(sql_shorten_with_join.sv() ==
    "SELECT INT,BOOL FROM t1 JOIN t2 ON t1.id=t2.id");
  REQUIRE(sql_shorten_with_join.sv() ==
    "SELECT INT,BOOL FROM t1 JOIN t2 ON t1.id=t2.id");
}

TEST_CASE("minify_sql - 回帰: shorten_types + remove_as", "[minifier]")
{
  auto constexpr sql_shorten_with_remove_as = minify_sql(
    "SELECT INTEGER AS i FROM tbl"_fs,
    minify_sql_opt::shorten_types | minify_sql_opt::remove_as);
  static_assert(sql_shorten_with_remove_as.sv() == "SELECT INT i FROM tbl");
  REQUIRE(sql_shorten_with_remove_as.sv() == "SELECT INT i FROM tbl");
}

// ═════════════════════════════════════════════════════════════════════════════
// sql_uppercase_keywords
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("sql_uppercase_keywords", "[minifier]")
{
  auto constexpr q1 = sql_uppercase_keywords(
    "select * from users where id = 1"_fs);
  static_assert(q1.sv() == "SELECT * FROM users WHERE id = 1");
  REQUIRE(q1.sv() == "SELECT * FROM users WHERE id = 1");

  auto constexpr q2 = sql_uppercase_keywords(
    "insert into users (name, age) values ('alice', 30)"_fs);
  static_assert(q2.sv() == "INSERT INTO users (name, age) VALUES ('alice', 30)");
  REQUIRE(q2.sv() == "INSERT INTO users (name, age) VALUES ('alice', 30)");

  auto constexpr q3 = sql_uppercase_keywords(
    "select * from tbl where name = 'select'"_fs);
  static_assert(q3.sv() == "SELECT * FROM tbl WHERE name = 'select'");
  REQUIRE(q3.sv() == "SELECT * FROM tbl WHERE name = 'select'");

  auto constexpr q4 = sql_uppercase_keywords(
    "select * from `users` where `id` = 1"_fs);
  static_assert(q4.sv() == "SELECT * FROM `users` WHERE `id` = 1");
  REQUIRE(q4.sv() == "SELECT * FROM `users` WHERE `id` = 1");

  auto constexpr q5 = sql_uppercase_keywords(
    "select * from [users] where [id] = 1"_fs);
  static_assert(q5.sv() == "SELECT * FROM [users] WHERE [id] = 1");
  REQUIRE(q5.sv() == "SELECT * FROM [users] WHERE [id] = 1");

  auto constexpr q6 = sql_uppercase_keywords(
    "select -- comment\n* from tbl"_fs);
  static_assert(q6.sv() == "SELECT -- comment\n* FROM tbl");
  REQUIRE(q6.sv() == "SELECT -- comment\n* FROM tbl");

  auto constexpr q7 = sql_uppercase_keywords(
    "create table if not exists users (id int primary key, name varchar(100) not null)"_fs);
  static_assert(q7.sv() ==
    "CREATE TABLE IF NOT EXISTS users (id INT PRIMARY KEY, name VARCHAR(100) NOT NULL)");
  REQUIRE(q7.sv() ==
    "CREATE TABLE IF NOT EXISTS users (id INT PRIMARY KEY, name VARCHAR(100) NOT NULL)");
}

// ═════════════════════════════════════════════════════════════════════════════
// ops パイプアダプタ
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("minify_cypher - ops パイプ演算子", "[minifier]")
{
  SECTION("FrozenString からパイプ")
  {
    auto constexpr result = "MATCH (n) // comment\nRETURN n"_fs
      | frozenchars::ops::minify_cypher;
    static_assert(result.sv() == "MATCH(n) RETURN n");
    REQUIRE(result.sv() == "MATCH(n) RETURN n");
  }

  SECTION("空文字列")
  {
    auto constexpr result = ""_fs | frozenchars::ops::minify_cypher;
    static_assert(result.sv() == "");
    REQUIRE(result.sv() == "");
  }

  SECTION("末尾セミコロン除去")
  {
    auto constexpr result = "RETURN 1;"_fs | frozenchars::ops::minify_cypher;
    static_assert(result.sv() == "RETURN 1");
    REQUIRE(result.sv() == "RETURN 1");
  }

  SECTION("複合パイプ: trim + minify_cypher")
  {
    auto constexpr result = "  MATCH (n) RETURN n  "_fs
      | frozenchars::ops::trim
      | frozenchars::ops::minify_cypher;
    static_assert(result.sv() == "MATCH(n) RETURN n");
    REQUIRE(result.sv() == "MATCH(n) RETURN n");
  }
}

TEST_CASE("minify_html - ops パイプ演算子", "[minifier]")
{
  auto constexpr result = "<div>  hi  </div>"_fs
    | frozenchars::ops::minify_html;
  static_assert(result.sv() == "<div>hi</div>");
  REQUIRE(result.sv() == "<div>hi</div>");
}

TEST_CASE("minify_xml - ops パイプ演算子", "[minifier]")
{
  auto constexpr result = "<root>  hi  </root>"_fs
    | frozenchars::ops::minify_xml;
  static_assert(result.sv() == "<root>hi</root>");
  REQUIRE(result.sv() == "<root>hi</root>");
}

TEST_CASE("minify_json - ops パイプ演算子", "[minifier]")
{
  auto constexpr result = "{  \"k\": 1  // c\n}"_fs
    | frozenchars::ops::minify_json;
  static_assert(result.sv() == "{\"k\":1}");
  REQUIRE(result.sv() == "{\"k\":1}");
}

TEST_CASE("minify_yaml - ops パイプ演算子", "[minifier]")
{
  auto constexpr result = "key: value   # comment\n"_fs
    | frozenchars::ops::minify_yaml;
  static_assert(result.sv() == "key: value\n");
  REQUIRE(result.sv() == "key: value\n");
}

TEST_CASE("minify_sql - ops パイプ演算子", "[minifier]")
{
  auto constexpr result = "SELECT  a  FROM  tbl"_fs
    | frozenchars::ops::minify_sql;
  static_assert(result.sv() == "SELECT a FROM tbl");
  REQUIRE(result.sv() == "SELECT a FROM tbl");
}

TEST_CASE("minify_sql - ops パイプ演算子 (オプション指定)", "[minifier]")
{
  auto constexpr result = "SELECT INTEGER FROM tbl"_fs
    | frozenchars::ops::minify_sql(minify_sql_opt::none);
  static_assert(result.sv() == "SELECT INTEGER FROM tbl");
  REQUIRE(result.sv() == "SELECT INTEGER FROM tbl");
}
