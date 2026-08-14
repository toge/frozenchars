#include "frozenchars/log.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

// dump<g_log>() は意図的にコンパイルエラーを発生させ、
// 診断メッセージにログ全体の内容（NTTP 値）を出力する。
consteval auto make_log() {
  constexpr_log<256, 8> log;
  log.log("DUMPLOG_PROBE_MARKER first line");
  log.log_format<"value={}"_fs>(42);
  return log;
}

constexpr auto g_log = make_log();
auto           d     = dump<g_log>();