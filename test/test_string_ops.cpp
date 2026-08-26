#include "catch2/catch_all.hpp"

#include "frozenchars/frozen_format.hpp"
#include "frozenchars/json/crush.hpp"
#include "frozenchars/ops.hpp"
#include "frozenchars/trie_set.hpp"

/** @brief FrozenString の検索・変換・分割などの文字列操作関数のテスト */

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

  // Br → Nl
  static_assert(convert_linebreak<LineBreak::Br, LineBreak::Nl>("a<br>b"_fs).sv() == "a\nb");
  // Nl → Br
  static_assert(convert_linebreak<LineBreak::Nl, LineBreak::Br>("a\nb"_fs).sv() == "a<br>b");
  // Br → EscN
  static_assert(convert_linebreak<LineBreak::Br, LineBreak::EscN>("a<br>b"_fs).sv() == "a\\nb");
  // EscN → Br
  static_assert(convert_linebreak<LineBreak::EscN, LineBreak::Br>(R"(a\nb)"_fs).sv() == "a<br>b");
  // Nl → EscN
  static_assert(convert_linebreak<LineBreak::Nl, LineBreak::EscN>("a\nb"_fs).sv() == "a\\nb");
  // EscN → Nl
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
  STATIC_CHECK(("aaaa"_fs | ops::count_substring<"aa">) == 2);  // 重複なし
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

TEST_CASE("repeat_char and abbreviate", "[string_ops]") {
  STATIC_CHECK(repeat_char<4, '-'>().sv() == "----");
  STATIC_CHECK(repeat_char<3, 'x'>().sv() == "xxx");
  STATIC_CHECK(abbreviate<8>("Hello, World!"_fs).sv() == "Hello...");
  STATIC_CHECK(abbreviate<8, "..."_fs>("Hello, World!"_fs).sv() == "Hello...");
  // MaxLen=5, デフォルト Suffix="..."(length=3) → プレフィックス 2 文字 + Suffix = "ab..."
  STATIC_CHECK(abbreviate<5>("abcdef"_fs).sv() == "ab...");
  // MaxLen < Suffix.length のケースは Suffix の先頭 MaxLen 文字を返す
  STATIC_CHECK(abbreviate<2, "..."_fs>("abcdef"_fs).sv() == "..");
}

TEST_CASE("normalize_whitespace and squeeze", "[string_ops]") {
  STATIC_CHECK(normalize_whitespace("  hello   world  "_fs).sv() == "hello world");
  STATIC_CHECK(normalize_whitespace("hello\n\tworld"_fs).sv() == "hello world");
  STATIC_CHECK(squeeze("  hello   world  "_fs).sv() == "hello world");
}

TEST_CASE("indent<Width>(str)", "[string_ops]") {
  STATIC_CHECK(("abc"_fs | ops::indent<1>).sv() == "\tabc"_fs);
  STATIC_CHECK((""_fs | ops::indent<1>).sv() == ""_fs);
  // 空行はインデントしない
  STATIC_CHECK(("\n"_fs | ops::indent<1>).sv() == "\n"_fs);
  STATIC_CHECK(("a\nb"_fs | ops::indent<1>).sv() == "\ta\n\tb"_fs);
  STATIC_CHECK(("a\n"_fs | ops::indent<1>).sv() == "\ta\n"_fs);
  STATIC_CHECK(("abc"_fs | ops::indent<2>).sv() == "\t\tabc"_fs);
  // カスタムインデント文字
  STATIC_CHECK(("a\nb"_fs | ops::indent<1, ' '>).sv() == " a\n b"_fs);
}

TEST_CASE("dedent(str)", "[string_ops]") {
  STATIC_CHECK(dedent(""_fs).sv() == "");
  STATIC_CHECK(dedent("abc"_fs).sv() == "abc");
  STATIC_CHECK(dedent("  abc"_fs).sv() == "abc");
  STATIC_CHECK(dedent("  abc\n  def"_fs).sv() == "abc\ndef");
  STATIC_CHECK(dedent("  abc\n    def"_fs).sv() == "abc\n  def");
  STATIC_CHECK(dedent("    abc"_fs).sv() == "abc");
  // 混在インデント
  STATIC_CHECK(dedent("  abc\n  \t  def"_fs).sv() == "abc\n\t  def");
}

TEST_CASE("levenshtein_distance", "[string_ops]") {
  STATIC_CHECK(levenshtein_distance<"kitten"_fs, "sitting"_fs>() == 3);
  STATIC_CHECK(levenshtein_distance<"git comit"_fs, "git commit"_fs>() == 1);
  STATIC_CHECK(levenshtein_distance<"abc"_fs, "abc"_fs>() == 0);
  STATIC_CHECK(levenshtein_distance<"abc"_fs, "xyz"_fs>() == 3);
  STATIC_CHECK(levenshtein_distance("kitten"_fs, "sitting"_fs) == 3);
  STATIC_CHECK(levenshtein_distance(""_fs, ""_fs) == 0);
  STATIC_CHECK(levenshtein_distance("abc"_fs, "abc"_fs) == 0);
}

TEST_CASE("lcp", "[string_ops]") {
  STATIC_CHECK(lcp<"kitten"_fs, "kitchen"_fs>().sv() == "kit");
  STATIC_CHECK(lcp<"abc"_fs, "abd"_fs, "abx"_fs>().sv() == "ab");
  STATIC_CHECK(lcp<"abc"_fs, "abc"_fs>().sv() == "abc");
  STATIC_CHECK(lcp<"abc"_fs, "xyz"_fs>().sv() == "");
  STATIC_CHECK(lcp<"abc"_fs>().sv() == "abc");
  STATIC_CHECK(lcp<>().sv() == "");
}

TEST_CASE("levenshtein_distance mixed NTTP/runtime overload", "[string_ops]") {
  STATIC_CHECK(levenshtein_distance<"kitten"_fs>("sitting") == 3);
  STATIC_CHECK(levenshtein_distance<"kitten"_fs>("kitten") == 0);
  STATIC_CHECK(levenshtein_distance<"abc"_fs>("") == 3);
  STATIC_CHECK(levenshtein_distance<"abc"_fs>(std::string_view{"sitting"}) == 7);
  STATIC_CHECK(levenshtein_distance<"git commit"_fs>(std::string_view{"git comit"}) == 1);
}

TEST_CASE("levenshtein_distance runtime string_view overload", "[string_ops]") {
  STATIC_CHECK(levenshtein_distance(std::string_view{"kitten"}, std::string_view{"sitting"}) == 3);
  STATIC_CHECK(levenshtein_distance(std::string_view{""}, std::string_view{"abc"}) == 3);
  STATIC_CHECK(levenshtein_distance(std::string_view{"abc"}, std::string_view{"abc"}) == 0);
  STATIC_CHECK(levenshtein_distance("kitten", "sitting") == 3);  // char[] は string_view へ変換
}

TEST_CASE("levenshtein_distance mixed falls back to heap for huge NTTP", "[string_ops]") {
  constexpr auto long_x = repeat_char<3000, 'x'>();  // detail::k_levenshtein_stack_cap 超過 → ヒープ版
  STATIC_CHECK(long_x.size() > detail::k_levenshtein_stack_cap);
  static_assert(!noexcept(levenshtein_distance<long_x>(std::string_view{})));  // ヒープ版は noexcept でない
  static_assert(noexcept(levenshtein_distance<"commit"_fs>(std::string_view{})));  // 小さい NTTP は noexcept
  STATIC_CHECK(levenshtein_distance<long_x>("") == 3000);
  STATIC_CHECK(levenshtein_distance<long_x>("x") == 2999);
  STATIC_CHECK(levenshtein_distance<long_x>("xxxx") == 2996);
  STATIC_CHECK(levenshtein_distance<"abc"_fs, 2>("abc") == 0);  // StackCap で小さい NTTP もヒープ版へ
  static_assert(!noexcept(levenshtein_distance<"abc"_fs, 2>(std::string_view{})));
  static_assert(noexcept(levenshtein_distance<"abc"_fs, 128>(std::string_view{})));
}

TEST_CASE("suggest nearest candidate for CLI typo", "[string_ops]") {
  STATIC_CHECK(suggest<"commit", "status", "checkout", "branch">(std::string_view{"comit"}, 2) == "commit");
  STATIC_CHECK(suggest<"commit", "status">(std::string_view{"commit"}, 2) == "commit");  // 完全一致
  STATIC_CHECK(suggest<"commit", "status">(std::string_view{"comit"}, 1) == "commit");
  STATIC_CHECK(suggest<"commit", "status">(std::string_view{"comit"}, 0) == std::nullopt);  // 閾値超過
  STATIC_CHECK(suggest<"commit", "status">(std::string_view{"zzzzzzzz"}, 2) == std::nullopt);
  STATIC_CHECK(suggest<>("x", 5) == std::nullopt);  // 候補なし
  STATIC_CHECK(suggest<"cat", "bat">(std::string_view{"dat"}, 1) == "cat");  // 同距離は先頭優先
}

TEST_CASE("substr edge cases", "[string_ops]") {
  // pos が length を超える場合は空文字
  STATIC_CHECK(substr("abc"_fs, 10, 5).sv() == "");
  // len = 0
  STATIC_CHECK(substr("abc"_fs, 1, 0).sv() == "");
  // 負の len: pos の左側から abs(len) 文字
  STATIC_CHECK(substr("abcdef"_fs, 4, -2).sv() == "cd");
  // anchor == length, len<0: 1 文字左を返す
  STATIC_CHECK(substr("abc"_fs, 3, -1).sv() == "c");
  // pos=0, len<0: requested_len=abs(len), actual_len=min(|len|, anchor=0)=0
  STATIC_CHECK(substr("abc"_fs, 0, -1).sv() == "");
}

TEST_CASE("FrozenString embedded NUL contract", "[string_ops]") {
  // 埋め込まれた NUL は length には反映されないが buffer にはそのまま入る
  constexpr auto s = FrozenString<4>{"a\0b"};
  STATIC_CHECK(s.length == 3);  // 契約: length = N-1
  STATIC_CHECK(s.buffer[0] == 'a');
  STATIC_CHECK(s.buffer[1] == '\0');
  STATIC_CHECK(s.buffer[2] == 'b');
  STATIC_CHECK(s.buffer[3] == '\0');
}

TEST_CASE("frozen_format zero fields and zero args", "[frozen_format]") {
  // フィールド 0、引数 0: そのままリテラル
  STATIC_CHECK(frozen_format<"no fields"_fs>().sv() == "no fields");
}

TEST_CASE("frozen_trie_index find LUT boundary", "[trie_index]") {
  // 1 キーの最小ケース (LUT 不発、線形)
  using small = frozen_trie_set<"a"_fs>;
  STATIC_CHECK(small::contains("a"));
  STATIC_CHECK_FALSE(small::contains("b"));
  STATIC_CHECK(small::count("a") == 1);
  // ルート直値（空キー以外）を持つノードで 1 文字ルックアップ
  using multi = frozen_trie_set<"alpha"_fs, "alphabet"_fs, "bravo"_fs, "charlie"_fs, "delta"_fs, "echo"_fs, "foxtrot"_fs, "golf"_fs, "hotel"_fs>;
  STATIC_CHECK(multi::contains("alpha"));
  STATIC_CHECK(multi::contains("charlie"));
  STATIC_CHECK_FALSE(multi::contains("zulu"));
}
