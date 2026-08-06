#include "catch2/catch_all.hpp"

#include "frozenchars/string.hpp"
#include "frozenchars/literals.hpp"
#include "frozenchars/wildcard.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

/** @brief ワイルドカードパターンマッチングのテスト。
    @details `*`, `?`, 文字集合, 代替パターン (alternatives) の各機能を検証する。
    @see frozenchars/wildcard.hpp */

TEST_CASE("wildcard: basic * matching") {
  REQUIRE(wildcard_match<"a*c">("abc"));
  REQUIRE(wildcard_match<"a*c">("axc"));
  REQUIRE(wildcard_match<"a*c">("ac"));
  REQUIRE(wildcard_match<"a*c">("abxc"));
  REQUIRE(wildcard_match<"a*c">("aabbbbbc"));
}

TEST_CASE("wildcard: simple glob greedy matcher") {
  REQUIRE(detail::wildcard_match_simple_glob<"a*b?c">("axbyc"));
  REQUIRE(detail::wildcard_match_simple_glob<"*suffix">("prefix_suffix"));
  REQUIRE(detail::wildcard_match_simple_glob<"file-??.txt">("file-ab.txt"));
  REQUIRE_FALSE(detail::wildcard_match_simple_glob<"a*b?c">("axbc"));
  REQUIRE_FALSE(detail::wildcard_match_simple_glob<"file-??.txt">("file-a.txt"));
}

TEST_CASE("wildcard: precomputed bracket metadata") {
  using plan = detail::wildcard_plan<"[!a-c\\-x]">;

  STATIC_REQUIRE(plan::close_brackets[0] == 8);
  STATIC_REQUIRE(plan::close_parens[0] == 9);
  STATIC_REQUIRE_FALSE(plan::char_class_tables[0][static_cast<unsigned char>('a')]);
  STATIC_REQUIRE_FALSE(plan::char_class_tables[0][static_cast<unsigned char>('b')]);
  STATIC_REQUIRE_FALSE(plan::char_class_tables[0][static_cast<unsigned char>('-')]);
  STATIC_REQUIRE_FALSE(plan::char_class_tables[0][static_cast<unsigned char>('x')]);
  STATIC_REQUIRE(plan::char_class_tables[0][static_cast<unsigned char>('z')]);
}

TEST_CASE("wildcard: precomputed alternative metadata") {
  using plan = detail::wildcard_plan<"(ab|cd|e(f|g))">;

  STATIC_REQUIRE(plan::close_parens[0] == 13);
  STATIC_REQUIRE(plan::close_parens[8] == 12);
  STATIC_REQUIRE(plan::first_branch[0] == 1);
  STATIC_REQUIRE(plan::next_branch[1] == 4);
  STATIC_REQUIRE(plan::next_branch[4] == 7);
  STATIC_REQUIRE(plan::next_branch[7] == plan::NO_BRANCH);
  STATIC_REQUIRE(plan::branch_end[1] == 3);
  STATIC_REQUIRE(plan::branch_end[4] == 6);
  STATIC_REQUIRE(plan::branch_end[7] == 13);
}

TEST_CASE("wildcard: nested alternative metadata keeps branch ends separate from iteration sentinel") {
  using plan = detail::wildcard_plan<"*(a|(bc|de))*">;

  STATIC_REQUIRE(plan::close_parens[1] == 11);
  STATIC_REQUIRE(plan::first_branch[1] == 2);
  STATIC_REQUIRE(plan::branch_end[2] == 3);
  STATIC_REQUIRE(plan::next_branch[2] == 4);
  STATIC_REQUIRE(plan::branch_end[4] == 11);
  STATIC_REQUIRE(plan::next_branch[4] == plan::NO_BRANCH);

  STATIC_REQUIRE(plan::close_parens[4] == 10);
  STATIC_REQUIRE(plan::first_branch[4] == 5);
  STATIC_REQUIRE(plan::branch_end[5] == 7);
  STATIC_REQUIRE(plan::next_branch[5] == 8);
  STATIC_REQUIRE(plan::branch_end[8] == 10);
  STATIC_REQUIRE(plan::next_branch[8] == plan::NO_BRANCH);
}

TEST_CASE("wildcard: precomputed fixed prefix metadata") {
  using literal_plan = detail::wildcard_plan<"abc*de">;
  using star_plan = detail::wildcard_plan<"*abc">;
  using set_plan = detail::wildcard_plan<"ab[cd]e">;

  STATIC_REQUIRE(literal_plan::fixed_prefix_length == 3);
  STATIC_REQUIRE(star_plan::fixed_prefix_length == 0);
  STATIC_REQUIRE(set_plan::fixed_prefix_length == 2);
}

TEST_CASE("wildcard: * non-matching") {
  REQUIRE_FALSE(wildcard_match<"a*c">("ab"));
  REQUIRE_FALSE(wildcard_match<"a*c">("abxd"));
  REQUIRE_FALSE(wildcard_match<"a*c">("xab"));
}

TEST_CASE("wildcard: basic ? matching") {
  REQUIRE(wildcard_match<"a?c">("abc"));
  REQUIRE(wildcard_match<"a?c">("axc"));
}

TEST_CASE("wildcard: ? non-matching") {
  REQUIRE_FALSE(wildcard_match<"a?c">("ac"));
  REQUIRE_FALSE(wildcard_match<"a?c">("abxc"));
}

TEST_CASE("wildcard: edge cases") {
  REQUIRE(wildcard_match<"*">("anything"));
  REQUIRE(wildcard_match<"*">(""));

  REQUIRE(wildcard_match<"">(""));
  REQUIRE_FALSE(wildcard_match<"">("a"));

  REQUIRE(wildcard_match<"a">("a"));
  REQUIRE_FALSE(wildcard_match<"a">("b"));
}

TEST_CASE("wildcard: combined * and ?") {
  REQUIRE(wildcard_match<"a*b?c">("axbyc"));
  REQUIRE_FALSE(wildcard_match<"a*b?c">("axbc"));
  REQUIRE_FALSE(wildcard_match<"a*b?c">("axby"));
}

TEST_CASE("wildcard: multiple *") {
  REQUIRE(wildcard_match<"**a">("ba"));
  REQUIRE(wildcard_match<"*a*">("xyzaxyz"));
  REQUIRE_FALSE(wildcard_match<"*a*">("xyzbyxyz"));
}

TEST_CASE("wildcard: runtime text") {
  auto text = std::string("hello world");

  REQUIRE(wildcard_match<"hello*">(text));
  REQUIRE(wildcard_match<"*world">(text));
  REQUIRE(wildcard_match<"hello*world">(text));
  REQUIRE_FALSE(wildcard_match<"goodbye*">(text));
  REQUIRE_FALSE(wildcard_match<"hello*earth">(text));
}

TEST_CASE("wildcard: runtime text with ?") {
  auto text = std::string("hello");

  REQUIRE(wildcard_match<"hell?">(text));
  REQUIRE_FALSE(wildcard_match<"hell?o">(text));
  REQUIRE_FALSE(wildcard_match<"hell">(text));
}

TEST_CASE("wildcard: longer patterns") {
  REQUIRE(wildcard_match<"*.txt">("file.txt"));
  REQUIRE_FALSE(wildcard_match<"*.txt">("file.pdf"));
  REQUIRE(wildcard_match<"file.*">("file.txt"));
  REQUIRE(wildcard_match<"f*.*">("file.txt"));
}

TEST_CASE("wildcard: escape sequences") {
  REQUIRE(wildcard_match<"a\\*b">("a*b"));
  REQUIRE_FALSE(wildcard_match<"a\\*b">("axb"));
  REQUIRE(wildcard_match<"a\\?b">("a?b"));
  REQUIRE_FALSE(wildcard_match<"a\\?b">("aab"));
  REQUIRE(wildcard_match<"\\\\">("\\"));
  REQUIRE(wildcard_match<"a\\[b">("a[b"));
}

TEST_CASE("wildcard: escaped bracket is not a negation marker") {
  using helper = detail::wildcard_to_regex_helper<"\\[!">;

  STATIC_REQUIRE(helper::can_delegate);
  STATIC_REQUIRE(helper::regex_pattern.sv() == "\\[!");
  REQUIRE(wildcard_match<"\\[!">("[!"));
  REQUIRE_FALSE(wildcard_match<"\\[!">("!"));
}


TEST_CASE("wildcard: character sets") {
  REQUIRE(wildcard_match<"[abc]">("a"));
  REQUIRE(wildcard_match<"[abc]">("b"));
  REQUIRE(wildcard_match<"[abc]">("c"));
  REQUIRE_FALSE(wildcard_match<"[abc]">("d"));
  REQUIRE_FALSE(wildcard_match<"[abc]">("ab"));
  REQUIRE_FALSE(wildcard_match<"[abc]">(""));
}

TEST_CASE("wildcard: negated character sets") {
  REQUIRE(wildcard_match<"[!abc]">("d"));
  REQUIRE(wildcard_match<"[!abc]">("x"));
  REQUIRE_FALSE(wildcard_match<"[!abc]">("a"));
  REQUIRE_FALSE(wildcard_match<"[!abc]">("b"));
  REQUIRE_FALSE(wildcard_match<"[!abc]">("c"));
}

TEST_CASE("wildcard: sets with * and ?") {
  REQUIRE(wildcard_match<"file[0-9].txt">("file0.txt"));
  REQUIRE(wildcard_match<"file[0-9].txt">("file9.txt"));
  REQUIRE(wildcard_match<"a[xyz]c">("axc"));
  REQUIRE(wildcard_match<"a[xyz]c">("azc"));
  REQUIRE_FALSE(wildcard_match<"a[xyz]c">("abc"));
  REQUIRE(wildcard_match<"*[xyz]*">("prefix_z_suffix"));
  REQUIRE_FALSE(wildcard_match<"*[xyz]*">("abc_def_ghi"));
}

TEST_CASE("wildcard: alternatives") {
  REQUIRE(wildcard_match<"(ab|cd)">("ab"));
  REQUIRE(wildcard_match<"(ab|cd)">("cd"));
  REQUIRE_FALSE(wildcard_match<"(ab|cd)">("ac"));
  REQUIRE_FALSE(wildcard_match<"(ab|cd)">("abc"));
}

TEST_CASE("wildcard: alternatives with more branches") {
  REQUIRE(wildcard_match<"(a|b|c)">("a"));
  REQUIRE(wildcard_match<"(a|b|c)">("b"));
  REQUIRE(wildcard_match<"(a|b|c)">("c"));
  REQUIRE_FALSE(wildcard_match<"(a|b|c)">("d"));
}

TEST_CASE("wildcard: alternatives with surrounding pattern") {
  REQUIRE(wildcard_match<"prefix_(ab|cd)_suffix">("prefix_ab_suffix"));
  REQUIRE(wildcard_match<"prefix_(ab|cd)_suffix">("prefix_cd_suffix"));
  REQUIRE_FALSE(wildcard_match<"prefix_(ab|cd)_suffix">("prefix_xy_suffix"));
  REQUIRE(wildcard_match<"*(ab|cd)*">("xyz_ab_xyz"));
  REQUIRE(wildcard_match<"*(ab|cd)*">("cd"));
  REQUIRE_FALSE(wildcard_match<"*(ab|cd)*">("xy"));
}

TEST_CASE("wildcard: alternatives with * and ?") {
  REQUIRE(wildcard_match<"(ab|cd)e?">("abex"));
  REQUIRE(wildcard_match<"(ab|cd)e?">("cdey"));
  REQUIRE_FALSE(wildcard_match<"(ab|cd)e?">("abe"));
  REQUIRE_FALSE(wildcard_match<"(ab|cd)e?">("cdexy"));
}

TEST_CASE("wildcard: alternatives with sets") {
  REQUIRE(wildcard_match<"([abc]|xyz)">("a"));
  REQUIRE(wildcard_match<"([abc]|xyz)">("xyz"));
  REQUIRE_FALSE(wildcard_match<"([abc]|xyz)">("d"));
}

TEST_CASE("wildcard: alternatives with nested alternatives") {
  REQUIRE(wildcard_match<"(a|(bc|de))">("a"));
  REQUIRE(wildcard_match<"(a|(bc|de))">("bc"));
  REQUIRE(wildcard_match<"(a|(bc|de))">("de"));
  REQUIRE_FALSE(wildcard_match<"(a|(bc|de))">("bd"));
}

TEST_CASE("wildcard: runtime nested alternatives after star use metadata matcher") {
  STATIC_REQUIRE_FALSE(detail::wildcard_to_regex_helper<"*(a|(bc|de))*">::can_delegate);

  auto const positive = std::string{"zzdezz"};
  auto const negative = std::string{"zzbdzz"};

  REQUIRE(wildcard_match<"*(a|(bc|de))*">(positive));
  REQUIRE_FALSE(wildcard_match<"*(a|(bc|de))*">(negative));
}

TEST_CASE("wildcard: unbalanced alternatives") {
  // 対応の取れていない '(' はリテラルとして扱われる
  REQUIRE(wildcard_match<"(ab">("(ab"));
  REQUIRE_FALSE(wildcard_match<"(ab">("ab"));

  // 入れ子の対応の取れていない '('
  REQUIRE(wildcard_match<"((ab">("((ab"));
  REQUIRE_FALSE(wildcard_match<"((ab">("(ab"));
}

TEST_CASE("wildcard: unbalanced bracket stays literal") {
  REQUIRE(wildcard_match<"[ab">("[ab"));
  REQUIRE_FALSE(wildcard_match<"[ab">("a"));
}

TEST_CASE("wildcard_find: basic * matching") {
  auto r = wildcard_find<"a*c">("xxabcxx");
  REQUIRE(r.has_value());
  REQUIRE(*r == "abc");
}

TEST_CASE("wildcard_find: simple glob keeps shortest non-empty match") {
  auto const r = wildcard_find<"*world">("hello world");
  REQUIRE(r.has_value());
  REQUIRE(*r == "hello world");
}

TEST_CASE("wildcard_find: simple glob helper keeps shortest non-empty match") {
  auto const r = detail::wildcard_find_simple_glob<"a*">("baaa");
  REQUIRE(r.has_value());
  REQUIRE(*r == "a");
}

TEST_CASE("wildcard_find: first start wins even for greedy-capable patterns") {
  auto const text = std::string_view{"baaa"};
  auto const r = wildcard_find<"a*">(text);
  REQUIRE(r.has_value());
  REQUIRE(*r == "a");
  REQUIRE(r->data() == text.data() + 1);
}

TEST_CASE("wildcard_find: first match position") {
  auto r = wildcard_find<"abc">("abcabcabc");
  REQUIRE(r.has_value());
  REQUIRE(*r == "abc");
  REQUIRE(r->data() == std::string_view{"abcabcabc"}.data());
}

TEST_CASE("wildcard_find: no match") {
  REQUIRE_FALSE(wildcard_find<"abc">("xxxxx").has_value());
  REQUIRE_FALSE(wildcard_find<"a?c">("ac").has_value());
  REQUIRE_FALSE(wildcard_find<"*">("hello").has_value());
}

TEST_CASE("wildcard_find: edge cases") {
  REQUIRE_FALSE(wildcard_find<"">("").has_value());
  REQUIRE_FALSE(wildcard_find<"">("a").has_value());
  REQUIRE_FALSE(wildcard_find<"a">("").has_value());
  REQUIRE(wildcard_find<"a">("a")->size() == 1);
}

TEST_CASE("wildcard_find: ? matching") {
  auto r = wildcard_find<"a?c">("xabcyaac");
  REQUIRE(r.has_value());
  REQUIRE(*r == "abc");
}

TEST_CASE("wildcard_find: runtime text") {
  auto text = std::string("hello world");
  auto r = wildcard_find<"hello">(text);
  REQUIRE(r.has_value());
  REQUIRE(*r == "hello");
  auto r2 = wildcard_find<"*world">(text);
  REQUIRE(r2.has_value());
  REQUIRE(*r2 == "hello world");
}

TEST_CASE("wildcard_find: character sets") {
  auto r = wildcard_find<"[abc]">("xyzcba");
  REQUIRE(r.has_value());
  REQUIRE(*r == "c");
}

TEST_CASE("wildcard_find: negated character sets") {
  auto r = wildcard_find<"[!abc]">("aaxby");
  REQUIRE(r.has_value());
  REQUIRE(*r == "x");
}

TEST_CASE("wildcard_find: alternatives") {
  auto r = wildcard_find<"(ab|cd)">("xyzcdab");
  REQUIRE(r.has_value());
  REQUIRE(*r == "cd");
}

TEST_CASE("wildcard_find_all: multiple matches") {
  auto count = 0;
  for (auto sv : wildcard_find_all<"abc">("abcabcabc")) {
    ++count;
    REQUIRE(sv == "abc");
  }
  REQUIRE(count == 3);
}

TEST_CASE("wildcard_find_all: multiple matches with ?") {
  auto expected = std::array{"abc", "aac", "azc"};
  auto i = 0;
  for (auto sv : wildcard_find_all<"a?c">("xabcyaaczazc")) {
    REQUIRE(sv == expected[i++]);
  }
  REQUIRE(i == 3);
}

TEST_CASE("wildcard_find_all: no matches") {
  auto count = 0;
  for ([[maybe_unused]] auto _ : wildcard_find_all<"abc">("xxxxx")) ++count;
  REQUIRE(count == 0);
}

TEST_CASE("wildcard_find_all: non-overlapping") {
  // "aaaa" 内の "aa" は重複なしで2つ見つかる
  auto count = 0;
  for ([[maybe_unused]] auto _ : wildcard_find_all<"aa">("aaaa")) ++count;
  REQUIRE(count == 2);
}

TEST_CASE("wildcard_find_all: simple glob keeps non-overlapping shortest matches") {
  auto const expected = std::array{"aa", "aa"};
  auto i = 0;
  for (auto sv : wildcard_find_all<"aa*">("aaaa")) {
    REQUIRE(sv == expected[i++]);
  }
  REQUIRE(i == 2);
}

TEST_CASE("wildcard_find_all: edge cases") {
  auto count = 0;
  for ([[maybe_unused]] auto _ : wildcard_find_all<"">("")) ++count;
  REQUIRE(count == 0);

  count = 0;
  for ([[maybe_unused]] auto _ : wildcard_find_all<"">("abc")) ++count;
  REQUIRE(count == 0);

  count = 0;
  for ([[maybe_unused]] auto _ : wildcard_find_all<"a">("")) ++count;
  REQUIRE(count == 0);
}

TEST_CASE("wildcard_find_all: runtime text") {
  auto text = std::string("ab_ab_ab");
  auto count = 0;
  for (auto sv : wildcard_find_all<"ab">(text)) {
    ++count;
    REQUIRE(sv == "ab");
  }
  REQUIRE(count == 3);
}

TEST_CASE("wildcard_find_all: character sets") {
  auto expected = std::array{"c", "b", "a"};
  auto i = 0;
  for (auto sv : wildcard_find_all<"[abc]">("dcbad")) {
    REQUIRE(sv == expected[i++]);
  }
  REQUIRE(i == 3);
}

TEST_CASE("wildcard_find: leading star alternatives skip empty current-start match") {
  auto const r = wildcard_find<"*(a|)">(std::string_view{"ba"});
  REQUIRE(r.has_value());
  REQUIRE(*r == "a");
}

TEST_CASE("wildcard_find_all: leading star alternatives keep later non-empty matches") {
  auto expected = std::array{"a", "a"};
  auto i = 0;
  for (auto sv : wildcard_find_all<"*(a|)">(std::string_view{"baba"})) {
    REQUIRE(sv == expected[i++]);
  }
  REQUIRE(i == 2);
}

TEST_CASE("wildcard_find_all: alternatives") {
  auto expected = std::array{"ab", "cd"};
  auto i = 0;
  for (auto sv : wildcard_find_all<"(ab|cd)">("xyzabcd")) {
    REQUIRE(sv == expected[i++]);
  }
  REQUIRE(i == 2);
}
