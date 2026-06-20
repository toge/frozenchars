#include "catch2/catch_all.hpp"
#include <string>
#include <string_view>

#include "frozenchars/literals.hpp"
#include "frozenchars/json/compress.hpp"

using namespace frozenchars::literals;

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
