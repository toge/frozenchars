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
