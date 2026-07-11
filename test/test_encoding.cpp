#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

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
