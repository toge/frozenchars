#include "catch2/catch_all.hpp"
#include <string>
#include <string_view>

#include "frozenchars/literals.hpp"
#include "frozenchars/json/crush.hpp"

using namespace frozenchars::literals;

/** @brief JSON 圧縮 (crush) 関数のテスト。
    @details トークン置換による JSON 構造の圧縮を検証する。*/

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
  // "value" が3回出現するため、置換文字（デリミタ U+0001）が付与される
  CHECK(sv.contains('\x01'));
  // 圧縮前より短くなること（入力は33バイト）
  CHECK(sv.size() < input.length);
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

TEST_CASE("crush handles surrogate pairs", "[json][crush]") {
  // U+1F600 はサロゲートペア。候補分割でペアを壊してはいけない
  constexpr auto crushed = frozenchars::json::crush<R"({"a":"😀","b":"😀"})"_fs>();
  auto const sv = crushed.sv();
  CHECK(sv.back() == '_');
  CHECK_FALSE(sv.empty());
}
