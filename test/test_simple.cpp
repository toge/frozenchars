#include "catch2/catch_all.hpp"

#include <tuple>

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

namespace {

constexpr bool is_comma(char c) noexcept {
  return c == ',';
}

constexpr bool is_semicolon(char c) noexcept {
  return c == ';';
}

auto constexpr namespace_scope_replace =
  replace<"World", "C++">("Hello World"_fs);
static_assert(namespace_scope_replace.sv() == "Hello C++");

auto constexpr namespace_scope_replace_all =
  replace_all<"aa", "a">("aaaa"_fs);
static_assert(namespace_scope_replace_all.sv() == "aa");

auto constexpr namespace_scope_from = ops::trim(" x "_fs);
auto constexpr namespace_scope_to = ops::trim(" y "_fs);
auto constexpr namespace_scope_replace_trimmed =
  replace<namespace_scope_from, namespace_scope_to>("x x"_fs);
static_assert(namespace_scope_replace_trimmed.sv() == "y x");

}

TEST_CASE("simple string") {
  auto constexpr star = "*"_fs;

  static_assert(star.sv() == "*");
  static_assert(star.data() == star.buffer.data());
  REQUIRE(star.sv() == "*");
  REQUIRE(star.data() == star.buffer.data());
}

TEST_CASE("shrink_to_fit") {
  auto constexpr plain_input = "hello"_fs;
  auto constexpr plain = shrink_to_fit<plain_input>();
  static_assert(std::same_as<std::remove_cvref_t<decltype(plain)>, FrozenString<6>>);
  static_assert(plain.sv() == "hello");
  REQUIRE(plain.sv() == "hello");

  auto constexpr embedded = [] {
    auto s = FrozenString<8>{};
    s.buffer = {'a', 'b', '\0', 'c', 'd', '\0', 'x', 'x'};
    s.length = 7;
    return s;
  }();

  auto constexpr embedded_shrunk = shrink_to_fit<embedded>();
  static_assert(std::same_as<std::remove_cvref_t<decltype(embedded_shrunk)>, FrozenString<3>>);
  static_assert(embedded_shrunk.sv() == "ab");
  REQUIRE(embedded_shrunk.sv() == "ab");

  auto constexpr no_null = [] {
    auto s = FrozenString<5>{};
    s.buffer = {'a', 'b', 'c', 'd', 'e'};
    s.length = 4;
    return s;
  }();
  auto constexpr no_null_shrunk = shrink_to_fit<no_null>();
  static_assert(std::same_as<std::remove_cvref_t<decltype(no_null_shrunk)>, FrozenString<5>>);
  static_assert(no_null_shrunk.sv() == "abcd");
  REQUIRE(no_null_shrunk.sv() == "abcd");

  auto constexpr late_terminator = [] {
    auto s = FrozenString<6>{};
    s.buffer = {'a', 'b', 'c', 'd', '\0', 'x'};
    s.length = 4; // Length including d
    return s;
  }();
  auto constexpr late_terminator_shrunk = shrink_to_fit<late_terminator>();
  static_assert(std::same_as<std::remove_cvref_t<decltype(late_terminator_shrunk)>, FrozenString<5>>);
  static_assert(late_terminator_shrunk.sv() == "abcd");
  REQUIRE(late_terminator_shrunk.sv() == "abcd");

  auto constexpr empty_input = ""_fs;
  auto constexpr empty = shrink_to_fit<empty_input>();
  static_assert(std::same_as<std::remove_cvref_t<decltype(empty)>, FrozenString<1>>);
  static_assert(empty.sv() == "");
  REQUIRE(empty.sv() == "");
}

TEST_CASE("simple repeat") {
  auto constexpr starline = repeat<10>("*"_fs);
  static_assert(starline.sv() == "**********");
  REQUIRE(starline.sv() == "**********");
}

TEST_CASE("simple concat") {
  auto constexpr hello   = "Hello, "_fs;
  auto constexpr world   = "world!"_fs;
  auto constexpr message = hello + world;

  static_assert(message.sv() == "Hello, world!");
  REQUIRE(message.sv() == "Hello, world!");
}

TEST_CASE("complex concat") {
  auto constexpr part1 = "Part 1"_fs;
  auto constexpr part2 = "Part 2"_fs;
  auto constexpr part3 = "Part 3"_fs;
  auto constexpr result = part1 + " " + part2 + " " + part3;

  static_assert(result.sv() == "Part 1 Part 2 Part 3");
  REQUIRE(result.sv() == "Part 1 Part 2 Part 3");
}

TEST_CASE("toupper") {
  auto constexpr s1 = frozenchars::toupper("hello, world!"_fs);
  static_assert(s1.sv() == "HELLO, WORLD!");
  REQUIRE(s1.sv() == "HELLO, WORLD!");

  auto constexpr s2 = frozenchars::toupper("Already Upper"_fs);
  static_assert(s2.sv() == "ALREADY UPPER");
  REQUIRE(s2.sv() == "ALREADY UPPER");

  auto constexpr s3 = frozenchars::toupper("123 abc XYZ!"_fs);
  static_assert(s3.sv() == "123 ABC XYZ!");
  REQUIRE(s3.sv() == "123 ABC XYZ!");
}

TEST_CASE("tolower") {
  auto constexpr s1 = frozenchars::tolower("HELLO, WORLD!"_fs);
  static_assert(s1.sv() == "hello, world!");
  REQUIRE(s1.sv() == "hello, world!");

  auto constexpr s2 = frozenchars::tolower("Already lower"_fs);
  static_assert(s2.sv() == "already lower");
  REQUIRE(s2.sv() == "already lower");

  auto constexpr s3 = frozenchars::tolower("123 abc XYZ!"_fs);
  static_assert(s3.sv() == "123 abc xyz!");
  REQUIRE(s3.sv() == "123 abc xyz!");
}

TEST_CASE("substr") {
  auto constexpr s1 = substr("Hello, World!"_fs, 0, 5);
  static_assert(s1.sv() == "Hello");
  REQUIRE(s1.sv() == "Hello");

  auto constexpr s2 = substr("Hello, World!"_fs, 7, 5);
  static_assert(s2.sv() == "World");
  REQUIRE(s2.sv() == "World");

  auto constexpr s3 = substr(FrozenString{"Hello, World!"}, 0, 5);
  static_assert(s3.sv() == "Hello");
  REQUIRE(s3.sv() == "Hello");

  // Pos beyond length returns empty
  auto constexpr s4 = substr("Hello"_fs, 20, 5);
  static_assert(s4.sv() == "");
  REQUIRE(s4.sv() == "");

  // Len extends beyond end: truncate to available
  auto constexpr s5 = substr("Hello"_fs, 3, 10);
  static_assert(s5.sv() == "lo");
  REQUIRE(s5.sv() == "lo");

  // Negative Len extracts to the left of Pos
  auto constexpr s6 = substr("Hello, World!"_fs, 5, -5);
  static_assert(s6.sv() == "Hello");
  REQUIRE(s6.sv() == "Hello");

  auto constexpr s7 = substr(FrozenString{"Hello, World!"}, 7, -2);
  static_assert(s7.sv() == ", ");
  REQUIRE(s7.sv() == ", ");

  auto constexpr s8 = substr("Hello"_fs, 3, -10);
  static_assert(s8.sv() == "Hel");
  REQUIRE(s8.sv() == "Hel");
}

TEST_CASE("trim helpers") {
  auto constexpr s1 = ops::trim("  hello  "_fs);
  static_assert(s1.sv() == "hello");
  REQUIRE(s1.sv() == "hello");

  auto constexpr s2 = ops::ltrim("  hello  "_fs);
  static_assert(s2.sv() == "hello  ");
  REQUIRE(s2.sv() == "hello  ");

  auto constexpr s3 = ops::ltrim("  hello  ");
  static_assert(s3.sv() == "hello  ");
  REQUIRE(s3.sv() == "hello  ");

  auto constexpr s4 = ops::rtrim("  hello  "_fs);
  static_assert(s4.sv() == "  hello");
  REQUIRE(s4.sv() == "  hello");

  auto constexpr s5 = trim<'-'>("---hello---"_fs);
  static_assert(s5.sv() == "hello");
  REQUIRE(s5.sv() == "hello");

  auto constexpr s6 = trim<'-'>("---hello---");
  static_assert(s6.sv() == "hello");
  REQUIRE(s6.sv() == "hello");

  auto constexpr s7 = trim<'-'>(FrozenString{"---hello---"});
  static_assert(s7.sv() == "hello");
  REQUIRE(s7.sv() == "hello");
}

TEST_CASE("split_count") {
  auto constexpr count1 = split_count("a,b,c"_fs, is_comma);
  static_assert(count1 == 3);
  REQUIRE(count1 == 3);

  auto constexpr count2 = split_count("a,b,c", is_comma);
  static_assert(count2 == 3);
  REQUIRE(count2 == 3);

  auto constexpr count3 = split_count("apple banana cherry"_fs);
  static_assert(count3 == 3);
  REQUIRE(count3 == 3);

  auto constexpr count4 = split_count("  extra  spaces  "_fs);
  static_assert(count4 == 2);
  REQUIRE(count4 == 2);

  auto constexpr count5 = split_count("no_delimiters"_fs, is_comma);
  static_assert(count5 == 1);
  REQUIRE(count5 == 1);

  auto constexpr count6 = split_count(""_fs, is_comma);
  static_assert(count6 == 0);
  REQUIRE(count6 == 0);
}

TEST_CASE("split to array") {
  auto constexpr tokens = split<3>("a,b,c"_fs, is_comma);
  static_assert(tokens.size() == 3);
  static_assert(tokens[0].sv() == "a");
  static_assert(tokens[1].sv() == "b");
  static_assert(tokens[2].sv() == "c");

  REQUIRE(tokens.size() == 3);
  CHECK(tokens[0].sv() == "a");
  CHECK(tokens[1].sv() == "b");
  CHECK(tokens[2].sv() == "c");

  auto constexpr tokens2 = split<2>("apple banana cherry"_fs);
  static_assert(tokens2.size() == 2);
  static_assert(tokens2[0].sv() == "apple");
  static_assert(tokens2[1].sv() == "banana");

  REQUIRE(tokens2.size() == 2);
  CHECK(tokens2[0].sv() == "apple");
  CHECK(tokens2[1].sv() == "banana");

  auto constexpr tokens3 = split<5>("one;two;three"_fs, is_semicolon);
  static_assert(tokens3.size() == 5);
  static_assert(tokens3[0].sv() == "one");
  static_assert(tokens3[1].sv() == "two");
  static_assert(tokens3[2].sv() == "three");
  static_assert(tokens3[3].sv() == "");
  static_assert(tokens3[4].sv() == "");

  REQUIRE(tokens3.size() == 5);
  CHECK(tokens3[0].sv() == "one");
  CHECK(tokens3[1].sv() == "two");
  CHECK(tokens3[2].sv() == "three");
  CHECK(tokens3[3].sv() == "");
  CHECK(tokens3[4].sv() == "");
}

TEST_CASE("split exact") {
  auto constexpr tokens = split<"apple banana cherry"_fs>();
  static_assert(tokens.size() == 3);
  static_assert(tokens[0].sv() == "apple");
  static_assert(tokens[1].sv() == "banana");
  static_assert(tokens[2].sv() == "cherry");

  REQUIRE(tokens.size() == 3);
  CHECK(tokens[0].sv() == "apple");
  CHECK(tokens[1].sv() == "banana");
  CHECK(tokens[2].sv() == "cherry");

  auto constexpr tokens2 = split<"a,b,c"_fs, is_comma>();
  static_assert(tokens2.size() == 3);
  static_assert(tokens2[0].sv() == "a");
  static_assert(tokens2[1].sv() == "b");
  static_assert(tokens2[2].sv() == "c");

  REQUIRE(tokens2.size() == 3);
  CHECK(tokens2[0].sv() == "a");
  CHECK(tokens2[1].sv() == "b");
  CHECK(tokens2[2].sv() == "c");
}

TEST_CASE("join") {
  auto constexpr tokens = std::array{"a"_fs, "b"_fs, "c"_fs};
  auto constexpr joined = join<",">(tokens);
  static_assert(joined.sv() == "a,b,c");
  REQUIRE(joined.sv() == "a,b,c");

  auto constexpr joined2 = join<" ">(tokens);
  static_assert(joined2.sv() == "a b c");
  REQUIRE(joined2.sv() == "a b c");

  auto constexpr single = std::array{"solo"_fs};
  auto constexpr joined3 = join<", ">(single);
  static_assert(joined3.sv() == "solo");
  REQUIRE(joined3.sv() == "solo");
}

TEST_CASE("variadic join") {
  auto constexpr res = join<", ">("apple", "banana", "cherry");
  static_assert(res.sv() == "apple, banana, cherry");
  REQUIRE(res.sv() == "apple, banana, cherry");

  auto constexpr res2 = join<" - ">("one"_fs, "two"_fs);
  static_assert(res2.sv() == "one - two");
  REQUIRE(res2.sv() == "one - two");
}

TEST_CASE("pad_left") {
  auto constexpr s1 = pad_left<5>("abc"_fs);
  static_assert(s1.sv() == "  abc");
  REQUIRE(s1.sv() == "  abc");

  auto constexpr s2 = pad_left<5, '.'>("abc"_fs);
  static_assert(s2.sv() == "..abc");
  REQUIRE(s2.sv() == "..abc");

  auto constexpr s3 = pad_left<3>("abc"_fs);
  static_assert(s3.sv() == "abc");
  REQUIRE(s3.sv() == "abc");

  auto constexpr s4 = pad_left<2>("abc"_fs);
  static_assert(s4.sv() == "abc");
  REQUIRE(s4.sv() == "abc");

  auto constexpr s5 = pad_left<5>(123);
  static_assert(s5.sv() == "00123");
  REQUIRE(s5.sv() == "00123");

  auto constexpr s6 = pad_left<5, ' '>(123);
  static_assert(s6.sv() == "  123");
  REQUIRE(s6.sv() == "  123");
}

TEST_CASE("pad_right") {
  auto constexpr s1 = pad_right<5>("abc"_fs);
  static_assert(s1.sv() == "abc  ");
  REQUIRE(s1.sv() == "abc  ");

  auto constexpr s2 = pad_right<5, '.'>("abc"_fs);
  static_assert(s2.sv() == "abc..");
  REQUIRE(s2.sv() == "abc..");

  auto constexpr s3 = pad_right<3>("abc"_fs);
  static_assert(s3.sv() == "abc");
  REQUIRE(s3.sv() == "abc");

  auto constexpr s4 = pad_right<2>("abc"_fs);
  static_assert(s4.sv() == "abc");
  REQUIRE(s4.sv() == "abc");

  auto constexpr s5 = pad_right<5>(123);
  static_assert(s5.sv() == "12300");
  REQUIRE(s5.sv() == "12300");

  auto constexpr s6 = pad_right<5, ' '>(123);
  static_assert(s6.sv() == "123  ");
  REQUIRE(s6.sv() == "123  ");
}

TEST_CASE("center") {
  auto constexpr s1 = center<7>("abc"_fs);
  static_assert(s1.sv() == "  abc  ");
  REQUIRE(s1.sv() == "  abc  ");

  auto constexpr s2 = center<8, '-'>("abc"_fs);
  static_assert(s2.sv() == "--abc---");
  REQUIRE(s2.sv() == "--abc---");

  auto constexpr s3 = center<3>("abcdef"_fs);
  static_assert(s3.sv() == "abcdef");
  REQUIRE(s3.sv() == "abcdef");

  auto constexpr s4 = center<7>(FrozenString{"abc"});
  static_assert(s4.sv() == "  abc  ");
  REQUIRE(s4.sv() == "  abc  ");
}

TEST_CASE("replace") {
  auto constexpr s1 = replace<"World", "C++">("Hello World"_fs);
  static_assert(s1.sv() == "Hello C++");
  REQUIRE(s1.sv() == "Hello C++");

  auto constexpr s2 = replace<"apple", "orange">("I like apple pie"_fs);
  static_assert(s2.sv() == "I like orange pie");
  REQUIRE(s2.sv() == "I like orange pie");

  auto constexpr s3 = replace<"notfound", "found">("Hello World"_fs);
  static_assert(s3.sv() == "Hello World");
  REQUIRE(s3.sv() == "Hello World");

  auto constexpr s4 = replace<"a", "bc">("a"_fs);
  static_assert(s4.sv() == "bc");
  REQUIRE(s4.sv() == "bc");
}

TEST_CASE("replace_all") {
  auto constexpr s1 = replace_all<"aa", "a">("aaaa"_fs);
  static_assert(s1.sv() == "aa");
  REQUIRE(s1.sv() == "aa");

  auto constexpr s2 = replace_all<" ", "-">("a b c d"_fs);
  static_assert(s2.sv() == "a-b-c-d");
  REQUIRE(s2.sv() == "a-b-c-d");

  auto constexpr s3 = replace_all<".", " ">("word.word.word"_fs);
  static_assert(s3.sv() == "word word word");
  REQUIRE(s3.sv() == "word word word");

  auto constexpr s4 = replace_all<"no", "yes">("miss miss"_fs);
  static_assert(s4.sv() == "miss miss");
  REQUIRE(s4.sv() == "miss miss");
}

TEST_CASE("capitalize") {
  auto constexpr s1 = capitalize("hello"_fs);
  static_assert(s1.sv() == "Hello");
  REQUIRE(s1.sv() == "Hello");

  auto constexpr s2 = capitalize("WORLD"_fs);
  static_assert(s2.sv() == "World");
  REQUIRE(s2.sv() == "World");

  auto constexpr s3 = capitalize("a"_fs);
  static_assert(s3.sv() == "A");
  REQUIRE(s3.sv() == "A");

  auto constexpr s4 = capitalize(""_fs);
  static_assert(s4.sv() == "");
  REQUIRE(s4.sv() == "");

  auto constexpr s5 = capitalize("123"_fs);
  static_assert(s5.sv() == "123");
  REQUIRE(s5.sv() == "123");
}

TEST_CASE("case conversions") {
  auto constexpr input = "helloWorldExample"_fs;

  auto constexpr snake = to_snake_case(input);
  static_assert(snake.sv() == "hello_world_example");
  REQUIRE(snake.sv() == "hello_world_example");

  auto constexpr camel = to_camel_case(snake);
  static_assert(camel.sv() == "helloWorldExample");
  REQUIRE(camel.sv() == "helloWorldExample");

  auto constexpr pascal = to_pascal_case(snake);
  static_assert(pascal.sv() == "HelloWorldExample");
  REQUIRE(pascal.sv() == "HelloWorldExample");

  auto constexpr from_pascal = to_snake_case(pascal);
  static_assert(from_pascal.sv() == "hello_world_example");
  REQUIRE(from_pascal.sv() == "hello_world_example");
}

TEST_CASE("encoding") {
  auto constexpr input = "hello world!"_fs;
  auto constexpr encoded = url_encode(input);
  static_assert(encoded.sv() == "hello%20world%21");
  REQUIRE(encoded.sv() == "hello%20world%21");

  auto constexpr decoded = url_decode(encoded);
  static_assert(decoded.sv() == "hello world!");
  REQUIRE(decoded.sv() == "hello world!");

  auto constexpr b64_input = "Hello, FrozenChars!"_fs;
  auto constexpr b64_encoded = base64_encode(b64_input);
  static_assert(b64_encoded.sv() == "SGVsbG8sIEZyb3plbkNoYXJzIQ==");
  REQUIRE(b64_encoded.sv() == "SGVsbG8sIEZyb3plbkNoYXJzIQ==");

  auto constexpr b64_decoded = base64_decode(b64_encoded);
  static_assert(b64_decoded.sv() == "Hello, FrozenChars!");
  REQUIRE(b64_decoded.sv() == "Hello, FrozenChars!");
}

TEST_CASE("literals") {
  using namespace frozenchars::literals;
  auto constexpr s1 = "hello"_fs;
  static_assert(std::same_as<std::remove_cvref_t<decltype(s1)>, FrozenString<6>>);
  static_assert(s1.sv() == "hello");
}

TEST_CASE("split complexity") {
  auto constexpr s = "a, b , c  ,d"_fs;

  auto constexpr tokens = split<4>(s, [](char c) { return c == ',' || c == ' '; });
  static_assert(tokens.size() == 4);
  static_assert(tokens[0].sv() == "a");
  static_assert(tokens[1].sv() == "b");
  static_assert(tokens[2].sv() == "c");
  static_assert(tokens[3].sv() == "d");
}

TEST_CASE("parse_number basics") {
  auto constexpr n1 = parse_number<int>("123"_fs);
  static_assert(n1 == 123);
  REQUIRE(n1 == 123);

  auto constexpr n2 = parse_number<long>("-456"_fs);
  static_assert(n2 == -456);
  REQUIRE(n2 == -456);

  auto constexpr n3 = parse_number<unsigned int>("789"_fs);
  static_assert(n3 == 789);
  REQUIRE(n3 == 789);

  // Use WithinRel for float comparison
  auto constexpr n4 = parse_number<float>("123.45"_fs);
  REQUIRE_THAT(n4, Catch::Matchers::WithinRel(123.45f));

  auto constexpr n5 = parse_number<double>("-0.00123"_fs);
  REQUIRE_THAT(n5, Catch::Matchers::WithinRel(-0.00123));
}

TEST_CASE("split_numbers basics") {
  auto constexpr input = "1, 2, 3, 4, 5"_fs;
  auto constexpr nums = split_numbers(input, [](char c) { return c == ',' || c == ' '; });

  static_assert(nums[0] == 1);
  static_assert(nums[1] == 2);
  static_assert(nums[2] == 3);
  static_assert(nums[3] == 4);
  static_assert(nums[4] == 5);

  REQUIRE(nums[0] == 1);
  REQUIRE(nums[1] == 2);
  REQUIRE(nums[2] == 3);
  REQUIRE(nums[3] == 4);
  REQUIRE(nums[4] == 5);
}

TEST_CASE("multiline helpers") {
  auto constexpr input =
    "  line1  \n"
    "  line2  \n"
    "  line3  "_fs;

  auto constexpr ltrimmed = remove_leading_spaces(input);
  static_assert(ltrimmed.sv() ==
    "line1  \n"
    "line2  \n"
    "line3  ");

  auto constexpr rtrimmed = remove_trailing_spaces(input);
  static_assert(rtrimmed.sv() ==
    "  line1\n"
    "  line2\n"
    "  line3");

  auto constexpr normalized = remove_leading_spaces(remove_trailing_spaces(input));
  static_assert(normalized.sv() ==
    "line1\n"
    "line2\n"
    "line3");

  auto constexpr joined = join_lines(normalized, ", ");
  static_assert(joined.sv() == "line1, line2, line3");
}

TEST_CASE("comment removal") {
  auto constexpr input =
    "line1 # comment\n"
    "line2 // another comment\n"
    "/* range comment */ line3\n"
    "# full line comment\n"
    "line4"_fs;

  auto constexpr no_hash = remove_comments(input, "#");
  auto constexpr no_slash = remove_comments(no_hash, "//");
  auto constexpr no_range = remove_range_comments(no_slash, "/*", "*/");
  auto constexpr cleaned = remove_comment_lines(no_range, "#"); // Already handled but for test

  (void)cleaned;
}

TEST_CASE("empty string behavior") {
  auto constexpr empty_count = split_count(""_fs);
  static_assert(empty_count == 0);
  REQUIRE(empty_count == 0);

  auto constexpr empty = split<0, frozenchars::detail::is_any_whitespace>(""_fs);
  static_assert(empty.size() == 0);
  REQUIRE(empty.size() == 0);
}

TEST_CASE("split_numbers") {
  auto constexpr input = "1 2 3"_fs;
  auto constexpr nums = split_numbers<int>(input);
  static_assert(nums[0] == 1);
  static_assert(nums[1] == 2);
  static_assert(nums[2] == 3);
}

TEST_CASE("trim types") {
  auto constexpr input = "  abc  "_fs;
  auto constexpr t1 = ops::trim(input);
  auto constexpr t2 = ops::ltrim(input);
  auto constexpr t3 = ops::rtrim(input);

  static_assert(t1.sv() == "abc");
  static_assert(t2.sv() == "abc  ");
  static_assert(t3.sv() == "  abc");
}

TEST_CASE("trim chars") {
  auto constexpr input = "---abc---"_fs;
  auto constexpr t1 = trim<'-'>(input);
  auto constexpr t2 = ltrim<'-'>(input);
  auto constexpr t3 = rtrim<'-'>(input);

  static_assert(t1.sv() == "abc");
  static_assert(t2.sv() == "abc---");
  static_assert(t3.sv() == "---abc");
}

TEST_CASE("split char") {
  auto constexpr input = "a,b,c"_fs;
  auto constexpr tokens = split<3>(input, is_comma);
  static_assert(tokens[0].sv() == "a");
  static_assert(tokens[1].sv() == "b");
  static_assert(tokens[2].sv() == "c");
}

TEST_CASE("pipe operator basic") {
  namespace fops = frozenchars::ops;
  auto constexpr input = "  Hello  "_fs;
  auto constexpr result = input | fops::trim | fops::toupper;
  static_assert(result.sv() == "HELLO");
  REQUIRE(result.sv() == "HELLO");
}

TEST_CASE("pipe operator complex") {
  namespace fops = frozenchars::ops;
  auto constexpr input = "  apple, banana, cherry  "_fs;
  auto constexpr result = input
    | fops::trim
    | fops::toupper;

  static_assert(result.sv() == "APPLE, BANANA, CHERRY");
}

TEST_CASE("pipe operator split join") {
  namespace fops = frozenchars::ops;
  auto constexpr input = "a,b,c"_fs;
  auto constexpr result = split<3>(input, is_comma)
    | fops::join<"-">;

  static_assert(result.sv() == "a-b-c");
  REQUIRE(result.sv() == "a-b-c");
}

TEST_CASE("pipe operator with numeric") {
  namespace fops = frozenchars::ops;
  auto constexpr input = 123;
  auto constexpr result = input | fops::pad_left<5, '0'>;
  static_assert(result.sv() == "00123");
}

TEST_CASE("pipe operator multiline") {
  namespace fops = frozenchars::ops;
  auto constexpr input =
    "  line1  \n"
    "  line2  "_fs;

  auto constexpr result = input
    | fops::remove_leading_spaces
    | fops::remove_trailing_spaces
    | fops::join_lines(", ");

  static_assert(result.sv() == "line1, line2");
}

TEST_CASE("pipe operator join pad replace adaptors") {
  namespace fops = frozenchars::ops;

  auto constexpr tokens = std::array{"A"_fs, "B"_fs, "C"_fs};
  auto constexpr joined = tokens | fops::join<", ">;
  static_assert(joined.sv() == "A, B, C");

  auto constexpr padded_left = "ABC"_fs | fops::pad_left<5, '.'>;
  static_assert(padded_left.sv() == "..ABC");

  auto constexpr padded_right = "ABC"_fs | fops::pad_right<5, '_'>;
  static_assert(padded_right.sv() == "ABC__");

  auto constexpr replaced = "FOO.BAR"_fs
    | fops::replace_all<".", "/">;
  static_assert(replaced.sv() == "FOO/BAR");

  auto constexpr replaced_first = "hello world"_fs
    | fops::replace<"world", "C++">;
  static_assert(replaced_first.sv() == "hello C++");

  auto constexpr replaced_fixed = "foo"_fs
    | fops::replace<"foo", "bar">;
  static_assert(replaced_fixed.sv() == "bar");
}

TEST_CASE("split_numbers float and double") {
  auto constexpr fvalues = split_numbers<float>("1.5 -2.25 +3.0"_fs);
  static_assert(fvalues[0] == 1.5f);
  static_assert(fvalues[1] == -2.25f);
  static_assert(fvalues[2] == 3.0f);
  REQUIRE(fvalues[0] == 1.5f);
  REQUIRE(fvalues[1] == -2.25f);
  REQUIRE(fvalues[2] == 3.0f);

  auto constexpr dvalues = split_numbers<is_comma, double>("1e2,-2.5e1,3.125"_fs);
  static_assert(dvalues[0] == 100.0);
  static_assert(dvalues[1] == -25.0);
  static_assert(dvalues[2] == 3.125);
  REQUIRE(dvalues[0] == 100.0);
  REQUIRE(dvalues[1] == -25.0);
  REQUIRE(dvalues[2] == 3.125);
}
