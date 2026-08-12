#include "catch2/catch_all.hpp"
#include <string>
#include <string_view>

#include "frozenchars/literals.hpp"
#include "frozenchars/json/compress.hpp"

using namespace frozenchars::literals;

/** @brief JSON 圧縮 (compress) 関数のテスト。
    @details 繰り返し値の辞書化による JSON 構造の圧縮を検証する。*/

TEST_CASE("compress simple object", "[json][compress]") {
  constexpr auto compressed = frozenchars::json::compress<R"({"a":"value"})"_fs>();
  auto const sv = compressed.sv();
  CHECK(sv.starts_with("{\"values\""));
  CHECK(sv.ends_with("}"));
}

TEST_CASE("compress with repeated values", "[json][compress]") {
  constexpr auto compressed = frozenchars::json::compress<
    R"([{"name":"item","val":1},{"name":"item","val":1}])"_fs>();
  auto const sv = compressed.sv();
  CHECK(sv.starts_with("{\"values\""));
  CHECK(sv.find("\"name\"") != std::string_view::npos);
  CHECK(sv.find("\"item\"") != std::string_view::npos);
}

TEST_CASE("compress empty array", "[json][compress]") {
  constexpr auto compressed = frozenchars::json::compress<R"([])"_fs>();
  CHECK(compressed.sv() == R"({"values":[],"root":[]})");
}

TEST_CASE("compress empty object", "[json][compress]") {
  constexpr auto compressed = frozenchars::json::compress<R"({})"_fs>();
  CHECK(compressed.sv() == R"({"values":[],"root":{}})");
}

TEST_CASE("compress preserve string values", "[json][compress]") {
  constexpr auto compressed = frozenchars::json::compress<R"({"key":"hello world"})"_fs>();
  auto const sv = compressed.sv();
  CHECK(sv.find("hello world") != std::string_view::npos);
}

TEST_CASE("crush_compress pipeline", "[json][pipeline]") {
  constexpr auto result = frozenchars::json::crush_compress<
    R"([{"name":"item","val":1},{"name":"item","val":1}])"_fs>();
  auto const sv = result.sv();
  CHECK(sv.back() == '_');
  CHECK_FALSE(sv.empty());
}

TEST_CASE("crush_compress empty", "[json][pipeline]") {
  constexpr auto result = frozenchars::json::crush_compress<R"({})"_fs>();
  CHECK(result.sv().back() == '_');
}

TEST_CASE("decompress roundtrip", "[json][compress]") {
  // 意味的に同一（空白は正規化される）
  STATIC_CHECK(frozenchars::json::decompress<
               frozenchars::json::compress<R"({"a":"value"})"_fs>()>().sv() == R"({"a":"value"})");

  // 小数が失われない
  STATIC_CHECK(frozenchars::json::decompress<
               frozenchars::json::compress<R"({"x":1.5})"_fs>()>().sv() == R"({"x":1.5})");

  // エスケープされた引用符が壊れない
  STATIC_CHECK(frozenchars::json::decompress<
               frozenchars::json::compress<R"({"a":"he\"llo"})"_fs>()>().sv() == R"({"a":"he\"llo"})");

  // 空配列
  STATIC_CHECK(frozenchars::json::decompress<
               frozenchars::json::compress<R"([])"_fs>()>().sv() == R"([])");

  // ネスト
  STATIC_CHECK(frozenchars::json::decompress<
               frozenchars::json::compress<R"([{"name":"item","val":1},{"name":"item","val":1}])"_fs>()>().sv()
               == R"([{"name":"item","val":1},{"name":"item","val":1}])");
}

TEST_CASE("compress output is valid json", "[json][compress]") {
  constexpr auto compressed = frozenchars::json::compress<
    R"([{"name":"item","val":1},{"name":"item","val":1}])"_fs>();
  CHECK(frozenchars::json::validate_json(compressed.sv()));
}
