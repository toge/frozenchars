#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

/** @brief 16進数エンコード/デコード、HTML エンコード/デコード、UTF-8 長さ計算のテスト。 */

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("encoding: hex_encode") {
  STATIC_CHECK(hex_encode("ABC").sv() == "414243");
  STATIC_CHECK(hex_encode("").sv() == "");
  STATIC_CHECK(hex_encode("0").sv() == "30");

  constexpr auto str = "hi"_fs;
  STATIC_CHECK(hex_encode(str).sv() == "6869");
}

TEST_CASE("encoding: hex_decode") {
  STATIC_CHECK(hex_decode("414243").sv() == "ABC");
  STATIC_CHECK(hex_decode("6869").sv() == "hi");
  STATIC_CHECK(hex_decode("").sv() == "");
}

TEST_CASE("encoding: hex round-trip (NTTP)") {
  STATIC_CHECK(hex_decode<hex_encode<"frozenchars"_fs>()>().sv() == "frozenchars");
}

TEST_CASE("encoding: to_ascii / from_ascii aliases") {
  STATIC_CHECK(to_ascii("Hi").sv() == hex_encode("Hi").sv());
  STATIC_CHECK(from_ascii("4869").sv() == "Hi");
}

TEST_CASE("encoding: html_encode") {
  STATIC_CHECK(html_encode("<a href=\"x\">").sv() == "&lt;a href=&quot;x&quot;&gt;");
  STATIC_CHECK(html_encode("a & b").sv() == "a &amp; b");
  STATIC_CHECK(html_encode("it's").sv() == "it&#39;s");
  STATIC_CHECK(html_encode("plain").sv() == "plain");
}

TEST_CASE("encoding: html_decode") {
  STATIC_CHECK(html_decode("&lt;a&gt;").sv() == "<a>");
  STATIC_CHECK(html_decode("a &amp; b").sv() == "a & b");
  STATIC_CHECK(html_decode("&quot;q&quot;").sv() == "\"q\"");
  STATIC_CHECK(html_decode("&#39;").sv() == "'");
  STATIC_CHECK(html_decode("&#x27;").sv() == "'");
  STATIC_CHECK(html_decode("&unknown;").sv() == "&unknown;");
}

TEST_CASE("encoding: html round-trip (NTTP)") {
  STATIC_CHECK(html_decode<html_encode<"<tag attr='v'>"_fs>()>().sv() == "<tag attr='v'>");
}

TEST_CASE("encoding: utf8_length") {
  STATIC_CHECK(utf8_length("hello") == 5);
  STATIC_CHECK(utf8_length("") == 0);
  // 2-byte (U+00E9 é), 3-byte (U+3042 あ), 4-byte (U+1F600 😀)
  STATIC_CHECK(utf8_length("\xC3\xA9") == 1);
  STATIC_CHECK(utf8_length("\xE3\x81\x82") == 1);
  STATIC_CHECK(utf8_length("\xF0\x9F\x98\x80") == 1);
  STATIC_CHECK(utf8_length("a\xC3\xA9z") == 3);

  STATIC_CHECK(utf8_length<"\xE3\x81\x82\xE3\x81\x84"_fs>() == 2);
}

TEST_CASE("encoding: utf8 helpers") {
  STATIC_CHECK(utf8_substr<1, 2>("abcあいd"_fs).sv() == "bc");
  STATIC_CHECK(utf8_substr<1, 2>("AéB"_fs).sv() == "éB");
  STATIC_CHECK(utf8_reverse("abcあい"_fs).sv() == "いあcba");
  STATIC_CHECK(codepoint_to_utf8<0x41>().sv() == "A");
  STATIC_CHECK(codepoint_to_utf8<0x3042>().sv() == "\xE3\x81\x82");
  STATIC_CHECK(escape_c("line\nvalue"_fs).sv() == "line\\nvalue");
  STATIC_CHECK(unescape_c("line\\nvalue"_fs).sv() == "line\nvalue");
}

TEST_CASE("encoding: escape_c extended control escapes", "[encoding]") {
  STATIC_CHECK(escape_c("a\tb"_fs).sv() == "a\\tb");
  STATIC_CHECK(escape_c("\a\b\f\v"_fs).sv() == "\\a\\b\\f\\v");
  STATIC_CHECK(escape_c("a\x01\x7F"_fs).sv() == "a\\x01\\x7F");
  STATIC_CHECK(escape_c("it's \"quoted\""_fs).sv() == "it\\'s \\\"quoted\\\"");
}

TEST_CASE("encoding: escape_c unicode to \\u / \\U", "[encoding]") {
  STATIC_CHECK(escape_c("あ"_fs).sv() == "\\u3042");
  STATIC_CHECK(escape_c("😀"_fs).sv() == "\\U0001F600");
  STATIC_CHECK(escape_c("aあz"_fs).sv() == "a\\u3042z");
  // 不正 UTF-8 バイトは \xHH
  STATIC_CHECK(escape_c("\x80"_fs).sv() == "\\x80");
}

TEST_CASE("encoding: escape_c / unescape_c NTTP variants", "[encoding]") {
  STATIC_CHECK(escape_c<"a\tb"_fs>().sv() == "a\\tb");
  STATIC_CHECK(unescape_c<escape_c<"こんにちは 😀"_fs>()>().sv() == "こんにちは 😀");
}

TEST_CASE("encoding: unescape_c extended escapes", "[encoding]") {
  STATIC_CHECK(unescape_c("\\a\\b\\f\\v"_fs).sv() == "\a\b\f\v");
  STATIC_CHECK(unescape_c("\\x41\\x42"_fs).sv() == "AB");
  STATIC_CHECK(unescape_c("\\x1"_fs).sv() == "\x01");
  STATIC_CHECK(unescape_c("\\u3042"_fs).sv() == "あ");
  STATIC_CHECK(unescape_c("\\U0001F600"_fs).sv() == "\xF0\x9F\x98\x80");  // U+1F600 の UTF-8
  // 不正・範囲外シーケンスは保持（runtime 版は寛容）
  STATIC_CHECK(unescape_c("\\xZZ"_fs).sv() == "\\xZZ");
  STATIC_CHECK(unescape_c("\\uD800"_fs).sv() == "\\uD800");
  STATIC_CHECK(unescape_c("\\U00110000"_fs).sv() == "\\U00110000");
  STATIC_CHECK(unescape_c("\\u12"_fs).sv() == "\\u12");
}
