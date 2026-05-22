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

TEST_CASE("FrozenString output should not include null terminator", "[format]") {
  // Create a FrozenString and force length to include a null terminator
  auto fs_null = FrozenString<6>{};
  for (size_t i = 0; i < 5; ++i) fs_null.buffer[i] = "hello"[i];
  fs_null.buffer[5] = '\0';
  fs_null.length = 6; // Force length to include the null

  SECTION("ostream output") {
    std::stringstream ss;
    ss << fs_null;
    std::string s = ss.str();
    CHECK(s.length() == 5);
    bool has_null = false;
    for (char c : s) if (c == '\0') has_null = true;
    CHECK_FALSE(has_null);
  }

#if defined(__cpp_lib_format)
  SECTION("format output") {
    std::string s = std::format("{}", fs_null);
    CHECK(s.length() == 5);
    bool has_null = false;
    for (char c : s) if (c == '\0') has_null = true;
    CHECK_FALSE(has_null);
  }
#endif
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
