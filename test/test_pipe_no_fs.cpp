#include "catch2/catch_all.hpp"
#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

/** @brief FrozenString を経由しない raw 文字列リテラル向けパイプ演算子アダプタのテスト。
    @details ステートレス・NTTP・ファクトリ・コンポーズド各アダプタの動作を検証する。*/

TEST_CASE("pipe: raw string | stateless adaptor", "[pipe][no_fs]") {
  SECTION("toupper") {
    STATIC_CHECK(("hello" | ops::toupper).sv() == "HELLO");
  }
  SECTION("tolower") {
    STATIC_CHECK(("HELLO" | ops::tolower).sv() == "hello");
  }
  SECTION("capitalize") {
    STATIC_CHECK(("hello world" | ops::capitalize).sv() == "Hello world");
  }
  SECTION("trim") {
    STATIC_CHECK(("  hello  " | ops::trim).sv() == "hello");
  }
  SECTION("ltrim") {
    STATIC_CHECK(("  hello  " | ops::ltrim).sv() == "hello  ");
  }
  SECTION("rtrim") {
    STATIC_CHECK(("  hello  " | ops::rtrim).sv() == "  hello");
  }
  SECTION("url_encode") {
    STATIC_CHECK(("hello world" | ops::url_encode).sv() == "hello%20world");
  }
  SECTION("url_decode") {
    STATIC_CHECK(("hello%20world" | ops::url_decode).sv() == "hello world");
  }
  SECTION("base64_encode") {
    STATIC_CHECK(("hello" | ops::base64_encode).sv() == "aGVsbG8=");
  }
  SECTION("base64_decode") {
    STATIC_CHECK(("aGVsbG8=" | ops::base64_decode).sv() == "hello");
  }
  SECTION("hex_encode") {
    STATIC_CHECK(("hello" | ops::hex_encode).sv() == "68656C6C6F");
  }
  SECTION("hex_decode") {
    STATIC_CHECK(("68656C6C6F" | ops::hex_decode).sv() == "hello");
  }
  SECTION("html_encode") {
    STATIC_CHECK(("<tag>" | ops::html_encode).sv() == "&lt;tag&gt;");
  }
  SECTION("html_decode") {
    STATIC_CHECK(("&lt;tag&gt;" | ops::html_decode).sv() == "<tag>");
  }
  SECTION("remove_empty_lines") {
    STATIC_CHECK(("a\n\nb" | ops::remove_empty_lines).sv() == "a\nb");
  }
  SECTION("collapse_empty_lines") {
    STATIC_CHECK(("a\n\n\nb" | ops::collapse_empty_lines).sv() == "a\n\nb");
  }
  SECTION("to_snake_case") {
    STATIC_CHECK(("helloWorld" | ops::to_snake_case).sv() == "hello_world");
  }
  SECTION("to_camel_case") {
    STATIC_CHECK(("hello_world" | ops::to_camel_case).sv() == "helloWorld");
  }
  SECTION("to_pascal_case") {
    STATIC_CHECK(("hello_world" | ops::to_pascal_case).sv() == "HelloWorld");
  }
  SECTION("trim_trailing_spaces") {
    STATIC_CHECK(("hello  \nworld" | ops::trim_trailing_spaces).sv() == "hello\nworld");
  }
  SECTION("collapse_spaces") {
    STATIC_CHECK(("a   b" | ops::collapse_spaces).sv() == "a b");
  }
}

TEST_CASE("pipe: raw string | NTTP adaptor", "[pipe][no_fs]") {
  SECTION("contains") {
    STATIC_CHECK("hello world" | ops::contains<"world">);
    STATIC_CHECK(!("hello world" | ops::contains<"xyz">));
  }
  SECTION("starts_with") {
    STATIC_CHECK("hello" | ops::starts_with<"hel">);
    STATIC_CHECK(!("hello" | ops::starts_with<"llo">));
  }
  SECTION("ends_with") {
    STATIC_CHECK("hello" | ops::ends_with<"llo">);
    STATIC_CHECK(!("hello" | ops::ends_with<"hel">));
  }
  SECTION("replace") {
    STATIC_CHECK(("hello world" | ops::replace<"world", "there">).sv() == "hello there");
  }
  SECTION("replace_all") {
    STATIC_CHECK(("foo bar foo" | ops::replace_all<"foo", "baz">).sv() == "baz bar baz");
  }
  SECTION("pad_left") {
    STATIC_CHECK(("hello" | ops::pad_left<8>).sv() == "   hello");
  }
  SECTION("pad_right") {
    STATIC_CHECK(("hello" | ops::pad_right<8>).sv() == "hello   ");
  }
}

TEST_CASE("pipe: raw string | factory adaptor", "[pipe][no_fs]") {
  SECTION("substr") {
    STATIC_CHECK(("hello world" | ops::substr(0, 5)).sv() == "hello");
  }
  SECTION("word_wrap") {
    auto constexpr wrapped = "hello world" | ops::word_wrap(5);
    STATIC_CHECK(wrapped.sv() == "hello\nworld");
  }
  SECTION("join_lines") {
    STATIC_CHECK(("a\nb\nc" | ops::join_lines(" ")).sv() == "a b c");
  }
}

TEST_CASE("pipe: raw string | composed adaptor", "[pipe][no_fs]") {
  SECTION("toupper + trim") {
    auto constexpr normalize = compose(ops::trim, ops::toupper);
    STATIC_CHECK(("  hello  " | normalize).sv() == "HELLO");
  }
  SECTION("multi-step transform") {
    auto constexpr pipeline = compose(ops::trim, ops::tolower, ops::capitalize);
    STATIC_CHECK(("  HELLO WORLD  " | pipeline).sv() == "Hello world");
  }
  SECTION("url encode + decode roundtrip") {
    auto constexpr pipeline = compose(ops::url_encode, ops::url_decode);
    STATIC_CHECK(("a b c" | pipeline).sv() == "a b c");
  }
  SECTION("chained operator| without compose") {
    auto constexpr step1 = "  hello  " | ops::trim;
    auto constexpr step2 = step1 | ops::toupper;
    STATIC_CHECK(step2.sv() == "HELLO");
  }
}

TEST_CASE("pipe: raw string | adaptor with runtime state", "[pipe][no_fs]") {
  SECTION("remove_comments via explicit adaptor") {
    STATIC_CHECK(("a # comment\nb" | ops::remove_comments_adaptor{"#"}).sv() == "a \nb");
  }
  SECTION("remove_comment_lines via explicit adaptor") {
    STATIC_CHECK(("a\n# comment\nb" | ops::remove_comment_lines_adaptor{"#"}).sv() == "a\nb");
  }
  SECTION("remove_range_comments via explicit adaptor") {
    STATIC_CHECK(("a /* comment */ b" | ops::remove_range_comments_adaptor{"/*", "*/"}).sv() == "a  b");
  }
}

TEST_CASE("pipe: raw string | multiline adaptor", "[pipe][no_fs]") {
  SECTION("prefix_lines") {
    STATIC_CHECK(("a\nb" | ops::prefix_lines("> "_fs)).sv() == "> a\n> b");
  }
  SECTION("postfix_lines") {
    STATIC_CHECK(("a\nb" | ops::postfix_lines("!"_fs)).sv() == "a!\nb!");
  }
  SECTION("surround_lines") {
    STATIC_CHECK(("a\nb" | ops::surround_lines("<"_fs, ">"_fs)).sv() == "<a>\n<b>");
  }
  SECTION("linebreak conversion") {
    STATIC_CHECK(("a<br>b" | ops::br_to_nl).sv() == "a\nb");
  }
}

TEST_CASE("pipe: raw string | remove_leading_spaces / remove_trailing_spaces", "[pipe][no_fs]") {
  SECTION("remove_leading_spaces") {
    STATIC_CHECK(("  hello\n  world" | ops::remove_leading_spaces(2)).sv() == "hello\nworld");
  }
  SECTION("remove_trailing_spaces") {
    STATIC_CHECK(("hello  \nworld  " | ops::remove_trailing_spaces(2)).sv() == "hello\nworld");
  }
}

TEST_CASE("pipe: raw string | partition", "[pipe][no_fs]") {
  auto const [before, delim, after] = "a,b,c" | ops::partition<",">;
  CHECK(before.sv() == "a");
  CHECK(delim.sv() == ",");
  CHECK(after.sv() == "b,c");
}
