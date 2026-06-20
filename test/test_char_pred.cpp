#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("char_pred: is_upper") {
  static_assert(is_upper('A'));
  static_assert(is_upper('Z'));
  static_assert(!is_upper('a'));
  static_assert(!is_upper('0'));
  static_assert(!is_upper(' '));

  REQUIRE(is_upper('A'));
  REQUIRE(!is_upper('a'));
}

TEST_CASE("char_pred: is_lower") {
  static_assert(is_lower('a'));
  static_assert(is_lower('z'));
  static_assert(!is_lower('A'));
  static_assert(!is_lower('0'));

  REQUIRE(is_lower('z'));
  REQUIRE(!is_lower('Z'));
}

TEST_CASE("char_pred: is_alpha") {
  static_assert(is_alpha('a'));
  static_assert(is_alpha('Z'));
  static_assert(!is_alpha('0'));
  static_assert(!is_alpha('_'));

  REQUIRE(is_alpha('m'));
  REQUIRE(!is_alpha('5'));
}

TEST_CASE("char_pred: is_digit") {
  static_assert(is_digit('0'));
  static_assert(is_digit('9'));
  static_assert(!is_digit('a'));
  static_assert(!is_digit('/'));

  REQUIRE(is_digit('5'));
  REQUIRE(!is_digit('F'));
}

TEST_CASE("char_pred: is_alnum") {
  static_assert(is_alnum('a'));
  static_assert(is_alnum('Z'));
  static_assert(is_alnum('5'));
  static_assert(!is_alnum('!'));
  static_assert(!is_alnum(' '));

  REQUIRE(is_alnum('x'));
  REQUIRE(!is_alnum('.'));
}

TEST_CASE("char_pred: is_xdigit") {
  static_assert(is_xdigit('0'));
  static_assert(is_xdigit('9'));
  static_assert(is_xdigit('a'));
  static_assert(is_xdigit('f'));
  static_assert(is_xdigit('A'));
  static_assert(is_xdigit('F'));
  static_assert(!is_xdigit('g'));
  static_assert(!is_xdigit('G'));
  static_assert(!is_xdigit('z'));

  REQUIRE(is_xdigit('E'));
  REQUIRE(!is_xdigit('X'));
}

TEST_CASE("char_pred: is_cntrl") {
  static_assert(is_cntrl('\0'));
  static_assert(is_cntrl('\t'));
  static_assert(is_cntrl('\n'));
  static_assert(is_cntrl('\x1F'));
  static_assert(is_cntrl('\x7F'));
  static_assert(!is_cntrl(' '));
  static_assert(!is_cntrl('A'));

  REQUIRE(is_cntrl('\0'));
  REQUIRE(!is_cntrl(' '));
}

TEST_CASE("char_pred: is_graph") {
  static_assert(is_graph('!'));
  static_assert(is_graph('~'));
  static_assert(is_graph('A'));
  static_assert(is_graph('0'));
  static_assert(!is_graph(' '));
  static_assert(!is_graph('\n'));

  REQUIRE(is_graph('#'));
  REQUIRE(!is_graph(' '));
}

TEST_CASE("char_pred: is_print") {
  static_assert(is_print(' '));
  static_assert(is_print('~'));
  static_assert(is_print('A'));
  static_assert(!is_print('\t'));
  static_assert(!is_print('\x7F'));

  REQUIRE(is_print(' '));
  REQUIRE(!is_print('\n'));
}

TEST_CASE("char_pred: is_punct") {
  static_assert(is_punct('!'));
  static_assert(is_punct('.'));
  static_assert(is_punct('~'));
  static_assert(is_punct('{'));
  static_assert(is_punct('_'));
  static_assert(!is_punct('a'));
  static_assert(!is_punct(' '));
  static_assert(!is_punct('0'));

  REQUIRE(is_punct('?'));
  REQUIRE(is_punct('_'));
  REQUIRE(!is_punct('a'));
}

TEST_CASE("char_pred: is_blank") {
  static_assert(is_blank(' '));
  static_assert(is_blank('\t'));
  static_assert(!is_blank('\n'));
  static_assert(!is_blank('a'));

  REQUIRE(is_blank(' '));
  REQUIRE(!is_blank('\r'));
}

TEST_CASE("char_pred: is_space") {
  static_assert(is_space(' '));
  static_assert(is_space('\t'));
  static_assert(is_space('\n'));
  static_assert(is_space('\r'));
  static_assert(is_space('\f'));
  static_assert(is_space('\v'));
  static_assert(!is_space('a'));
  static_assert(!is_space('_'));

  REQUIRE(is_space('\n'));
  REQUIRE(!is_space('X'));
}

TEST_CASE("char_pred: all categories cover ASCII without overlap for standard chars") {
  // 'A' should be only alpha/alnum/upper/graph/print/xdigit
  REQUIRE(is_alpha('A'));
  REQUIRE(is_upper('A'));
  REQUIRE(!is_lower('A'));
  REQUIRE(is_alnum('A'));
  REQUIRE(is_graph('A'));
  REQUIRE(is_print('A'));
  REQUIRE(is_xdigit('A'));
  REQUIRE(!is_digit('A'));
  REQUIRE(!is_punct('A'));
  REQUIRE(!is_space('A'));
  REQUIRE(!is_cntrl('A'));
  REQUIRE(!is_blank('A'));

  // '_' should be only punct/graph/print
  REQUIRE(is_punct('_'));
  REQUIRE(is_graph('_'));
  REQUIRE(is_print('_'));
  REQUIRE(!is_alnum('_'));
  REQUIRE(!is_space('_'));
  REQUIRE(!is_cntrl('_'));

  // ' ' should be only space/blank/print
  REQUIRE(is_space(' '));
  REQUIRE(is_blank(' '));
  REQUIRE(is_print(' '));
  REQUIRE(!is_graph(' '));
  REQUIRE(!is_alnum(' '));
  REQUIRE(!is_punct(' '));
  REQUIRE(!is_cntrl(' '));
}

TEST_CASE("html_encode: basic characters") {
  auto constexpr e1 = html_encode("<");
  static_assert(e1.sv() == "&lt;");
  REQUIRE(e1.sv() == "&lt;");

  auto constexpr e2 = html_encode(">");
  static_assert(e2.sv() == "&gt;");
  REQUIRE(e2.sv() == "&gt;");

  auto constexpr e3 = html_encode("&");
  static_assert(e3.sv() == "&amp;");
  REQUIRE(e3.sv() == "&amp;");

  auto constexpr e4 = html_encode("\"");
  static_assert(e4.sv() == "&quot;");
  REQUIRE(e4.sv() == "&quot;");

  auto constexpr e5 = html_encode("'");
  static_assert(e5.sv() == "&#39;");
  REQUIRE(e5.sv() == "&#39;");
}

TEST_CASE("html_encode: mixed string") {
  auto constexpr e = html_encode("<hello> & \"world\""_fs);
  static_assert(e.sv() == "&lt;hello&gt; &amp; &quot;world&quot;");
  REQUIRE(e.sv() == "&lt;hello&gt; &amp; &quot;world&quot;");

  auto constexpr e2 = html_encode("a'b"_fs);
  static_assert(e2.sv() == "a&#39;b");
  REQUIRE(e2.sv() == "a&#39;b");
}

TEST_CASE("html_encode: no special chars") {
  auto constexpr e = html_encode("hello world"_fs);
  static_assert(e.sv() == "hello world");
  REQUIRE(e.sv() == "hello world");
}

TEST_CASE("html_encode: empty string") {
  auto constexpr e = html_encode(""_fs);
  static_assert(e.sv() == "");
  REQUIRE(e.sv() == "");
}

TEST_CASE("html_encode: NTTP version") {
  auto constexpr e = html_encode<"<hello>"_fs>();
  static_assert(e.sv() == "&lt;hello&gt;");
  REQUIRE(e.sv() == "&lt;hello&gt;");
}

TEST_CASE("html_decode: basic entities") {
  auto constexpr d1 = html_decode("&lt;"_fs);
  static_assert(d1.sv() == "<");
  REQUIRE(d1.sv() == "<");

  auto constexpr d2 = html_decode("&gt;"_fs);
  static_assert(d2.sv() == ">");
  REQUIRE(d2.sv() == ">");

  auto constexpr d3 = html_decode("&amp;"_fs);
  static_assert(d3.sv() == "&");
  REQUIRE(d3.sv() == "&");

  auto constexpr d4 = html_decode("&quot;"_fs);
  static_assert(d4.sv() == "\"");
  REQUIRE(d4.sv() == "\"");

  auto constexpr d5 = html_decode("&#39;"_fs);
  static_assert(d5.sv() == "'");
  REQUIRE(d5.sv() == "'");
}

TEST_CASE("html_decode: mixed string") {
  auto constexpr d = html_decode("&lt;hello&gt; &amp; &quot;world&quot;"_fs);
  static_assert(d.sv() == "<hello> & \"world\"");
  REQUIRE(d.sv() == "<hello> & \"world\"");
}

TEST_CASE("html_decode: no entities") {
  auto constexpr d = html_decode("hello world"_fs);
  static_assert(d.sv() == "hello world");
  REQUIRE(d.sv() == "hello world");
}

TEST_CASE("html_decode: empty string") {
  auto constexpr d = html_decode(""_fs);
  static_assert(d.sv() == "");
  REQUIRE(d.sv() == "");
}

TEST_CASE("html_decode: unknown entity passes through") {
  auto constexpr d = html_decode("foo &unknown; bar"_fs);
  static_assert(d.sv() == "foo &unknown; bar");
  REQUIRE(d.sv() == "foo &unknown; bar");
}

TEST_CASE("html_decode: NTTP version") {
  auto constexpr d = html_decode<"&lt;hello&gt;"_fs>();
  static_assert(d.sv() == "<hello>");
  REQUIRE(d.sv() == "<hello>");
}

TEST_CASE("html_encode/decode: roundtrip") {
  auto constexpr original = "hello <world> & \"foo\" 'bar'"_fs;
  auto constexpr encoded = html_encode(original);
  auto constexpr decoded = html_decode(encoded);
  static_assert(decoded.sv() == original.sv());
  REQUIRE(decoded.sv() == original.sv());
}

TEST_CASE("html_encode: pipe adaptor") {
  namespace fops = frozenchars::ops;
  auto constexpr e = "<test>"_fs | fops::html_encode;
  static_assert(e.sv() == "&lt;test&gt;");
  REQUIRE(e.sv() == "&lt;test&gt;");
}

TEST_CASE("html_decode: pipe adaptor") {
  namespace fops = frozenchars::ops;
  auto constexpr d = "&lt;test&gt;"_fs | fops::html_decode;
  static_assert(d.sv() == "<test>");
  REQUIRE(d.sv() == "<test>");
}

TEST_CASE("html_encode: string literal") {
  auto constexpr e = html_encode("<test>");
  static_assert(e.sv() == "&lt;test&gt;");
  REQUIRE(e.sv() == "&lt;test&gt;");
}

TEST_CASE("word_wrap: short text no wrap needed") {
  auto constexpr w = word_wrap("hello world", 80);
  static_assert(w.sv() == "hello world");
  REQUIRE(w.sv() == "hello world");
}

TEST_CASE("word_wrap: wrap at word boundary") {
  auto constexpr w = word_wrap("hello world foo bar", 10);
  REQUIRE(w.sv() == "hello\nworld foo\nbar");
}

TEST_CASE("word_wrap: preserve existing newlines") {
  auto constexpr w = word_wrap("hello\nworld foo bar", 10);
  REQUIRE(w.sv() == "hello\nworld foo\nbar");
}

TEST_CASE("word_wrap: width 1") {
  auto constexpr w = word_wrap("a b c", 1);
  REQUIRE(w.sv() == "a\nb\nc");
}

TEST_CASE("word_wrap: width 0 treated as 1") {
  auto constexpr w = word_wrap("a b", 0);
  REQUIRE(w.sv() == "a\nb");
}

TEST_CASE("word_wrap: empty string") {
  auto constexpr w = word_wrap(""_fs, 10);
  static_assert(w.sv() == "");
  REQUIRE(w.sv() == "");
}

TEST_CASE("word_wrap: single word longer than width") {
  auto constexpr w = word_wrap("hello"_fs, 3);
  static_assert(w.sv() == "hello");
  REQUIRE(w.sv() == "hello");
}

TEST_CASE("word_wrap: multiple spaces compressed") {
  auto constexpr w = word_wrap("hello    world", 5);
  REQUIRE(w.sv() == "hello\nworld");
}

TEST_CASE("word_wrap: leading and trailing spaces") {
  auto constexpr w = word_wrap("  hello world  ", 20);
  REQUIRE(w.sv() == "hello world");
}

TEST_CASE("word_wrap: long string") {
  auto constexpr w = word_wrap("the quick brown fox jumps over the lazy dog", 10);
  REQUIRE(w.sv() == "the quick\nbrown fox\njumps over\nthe lazy\ndog");
}

TEST_CASE("word_wrap: pipe adaptor") {
  namespace fops = frozenchars::ops;
  auto constexpr w = "hello world foo"_fs | fops::word_wrap(8);
  REQUIRE(w.sv() == "hello\nworld\nfoo");
}

TEST_CASE("word_wrap: string literal") {
  auto constexpr w = word_wrap("hello world foo", 8);
  REQUIRE(w.sv() == "hello\nworld\nfoo");
}

TEST_CASE("utf8_length: ASCII only") {
  auto constexpr len = utf8_length("hello"_fs);
  static_assert(len == 5);
  REQUIRE(len == 5);
}

TEST_CASE("utf8_length: empty string") {
  auto constexpr len = utf8_length(""_fs);
  static_assert(len == 0);
  REQUIRE(len == 0);
}

TEST_CASE("utf8_length: multi-byte characters") {
  // 2-byte: \xC3\xA9 = é
  // 3-byte: \xE3\x81\x82 = あ
  // 4-byte: \xF0\x9F\x90\xB1 = 🐱 (cat emoji UTF-8)
  auto constexpr len = utf8_length("\xC3\xA9\xE3\x81\x82\xF0\x9F\x90\xB1"_fs);
  static_assert(len == 3);
  REQUIRE(len == 3);
}

TEST_CASE("utf8_length: mixed ASCII and multi-byte") {
  // "Hello 世界" = H(1) e(1) l(1) l(1) o(1) space(1) 世(3) 界(3) = 8 codepoints
  auto constexpr len = utf8_length("Hello \xE4\xB8\x96\xE7\x95\x8C"_fs);
  static_assert(len == 8);
  REQUIRE(len == 8);
}

TEST_CASE("utf8_length: continuation bytes without leading byte") {
  // stray continuation byte \x80 should count as 1 codepoint
  auto constexpr len = utf8_length("\x80"_fs);
  REQUIRE(len == 1);
}

TEST_CASE("utf8_length: NTTP version") {
  auto constexpr len = utf8_length<"hello"_fs>();
  static_assert(len == 5);
  REQUIRE(len == 5);
}

TEST_CASE("utf8_length: string literal") {
  auto constexpr len = utf8_length("hello");
  static_assert(len == 5);
  REQUIRE(len == 5);
}
