#include "catch2/catch_all.hpp"

#include "frozenchars/ops.hpp"

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

TEST_CASE("encoding: is_valid_utf8", "[encoding]") {
  STATIC_CHECK(is_valid_utf8(""_fs));
  STATIC_CHECK(is_valid_utf8("hello"_fs));
  STATIC_CHECK(is_valid_utf8("a\xC3\xA9"_fs));  // 2-byte é
  STATIC_CHECK(is_valid_utf8("\xE3\x81\x82"_fs));  // 3-byte あ
  STATIC_CHECK(is_valid_utf8("\xF0\x9F\x98\x80"_fs));  // 4-byte 😀
  STATIC_CHECK(is_valid_utf8("aあ😀"_fs));
  // 不正ケース
  STATIC_CHECK(!is_valid_utf8("\xFF"_fs));
  STATIC_CHECK(!is_valid_utf8("\x80"_fs));  // 単独継続バイト
  STATIC_CHECK(!is_valid_utf8("\xC0\x80"_fs));  // オーバーロング
  STATIC_CHECK(!is_valid_utf8("\xE0\x80\x80"_fs));  // オーバーロング 3-byte
  STATIC_CHECK(!is_valid_utf8("\xED\xA0\x80"_fs));  // サロゲート
  STATIC_CHECK(!is_valid_utf8("\xF4\x90\x80\x80"_fs));  // U+110000 超え
  STATIC_CHECK(!is_valid_utf8("\xC2"_fs));  // 途中で切れた 2-byte
  STATIC_CHECK(!is_valid_utf8("\xE3\x81"_fs));  // 途中で切れた 3-byte
  STATIC_CHECK(!is_valid_utf8("\xF0\x9F\x98"_fs));  // 途中で切れた 4-byte
  // 文字列リテラル版
  STATIC_CHECK(is_valid_utf8("hello"));
  STATIC_CHECK(!is_valid_utf8("\xFF"));
  // NTTP版
  STATIC_CHECK(is_valid_utf8<""_fs>());
  STATIC_CHECK(is_valid_utf8<"hello"_fs>());
  STATIC_CHECK(!is_valid_utf8<"\xFF"_fs>());
  STATIC_CHECK(!is_valid_utf8<"\xC0\x80"_fs>());
}

TEST_CASE("encoding: utf8_to_codepoints", "[encoding]") {
  // 空
  {
    constexpr auto arr = utf8_to_codepoints(""_fs);
    (void)arr;
    constexpr size_t n = [] {
      size_t c = 0;
      (void)utf8_to_codepoints(""_fs, c);
      return c;
    }();
    STATIC_CHECK(n == 0);
  }
  // ASCII
  {
    constexpr auto arr = utf8_to_codepoints("ABC"_fs);
    STATIC_CHECK(arr[0] == U'A' && arr[1] == U'B' && arr[2] == U'C');
    constexpr size_t n = [] {
      size_t c = 0;
      auto a = utf8_to_codepoints("ABC"_fs, c);
      (void)a;
      return c;
    }();
    STATIC_CHECK(n == 3);
  }
  // 2/3/4-byte 混在
  {
    constexpr auto s = "aあ😀"_fs;  // 1 + 3 + 4 バイト、3符号点
    constexpr auto arr = utf8_to_codepoints(s);
    STATIC_CHECK(arr[0] == U'a');
    STATIC_CHECK(arr[1] == U'\u3042');
    STATIC_CHECK(arr[2] == U'\U0001F600');
    constexpr size_t n = [] {
      constexpr auto inner = "aあ😀"_fs;
      size_t c = 0;
      auto a = utf8_to_codepoints(inner, c);
      (void)a;
      return c;
    }();
    STATIC_CHECK(n == 3);
    // 未使用要素は 0
    STATIC_CHECK(arr[3] == U'\0');
  }
  // 文字列リテラル版
  {
    constexpr auto arr = utf8_to_codepoints("hi");
    STATIC_CHECK(arr[0] == U'h' && arr[1] == U'i');
  }
  // NTTP版
  {
    constexpr auto arr = utf8_to_codepoints<"aあ"_fs>();
    STATIC_CHECK(arr[0] == U'a' && arr[1] == U'\u3042');
  }
  // char32_t 型であること
  {
    constexpr auto arr = utf8_to_codepoints("A"_fs);
    STATIC_CHECK(std::is_same_v<decltype(arr)::value_type, char32_t>);
  }
  // 不正バイトはフェイルソフト（1バイト=1符号点）
  {
    constexpr auto arr = utf8_to_codepoints("\xFF\x80"_fs);
    // \xFF, \x80 がそれぞれ1符号点として扱われる
    STATIC_CHECK(arr[0] == static_cast<char32_t>(0xFF));
  }
  // strict NTTP版（有効のみ）
  {
    constexpr auto arr = utf8_to_codepoints_strict<"hello"_fs>();
    STATIC_CHECK(arr[0] == U'h');
  }
}

TEST_CASE("encoding: try_utf8_to_codepoints", "[encoding]") {
  // 有効
  {
    constexpr auto ok = [] {
      size_t c = 0;
      bool ok = false;
      auto a = try_utf8_to_codepoints("あ"_fs, c, ok);
      return ok && c == 1 && a[0] == U'\u3042';
    }();
    STATIC_CHECK(ok);
  }
  // 無効
  {
    constexpr auto bad = [] {
      size_t c = 0;
      bool ok = true;
      auto a = try_utf8_to_codepoints("\xFF"_fs, c, ok);
      (void)a;
      (void)c;
      return !ok;
    }();
    STATIC_CHECK(bad);
  }
  // 文字列リテラル版
  {
    size_t c = 0;
    bool ok = false;
    auto a = try_utf8_to_codepoints("hi", c, ok);
    CHECK(ok);
    CHECK(c == 2);
    CHECK(a[0] == U'h');
  }
}

TEST_CASE("encoding: codepoints_to_utf8", "[encoding]") {
  // 空
  {
    constexpr std::array<char32_t, 1> arr{};
    constexpr auto s = codepoints_to_utf8(arr, 0);
    STATIC_CHECK(s.sv() == "");
  }
  // ASCII
  {
    constexpr std::array<char32_t, 3> arr{U'A', U'B', U'C'};
    constexpr auto s = codepoints_to_utf8(arr, 3);
    STATIC_CHECK(s.sv() == "ABC");
  }
  // 2/3/4-byte 混在
  {
    constexpr std::array<char32_t, 3> arr{U'a', U'\u3042', U'\U0001F600'};
    constexpr auto s = codepoints_to_utf8(arr, 3);
    STATIC_CHECK(s.sv() == "aあ😀");
  }
  // C配列版
  {
    constexpr char32_t cs[] = {U'a', U'\u3042'};
    constexpr auto s = codepoints_to_utf8(cs, 2);
    STATIC_CHECK(s.sv() == "aあ");
  }
  // round-trip
  {
    constexpr auto rt = [] {
      constexpr auto inner = "aあ😀"_fs;
      size_t c = 0;
      auto arr = utf8_to_codepoints(inner, c);
      return codepoints_to_utf8(arr, c);
    }();
    STATIC_CHECK(rt.sv() == "aあ😀");
  }
  // 部分カウント
  {
    constexpr std::array<char32_t, 5> arr{U'h', U'e', U'l', U'l', U'o'};
    constexpr auto s = codepoints_to_utf8(arr, 2);
    STATIC_CHECK(s.sv() == "he");
  }
}
