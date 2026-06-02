#include "catch2/catch_all.hpp"

#if defined(__has_include)
#  if __has_include(<format>)
#    include <format>
#  endif
#  if __has_include(<print>)
#    include <print>
#  endif
#endif

#include <array>
#include <cstdio>
#include <iomanip>
#include <iterator>
#include <locale>
#include <sstream>
#include <string_view>

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

#if defined(__cpp_lib_format)
TEST_CASE("FrozenString works with std::format", "[format]") {
  auto constexpr prefix = "MyApp"_fs;

  auto const formatted = std::format("[{}] started", prefix);
  REQUIRE(formatted == "[MyApp] started");
  REQUIRE(std::format("{:>12}", prefix) == "       MyApp");
}

TEST_CASE("to_sv bridges FrozenString NTTP into std format family", "[format]") {
  auto formatted = std::string{"prefix:"};
  std::ignore = std::format_to(std::back_inserter(formatted), frozenchars::to_sv<"{} {}"_fs>(),
                                "hello", 42);
  REQUIRE(formatted == "prefix:hello 42");

  auto buffer = std::array<char, 5>{};
  auto const truncated =
      std::format_to_n(buffer.begin(), 5, frozenchars::to_sv<"{}"_fs>(), "abcdef");
  REQUIRE(truncated.size == 6);
  REQUIRE(std::string_view{buffer.data(), buffer.size()} == "abcde");

  REQUIRE(std::formatted_size(frozenchars::to_sv<"{}-{}"_fs>(), "ab", 12) == 5);

  auto const classic = std::locale::classic();
  REQUIRE(std::format(classic, frozenchars::to_sv<"{} {}"_fs>(), "a", 1) == "a 1");

  auto localized = std::string{};
  std::ignore = std::format_to(std::back_inserter(localized),
                               classic, frozenchars::to_sv<"{}"_fs>(), 1234);
  REQUIRE(localized == "1234");

  auto locale_buffer = std::array<char, 3>{};
  auto const locale_truncated = std::format_to_n(
      locale_buffer.begin(), 3, classic, frozenchars::to_sv<"{}"_fs>(), "xyz123");
  REQUIRE(locale_truncated.size == 6);
  REQUIRE(std::string_view{locale_buffer.data(), locale_buffer.size()} == "xyz");
  REQUIRE(std::formatted_size(classic, frozenchars::to_sv<"{}"_fs>(), "xyz123") == 6);
}
#endif

TEST_CASE("FrozenString works with std::ostream insertion", "[format]") {
  auto constexpr prefix = "MyApp"_fs;

  auto out = std::ostringstream{};
  auto& chained = (out << prefix << " ready");

  REQUIRE(&chained == &out);
  REQUIRE(out.str() == "MyApp ready");
}

TEST_CASE("FrozenString ostream insertion respects logical length and formatting", "[format]") {
  auto truncated = FrozenString<6>{};
  truncated.buffer = {'h', 'i', '\0', 'x', 'x', 'x'};
  truncated.length = 2;

  auto out = std::ostringstream{};
  auto& chained = (out << std::setw(6) << std::setfill('.') << truncated);

  REQUIRE(&chained == &out);
  REQUIRE(out.str() == "....hi");

  auto constexpr empty = FrozenString<1>{""};
  auto empty_out = std::ostringstream{};
  auto& empty_chained = (empty_out << empty);

  REQUIRE(&empty_chained == &empty_out);
  REQUIRE(empty_out.str().empty());
}

#ifdef __cpp_lib_print
TEST_CASE("FrozenString works with std::print FILE output", "[format]") {
  auto constexpr prefix = "MyApp"_fs;
  auto* fp = std::tmpfile();
  REQUIRE(fp != nullptr);

  std::print(fp, "{:>12}", prefix);
  std::fflush(fp);
  std::rewind(fp);

  auto buffer = std::array<char, 32>{};
  auto const read = std::fread(buffer.data(), 1, buffer.size(), fp);
  std::fclose(fp);

  REQUIRE(std::string_view{buffer.data(), read} == "       MyApp");
}
#endif
