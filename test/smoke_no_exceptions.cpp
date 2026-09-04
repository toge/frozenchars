/**
 * @file test/smoke_no_exceptions.cpp
 * @brief 例外なし既定の検証。
 *
 * -fno-exceptions 付きでビルドされる（Catch2 は例外を要するため素の main を使用）。
 * 全モジュール（数値変換・コンテナ・正規表現・フォーマット・json）が例外なしで
 * コンパイル・評価・実行できることを確認する。
 */
#include "frozenchars.hpp"
#include "frozenchars/json/compress.hpp"
#include "frozenchars/json/crush.hpp"

#include <array>
#include <cassert>
#include <system_error>

using namespace frozenchars::literals;

// ---- concat ----
constexpr auto HELLO_WORLD = "hello"_fs + " "_fs + "world"_fs;
static_assert(HELLO_WORLD.sv() == "hello world");

// ---- FROZENCHARS_CONSTEVAL_FAIL を含むモジュールが例外なしでコンパイル・評価できる ----
static_assert(*frozenchars::parse_number<int>("42"_fs) == 42);
static_assert(!frozenchars::parse_number<int>("abc"_fs).has_value());
static_assert(frozenchars::parse_number<int>("abc"_fs).error() == std::errc::invalid_argument);
static_assert(frozenchars::hex_decode("41"_fs).sv() == "A");
constexpr frozenchars::frozen_map<int, "a"_fs, "b"_fs> MAP{std::array<int, 2>{1, 2}};
static_assert(MAP.at("b")->get() == 2);
static_assert(!MAP.at("z").has_value());
static_assert(frozenchars::frozen_regex<"(a|b)c">::keys().size() == 2);
static_assert(frozenchars::frozen_format<"{}-{}"_fs>(1, "x").sv() == "1-x");
static_assert(frozenchars::json::compress<R"({"a":[1,2]})"_fs>().size() > 0);
static_assert(frozenchars::json::validate_json(R"({"a":1})"));
static_assert(!frozenchars::json::validate_json("{bad}"));

int main() {
  auto v = frozenchars::parse_number<int>("123"_fs);
  assert(v && *v == 123);
  auto const bad = frozenchars::parse_number<int>("xyz"_fs);
  assert(!bad && bad.error() == std::errc::invalid_argument);
  auto const hit = MAP.at("a");
  auto const miss = MAP.at("nope");
  assert(hit && hit->get() == 1);
  assert(!miss && miss.error() == std::errc::invalid_argument);
  assert(frozenchars::json::validate_json("[1,2,3]"));
  assert(!frozenchars::json::validate_json("nope"));
  return 0;
}
