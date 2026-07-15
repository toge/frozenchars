#include "catch2/catch_all.hpp"

#include <tuple>

#include "frozenchars/color.hpp"

/** @brief 16進色コードのパース（RGB/RGBA）および色空間変換（BGR, BGRA, ABGR）のテスト。 */

using namespace frozenchars;

// NOTE: parse_hex_rgb / parse_hex_rgba は consteval のため、
//       不正な引数はコンパイルエラーになる（ランタイム例外は発生しない）。
//       有効な入力のみ Catch2 で検証し、無効な入力は compile_fail テストで別途扱う。

TEST_CASE("parse_hex_rgb") {
  // #RRGGBB 形式
  {
    auto constexpr c = parse_hex_rgb("#ff0000");
    static_assert(std::get<0>(c) == 0xff);
    static_assert(std::get<1>(c) == 0x00);
    static_assert(std::get<2>(c) == 0x00);
    REQUIRE(std::get<0>(c) == 0xff);
    REQUIRE(std::get<1>(c) == 0x00);
    REQUIRE(std::get<2>(c) == 0x00);
  }
  {
    auto constexpr c = parse_hex_rgb("#1a2b3c");
    static_assert(std::get<0>(c) == 0x1a);
    static_assert(std::get<1>(c) == 0x2b);
    static_assert(std::get<2>(c) == 0x3c);
    REQUIRE(std::get<0>(c) == 0x1a);
    REQUIRE(std::get<1>(c) == 0x2b);
    REQUIRE(std::get<2>(c) == 0x3c);
  }
  // #RGB 短縮形式（各桁を 2 倍に展開）
  {
    auto constexpr c = parse_hex_rgb("#f00");
    static_assert(std::get<0>(c) == 0xff);
    static_assert(std::get<1>(c) == 0x00);
    static_assert(std::get<2>(c) == 0x00);
    REQUIRE(std::get<0>(c) == 0xff);
    REQUIRE(std::get<1>(c) == 0x00);
    REQUIRE(std::get<2>(c) == 0x00);
  }
  {
    auto constexpr c = parse_hex_rgb("#abc");
    static_assert(std::get<0>(c) == 0xaa);
    static_assert(std::get<1>(c) == 0xbb);
    static_assert(std::get<2>(c) == 0xcc);
    REQUIRE(std::get<0>(c) == 0xaa);
    REQUIRE(std::get<1>(c) == 0xbb);
    REQUIRE(std::get<2>(c) == 0xcc);
  }
  // string_view 版と char const (&)[N] 版が同一結果を返す
  {
    auto constexpr sv_result  = parse_hex_rgb(std::string_view{"#abc"});
    auto constexpr lit_result = parse_hex_rgb("#abc");
    static_assert(sv_result == lit_result);
  }
}

TEST_CASE("parse_hex_rgba") {
  // #RRGGBBAA 形式
  {
    auto constexpr c = parse_hex_rgba("#ff000080");
    static_assert(std::get<0>(c) == 0xff);
    static_assert(std::get<1>(c) == 0x00);
    static_assert(std::get<2>(c) == 0x00);
    static_assert(std::get<3>(c) == 0x80);
    REQUIRE(std::get<0>(c) == 0xff);
    REQUIRE(std::get<1>(c) == 0x00);
    REQUIRE(std::get<2>(c) == 0x00);
    REQUIRE(std::get<3>(c) == 0x80);
  }
  // #RGBA 短縮形式
  {
    auto constexpr c = parse_hex_rgba("#f00f");
    static_assert(std::get<0>(c) == 0xff);
    static_assert(std::get<1>(c) == 0x00);
    static_assert(std::get<2>(c) == 0x00);
    static_assert(std::get<3>(c) == 0xff);
    REQUIRE(std::get<0>(c) == 0xff);
    REQUIRE(std::get<1>(c) == 0x00);
    REQUIRE(std::get<2>(c) == 0x00);
    REQUIRE(std::get<3>(c) == 0xff);
  }
  {
    auto constexpr c = parse_hex_rgba("#1a2b3c4d");
    static_assert(std::get<0>(c) == 0x1a);
    static_assert(std::get<1>(c) == 0x2b);
    static_assert(std::get<2>(c) == 0x3c);
    static_assert(std::get<3>(c) == 0x4d);
    REQUIRE(std::get<0>(c) == 0x1a);
    REQUIRE(std::get<1>(c) == 0x2b);
    REQUIRE(std::get<2>(c) == 0x3c);
    REQUIRE(std::get<3>(c) == 0x4d);
  }
}

TEST_CASE("to_bgr") {
  {
    auto constexpr rgb = parse_hex_rgb("#1a2b3c");
    auto constexpr bgr = to_bgr(rgb);
    static_assert(std::get<0>(bgr) == 0x3c); // B
    static_assert(std::get<1>(bgr) == 0x2b); // G
    static_assert(std::get<2>(bgr) == 0x1a); // R
    REQUIRE(std::get<0>(bgr) == 0x3c);
    REQUIRE(std::get<1>(bgr) == 0x2b);
    REQUIRE(std::get<2>(bgr) == 0x1a);
  }
}

TEST_CASE("to_bgra") {
  {
    auto constexpr rgba = parse_hex_rgba("#1a2b3c4d");
    auto constexpr bgra = to_bgra(rgba);
    static_assert(std::get<0>(bgra) == 0x3c); // B
    static_assert(std::get<1>(bgra) == 0x2b); // G
    static_assert(std::get<2>(bgra) == 0x1a); // R
    static_assert(std::get<3>(bgra) == 0x4d); // A
    REQUIRE(std::get<0>(bgra) == 0x3c);
    REQUIRE(std::get<1>(bgra) == 0x2b);
    REQUIRE(std::get<2>(bgra) == 0x1a);
    REQUIRE(std::get<3>(bgra) == 0x4d);
  }
}

TEST_CASE("to_abgr") {
  {
    auto constexpr rgba = parse_hex_rgba("#1a2b3c4d");
    auto constexpr abgr = to_abgr(rgba);
    static_assert(std::get<0>(abgr) == 0x4d); // A
    static_assert(std::get<1>(abgr) == 0x3c); // B
    static_assert(std::get<2>(abgr) == 0x2b); // G
    static_assert(std::get<3>(abgr) == 0x1a); // R
    REQUIRE(std::get<0>(abgr) == 0x4d);
    REQUIRE(std::get<1>(abgr) == 0x3c);
    REQUIRE(std::get<2>(abgr) == 0x2b);
    REQUIRE(std::get<3>(abgr) == 0x1a);
  }
}
