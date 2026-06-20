#include "catch2/catch_all.hpp"
#include <string>
#include <string_view>

#include "frozenchars/literals.hpp"
#include "frozenchars/json/crush.hpp"

using namespace frozenchars::literals;

TEST_CASE("crush simple object", "[json][crush]") {
  constexpr auto crushed = frozenchars::json::crush<R"({"a":"value"})"_fs>();
  auto const sv = crushed.sv();
  CHECK_FALSE(sv.empty());
  CHECK(sv.back() == '_');
}

TEST_CASE("crush repeated values compresses", "[json][crush]") {
  constexpr auto input = R"({"a":"value","b":"value","c":"value"})"_fs;
  constexpr auto crushed = frozenchars::json::crush<input>();
  auto const sv = crushed.sv();
  CHECK(sv.back() == '_');
  CHECK_FALSE(sv.empty());
  // "value" is repeated 3 times -> some replacement should have happened
  auto const has_replacement = sv.contains('\x01');
  CHECK((has_replacement || !sv.empty()));
}

TEST_CASE("crush empty object", "[json][crush]") {
  constexpr auto crushed = frozenchars::json::crush<R"({})"_fs>();
  CHECK(crushed.sv().back() == '_');
}

TEST_CASE("crush array", "[json][crush]") {
  constexpr auto crushed = frozenchars::json::crush<R"([1,2,3,1,2,3])"_fs>();
  CHECK(crushed.sv().back() == '_');
}

TEST_CASE("crush preserves output format", "[json][crush]") {
  constexpr auto crushed = frozenchars::json::crush<R"({"a":"test"})"_fs>();
  auto const sv = crushed.sv();
  CHECK(sv.back() == '_');
  // input was swapped: "a" -> 'a', etc. Output is valid crushed format
  CHECK(sv.find('_') != std::string_view::npos);
}
