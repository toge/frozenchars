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

auto constexpr namespace_scope_from = trim(" x "_fs);
auto constexpr namespace_scope_to = trim(" y "_fs);
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
    s.length = 2;
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

TEST_CASE("simple concat with literal rhs") {
  auto constexpr hello   = "Hello, "_fs;
  auto constexpr message = hello + "world!";

  static_assert(message.sv() == "Hello, world!");
  REQUIRE(message.sv() == "Hello, world!");
}

TEST_CASE("simple right align") {
  auto constexpr s1 = right<5>("abc"_fs);
  static_assert(s1.sv() == "  abc");
  REQUIRE(s1.sv() == "  abc");

  auto constexpr s2 = right<6, '.'>("abc"_fs);
  static_assert(s2.sv() == "...abc");
  REQUIRE(s2.sv() == "...abc");

  auto constexpr s3 = right<3>("abcdef"_fs);
  static_assert(s3.sv() == "abcdef");
  REQUIRE(s3.sv() == "abcdef");

  auto constexpr s4 = right<5>("abc"_fs);
  static_assert(s4.sv() == "  abc");
  REQUIRE(s4.sv() == "  abc");
}

TEST_CASE("simple center align") {
  auto constexpr s1 = center<7>("abc"_fs);
  static_assert(s1.sv() == "  abc  ");
  REQUIRE(s1.sv() == "  abc  ");

  auto constexpr s2 = center<8, '-'>("abc"_fs);
  static_assert(s2.sv() == "--abc---");
  REQUIRE(s2.sv() == "--abc---");

  auto constexpr s3 = center<3>("abcdef"_fs);
  static_assert(s3.sv() == "abcdef");
  REQUIRE(s3.sv() == "abcdef");

  auto constexpr s4 = center<7>("abc"_fs);
  static_assert(s4.sv() == "  abc  ");
  REQUIRE(s4.sv() == "  abc  ");
}

TEST_CASE("freeze with integers") {
  auto constexpr num42 = freeze(42);
  static_assert(num42.sv() == "42");
  REQUIRE(num42.sv() == "42");

  auto constexpr num0 = freeze(0);
  static_assert(num0.sv() == "0");
  REQUIRE(num0.sv() == "0");

  auto constexpr numneg = freeze(-123);
  static_assert(numneg.sv() == "-123");
  REQUIRE(numneg.sv() == "-123");
}

TEST_CASE("freeze with Hex") {
  auto constexpr hex255 = freeze(Hex(255));
  static_assert(hex255.sv() == "ff");
  REQUIRE(hex255.sv() == "ff");

  auto constexpr hex0 = freeze(Hex(0));
  static_assert(hex0.sv() == "0");
  REQUIRE(hex0.sv() == "0");
}

TEST_CASE("freeze with Bin") {
  auto constexpr bin255 = freeze(Bin(255));
  static_assert(bin255.sv() == "11111111");
  REQUIRE(bin255.sv() == "11111111");

  auto constexpr bin0 = freeze(Bin(0));
  static_assert(bin0.sv() == "0");
  REQUIRE(bin0.sv() == "0");
}

TEST_CASE("freeze with Oct") {
  auto constexpr oct255 = freeze(Oct(255));
  static_assert(oct255.sv() == "377");
  REQUIRE(oct255.sv() == "377");

  auto constexpr oct0 = freeze(Oct(0));
  static_assert(oct0.sv() == "0");
  REQUIRE(oct0.sv() == "0");
}

TEST_CASE("freeze with Precision") {
  auto constexpr pi_2 = freeze(Precision(3.14159265, 2));
  static_assert(pi_2.sv() == "3.14");
  REQUIRE(pi_2.sv() == "3.14");

  auto constexpr pi_4 = freeze(Precision(3.14159265, 4));
  static_assert(pi_4.sv() == "3.1415");
  REQUIRE(pi_4.sv() == "3.1415");

  auto constexpr frac = freeze(Precision(0.5, 1));
  static_assert(frac.sv() == "0.5");
  REQUIRE(frac.sv() == "0.5");
}

TEST_CASE("freeze with constexpr std::array<char, N>") {
  auto constexpr arr1 = std::array<char, 8>{'a', 'r', 'r', 'a', 'y', '\0', 'x', 'x'};
  auto constexpr s1   = freeze(arr1);
  static_assert(s1.sv() == "array");
  REQUIRE(s1.sv() == "array");

  auto constexpr arr2 = std::array<char, 4>{'a', 'b', 'c', 'd'};
  auto constexpr s2   = freeze(arr2);
  static_assert(s2.sv() == "abcd");
  REQUIRE(s2.sv() == "abcd");
}

TEST_CASE("freeze with constexpr std::span<char>") {
  auto constexpr buf1 = std::array<char, 8>{'s', 'p', 'a', 'n', '\0', 'x', 'x', 'x'};
  auto constexpr s1   = freeze(std::span<char const>{buf1});
  static_assert(s1.sv() == "span");
  REQUIRE(s1.sv() == "span");

  auto constexpr buf2 = std::array<char, 5>{'h', 'e', 'l', 'l', 'o'};
  auto constexpr s2   = freeze(std::span<char const>{buf2});
  static_assert(s2.sv() == "hello");
  REQUIRE(s2.sv() == "hello");
}

TEST_CASE("freeze with constexpr signed char buffers") {
  auto constexpr arr = std::array<signed char, 6>{'s', '8', '\0', 'x', 'x', 'x'};
  auto constexpr s1  = freeze(arr);
  static_assert(s1.sv() == "s8");
  REQUIRE(s1.sv() == "s8");

  auto constexpr s3 = freeze(std::span<signed char const>{arr});
  static_assert(s3.sv() == "s8");
  REQUIRE(s3.sv() == "s8");
}

TEST_CASE("freeze with constexpr unsigned char buffers") {
  auto constexpr arr = std::array<unsigned char, 6>{'u', '8', '\0', 'x', 'x', 'x'};
  auto constexpr s1  = freeze(arr);
  static_assert(s1.sv() == "u8");
  REQUIRE(s1.sv() == "u8");

  auto constexpr s3 = freeze(std::span<unsigned char const>{arr});
  static_assert(s3.sv() == "u8");
  REQUIRE(s3.sv() == "u8");
}

TEST_CASE("freeze with constexpr std::byte buffers") {
  auto const to_b = [](char c) { return std::byte{static_cast<unsigned char>(c)}; };

  auto constexpr arr = std::array<std::byte, 7>{to_b('b'), to_b('y'), to_b('t'), to_b('e'), std::byte{0}, to_b('x'), to_b('x')};
  auto constexpr s1  = freeze(arr);
  REQUIRE(s1.sv() == "byte");

  auto constexpr s3 = freeze(std::span<std::byte const>{arr});
  static_assert(s3.sv() == "byte");
  REQUIRE(s3.sv() == "byte");
}

TEST_CASE("toupper") {
  auto constexpr s1 = toupper("hello"_fs);
  static_assert(s1.sv() == "HELLO");
  REQUIRE(s1.sv() == "HELLO");

  auto constexpr s2 = toupper("Hello, World!");
  static_assert(s2.sv() == "HELLO, WORLD!");
  REQUIRE(s2.sv() == "HELLO, WORLD!");

  auto constexpr s3 = toupper("ALREADY"_fs);
  static_assert(s3.sv() == "ALREADY");
  REQUIRE(s3.sv() == "ALREADY");

  auto constexpr s4 = toupper("abc123xyz"_fs);
  static_assert(s4.sv() == "ABC123XYZ");
  REQUIRE(s4.sv() == "ABC123XYZ");
}

TEST_CASE("tolower") {
  auto constexpr s1 = tolower("HELLO"_fs);
  static_assert(s1.sv() == "hello");
  REQUIRE(s1.sv() == "hello");

  auto constexpr s2 = tolower("Hello, World!");
  static_assert(s2.sv() == "hello, world!");
  REQUIRE(s2.sv() == "hello, world!");

  auto constexpr s3 = tolower("already"_fs);
  static_assert(s3.sv() == "already");
  REQUIRE(s3.sv() == "already");

  auto constexpr s4 = tolower("ABC123XYZ"_fs);
  static_assert(s4.sv() == "abc123xyz");
  REQUIRE(s4.sv() == "abc123xyz");
}

TEST_CASE("substr") {
  auto constexpr s1 = substr<0, 5>("Hello, World!"_fs);
  static_assert(s1.sv() == "Hello");
  REQUIRE(s1.sv() == "Hello");

  auto constexpr s2 = substr<7, 5>("Hello, World!"_fs);
  static_assert(s2.sv() == "World");
  REQUIRE(s2.sv() == "World");

  auto constexpr s3 = substr<0, 5>("Hello, World!");
  static_assert(s3.sv() == "Hello");
  REQUIRE(s3.sv() == "Hello");

  // Pos beyond length returns empty
  auto constexpr s4 = substr<20, 5>("Hello"_fs);
  static_assert(s4.sv() == "");
  REQUIRE(s4.sv() == "");

  // Len extends beyond end: truncate to available
  auto constexpr s5 = substr<3, 10>("Hello"_fs);
  static_assert(s5.sv() == "lo");
  REQUIRE(s5.sv() == "lo");

  // Negative Len extracts to the left of Pos
  auto constexpr s6 = substr<5, -5>("Hello, World!"_fs);
  static_assert(s6.sv() == "Hello");
  REQUIRE(s6.sv() == "Hello");

  auto constexpr s7 = substr<7, -2>("Hello, World!");
  static_assert(s7.sv() == ", ");
  REQUIRE(s7.sv() == ", ");

  auto constexpr s8 = substr<3, -10>("Hello"_fs);
  static_assert(s8.sv() == "Hel");
  REQUIRE(s8.sv() == "Hel");
}

TEST_CASE("trim helpers") {
  auto constexpr s1 = trim("  hello  "_fs);
  static_assert(s1.sv() == "hello");
  REQUIRE(s1.sv() == "hello");

  auto constexpr s2 = ltrim("  hello  "_fs);
  static_assert(s2.sv() == "hello  ");
  REQUIRE(s2.sv() == "hello  ");

  auto constexpr s3 = ltrim("  hello  ");
  static_assert(s3.sv() == "hello  ");
  REQUIRE(s3.sv() == "hello  ");

  auto constexpr s4 = rtrim("  hello  "_fs);
  static_assert(s4.sv() == "  hello");
  REQUIRE(s4.sv() == "  hello");

  auto constexpr s5 = trim("  hello  ");
  static_assert(s5.sv() == "hello");
  REQUIRE(s5.sv() == "hello");

  auto constexpr s6 = rtrim("  hello  ");
  static_assert(s6.sv() == "  hello");
  REQUIRE(s6.sv() == "  hello");

  auto constexpr s7 = trim<'-'>("---a-b---");
  static_assert(s7.sv() == "a-b");
  REQUIRE(s7.sv() == "a-b");

  auto constexpr s8 = trim<'-'>("---a-b---"_fs);
  static_assert(s8.sv() == "a-b");
  REQUIRE(s8.sv() == "a-b");

  auto constexpr s9 = ltrim<'-'>("---a-b---"_fs);
  static_assert(s9.sv() == "a-b---");
  REQUIRE(s9.sv() == "a-b---");

  auto constexpr s10 = rtrim<'-'>("---a-b---");
  static_assert(s10.sv() == "---a-b");
  REQUIRE(s10.sv() == "---a-b");

  auto constexpr s11 = trim("      "_fs);
  static_assert(s11.sv() == "");
  REQUIRE(s11.sv() == "");

  auto constexpr s12 = ltrim("      "_fs);
  static_assert(s12.sv() == "");
  REQUIRE(s12.sv() == "");

  auto constexpr s13 = rtrim(""_fs);
  static_assert(s13.sv() == "");
  REQUIRE(s13.sv() == "");
}

TEST_CASE("join") {
  auto constexpr empty = join<",">();
  static_assert(empty.sv() == "");
  REQUIRE(empty.sv() == "");

  auto constexpr frozen_delim = ", "_fs;
  auto constexpr trimmed_delim = trim(" , "_fs);

  auto constexpr empty_parts = std::array<FrozenString<1>, 0>{};
  auto constexpr joined_empty_parts = join<", ">(empty_parts);
  static_assert(joined_empty_parts.sv() == "");
  REQUIRE(joined_empty_parts.sv() == "");

  auto constexpr single_part = join<", ">("solo"_fs);
  static_assert(single_part.sv() == "solo");
  REQUIRE(single_part.sv() == "solo");

  auto constexpr parts = std::array{"aa"_fs, "bb"_fs, "cc"_fs};
  auto constexpr joined = join<", ">(parts);
  static_assert(joined.sv() == "aa, bb, cc");
  REQUIRE(joined.sv() == "aa, bb, cc");

  auto constexpr variadic = join<"-">("a"_fs, "bb"_fs, "ccc"_fs);
  static_assert(variadic.sv() == "a-bb-ccc");
  REQUIRE(variadic.sv() == "a-bb-ccc");

  auto constexpr literal_variadic = join<",">("a", "bb", "ccc");
  static_assert(literal_variadic.sv() == "a,bb,ccc");
  REQUIRE(literal_variadic.sv() == "a,bb,ccc");

  auto constexpr frozen_delim_variadic = join<frozen_delim>("a"_fs, "bb"_fs, "ccc"_fs);
  static_assert(frozen_delim_variadic.sv() == "a, bb, ccc");
  REQUIRE(frozen_delim_variadic.sv() == "a, bb, ccc");

  auto constexpr trimmed_delim_variadic = join<trimmed_delim>("a"_fs, "bb"_fs, "ccc"_fs);
  static_assert(trimmed_delim_variadic.sv() == "a,bb,ccc");
  REQUIRE(trimmed_delim_variadic.sv() == "a,bb,ccc");

  auto constexpr empty_delim = join<"">("a"_fs, "bb"_fs, "ccc"_fs);
  static_assert(empty_delim.sv() == "abbccc");
  REQUIRE(empty_delim.sv() == "abbccc");
}

TEST_CASE("pad left and right") {
  auto constexpr left_num = pad_left<6>(42);
  static_assert(left_num.sv() == "000042");
  REQUIRE(left_num.sv() == "000042");

  auto constexpr right_num = pad_right<6>(42);
  static_assert(right_num.sv() == "420000");
  REQUIRE(right_num.sv() == "420000");

  auto constexpr left_str = pad_left<2>("abc"_fs);
  static_assert(left_str.sv() == "abc");
  REQUIRE(left_str.sv() == "abc");

  auto constexpr left_str_equal = pad_left<3>("abc"_fs);
  static_assert(left_str_equal.sv() == "abc");
  REQUIRE(left_str_equal.sv() == "abc");

  auto constexpr right_str = pad_right<5>("abc"_fs);
  static_assert(right_str.sv() == "abc  ");
  REQUIRE(right_str.sv() == "abc  ");

  auto constexpr right_str_equal = pad_right<3>("abc"_fs);
  static_assert(right_str_equal.sv() == "abc");
  REQUIRE(right_str_equal.sv() == "abc");

  auto constexpr left_str_fill = pad_left<5, '.'>("abc"_fs);
  static_assert(left_str_fill.sv() == "..abc");
  REQUIRE(left_str_fill.sv() == "..abc");

  auto constexpr right_str_fill = pad_right<5, '_'>("abc"_fs);
  static_assert(right_str_fill.sv() == "abc__");
  REQUIRE(right_str_fill.sv() == "abc__");

  auto constexpr left_empty = pad_left<4>(""_fs);
  static_assert(left_empty.sv() == "    ");
  REQUIRE(left_empty.sv() == "    ");

  auto constexpr right_empty = pad_right<4>(""_fs);
  static_assert(right_empty.sv() == "    ");
  REQUIRE(right_empty.sv() == "    ");
}

TEST_CASE("replace and replace all") {
  auto constexpr from = "World"_fs;
  auto constexpr to = "C++"_fs;
  auto constexpr trimmed_from = trim(" x "_fs);
  auto constexpr trimmed_to = trim(" y "_fs);

  auto constexpr first = replace<"World", "C++">("Hello World World"_fs);
  static_assert(first.sv() == "Hello C++ World");
  REQUIRE(first.sv() == "Hello C++ World");

  auto constexpr first_frozen_nttp = replace<from, to>("Hello World"_fs);
  static_assert(first_frozen_nttp.sv() == "Hello C++");
  REQUIRE(first_frozen_nttp.sv() == "Hello C++");

  auto constexpr trimmed_frozen_nttp = replace<trimmed_from, trimmed_to>("x x"_fs);
  static_assert(trimmed_frozen_nttp.sv() == "y x");
  REQUIRE(trimmed_frozen_nttp.sv() == "y x");

  auto constexpr no_match = replace<"z", "x">("abc"_fs);
  static_assert(no_match.sv() == "abc");
  REQUIRE(no_match.sv() == "abc");

  auto constexpr all = replace_all<".", "_">("a.b.c"_fs);
  static_assert(all.sv() == "a_b_c");
  REQUIRE(all.sv() == "a_b_c");

  auto constexpr shrink = replace_all<"abc", "x">("abcabc"_fs);
  static_assert(shrink.sv() == "xx");
  REQUIRE(shrink.sv() == "xx");

  auto constexpr overlap = replace_all<"aa", "a">("aaaa"_fs);
  static_assert(overlap.sv() == "aa");
  REQUIRE(overlap.sv() == "aa");

  auto constexpr expand = replace_all<"a", "aaa">("aba"_fs);
  static_assert(expand.sv() == "aaabaaa");
  REQUIRE(expand.sv() == "aaabaaa");

  auto constexpr miss = replace_all<"z", "x">("abc"_fs);
  static_assert(miss.sv() == "abc");
  REQUIRE(miss.sv() == "abc");
}

TEST_CASE("pipe operator fixed adaptors") {
  namespace fops = frozenchars::ops;

  auto constexpr s1 = "hello"_fs | fops::toupper;
  static_assert(s1.sv() == "HELLO");
  REQUIRE(s1.sv() == "HELLO");

  auto constexpr s2 = "  hello  "_fs | fops::trim | fops::toupper;
  static_assert(s2.sv() == "HELLO");
  REQUIRE(s2.sv() == "HELLO");

  auto constexpr s3 = "hello_world"_fs | fops::to_pascal_case;
  static_assert(s3.sv() == "HelloWorld");
  REQUIRE(s3.sv() == "HelloWorld");

  auto constexpr direct = frozenchars::toupper(frozenchars::trim("  hello  "_fs));
  auto constexpr piped  = "  hello  "_fs | fops::trim | fops::toupper;
  static_assert(direct.sv() == piped.sv());
  REQUIRE(direct.sv() == piped.sv());
}

TEST_CASE("pipe operator join pad replace adaptors") {
  namespace fops = frozenchars::ops;
  auto constexpr frozen_delim = ", "_fs;
  auto constexpr trimmed_delim = trim(" , "_fs);
  auto constexpr from = "foo"_fs;
  auto constexpr to = "bar"_fs;
  auto constexpr trimmed_from = trim(" x "_fs);
  auto constexpr trimmed_to = trim(" y "_fs);

  auto constexpr joined = std::array{"aa"_fs, "bb"_fs, "cc"_fs}
    | fops::join<", ">
    | fops::trim
    | fops::toupper;
  static_assert(joined.sv() == "AA, BB, CC");
  REQUIRE(joined.sv() == "AA, BB, CC");

  auto constexpr joined_frozen_nttp = std::array{"aa"_fs, "bb"_fs}
    | fops::join<frozen_delim>;
  static_assert(joined_frozen_nttp.sv() == "aa, bb");
  REQUIRE(joined_frozen_nttp.sv() == "aa, bb");

  auto constexpr joined_trimmed_nttp = std::array{"aa"_fs, "bb"_fs}
    | fops::join<trimmed_delim>;
  static_assert(joined_trimmed_nttp.sv() == "aa,bb");
  REQUIRE(joined_trimmed_nttp.sv() == "aa,bb");

  auto constexpr padded_left = "abc"_fs
    | fops::pad_left<5>
    | fops::trim
    | fops::toupper;
  static_assert(padded_left.sv() == "ABC");
  REQUIRE(padded_left.sv() == "ABC");

  auto constexpr padded_right = "abc"_fs
    | fops::pad_right<5>
    | fops::trim
    | fops::toupper;
  static_assert(padded_right.sv() == "ABC");
  REQUIRE(padded_right.sv() == "ABC");

  auto constexpr replaced = "  foo.foo  "_fs
    | fops::trim
    | fops::replace_all<".", "/">
    | fops::toupper;
  static_assert(replaced.sv() == "FOO/FOO");
  REQUIRE(replaced.sv() == "FOO/FOO");

  auto constexpr direct_replace = "hello world"_fs
    | fops::replace<"world", "C++">;
  static_assert(direct_replace.sv() == "hello C++");
  REQUIRE(direct_replace.sv() == "hello C++");

  auto constexpr frozen_nttp_replace = "foo"_fs
    | fops::replace<from, to>;
  static_assert(frozen_nttp_replace.sv() == "bar");
  REQUIRE(frozen_nttp_replace.sv() == "bar");

  auto constexpr trimmed_nttp_replace = "x x"_fs
    | fops::replace<trimmed_from, trimmed_to>;
  static_assert(trimmed_nttp_replace.sv() == "y x");
  REQUIRE(trimmed_nttp_replace.sv() == "y x");
}

TEST_CASE("pipe operator substr adaptor") {
  namespace fops = frozenchars::ops;

  auto constexpr s1 = "Hello, World!"_fs | fops::substr(7, 5);
  static_assert(s1.sv() == "World");
  REQUIRE(s1.sv() == "World");

  auto constexpr s2 = "Hello, World!"_fs | fops::substr(5, -5);
  static_assert(s2.sv() == "Hello");
  REQUIRE(s2.sv() == "Hello");

  auto constexpr s3 = "Hello"_fs | fops::substr(20, 5);
  static_assert(s3.sv() == "");
  REQUIRE(s3.sv() == "");

  auto constexpr s4 = "  abcdef  "_fs | fops::trim | fops::toupper | fops::substr(0, 3);
  static_assert(s4.sv() == "ABC");
  REQUIRE(s4.sv() == "ABC");
}

TEST_CASE("split") {
  auto constexpr count = split_count("  alpha  beta\tgamma\n");
  static_assert(count == 3);
  REQUIRE(count == 3);

  auto constexpr parts = split("  alpha  beta\tgamma\n"_fs);
  static_assert(parts[0].sv() == "alpha");
  static_assert(parts[1].sv() == "beta");
  static_assert(parts[2].sv() == "gamma");
  REQUIRE(parts[0].sv() == "alpha");
  REQUIRE(parts[1].sv() == "beta");
  REQUIRE(parts[2].sv() == "gamma");

  auto constexpr padded = split("one two");
  static_assert(padded[0].sv() == "one");
  static_assert(padded[1].sv() == "two");
  REQUIRE(padded[0].sv() == "one");
  REQUIRE(padded[1].sv() == "two");

  auto constexpr empty_count = split_count(""_fs);
  static_assert(empty_count == 0);
  REQUIRE(empty_count == 0);

  auto constexpr empty = split<0>(""_fs);
  static_assert(empty.size() == 0);
  REQUIRE(empty.size() == 0);
}

TEST_CASE("split_numbers") {
  auto constexpr count = split_count("  10  -20\t+30\n");
  static_assert(count == 3);
  REQUIRE(count == 3);

  auto constexpr values = split_numbers("  10  -20\t+30\n"_fs);
  static_assert(values[0] == 10);
  static_assert(values[1] == -20);
  static_assert(values[2] == 30);
  REQUIRE(values[0] == 10);
  REQUIRE(values[1] == -20);
  REQUIRE(values[2] == 30);

  auto constexpr padded = split_numbers("1 2");
  static_assert(padded[0] == 1);
  static_assert(padded[1] == 2);
  REQUIRE(padded[0] == 1);
  REQUIRE(padded[1] == 2);

  auto constexpr min_value = split_numbers("-2147483648");
  static_assert(min_value[0] == std::numeric_limits<int>::min());
  REQUIRE(min_value[0] == std::numeric_limits<int>::min());

  /*
  REQUIRE_THROWS_AS(split_numbers("abc"), std::invalid_argument);
  REQUIRE_THROWS_AS(split_numbers("2147483648"), std::out_of_range);
  */
}

TEST_CASE("split_numbers with custom delimiter predicate") {
  auto constexpr values = split_numbers<is_semicolon>("10;;-20;+30");
  static_assert(values[0] == 10);
  static_assert(values[1] == -20);
  static_assert(values[2] == 30);
  static_assert(values[3] == 0);
  REQUIRE(values[0] == 10);
  REQUIRE(values[1] == -20);
  REQUIRE(values[2] == 30);
  REQUIRE(values[3] == 0);

  auto constexpr csv = split_numbers<is_comma>("1,-2,+3");
  static_assert(csv[0] == 1);
  static_assert(csv[1] == -2);
  static_assert(csv[2] == 3);
  REQUIRE(csv[0] == 1);
  REQUIRE(csv[1] == -2);
  REQUIRE(csv[2] == 3);
}

TEST_CASE("split_numbers with explicit integer type") {
  auto constexpr signed_values = split_numbers<long long>("9223372036854775807 -9223372036854775808");
  static_assert(signed_values[0] == std::numeric_limits<long long>::max());
  static_assert(signed_values[1] == std::numeric_limits<long long>::min());
  REQUIRE(signed_values[0] == std::numeric_limits<long long>::max());
  REQUIRE(signed_values[1] == std::numeric_limits<long long>::min());

  auto constexpr unsigned_values = split_numbers<unsigned int>("1 +2 3");
  static_assert(unsigned_values[0] == 1u);
  static_assert(unsigned_values[1] == 2u);
  static_assert(unsigned_values[2] == 3u);
  REQUIRE(unsigned_values[0] == 1u);
  REQUIRE(unsigned_values[1] == 2u);
  REQUIRE(unsigned_values[2] == 3u);

  auto constexpr csv = split_numbers<is_comma, unsigned long>("10,20,30");
  static_assert(csv[0] == 10ul);
  static_assert(csv[1] == 20ul);
  static_assert(csv[2] == 30ul);
  REQUIRE(csv[0] == 10ul);
  REQUIRE(csv[1] == 20ul);
  REQUIRE(csv[2] == 30ul);

  /*
  REQUIRE_THROWS_AS(split_numbers<unsigned int>("-1"), std::out_of_range);
  */
}

TEST_CASE("split_numbers with floating-point type") {
  auto constexpr values = split_numbers<float>("1.5 -2.25 +3.0");
  static_assert(values[0] == 1.5f);
  static_assert(values[1] == -2.25f);
  static_assert(values[2] == 3.0f);
  REQUIRE(values[0] == 1.5f);
  REQUIRE(values[1] == -2.25f);
  REQUIRE(values[2] == 3.0f);

  auto constexpr exp_values = split_numbers<is_comma, double>("1e2,-2.5e1,3.125");
  static_assert(exp_values[0] == 100.0);
  static_assert(exp_values[1] == -25.0);
  static_assert(exp_values[2] == 3.125);
  REQUIRE(exp_values[0] == 100.0);
  REQUIRE(exp_values[1] == -25.0);
  REQUIRE(exp_values[2] == 3.125);

  /*
  REQUIRE_THROWS_AS(split_numbers<float>("1.0.0"), std::invalid_argument);
  REQUIRE_THROWS_AS(split_numbers<float>("1e"), std::invalid_argument);
  REQUIRE_THROWS_AS(split_numbers<float>("1e999"), std::out_of_range);
  */
}

TEST_CASE("parse_hex_rgb constexpr") {
  auto constexpr rgb = parse_hex_rgb("#335577");
  static_assert(std::get<0>(rgb) == 0x33);
  static_assert(std::get<1>(rgb) == 0x55);
  static_assert(std::get<2>(rgb) == 0x77);
  REQUIRE(std::get<0>(rgb) == 0x33);
  REQUIRE(std::get<1>(rgb) == 0x55);
  REQUIRE(std::get<2>(rgb) == 0x77);

  auto constexpr mixed = parse_hex_rgb("#aBcDeF");
  static_assert(std::get<0>(mixed) == 0xab);
  static_assert(std::get<1>(mixed) == 0xcd);
  static_assert(std::get<2>(mixed) == 0xef);
  REQUIRE(std::get<0>(mixed) == 0xab);
  REQUIRE(std::get<1>(mixed) == 0xcd);
  REQUIRE(std::get<2>(mixed) == 0xef);

  auto constexpr short_rgb = parse_hex_rgb("#532");
  static_assert(std::get<0>(short_rgb) == 0x55);
  static_assert(std::get<1>(short_rgb) == 0x33);
  static_assert(std::get<2>(short_rgb) == 0x22);
  REQUIRE(std::get<0>(short_rgb) == 0x55);
  REQUIRE(std::get<1>(short_rgb) == 0x33);
  REQUIRE(std::get<2>(short_rgb) == 0x22);
}

TEST_CASE("parse_hex_rgb can initialize user color types") {
  struct RgbAggregate {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
  };

  struct RgbClass {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;

    constexpr RgbClass(std::uint8_t red, std::uint8_t green, std::uint8_t blue) noexcept
    : r(red), g(green), b(blue)
    {}
  };

  auto constexpr aggregate = std::apply([](auto... channels) constexpr {
    return RgbAggregate{channels...};
  }, parse_hex_rgb("#123456"));
  static_assert(aggregate.r == 0x12);
  static_assert(aggregate.g == 0x34);
  static_assert(aggregate.b == 0x56);
  REQUIRE(aggregate.r == 0x12);
  REQUIRE(aggregate.g == 0x34);
  REQUIRE(aggregate.b == 0x56);

  auto constexpr color = std::make_from_tuple<RgbClass>(parse_hex_rgb("#abcdef"));
  static_assert(color.r == 0xab);
  static_assert(color.g == 0xcd);
  static_assert(color.b == 0xef);
  REQUIRE(color.r == 0xab);
  REQUIRE(color.g == 0xcd);
  REQUIRE(color.b == 0xef);
}

TEST_CASE("parse_hex_rgba constexpr") {
  auto constexpr rgba = parse_hex_rgba("#335577cc");
  static_assert(std::get<0>(rgba) == 0x33);
  static_assert(std::get<1>(rgba) == 0x55);
  static_assert(std::get<2>(rgba) == 0x77);
  static_assert(std::get<3>(rgba) == 0xcc);
  REQUIRE(std::get<0>(rgba) == 0x33);
  REQUIRE(std::get<1>(rgba) == 0x55);
  REQUIRE(std::get<2>(rgba) == 0x77);
  REQUIRE(std::get<3>(rgba) == 0xcc);

  auto constexpr short_rgba = parse_hex_rgba("#5a3c");
  static_assert(std::get<0>(short_rgba) == 0x55);
  static_assert(std::get<1>(short_rgba) == 0xaa);
  static_assert(std::get<2>(short_rgba) == 0x33);
  static_assert(std::get<3>(short_rgba) == 0xcc);
  REQUIRE(std::get<0>(short_rgba) == 0x55);
  REQUIRE(std::get<1>(short_rgba) == 0xaa);
  REQUIRE(std::get<2>(short_rgba) == 0x33);
  REQUIRE(std::get<3>(short_rgba) == 0xcc);
}

TEST_CASE("parse_hex_rgba can initialize user color types") {
  struct RgbaAggregate {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
  };

  struct RgbaClass {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;

    constexpr RgbaClass(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha) noexcept
    : r(red), g(green), b(blue), a(alpha)
    {}
  };

  auto constexpr aggregate = std::apply([](auto... channels) constexpr {
    return RgbaAggregate{channels...};
  }, parse_hex_rgba("#1234"));
  static_assert(aggregate.r == 0x11);
  static_assert(aggregate.g == 0x22);
  static_assert(aggregate.b == 0x33);
  static_assert(aggregate.a == 0x44);
  REQUIRE(aggregate.r == 0x11);
  REQUIRE(aggregate.g == 0x22);
  REQUIRE(aggregate.b == 0x33);
  REQUIRE(aggregate.a == 0x44);

  auto constexpr color = std::make_from_tuple<RgbaClass>(parse_hex_rgba("#abcdef99"));
  static_assert(color.r == 0xab);
  static_assert(color.g == 0xcd);
  static_assert(color.b == 0xef);
  static_assert(color.a == 0x99);
  REQUIRE(color.r == 0xab);
  REQUIRE(color.g == 0xcd);
  REQUIRE(color.b == 0xef);
  REQUIRE(color.a == 0x99);
}

TEST_CASE("tuple channel reorder helpers are constexpr") {
  auto constexpr bgr = to_bgr(parse_hex_rgb("#123456"));
  static_assert(std::get<0>(bgr) == 0x56);
  static_assert(std::get<1>(bgr) == 0x34);
  static_assert(std::get<2>(bgr) == 0x12);
  REQUIRE(std::get<0>(bgr) == 0x56);
  REQUIRE(std::get<1>(bgr) == 0x34);
  REQUIRE(std::get<2>(bgr) == 0x12);

  auto constexpr bgra = to_bgra(parse_hex_rgba("#12345678"));
  static_assert(std::get<0>(bgra) == 0x56);
  static_assert(std::get<1>(bgra) == 0x34);
  static_assert(std::get<2>(bgra) == 0x12);
  static_assert(std::get<3>(bgra) == 0x78);
  REQUIRE(std::get<0>(bgra) == 0x56);
  REQUIRE(std::get<1>(bgra) == 0x34);
  REQUIRE(std::get<2>(bgra) == 0x12);
  REQUIRE(std::get<3>(bgra) == 0x78);

  auto constexpr abgr = to_abgr(parse_hex_rgba("#1234"));
  static_assert(std::get<0>(abgr) == 0x44);
  static_assert(std::get<1>(abgr) == 0x33);
  static_assert(std::get<2>(abgr) == 0x22);
  static_assert(std::get<3>(abgr) == 0x11);
  REQUIRE(std::get<0>(abgr) == 0x44);
  REQUIRE(std::get<1>(abgr) == 0x33);
  REQUIRE(std::get<2>(abgr) == 0x22);
  REQUIRE(std::get<3>(abgr) == 0x11);
}

TEST_CASE("tuple channel reorder helpers can initialize user color types") {
  struct BgrColor {
    std::uint8_t b;
    std::uint8_t g;
    std::uint8_t r;
  };

  struct AbgrColor {
    std::uint8_t a;
    std::uint8_t b;
    std::uint8_t g;
    std::uint8_t r;

    constexpr AbgrColor(std::uint8_t alpha, std::uint8_t blue, std::uint8_t green, std::uint8_t red) noexcept
    : a(alpha), b(blue), g(green), r(red)
    {}
  };

  auto constexpr aggregate = std::apply([](auto... channels) constexpr {
    return BgrColor{channels...};
  }, to_bgr(parse_hex_rgb("#abcdef")));
  static_assert(aggregate.b == 0xef);
  static_assert(aggregate.g == 0xcd);
  static_assert(aggregate.r == 0xab);
  REQUIRE(aggregate.b == 0xef);
  REQUIRE(aggregate.g == 0xcd);
  REQUIRE(aggregate.r == 0xab);

  auto constexpr color = std::make_from_tuple<AbgrColor>(to_abgr(parse_hex_rgba("#12345678")));
  static_assert(color.a == 0x78);
  static_assert(color.b == 0x56);
  static_assert(color.g == 0x34);
  static_assert(color.r == 0x12);
  REQUIRE(color.a == 0x78);
  REQUIRE(color.b == 0x56);
  REQUIRE(color.g == 0x34);
  REQUIRE(color.r == 0x12);
}

TEST_CASE("capitalize") {
  auto constexpr s1 = capitalize("hello"_fs);
  static_assert(s1.sv() == "Hello");
  REQUIRE(s1.sv() == "Hello");

  auto constexpr s2 = capitalize("hELLO wORLD");
  static_assert(s2.sv() == "Hello world");
  REQUIRE(s2.sv() == "Hello world");

  auto constexpr s3 = capitalize("ALREADY"_fs);
  static_assert(s3.sv() == "Already");
  REQUIRE(s3.sv() == "Already");

  auto constexpr s4 = capitalize("123abc"_fs);
  static_assert(s4.sv() == "123abc");
  REQUIRE(s4.sv() == "123abc");
}

TEST_CASE("to_snake_case") {
  auto constexpr s1 = to_snake_case("helloWorld"_fs);
  static_assert(s1.sv() == "hello_world");
  REQUIRE(s1.sv() == "hello_world");

  auto constexpr s2 = to_snake_case("HelloWorld");
  static_assert(s2.sv() == "hello_world");
  REQUIRE(s2.sv() == "hello_world");

  auto constexpr s3 = to_snake_case("myVariableName"_fs);
  static_assert(s3.sv() == "my_variable_name");
  REQUIRE(s3.sv() == "my_variable_name");

  auto constexpr s4 = to_snake_case("already_snake"_fs);
  static_assert(s4.sv() == "already_snake");
  REQUIRE(s4.sv() == "already_snake");
}

TEST_CASE("to_camel_case") {
  auto constexpr s1 = to_camel_case("hello_world"_fs);
  static_assert(s1.sv() == "helloWorld");
  REQUIRE(s1.sv() == "helloWorld");

  auto constexpr s2 = to_camel_case("my_variable_name");
  static_assert(s2.sv() == "myVariableName");
  REQUIRE(s2.sv() == "myVariableName");

  auto constexpr s3 = to_camel_case("already"_fs);
  static_assert(s3.sv() == "already");
  REQUIRE(s3.sv() == "already");

  auto constexpr s4 = to_camel_case("hello__world"_fs);
  static_assert(s4.sv() == "helloWorld");
  REQUIRE(s4.sv() == "helloWorld");
}

TEST_CASE("to_pascal_case") {
  auto constexpr s1 = to_pascal_case("hello_world"_fs);
  static_assert(s1.sv() == "HelloWorld");
  REQUIRE(s1.sv() == "HelloWorld");

  auto constexpr s2 = to_pascal_case("my_variable_name");
  static_assert(s2.sv() == "MyVariableName");
  REQUIRE(s2.sv() == "MyVariableName");

  auto constexpr s3 = to_pascal_case("already"_fs);
  static_assert(s3.sv() == "Already");
  REQUIRE(s3.sv() == "Already");

  auto constexpr s4 = to_pascal_case("hello__world"_fs);
  static_assert(s4.sv() == "HelloWorld");
  REQUIRE(s4.sv() == "HelloWorld");
}

TEST_CASE("FrozenString iterator interface") {
  auto constexpr fs = "hello"_fs;

  // begin/end and range-for
  std::string result;
  for (char c : fs) {
    result += c;
  }
  REQUIRE(result == "hello");

  // size and empty
  static_assert(fs.size() == 5);
  static_assert(!fs.empty());
  static_assert(""_fs.empty());

  // operator[]
  static_assert(fs[0] == 'h');
  static_assert(fs[4] == 'o');
  REQUIRE(fs[0] == 'h');

  // std::ranges compatibility
  auto it = std::ranges::find(fs, 'l');
  REQUIRE(it != fs.end());
  REQUIRE(*it == 'l');
}

TEST_CASE("FrozenString string_view conversion") {
  auto constexpr fs = "hello"_fs;

  // explicit operator std::string_view
  auto sv = static_cast<std::string_view>(fs);
  REQUIRE(sv == "hello");

  // constexpr context
  static_assert(static_cast<std::string_view>("world"_fs) == "world");
}

TEST_CASE("to_snake_case NTTP exact size") {
  // NTTP version returns precisely-sized buffer
  auto constexpr s1 = to_snake_case<"helloWorld"_fs>();
  static_assert(s1.sv() == "hello_world");
  REQUIRE(s1.sv() == "hello_world");
  // "helloWorld" length=10, 1 underscore → buffer size = 12
  static_assert(s1.buffer.size() == 12);

  auto constexpr s2 = to_snake_case<"already_snake"_fs>();
  static_assert(s2.sv() == "already_snake");
  REQUIRE(s2.sv() == "already_snake");
  // no underscores added → buffer size = length + 1 = 14
  static_assert(s2.buffer.size() == 14);

  auto constexpr s3 = to_snake_case<"myVariableName"_fs>();
  static_assert(s3.sv() == "my_variable_name");
  REQUIRE(s3.sv() == "my_variable_name");
  // "myVariableName" length=14, 2 underscores → buffer size = 17
  static_assert(s3.buffer.size() == 17);
}
