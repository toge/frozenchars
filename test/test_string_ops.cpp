#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("contains<Substr>(str) - NTTP version") {
  STATIC_CHECK("hello world"_fs | ops::contains<"world">);
  STATIC_CHECK(!("hello"_fs | ops::contains<"xyz">));
  STATIC_CHECK("abc"_fs | ops::contains<"abc">);
  STATIC_CHECK(!("abc"_fs | ops::contains<"abcd">));
}

TEST_CASE("contains<Substr>(str) - FrozenString version") {
  constexpr auto str       = "hello world"_fs;
  constexpr auto found     = "world"_fs;
  constexpr auto not_found = "xyz"_fs;
  STATIC_CHECK(contains<found>(str));
  STATIC_CHECK(!contains<not_found>(str));
}

TEST_CASE("contains<Substr>(str) - char[] version") {
  STATIC_CHECK(contains<"world">("hello world"));
  STATIC_CHECK(!contains<"xyz">("hello"));
}

TEST_CASE("starts_with<Prefix>(str) - NTTP version") {
  STATIC_CHECK("hello"_fs | ops::starts_with<"hel">);
  STATIC_CHECK(!("hello"_fs | ops::starts_with<"llo">));
  STATIC_CHECK("abc"_fs | ops::starts_with<"">);
  STATIC_CHECK(!("abc"_fs | ops::starts_with<"abcd">));
}

TEST_CASE("starts_with<Prefix>(str) - FrozenString version") {
  constexpr auto str        = "hello"_fs;
  constexpr auto prefix     = "hel"_fs;
  constexpr auto not_prefix = "xyz"_fs;
  STATIC_CHECK(starts_with<prefix>(str));
  STATIC_CHECK(!starts_with<not_prefix>(str));
}

TEST_CASE("starts_with<Prefix>(str) - char[] version") {
  STATIC_CHECK(starts_with<"hel">("hello"));
  STATIC_CHECK(!starts_with<"xyz">("hello"));
}

TEST_CASE("ends_with<Suffix>(str) - NTTP version") {
  STATIC_CHECK("hello"_fs | ops::ends_with<"llo">);
  STATIC_CHECK(!("hello"_fs | ops::ends_with<"hel">));
  STATIC_CHECK("abc"_fs | ops::ends_with<"">);
  STATIC_CHECK(!("abc"_fs | ops::ends_with<"abcd">));
}

TEST_CASE("ends_with<Suffix>(str) - FrozenString version") {
  constexpr auto str        = "hello"_fs;
  constexpr auto suffix     = "llo"_fs;
  constexpr auto not_suffix = "xyz"_fs;
  STATIC_CHECK(ends_with<suffix>(str));
  STATIC_CHECK(!ends_with<not_suffix>(str));
}

TEST_CASE("ends_with<Suffix>(str) - char[] version") {
  STATIC_CHECK(ends_with<"llo">("hello"));
  STATIC_CHECK(!ends_with<"xyz">("hello"));
}

TEST_CASE("partition<Delim>(str) - NTTP version") {
  auto const [before, delim, after] = "key=value"_fs | ops::partition<"=">;
  static_assert(std::get<0>("key=value"_fs | ops::partition<"=">).sv() == "key");
  static_assert(std::get<1>("key=value"_fs | ops::partition<"=">).sv() == "=");
  static_assert(std::get<2>("key=value"_fs | ops::partition<"=">).sv() == "value");
  REQUIRE(before.sv() == "key");
  REQUIRE(delim.sv() == "=");
  REQUIRE(after.sv() == "value");
}

TEST_CASE("partition<Delim>(str) - no match") {
  auto const [before, delim, after] = "hello"_fs | ops::partition<"=">;
  static_assert(std::get<0>("hello"_fs | ops::partition<"=">).sv() == "hello");
  static_assert(std::get<1>("hello"_fs | ops::partition<"=">).sv() == "");
  static_assert(std::get<2>("hello"_fs | ops::partition<"=">).sv() == "");
  REQUIRE(before.sv() == "hello");
  REQUIRE(delim.sv() == "");
  REQUIRE(after.sv() == "");
}

TEST_CASE("partition<Delim>(str) - multiple delimiters") {
  auto const [before, delim, after] = "a=b=c"_fs | ops::partition<"=">;
  static_assert(std::get<0>("a=b=c"_fs | ops::partition<"=">).sv() == "a");
  static_assert(std::get<1>("a=b=c"_fs | ops::partition<"=">).sv() == "=");
  static_assert(std::get<2>("a=b=c"_fs | ops::partition<"=">).sv() == "b=c");
  REQUIRE(before.sv() == "a");
  REQUIRE(delim.sv() == "=");
  REQUIRE(after.sv() == "b=c");
}

TEST_CASE("convert_linebreak - <br> variants", "[string_ops]") {
  using namespace frozenchars;

  auto constexpr input = "a<br>b<BR>c<br/>d<br />e<Br>f<bR/>g<BR />h"_fs;
  auto constexpr result = convert_linebreak<LineBreak::Br, LineBreak::Nl>(input);
  static_assert(result.sv() == "a\nb\nc\nd\ne\nf\ng\nh");
}

TEST_CASE("convert_linebreak - mutual conversion", "[string_ops]") {
  using namespace frozenchars;

  // Br -> Nl
  static_assert(convert_linebreak<LineBreak::Br, LineBreak::Nl>("a<br>b"_fs).sv() == "a\nb");
  // Nl -> Br
  static_assert(convert_linebreak<LineBreak::Nl, LineBreak::Br>("a\nb"_fs).sv() == "a<br>b");
  // Br -> EscN
  static_assert(convert_linebreak<LineBreak::Br, LineBreak::EscN>("a<br>b"_fs).sv() == "a\\nb");
  // EscN -> Br
  static_assert(convert_linebreak<LineBreak::EscN, LineBreak::Br>(R"(a\nb)"_fs).sv() == "a<br>b");
  // Nl -> EscN
  static_assert(convert_linebreak<LineBreak::Nl, LineBreak::EscN>("a\nb"_fs).sv() == "a\\nb");
  // EscN -> Nl
  static_assert(convert_linebreak<LineBreak::EscN, LineBreak::Nl>(R"(a\nb)"_fs).sv() == "a\nb");
}

TEST_CASE("find<Sub>(str) - NTTP", "[string_ops]") {
  STATIC_CHECK(("hello world"_fs | ops::find<"world">) == 6);
  STATIC_CHECK(("hello world"_fs | ops::find<"xyz">) == std::string_view::npos);
  STATIC_CHECK(("abc"_fs | ops::find<"abc">) == 0);
  STATIC_CHECK((""_fs | ops::find<"abc">) == std::string_view::npos);
  STATIC_CHECK(("aaaa"_fs | ops::find<"aa">) == 0);
}

TEST_CASE("find<Sub>(str) - FrozenString version", "[string_ops]") {
  constexpr auto str = "hello world"_fs;
  constexpr auto sub = "world"_fs;
  STATIC_CHECK(find<sub>(str) == 6);
}

TEST_CASE("rfind<Sub>(str) - NTTP", "[string_ops]") {
  STATIC_CHECK(("hello world"_fs | ops::rfind<"o">) == 7);
  STATIC_CHECK(("hello world"_fs | ops::rfind<"xyz">) == std::string_view::npos);
  STATIC_CHECK(("abcabc"_fs | ops::rfind<"abc">) == 3);
  STATIC_CHECK((""_fs | ops::rfind<"a">) == std::string_view::npos);
  STATIC_CHECK(("aaaa"_fs | ops::rfind<"aa">) == 2);
}

TEST_CASE("find_first_of<Chars>(str)", "[string_ops]") {
  STATIC_CHECK(("hello world"_fs | ops::find_first_of<"hw">) == 0);
  STATIC_CHECK(("hello world"_fs | ops::find_first_of<"xyz">) == std::string_view::npos);
  STATIC_CHECK(("hello"_fs | ops::find_first_of<"l">) == 2);
  STATIC_CHECK((""_fs | ops::find_first_of<"a">) == std::string_view::npos);
}

TEST_CASE("find_last_of<Chars>(str)", "[string_ops]") {
  STATIC_CHECK(("hello world"_fs | ops::find_last_of<"ld">) == 10);
  STATIC_CHECK(("hello world"_fs | ops::find_last_of<"h">) == 0);
  STATIC_CHECK((""_fs | ops::find_last_of<"a">) == std::string_view::npos);
  STATIC_CHECK(("abc"_fs | ops::find_last_of<"xyz">) == std::string_view::npos);
}

TEST_CASE("count_substring<Sub>(str)", "[string_ops]") {
  STATIC_CHECK(("hello hello"_fs | ops::count_substring<"hello">) == 2);
  STATIC_CHECK(("aaaa"_fs | ops::count_substring<"aa">) == 2);  // non-overlapping
  STATIC_CHECK(("abc"_fs | ops::count_substring<"xyz">) == 0);
  STATIC_CHECK((""_fs | ops::count_substring<"a">) == 0);
  STATIC_CHECK(("aaa"_fs | ops::count_substring<"aa">) == 1);
}

TEST_CASE("reverse(str)", "[string_ops]") {
  STATIC_CHECK(("abc"_fs | ops::reverse) == "cba"_fs);
  STATIC_CHECK((""_fs | ops::reverse) == ""_fs);
  STATIC_CHECK(("a"_fs | ops::reverse) == "a"_fs);
  STATIC_CHECK(("hello"_fs | ops::reverse) == "olleh"_fs);
}

TEST_CASE("indent<Width>(str)", "[string_ops]") {
  STATIC_CHECK(("abc"_fs | ops::indent<1>).sv() == "\tabc"_fs);
  STATIC_CHECK((""_fs | ops::indent<1>).sv() == ""_fs);
  // empty line is NOT indented
  STATIC_CHECK(("\n"_fs | ops::indent<1>).sv() == "\n"_fs);
  STATIC_CHECK(("a\nb"_fs | ops::indent<1>).sv() == "\ta\n\tb"_fs);
  STATIC_CHECK(("a\n"_fs | ops::indent<1>).sv() == "\ta\n"_fs);
  STATIC_CHECK(("abc"_fs | ops::indent<2>).sv() == "\t\tabc"_fs);
  // custom indent char
  STATIC_CHECK(("a\nb"_fs | ops::indent<1, ' '>).sv() == " a\n b"_fs);
}

TEST_CASE("dedent(str)", "[string_ops]") {
  STATIC_CHECK(dedent(""_fs).sv() == "");
  STATIC_CHECK(dedent("abc"_fs).sv() == "abc");
  STATIC_CHECK(dedent("  abc"_fs).sv() == "abc");
  STATIC_CHECK(dedent("  abc\n  def"_fs).sv() == "abc\ndef");
  STATIC_CHECK(dedent("  abc\n    def"_fs).sv() == "abc\n  def");
  STATIC_CHECK(dedent("    abc"_fs).sv() == "abc");
  // mixed indent
  STATIC_CHECK(dedent("  abc\n  \t  def"_fs).sv() == "abc\n\t  def");
}
