#include "catch2/catch_all.hpp"
#include <string>
#include <string_view>
#include <system_error>

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

TEST_CASE("json detail parser reports errors via expected", "[json][compress]") {
  auto const bad = frozenchars::json::detail::parse_json("not json");
  REQUIRE_FALSE(bad.has_value());
  REQUIRE(bad.error() == std::errc::invalid_argument);
  auto const trailing = frozenchars::json::detail::parse_json(R"({"a":1} extra)");
  REQUIRE_FALSE(trailing.has_value());
  REQUIRE_FALSE(frozenchars::json::validate_json("{bad}"));
  REQUIRE(frozenchars::json::validate_json(R"({"a":[1,2]})"));
}

TEST_CASE("validate_json enforces the nesting depth limit", "[json][compress]") {
  auto const within = [](std::size_t depth) {
    return std::string(depth, '[') + std::string(depth, ']');
  };
  REQUIRE(frozenchars::json::validate_json(within(frozenchars::json::max_depth)));
  REQUIRE_FALSE(frozenchars::json::validate_json(within(frozenchars::json::max_depth + 1)));
  // 配列・オブジェクト・オブジェクト内の配列の混在でも同じ制限が適用される
  auto const mixed = std::string(frozenchars::json::max_depth / 2, '[') + std::string(frozenchars::json::max_depth / 2, '{') +
                     std::string(frozenchars::json::max_depth / 2, '}') + std::string(frozenchars::json::max_depth / 2, ']');
  REQUIRE_FALSE(frozenchars::json::validate_json(mixed));
}

TEST_CASE("validate_json survives deeply nested untrusted input", "[json][compress]") {
  // 深度制限が無いと再帰でスタックが枯渇する
  REQUIRE_FALSE(frozenchars::json::validate_json(std::string(100'000, '[')));
  REQUIRE_FALSE(frozenchars::json::validate_json(std::string(100'000, '{')));
}

TEST_CASE("validate_json rejects malformed numbers", "[json][compress]") {
  // RFC 8259 の number Production: -? (0 | [1-9][0-9]*) (. [0-9]+)? ([eE] [+-]? [0-9]+)?
  for (auto const* bad : {"[-]", "[1.]", "[.1]", "[1e]", "[1e+]", "[1e-]", "[01]", "[--1]", "[+1]", "[1.2e]", "[1.2e+]"}) {
    CAPTURE(bad);
    REQUIRE_FALSE(frozenchars::json::validate_json(bad));
  }
  for (auto const* good : {"[0]", "[-0]", "[1]", "[-1]", "[1.5]", "[1.5e10]", "[1.5E-10]", "[0.5]", "[-1234.5678e+9]"}) {
    CAPTURE(good);
    REQUIRE(frozenchars::json::validate_json(good));
  }
}

TEST_CASE("validate_json rejects malformed string escapes and raw control characters", "[json][compress]") {
  for (auto const* bad : {R"(["a\qb"])", R"(["a\u12"])", R"(["a\uZZZZ"])", R"(["a\)", R"(["a\ud800A"])", R"(["a\udc00\ud800"])"}) {
    CAPTURE(bad);
    REQUIRE_FALSE(frozenchars::json::validate_json(bad));
  }
  // 文字列中の生の制御文字（改行・タブなど）は RFC 8259 で禁止
  REQUIRE_FALSE(frozenchars::json::validate_json("[\"a\nb\"]"));
  REQUIRE_FALSE(frozenchars::json::validate_json("[\"a\tb\"]"));
  // 正当なエスケープとサロゲートペアは受理される
  for (auto const* good : {R"(["a\nb"])", R"(["\/\t\"\\"])", R"(["\u0041\u00e9"])", R"(["\ud83d\ude00"])"}) {
    CAPTURE(good);
    REQUIRE(frozenchars::json::validate_json(good));
  }
}
