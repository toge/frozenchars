#include <catch2/catch_test_macros.hpp>

#include <array>
#include <string_view>

#include "frozenchars/log.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

/** @brief constexpr_log によるコンパイル時ロギングのテスト */

// --- コンパイル時蓄積 + 実行時 std::string_view / std::array<std::string_view> 取得 ---

consteval auto make_log() {
  constexpr_log<128, 8> log;
  log.log("first line");
  log.log_format<"value={}"_fs>(42);
  return log;
}

constexpr auto g_log = make_log();

static_assert(g_log.message_count() == 2, "constexpr_log: message count");
static_assert(g_log.sv() == std::string_view("first line\nvalue=42\n"), "constexpr_log: sv");
static_assert(g_log.message(0) == std::string_view("first line"), "constexpr_log: message(0)");
static_assert(g_log.message(1) == std::string_view("value=42"), "constexpr_log: message(1)");

TEST_CASE("constexpr_log runtime string_view access", "[constexpr_log]") {
  std::string_view sv = g_log.sv();
  REQUIRE(sv == "first line\nvalue=42\n");

  std::array<std::string_view, 8> msgs = g_log.messages();
  REQUIRE(msgs[0] == "first line");
  REQUIRE(msgs[1] == "value=42");
  REQUIRE(msgs[7].empty());
  REQUIRE(g_log.message_count() == 2);
}

// --- ログレベル: MinLevel 未満は蓄積されない ---

consteval auto make_level_log() {
  constexpr_log<256, 16, false, log_level::info> log;
  log.log<log_level::trace>("trace msg");
  log.log<log_level::debug>("debug msg");
  log.log<log_level::info>("info msg");
  log.log<log_level::error>("error msg");
  log.log("default info msg");
  return log;
}

constexpr auto g_level_log = make_level_log();

static_assert(g_level_log.message_count() == 3, "level: count");
static_assert(g_level_log.sv() == std::string_view("info msg\nerror msg\ndefault info msg\n"), "level: content");

TEST_CASE("constexpr_log filters below MinLevel", "[constexpr_log]") {
  REQUIRE(g_level_log.message_count() == 3);
  REQUIRE(g_level_log.messages()[0] == "info msg");
  REQUIRE(g_level_log.messages()[1] == "error msg");
  REQUIRE(g_level_log.sv() == "info msg\nerror msg\ndefault info msg\n");
}

// --- フィルタされたメッセージは書式化もスキップされる（不正フォーマットでもコンパイルが通る） ---

consteval auto make_filtered_bad_fmt_log() {
  constexpr_log<256, 16, false, log_level::info> log;
  // trace はフィルタされるため、不正なフォーマット文字列でも frozen_format の検証が走らない
  log.log_format<"{"_fs, log_level::trace>("x");
  log.log_format<"ok={}"_fs>(7);
  return log;
}

constexpr auto g_bad_fmt_log = make_filtered_bad_fmt_log();

static_assert(g_bad_fmt_log.message_count() == 1, "bad fmt: count");
static_assert(g_bad_fmt_log.sv() == std::string_view("ok=7\n"), "bad fmt: content");

// --- ビルド時タイムスタンプ付き ---

consteval auto make_ts_log() {
  constexpr_log<256, 8, true> log;
  log.log("hello");
  log.log("world");
  return log;
}

constexpr auto g_ts_log = make_ts_log();

static_assert(g_ts_log.message_count() == 2, "ts: count");
static_assert(g_ts_log.message(0).starts_with("[20"), "ts: starts with build year");
static_assert(g_ts_log.message(0).find("Z] hello") != std::string_view::npos, "ts: suffix");

TEST_CASE("constexpr_log with timestamp prefix", "[constexpr_log]") {
  auto const sv = g_ts_log.sv();
  REQUIRE(sv.front() == '[');
  REQUIRE(sv.size() > 40);
  REQUIRE(g_ts_log.message(1).find("Z] world") != std::string_view::npos);
}

// --- バッファ / メッセージ数上限の飽和 ---

consteval auto make_truncated_log() {
  constexpr_log<16, 4> log;
  log.log("0123456789");
  log.log("0123456789");
  log.log("abcdefghij");
  log.log("klmnopqrst");
  log.log("uvwxyz");
  return log;
}

constexpr auto g_trunc = make_truncated_log();

static_assert(g_trunc.sv().size() == 16, "trunc: capacity bound");
static_assert(g_trunc.message_count() == 4, "trunc: MaxMessages bound");

TEST_CASE("constexpr_log saturates at Capacity and MaxMessages", "[constexpr_log]") {
  REQUIRE(g_trunc.sv().size() == 16);
  REQUIRE(g_trunc.message_count() == 4);
}