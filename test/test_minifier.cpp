/// @file test_minifier.cpp
/// @brief minify 系関数の統合テスト
///
/// Cypher / HTML / XML / JSON / YAML / SQL の minify と
/// ops パイプアダプタの検証を含む。

#include "catch2/catch_all.hpp"

#include <string_view>
#include <iostream>

#include "frozenchars/map.hpp"
#include "frozenchars/ops.hpp"
#include "frozenchars/minify.hpp"

using frozenchars::minify_cypher;
using frozenchars::minify_js;
using frozenchars::minify_markup_opt;
using frozenchars::minify_sql_opt;
using namespace frozenchars::literals;

// ═════════════════════════════════════════════════════════════════════════════
// ═════════════════════════════════════════════════════════════════════════════
// JS minify — コンパイル時検証（static_assert）
// ═════════════════════════════════════════════════════════════════════════════

namespace {

// 1. 行コメント除去
static_assert(minify_js("var x = 1; // comment\n var y = 2;").sv() == std::string_view("var x = 1;var y = 2"));

// 2. ブロックコメント除去
static_assert(minify_js("var x = /* block */ 1;").sv() == std::string_view("var x = 1"));

// 3. 複数行ブロックコメント除去
static_assert(minify_js("var x = /*\n  multi\n  line\n*/ 1;").sv() == std::string_view("var x = 1"));

// 4. 文字列内のコメント記号は除去しない
static_assert(minify_js("var url = \"//not a comment\";").sv() == std::string_view("var url =\"//not a comment\""));
static_assert(minify_js("var url = \"/* also not */removed\";").sv() == std::string_view("var url =\"/* also not */removed\""));

// 5. シングルクォート文字列保護
static_assert(minify_js("var s = 'hello world';").sv() == std::string_view("var s ='hello world'"));

// 6. テンプレートリテラル保護
static_assert(minify_js("var s = `hello ${name}`;").sv() == std::string_view("var s =`hello ${name}`"));

// 7. エスケープシーケンス保護
static_assert(minify_js("var s = \"hello \\\"world\\\"\";").sv() == std::string_view("var s =\"hello \\\"world\\\"\""));
static_assert(minify_js("var s = 'hello \\'world\\'';").sv() == std::string_view("var s ='hello \\'world\\''"));

// 8. 空白圧縮（括弧周りなど）
static_assert(minify_js("function  f(  x , y )  {  return  x + y ; }").sv() ==
              std::string_view("function f(x,y){return x + y}"));

// 9. 識別子間のスペース保持、演算子前後
static_assert(minify_js("if (a < b) { console . log (\"hi\"); }").sv() ==
              std::string_view("if(a < b){console.log(\"hi\")}"));

// 10. ブロックコメント除去後に隣接トークン間のスペースを補完
static_assert(minify_js("var x/*comment*/=1;").sv() == std::string_view("var x =1"));

// 11. 空文字列
static_assert(minify_js("").sv() == std::string_view(""));

// 12. 空白のみ
static_assert(minify_js("   \t\n  ").sv() == std::string_view(""));

// 13. ダブルクォート内のエスケープされた quote は閉じない
static_assert(minify_js("var s = \" \\\" \";").sv() == std::string_view("var s =\" \\\" \""));

}

// ═════════════════════════════════════════════════════════════════════════════
// ═════════════════════════════════════════════════════════════════════════════
// Cypher minify — コンパイル時検証（static_assert）
// ═════════════════════════════════════════════════════════════════════════════

namespace {

// 1. 基本: 括弧・スペース除去、識別子間スペース保持
static_assert(minify_cypher("MATCH (n) RETURN n").sv() == std::string_view("MATCH(n) RETURN n"));

// 2a. 行コメント除去
static_assert(minify_cypher("MATCH (n) // comment\nRETURN n").sv() == std::string_view("MATCH(n) RETURN n"));

// 2b. ブロックコメント除去
static_assert(minify_cypher("MATCH /* block */ (n) RETURN n").sv() == std::string_view("MATCH(n) RETURN n"));

// 2c. 複数行ブロックコメント除去
static_assert(minify_cypher("MATCH /*\n  multi\n  line\n*/ (n) RETURN n").sv() ==
              std::string_view("MATCH(n) RETURN n"));

// 3. 文字列内のコメント記号は除去しない
static_assert(minify_cypher("RETURN '//not a comment'").sv() == std::string_view("RETURN '//not a comment'"));
static_assert(minify_cypher("RETURN '/* also not */removed'").sv() ==
              std::string_view("RETURN '/* also not */removed'"));

// 4. バッククォート識別子（空白・改行も含めてそのまま保存）
static_assert(minify_cypher("MATCH (`my node`) RETURN `my node`").sv() ==
              std::string_view("MATCH(`my node`) RETURN `my node`"));

// 5. 識別子間スペース挿入の各ケース
static_assert(minify_cypher("WHERE x = 1").sv() == std::string_view("WHERE x=1"));
static_assert(minify_cypher("RETURN 1").sv() == std::string_view("RETURN 1"));

// 6. 末尾セミコロン除去
static_assert(minify_cypher("RETURN 1;").sv() == std::string_view("RETURN 1"));

// 7. エスケープシーケンス: \' はそのまま維持
static_assert(minify_cypher("RETURN 'It\\'s fine'").sv() == std::string_view("RETURN 'It\\'s fine'"));

// 8. 複数文: 中間セミコロンは保持、末尾には ; なし
static_assert(minify_cypher("MATCH (n) RETURN n; MATCH (m) RETURN m").sv() ==
              std::string_view("MATCH(n) RETURN n;MATCH(m) RETURN m"));

// 9. 複数文末尾セミコロン: 最後の1つのみ除去
static_assert(minify_cypher("MATCH (n) RETURN n; MATCH (m) RETURN m;").sv() ==
              std::string_view("MATCH(n) RETURN n;MATCH(m) RETURN m"));

// 10. COPY (query) TO ... の識別子と ( の間のスペース保持
static_assert(minify_cypher("COPY (MATCH (p:Person) RETURN p.id) TO '/tmp/out.csv' (HEADER=true)").sv() ==
              std::string_view("COPY (MATCH(p:Person) RETURN p.id) TO '/tmp/out.csv' (HEADER=true)"));

// 11. 連続スペース・タブ・改行は1スペースに集約（識別子間のみ）
static_assert(minify_cypher("MATCH   \t  (n)").sv() == std::string_view("MATCH(n)"));
static_assert(minify_cypher("RETURN   \n\n  n").sv() == std::string_view("RETURN n"));

// 12. 行末のコメント後に識別子が続く場合
static_assert(minify_cypher("WHERE // eq\nn = 1").sv() == std::string_view("WHERE n=1"));

// 13. ダブルクォート文字列も保存
static_assert(minify_cypher("RETURN \"hello world\"").sv() == std::string_view("RETURN \"hello world\""));

// 14. 識別子同士が記号で区切られる場合はスペース不要
static_assert(minify_cypher("n.name = m.name").sv() == std::string_view("n.name=m.name"));
static_assert(minify_cypher("n:Person").sv() == std::string_view("n:Person"));

// 15. 先頭・末尾の空白は除去
static_assert(minify_cypher("  MATCH (n)  ").sv() == std::string_view("MATCH(n)"));

// 16. ブロックコメントが識別子間に挟まれる場合はスペース補完
static_assert(minify_cypher("MATCH/*comment*/n").sv() == std::string_view("MATCH n"));
static_assert(minify_cypher("WHERE/*comment*/n=1").sv() == std::string_view("WHERE n=1"));

// 17. 空文字列
static_assert(minify_cypher("").sv() == std::string_view(""));

// 18. 空白のみ
static_assert(minify_cypher("   \t\n  ").sv() == std::string_view(""));

// 19. バッククォート識別子内のコメント記号も保存
static_assert(minify_cypher("`foo // bar`").sv() == std::string_view("`foo // bar`"));

// 20. 文字列と数値の混在 RETURN
static_assert(minify_cypher("RETURN 'Hello' + ' ' + 'World' AS greeting, 42 AS answer, 3.14 AS pi").sv() ==
              std::string_view("RETURN 'Hello'+' '+'World' AS greeting,42 AS answer,3.14 AS pi"));

// 21. 様々な型の式を RETURN で並べる
static_assert(minify_cypher(
  "RETURN true AS bool_val, 42 AS int_val, 3.14159 AS float_val,"
  " 'text' AS str_val, [1,2,3] AS list_val, date('2024-01-15') AS date_val").sv() ==
  std::string_view(
    "RETURN true AS bool_val,42 AS int_val,3.14159 AS float_val,"
    "'text' AS str_val,[1,2,3] AS list_val,"
    "date('2024-01-15') AS date_val"));

} // namespace
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

  // button type=\"submit\" はデフォルトなので冗長
  auto constexpr btn = minify_html("<button type=\"submit\">OK</button>"_fs);
  static_assert(btn.sv() == "<button>OK</button>");
  REQUIRE(btn.sv() == "<button>OK</button>");

  // button type=\"button\" はデフォルトではないので保持
  auto constexpr btn2 = minify_html("<button type=\"button\">OK</button>"_fs);
  static_assert(btn2.sv() == "<button type=button>OK</button>");
  REQUIRE(btn2.sv() == "<button type=button>OK</button>");
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

TEST_CASE("minify_html - Mustache/injamm テンプレートタグ保護", "[minifier]")
{
  // 前後に空白（改行）を含むタグ: 内部に空白混入せずタグが完全に残る
  auto constexpr tpl = minify_html(
    "\n{{#items}}\n  <tr><td>{{id}}</td></tr>\n{{/items}}\n"_fs);
  static_assert(tpl.sv() == "{{#items}}<tr><td>{{id}}{{/items}}");
  REQUIRE(tpl.sv() == "{{#items}}<tr><td>{{id}}{{/items}}");

  // 属性値内・テキスト内の {{}} が保持される
  auto constexpr attr = minify_html(
    "<span class=\"{{cat_color}}\">{{category}}</span>"_fs);
  static_assert(attr.sv() == "<span class={{cat_color}}>{{category}}</span>");
  REQUIRE(attr.sv() == "<span class={{cat_color}}>{{category}}</span>");

  // 前後空白なしのタグは変化しない（回帰）
  auto constexpr compact = minify_html("{{#a}}x{{/a}}"_fs);
  static_assert(compact.sv() == "{{#a}}x{{/a}}");
  REQUIRE(compact.sv() == "{{#a}}x{{/a}}");

  // タグ内部の空白は透過コピーで保持される
  auto constexpr spaced = minify_html("a {{ x }} b"_fs);
  static_assert(spaced.sv() == "a{{ x }} b");
  REQUIRE(spaced.sv() == "a{{ x }} b");
}

TEST_CASE("minify_html - <script> 内 JS 行コメント除去", "[minifier]")
{
  auto constexpr r = minify_html(
    "<script>var x = 1; // comment\n var y = 2;</script>"_fs);
  static_assert(r.sv() == "<script>var x=1; var y=2</script>");
  REQUIRE(r.sv() == "<script>var x=1; var y=2</script>");
}

TEST_CASE("minify_html - <script> 内 JS ブロックコメント除去", "[minifier]")
{
  auto constexpr r = minify_html(
    "<script>var x = /* block */ 1;</script>"_fs);
  static_assert(r.sv() == "<script>var x=1</script>");
  REQUIRE(r.sv() == "<script>var x=1</script>");

  auto constexpr multi = minify_html(
    "<script>var x = /* multi\nline */ 1;</script>"_fs);
  static_assert(multi.sv() == "<script>var x=1</script>");
  REQUIRE(multi.sv() == "<script>var x=1</script>");
}

TEST_CASE("minify_html - <script> 内 JS 文字列リテラル保護", "[minifier]")
{
  auto constexpr dq = minify_html(
    "<script>var url = \"http://example.com\";</script>"_fs);
  static_assert(dq.sv() == "<script>var url=\"http://example.com\"</script>");
  REQUIRE(dq.sv() == "<script>var url=\"http://example.com\"</script>");

  auto constexpr sq = minify_html(
    "<script>var s = 'http://example.com';</script>"_fs);
  static_assert(sq.sv() == "<script>var s='http://example.com'</script>");
  REQUIRE(sq.sv() == "<script>var s='http://example.com'</script>");

  auto constexpr bt = minify_html(
    "<script>var s = `hello ${name}`;</script>"_fs);
  static_assert(bt.sv() == "<script>var s=`hello ${name}`</script>");
  REQUIRE(bt.sv() == "<script>var s=`hello ${name}`</script>");
}

TEST_CASE("minify_html - <script> 内行コメント直前に </script> がある場合", "[minifier]")
{
  auto constexpr r = minify_html(
    "<script>var s = 'ok'; // comment\n</script>"_fs);
  static_assert(r.sv() == "<script>var s='ok'</script>");
  REQUIRE(r.sv() == "<script>var s='ok'</script>");

  auto constexpr inline_c = minify_html(
    "<script>var s = 'x'; // comment</script>"_fs);
  static_assert(inline_c.sv() == "<script>var s='x'</script>");
  REQUIRE(inline_c.sv() == "<script>var s='x'</script>");
}

TEST_CASE("minify_html - <script> 内 <> は JS の一部として保持", "[minifier]")
{
  auto constexpr r = minify_html(
    "<script>if (a < b && c > d) {}</script>"_fs);
  static_assert(r.sv() == "<script>if (a<b && c>d) {}</script>");
  REQUIRE(r.sv() == "<script>if (a<b && c>d) {}</script>");
}

TEST_CASE("minify_html - 複数の <script> ブロック", "[minifier]")
{
  auto constexpr r = minify_html(
    "<div>a</div><script>// c\nvar x=1;</script><div>b</div>"_fs);
  static_assert(r.sv() == "<div>a</div><script>var x=1</script><div>b</div>");
  REQUIRE(r.sv() == "<div>a</div><script>var x=1</script><div>b</div>");
}

TEST_CASE("minify_html - <script> 外の // は JS コメント扱いしない", "[minifier]")
{
  auto constexpr r = minify_html(
    "<div>hello // world</div>"_fs);
  static_assert(r.sv() == "<div>hello//world</div>");
  REQUIRE(r.sv() == "<div>hello//world</div>");
}

TEST_CASE("minify_html - auto-preserve <py-script>", "[minifier]")
{
  auto constexpr r = minify_html(
    "<py-script>\n  print('hi')\n</py-script>"_fs);
  REQUIRE(r.sv() == "<py-script>print('hi')</py-script>");
}

TEST_CASE("minify_html - auto-preserve script[type=py]", "[minifier]")
{
  auto constexpr r = minify_html(
    "<script type=\"py\">\n  print('x')\n</script>"_fs);
  REQUIRE(r.sv() == "<script type=py>\n  print('x')\n</script>");
}

TEST_CASE("minify_html - auto-preserve script[type=text/python]", "[minifier]")
{
  auto constexpr r = minify_html(
    "<script type=\"text/python\">\n  print('y')\n</script>"_fs);
  REQUIRE(r.sv() == "<script type=text/python>\n  print('y')\n</script>");
}

TEST_CASE("minify_html - auto-preserve script[type=application/python]", "[minifier]")
{
  auto constexpr r = minify_html(
    "<script type=\"application/python\">\n  print('app')\n</script>"_fs);
  REQUIRE(r.sv() == "<script type=application/python>\n  print('app')\n</script>");
}

TEST_CASE("minify_html - auto-preserve script[type=text/x-python]", "[minifier]")
{
  auto constexpr r = minify_html(
    "<script type=\"text/x-python\">\n  print('x')\n</script>"_fs);
  REQUIRE(r.sv() == "<script type=text/x-python>\n  print('x')\n</script>");
}

TEST_CASE("minify_html - preserve_script で script 内をそのまま保持", "[minifier]")
{
  auto constexpr opt = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags
                     | minify_markup_opt::preserve_script;

  auto constexpr r = minify_html(
    "<script>var x = 1; // comment\n var y = 2;</script>"_fs, opt);
  static_assert(r.sv() == "<script>var x = 1; // comment\n var y = 2;</script>");
  REQUIRE(r.sv() == "<script>var x = 1; // comment\n var y = 2;</script>");
}

TEST_CASE("minify_html - preserve_script でブロックコメントも保持", "[minifier]")
{
  auto constexpr opt = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags
                     | minify_markup_opt::preserve_script;

  auto constexpr r = minify_html(
    "<script>var x = /* block */ 1;</script>"_fs, opt);
  static_assert(r.sv() == "<script>var x = /* block */ 1;</script>");
  REQUIRE(r.sv() == "<script>var x = /* block */ 1;</script>");
}

TEST_CASE("minify_html - preserve_script で空白をそのまま保持", "[minifier]")
{
  auto constexpr opt = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags
                     | minify_markup_opt::preserve_script;

  auto constexpr r = minify_html(
    "<script>function  f(  x , y )  {  return  x + y ; }</script>"_fs, opt);
  static_assert(r.sv() == "<script>function  f(  x , y )  {  return  x + y ; }</script>");
  REQUIRE(r.sv() == "<script>function  f(  x , y )  {  return  x + y ; }</script>");
}

TEST_CASE("minify_html - preserve_script で HTML 側は通常通り minify", "[minifier]")
{
  auto constexpr opt = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags
                     | minify_markup_opt::preserve_script;

  auto constexpr r = minify_html(
    "<div>  hi  </div><script>var x = 1; // comment\n</script>"_fs, opt);
  static_assert(r.sv() == "<div>hi</div><script>var x = 1; // comment\n</script>");
  REQUIRE(r.sv() == "<div>hi</div><script>var x = 1; // comment\n</script>");
}

TEST_CASE("minify_html - preserve_script ops パイプ", "[minifier]")
{
  auto constexpr opt = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags
                     | minify_markup_opt::preserve_script;

  auto constexpr r = "<script>var x = 1; // comment\n</script>"_fs
    | frozenchars::ops::minify_html(opt);
  static_assert(r.sv() == "<script>var x = 1; // comment\n</script>");
  REQUIRE(r.sv() == "<script>var x = 1; // comment\n</script>");
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

TEST_CASE("minify_json - エッジケース", "[minifier]")
{
  // 文字列内の // はコメント扱いにしない
  auto constexpr j1 = minify_json("{ \"a\": \"// not comment\" }"_fs);
  static_assert(j1.sv() == "{\"a\":\"// not comment\"}");
  REQUIRE(j1.sv() == "{\"a\":\"// not comment\"}");

  // 文字列内の /* */ も保持
  auto constexpr j2 = minify_json("{ \"a\": \"/* not comment */\" }"_fs);
  static_assert(j2.sv() == "{\"a\":\"/* not comment */\"}");
  REQUIRE(j2.sv() == "{\"a\":\"/* not comment */\"}");

  // 外部のコメント除去
  auto constexpr j3 = minify_json("{ /*c*/ \"k\":1 //e\n }"_fs);
  static_assert(j3.sv() == "{\"k\":1}");
  REQUIRE(j3.sv() == "{\"k\":1}");

  // エスケープされたクォートとバックスラッシュ
  auto constexpr j4 = minify_json("{ \"a\": \"He said \\\"Hi\\\"\\\\\" }"_fs);
  static_assert(j4.sv() == "{\"a\":\"He said \\\"Hi\\\"\\\\\"}");
  REQUIRE(j4.sv() == "{\"a\":\"He said \\\"Hi\\\"\\\\\"}");

  // Unicode エスケープの保持
  auto constexpr j5 = minify_json("{ \"s\": \"\\u2603\" }"_fs);
  static_assert(j5.sv() == "{\"s\":\"\\u2603\"}");
  REQUIRE(j5.sv() == "{\"s\":\"\\u2603\"}");

  // 配列・オブジェクト内のコメント・空白除去
  auto constexpr j6 = minify_json("{ /*c*/ \"arr\": [1, 2, /*x*/ 3] }"_fs);
  static_assert(j6.sv() == "{\"arr\":[1,2,3]}");
  REQUIRE(j6.sv() == "{\"arr\":[1,2,3]}");
}


// ═════════════════════════════════════════════════════════════════════════════
// importmap (<script type="importmap">) のランタイムテスト
//  constexpr 評価での膨張を避けるため、実行時バッファ版で検証する
// ═════════════════════════════════════════════════════════════════════════════

namespace {
// デバッグ用の簡易 importmap-minify 実装（テスト内で逐次ログ出力可能）
static std::string debug_minify_importmap(std::string_view inner)
{
  std::string out;
  bool in_str = false;
  bool escaped = false;
  size_t p = 0;
  size_t const j = inner.size();
  size_t step = 0;

  while (p < j) {
    ++step;
    if ((step & 0xffff) == 0) {
      // 大量ループになった場合に進行を観察できるよう stderr に出力
      std::cerr << "debug_minify_importmap step=" << step << " p=" << p << " j=" << j << "\n";
    }
    char const ch = inner[p];
    if (in_str) {
      out.push_back(ch);
      if (escaped) {
        escaped = false;
      } else if (ch == '\\') {
        escaped = true;
      } else if (ch == '"') {
        in_str = false;
      }
      ++p;
      continue;
    }
    if (ch == '"') {
      in_str = true;
      out.push_back(ch);
      ++p;
      continue;
    }
    if (ch == '/' && p + 1 < j && inner[p + 1] == '/') {
      p += 2;
      while (p < j && inner[p] != '\n') ++p;
      continue;
    }
    if (ch == '/' && p + 1 < j && inner[p + 1] == '*') {
      p += 2;
      while (p + 1 < j && !(inner[p] == '*' && inner[p + 1] == '/')) ++p;
      if (p + 1 < j) p += 2;
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch))) {
      ++p;
      continue;
    }
    out.push_back(ch);
    ++p;
  }
  std::cerr << "debug_minify_importmap finished steps=" << step << " out_len=" << out.size() << "\n";
  return out;
}
} // namespace


TEST_CASE("minify_html - <script type=importmap> JSON のエッジケース (runtime)", "[minifier]")
{
  // デバッグ版: ヘッダ実装ではなくテスト内のロジックで importmap 部分を処理して挙動を検証
  auto run_case = [](char const* html, std::string const& expected_inner){
    // find '>' after opening tag
    auto const s = std::string_view(html);
    auto const open = s.find('>');
    REQUIRE(open != std::string_view::npos);
    auto const close = s.find("</script>", open + 1);
    REQUIRE(close != std::string_view::npos);
    auto inner = s.substr(open + 1, close - (open + 1));

    // 実行時デバッグ minify
    auto out = debug_minify_importmap(inner);
    // inner 部分のみ比較
    REQUIRE(out == expected_inner);
  };

  run_case("<script type=\"importmap\">{\n  // comment\n  \"imports\": { \"x\": \"/x.js\" }\n}</script>",
           "{\"imports\":{\"x\":\"/x.js\"}}");

  run_case("<script type=\"importmap\">{ \"a\": \"//instring\", /*c*/ \"b\":2 }</script>",
           "{\"a\":\"//instring\",\"b\":2}");

  run_case("<script type=\"importmap\">{ \"s\": \"\\u2603\", \"t\": \"He \\\"X\\\"\" }</script>",
           "{\"s\":\"\\u2603\",\"t\":\"He \\\"X\\\"\"}");

  // 追加ケース: 深いネスト
  {
    std::string inner = "{";
    std::string expect = "{";
    int depth = 20;
    for (int i = 0; i < depth; ++i) {
      if (i) { inner += ","; expect += ","; }
      inner += "\"k" + std::to_string(i) + "\":{";
      expect += "\"k" + std::to_string(i) + "\":{";
    }
    inner += "\"leaf\": 1";
    expect += "\"leaf\":1";
    for (int i = 0; i < depth; ++i) { inner += "}"; expect += "}"; }
    std::string html = "<script type=\"importmap\">" + inner + "</script>";
    run_case(html.c_str(), expect);
  }

  // 追加ケース: 大きな配列 + コメント混入
  {
    int N = 200;
    std::string inner = "{\"arr\":[";
    std::string expect = "{\"arr\":[";
    for (int i = 1; i <= N; ++i) {
      if (i > 1) { inner += ", "; expect += ","; }
      inner += std::to_string(i);
      inner += " /*c*/";
      expect += std::to_string(i);
    }
    inner += "]}";
    expect += "]}";
    std::string html = "<script type=\"importmap\">" + inner + "</script>";
    run_case(html.c_str(), expect);
  }

  // 追加ケース: エスケープ・改行・コメントを含む文字列
  {
    std::string inner = "{ \"s\": \"Line1\\nLine2\\\\\\\"\\\"\", /*c*/ \"n\": 2 }";
    std::string expect = "{\"s\":\"Line1\\nLine2\\\\\\\"\\\"\",\"n\":2}";
    std::string html = "<script type=\"importmap\">" + inner + "</script>";
    run_case(html.c_str(), expect);
  }

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

TEST_CASE("minify_sql - DECIMAL 短縮", "[minifier]")
{
  auto constexpr sql_dec = minify_sql(
    "SELECT  DECIMAL(10,2)  FROM  tbl"_fs);
  static_assert(sql_dec.sv() == "SELECT DEC(10,2) FROM tbl");
  REQUIRE(sql_dec.sv() == "SELECT DEC(10,2) FROM tbl");
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

namespace {
/** @brief minify 出力のバッファ末尾がゼロ埋めされていることを検証する。
    @details size() が実コンテンツ長を返し、未使用領域がゼロであることを確認する。
    NTTP 変換時に末尾 NULL が出力に混入しないことの回帰チェック。
    @tparam N バッファサイズ
    @param s 検証対象の FrozenString
    @return 未使用領域がすべてゼロの場合 true */
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

TEST_CASE("minify ops - size() は実長を返しバッファ末尾はゼロ", "[minifier][regression]")
{
  // size() は入力容量ではなく minify 後の実コンテンツ長を返すこと
  static_assert(("MATCH (n) RETURN n"_fs | frozenchars::ops::minify_cypher).size() == 17);
  static_assert(("<div>  hi  </div>"_fs | frozenchars::ops::minify_html).size() == 13);
  static_assert(("<root>  x  </root>"_fs | frozenchars::ops::minify_xml).size() == 14);
  static_assert(("{ \"a\" : 1 }"_fs | frozenchars::ops::minify_json).size() == 7);
  static_assert(("a: 1\nb: 2"_fs | frozenchars::ops::minify_yaml).size() == 9);
  static_assert(("SELECT * FROM t"_fs | frozenchars::ops::minify_sql).size() == 15);
  static_assert(("var x = 1; // c\n var y = 2;"_fs | frozenchars::ops::minify_js).size() == 19);

  // size() が capacity より小さく、末尾以降のバッファがゼロであること
  auto constexpr c = "MATCH (n) RETURN n"_fs | frozenchars::ops::minify_cypher;
  static_assert(c.size() < c.buffer.size());
  static_assert(buffer_trailing_zero(c));

  auto constexpr h = "<div>  hi  </div>"_fs | frozenchars::ops::minify_html;
  static_assert(buffer_trailing_zero(h));

  auto constexpr j = "{ \"a\" : 1 }"_fs | frozenchars::ops::minify_json;
  static_assert(buffer_trailing_zero(j));

  REQUIRE(c.size() == 17);
  REQUIRE(h.size() == 13);
  REQUIRE(j.size() == 7);
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

TEST_CASE("minify_xml - 属性値のクォートは削除しない", "[minifier]")
{
  auto constexpr result = R"(<div class="foo" data-x="1">)"_fs
    | frozenchars::ops::minify_xml;
  static_assert(result.sv() == R"(<div class="foo" data-x="1">)");
  REQUIRE(result.sv() == R"(<div class="foo" data-x="1">)");
}

TEST_CASE("minify_xml - <style> 内の CSS を minify する", "[minifier]")
{
  // <style> 内の CSS コメント除去 + 空白詰め
  auto constexpr result = "<svg><style>/* comment */.foo { color: red; }</style></svg>"_fs
    | frozenchars::ops::minify_xml;
  static_assert(result.sv() == "<svg><style>.foo{color:red}</style></svg>");
  REQUIRE(result.sv() == "<svg><style>.foo{color:red}</style></svg>");
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

// ═════════════════════════════════════════════════════════════════════════════
// JS minify — 実行時検証（TEST_CASE）
// ═════════════════════════════════════════════════════════════════════════════

TEST_CASE("minify_js - 基本", "[minifier]")
{
  SECTION("行コメント除去")
  {
    auto constexpr r = minify_js("var x = 1; // comment\n var y = 2;");
    static_assert(r.sv() == "var x = 1;var y = 2");
    REQUIRE(r.sv() == "var x = 1;var y = 2");
  }

  SECTION("ブロックコメント除去")
  {
    auto constexpr r = minify_js("var x = /* block */ 1;");
    static_assert(r.sv() == "var x = 1");
    REQUIRE(r.sv() == "var x = 1");
  }

  SECTION("文字列保護")
  {
    auto constexpr r = minify_js("var url = \"http://example.com\";");
    static_assert(r.sv() == "var url =\"http://example.com\"");
    REQUIRE(r.sv() == "var url =\"http://example.com\"");
  }

  SECTION("テンプレートリテラル")
  {
    auto constexpr r = minify_js("var s = `hello ${name}`;");
    static_assert(r.sv() == "var s =`hello ${name}`");
    REQUIRE(r.sv() == "var s =`hello ${name}`");
  }

  SECTION("エスケープシーケンス")
  {
    auto constexpr r = minify_js("var s = \"hello \\\"world\\\"\";");
    static_assert(r.sv() == "var s =\"hello \\\"world\\\"\"");
    REQUIRE(r.sv() == "var s =\"hello \\\"world\\\"\"");
  }
}

TEST_CASE("minify_js - ops パイプ演算子", "[minifier]")
{
  SECTION("FrozenString からパイプ")
  {
    auto constexpr result = "var x = 1; // comment\n var y = 2;"_fs
      | frozenchars::ops::minify_js;
    static_assert(result.sv() == "var x = 1;var y = 2");
    REQUIRE(result.sv() == "var x = 1;var y = 2");
  }

  SECTION("空文字列")
  {
    auto constexpr result = ""_fs | frozenchars::ops::minify_js;
    static_assert(result.sv() == "");
    REQUIRE(result.sv() == "");
  }

  SECTION("複合パイプ: trim + minify_js")
  {
    auto constexpr result = "  var x = 1;  "_fs
      | frozenchars::ops::trim
      | frozenchars::ops::minify_js;
    static_assert(result.sv() == "var x = 1");
    REQUIRE(result.sv() == "var x = 1");
  }
}

TEST_CASE("minify_js - size() 検証", "[minifier]")
{
  auto constexpr r = "var x = /* comment */ 1;"_fs | frozenchars::ops::minify_js;
  static_assert(r.sv() == "var x = 1");
  static_assert(r.size() == 9);                // "var x = 1" = 9 chars
  REQUIRE(r.size() == 9);
}
