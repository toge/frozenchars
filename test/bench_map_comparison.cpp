#include "frozenchars/literals.hpp"
#include "frozenchars/map.hpp"
#include "frozenchars/trie_map.hpp"

#include <nanobench.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <string_view>
#include <unordered_map>

using namespace frozenchars;
using namespace frozenchars::literals;

/** @brief frozen_map, frozen_trie_map, std::map, std::unordered_map の検索性能比較ベンチマーク。
    @details 短キー・HTTP メソッド・共通接頭辞・NATO アルファベット・長キー・等長キーの
    6パターンで計測する。*/

namespace {

/** @brief 最適化防止用シンク変数。*/
volatile std::size_t g_sink = 0;

} // namespace

int main(int argc, char** argv) {
  auto iterations = std::uint64_t{500'000};
  if (argc > 1) {
    auto const parsed = std::strtoull(argv[1], nullptr, 10);
    if (parsed > 0) iterations = static_cast<std::uint64_t>(parsed);
  }

  // ---- パターン1: 短いキー（先頭文字が一意、3キー） ----
  constexpr auto p1_frozen = frozen_map<int, "a"_fs, "b"_fs, "c"_fs>{
    std::array<int, 3>{1, 2, 3}
  };
  constexpr auto p1_trie = frozen_trie_map<int, "a"_fs, "b"_fs, "c"_fs>{
    std::array<int, 3>{1, 2, 3}
  };
  auto const p1_stdmap = std::map<std::string_view, int>{{"a", 1}, {"b", 2}, {"c", 3}};
  auto const p1_stdunordered = std::unordered_map<std::string_view, int>{{"a", 1}, {"b", 2}, {"c", 3}};

  // ---- パターン2: HTTPメソッド（5キー、混在長） ----
  constexpr auto p2_frozen = frozen_map<int, "GET"_fs, "PUT"_fs, "POST"_fs, "DELETE"_fs, "HEAD"_fs>{
    std::array<int, 5>{1, 2, 3, 4, 5}
  };
  constexpr auto p2_trie = frozen_trie_map<int, "GET"_fs, "PUT"_fs, "POST"_fs, "DELETE"_fs, "HEAD"_fs>{
    std::array<int, 5>{1, 2, 3, 4, 5}
  };
  auto const p2_stdmap = std::map<std::string_view, int>{
    {"GET", 1}, {"PUT", 2}, {"POST", 3}, {"DELETE", 4}, {"HEAD", 5}
  };
  auto const p2_stdunordered = std::unordered_map<std::string_view, int>{
    {"GET", 1}, {"PUT", 2}, {"POST", 3}, {"DELETE", 4}, {"HEAD", 5}
  };

  // ---- パターン3: 共通接頭辞（4キー） ----
  constexpr auto p3_frozen = frozen_map<int, "timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>{
    std::array<int, 4>{1, 2, 3, 4}
  };
  constexpr auto p3_trie = frozen_trie_map<int, "timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>{
    std::array<int, 4>{1, 2, 3, 4}
  };
  auto const p3_stdmap = std::map<std::string_view, int>{
    {"timeout", 1}, {"timeout_ms", 2}, {"timeout_us", 3}, {"timeout_ns", 4}
  };
  auto const p3_stdunordered = std::unordered_map<std::string_view, int>{
    {"timeout", 1}, {"timeout_ms", 2}, {"timeout_us", 3}, {"timeout_ns", 4}
  };

  // ---- パターン4: NATOアルファベット（20キー） ----
  constexpr auto p4_frozen = frozen_map<int,
    "alpha"_fs, "bravo"_fs, "charlie"_fs, "delta"_fs, "echo"_fs,
    "foxtrot"_fs, "golf"_fs, "hotel"_fs, "india"_fs, "juliet"_fs,
    "kilo"_fs, "lima"_fs, "mike"_fs, "november"_fs, "oscar"_fs,
    "papa"_fs, "quebec"_fs, "romeo"_fs, "sierra"_fs, "tango"_fs>{
    std::array<int, 20>{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20}
  };
  constexpr auto p4_trie = frozen_trie_map<int,
    "alpha"_fs, "bravo"_fs, "charlie"_fs, "delta"_fs, "echo"_fs,
    "foxtrot"_fs, "golf"_fs, "hotel"_fs, "india"_fs, "juliet"_fs,
    "kilo"_fs, "lima"_fs, "mike"_fs, "november"_fs, "oscar"_fs,
    "papa"_fs, "quebec"_fs, "romeo"_fs, "sierra"_fs, "tango"_fs>{
    std::array<int, 20>{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20}
  };
  auto const p4_stdmap = std::map<std::string_view, int>{
    {"alpha", 1}, {"bravo", 2}, {"charlie", 3}, {"delta", 4}, {"echo", 5},
    {"foxtrot", 6}, {"golf", 7}, {"hotel", 8}, {"india", 9}, {"juliet", 10},
    {"kilo", 11}, {"lima", 12}, {"mike", 13}, {"november", 14}, {"oscar", 15},
    {"papa", 16}, {"quebec", 17}, {"romeo", 18}, {"sierra", 19}, {"tango", 20}
  };
  auto const p4_stdunordered = std::unordered_map<std::string_view, int>{
    {"alpha", 1}, {"bravo", 2}, {"charlie", 3}, {"delta", 4}, {"echo", 5},
    {"foxtrot", 6}, {"golf", 7}, {"hotel", 8}, {"india", 9}, {"juliet", 10},
    {"kilo", 11}, {"lima", 12}, {"mike", 13}, {"november", 14}, {"oscar", 15},
    {"papa", 16}, {"quebec", 17}, {"romeo", 18}, {"sierra", 19}, {"tango", 20}
  };

  // ---- パターン5: 長いキー（30文字超、5キー） ----
  constexpr auto p5_frozen = frozen_map<int,
    "configuration_timeout_ms"_fs, "maximum_retry_count_param"_fs,
    "connection_pool_size_setting"_fs, "authentication_token_secret_key"_fs,
    "response_body_encoding_format"_fs>{
    std::array<int, 5>{100, 200, 300, 400, 500}
  };
  constexpr auto p5_trie = frozen_trie_map<int,
    "configuration_timeout_ms"_fs, "maximum_retry_count_param"_fs,
    "connection_pool_size_setting"_fs, "authentication_token_secret_key"_fs,
    "response_body_encoding_format"_fs>{
    std::array<int, 5>{100, 200, 300, 400, 500}
  };
  auto const p5_stdmap = std::map<std::string_view, int>{
    {"configuration_timeout_ms", 100}, {"maximum_retry_count_param", 200},
    {"connection_pool_size_setting", 300}, {"authentication_token_secret_key", 400},
    {"response_body_encoding_format", 500}
  };
  auto const p5_stdunordered = std::unordered_map<std::string_view, int>{
    {"configuration_timeout_ms", 100}, {"maximum_retry_count_param", 200},
    {"connection_pool_size_setting", 300}, {"authentication_token_secret_key", 400},
    {"response_body_encoding_format", 500}
  };

  // ---- パターン6: 等長キー（10キー、各21文字） ----
  constexpr auto p6_frozen = frozen_map<int,
    "configuration_key_one"_fs, "configuration_key_two"_fs,
    "configuration_key_thr"_fs, "configuration_key_fou"_fs,
    "configuration_key_fiv"_fs, "configuration_key_six"_fs,
    "configuration_key_sev"_fs, "configuration_key_eig"_fs,
    "configuration_key_nin"_fs, "configuration_key_ten"_fs>{
    std::array<int, 10>{1,2,3,4,5,6,7,8,9,10}
  };
  constexpr auto p6_trie = frozen_trie_map<int,
    "configuration_key_one"_fs, "configuration_key_two"_fs,
    "configuration_key_thr"_fs, "configuration_key_fou"_fs,
    "configuration_key_fiv"_fs, "configuration_key_six"_fs,
    "configuration_key_sev"_fs, "configuration_key_eig"_fs,
    "configuration_key_nin"_fs, "configuration_key_ten"_fs>{
    std::array<int, 10>{1,2,3,4,5,6,7,8,9,10}
  };
  auto const p6_stdmap = std::map<std::string_view, int>{
    {"configuration_key_one", 1}, {"configuration_key_two", 2},
    {"configuration_key_thr", 3}, {"configuration_key_fou", 4},
    {"configuration_key_fiv", 5}, {"configuration_key_six", 6},
    {"configuration_key_sev", 7}, {"configuration_key_eig", 8},
    {"configuration_key_nin", 9}, {"configuration_key_ten", 10}
  };
  auto const p6_stdunordered = std::unordered_map<std::string_view, int>{
    {"configuration_key_one", 1}, {"configuration_key_two", 2},
    {"configuration_key_thr", 3}, {"configuration_key_fou", 4},
    {"configuration_key_fiv", 5}, {"configuration_key_six", 6},
    {"configuration_key_sev", 7}, {"configuration_key_eig", 8},
    {"configuration_key_nin", 9}, {"configuration_key_ten", 10}
  };

  // ---- パターン7: round-robin用の中程度キー ----
  constexpr auto p7_frozen = frozen_map<int,
    "timeout"_fs, "retry"_fs, "backoff"_fs, "endpoint"_fs, "headers"_fs,
    "method"_fs, "path"_fs, "query"_fs, "body"_fs, "status"_fs>{
    std::array<int, 10>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
  };
  constexpr auto p7_trie = frozen_trie_map<int,
    "timeout"_fs, "retry"_fs, "backoff"_fs, "endpoint"_fs, "headers"_fs,
    "method"_fs, "path"_fs, "query"_fs, "body"_fs, "status"_fs>{
    std::array<int, 10>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
  };
  auto const p7_stdmap = std::map<std::string_view, int>{
    {"timeout", 1}, {"retry", 2}, {"backoff", 3}, {"endpoint", 4}, {"headers", 5},
    {"method", 6}, {"path", 7}, {"query", 8}, {"body", 9}, {"status", 10}
  };
  auto const p7_stdunordered = std::unordered_map<std::string_view, int>{
    {"timeout", 1}, {"retry", 2}, {"backoff", 3}, {"endpoint", 4}, {"headers", 5},
    {"method", 6}, {"path", 7}, {"query", 8}, {"body", 9}, {"status", 10}
  };

  // ---- frozen_map ----
  {
    ankerl::nanobench::Bench bench;
    bench.title("frozen_map").unit("op").warmup(100).minEpochIterations(iterations);
    bench.run("P1 short(3) find", [&]{ auto r = p1_frozen.find("a") != p1_frozen.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P2 http(5) find", [&]{ auto r = p2_frozen.find("POST") != p2_frozen.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P3 prefix(4) find", [&]{ auto r = p3_frozen.find("timeout_ms") != p3_frozen.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P4 nato(20) find", [&]{ auto r = p4_frozen.find("golf") != p4_frozen.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P5 longkey(5) find", [&]{ auto r = p5_frozen.find("authentication_token_secret_key") != p5_frozen.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P6 samelen(10) find", [&]{ auto r = p6_frozen.find("configuration_key_fiv") != p6_frozen.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    (void)p7_frozen; (void)p7_stdmap; (void)p7_stdunordered;
  }

  // ---- frozen_trie_map ----
  {
    ankerl::nanobench::Bench bench;
    bench.title("frozen_trie_map").unit("op").warmup(100).minEpochIterations(iterations);
    bench.run("P1 short(3) find", [&]{ auto r = p1_trie.find("a") != p1_trie.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P2 http(5) find", [&]{ auto r = p2_trie.find("POST") != p2_trie.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P3 prefix(4) find", [&]{ auto r = p3_trie.find("timeout_ms") != p3_trie.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P4 nato(20) find", [&]{ auto r = p4_trie.find("golf") != p4_trie.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P5 longkey(5) find", [&]{ auto r = p5_trie.find("authentication_token_secret_key") != p5_trie.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P6 samelen(10) find", [&]{ auto r = p6_trie.find("configuration_key_fiv") != p6_trie.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  }

  // ---- std::map ----
  {
    ankerl::nanobench::Bench bench;
    bench.title("std::map").unit("op").warmup(100).minEpochIterations(iterations);
    bench.run("P1 short(3) find", [&]{ auto r = p1_stdmap.find("a") != p1_stdmap.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P2 http(5) find", [&]{ auto r = p2_stdmap.find("POST") != p2_stdmap.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P3 prefix(4) find", [&]{ auto r = p3_stdmap.find("timeout_ms") != p3_stdmap.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P4 nato(20) find", [&]{ auto r = p4_stdmap.find("golf") != p4_stdmap.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P5 longkey(5) find", [&]{ auto r = p5_stdmap.find("authentication_token_secret_key") != p5_stdmap.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P6 samelen(10) find", [&]{ auto r = p6_stdmap.find("configuration_key_fiv") != p6_stdmap.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  }

  // ---- std::unordered_map ----
  {
    ankerl::nanobench::Bench bench;
    bench.title("std::unordered_map").unit("op").warmup(100).minEpochIterations(iterations);
    bench.run("P1 short(3) find", [&]{ auto r = p1_stdunordered.find("a") != p1_stdunordered.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P2 http(5) find", [&]{ auto r = p2_stdunordered.find("POST") != p2_stdunordered.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P3 prefix(4) find", [&]{ auto r = p3_stdunordered.find("timeout_ms") != p3_stdunordered.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P4 nato(20) find", [&]{ auto r = p4_stdunordered.find("golf") != p4_stdunordered.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P5 longkey(5) find", [&]{ auto r = p5_stdunordered.find("authentication_token_secret_key") != p5_stdunordered.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("P6 samelen(10) find", [&]{ auto r = p6_stdunordered.find("configuration_key_fiv") != p6_stdunordered.end(); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  }

  ankerl::nanobench::doNotOptimizeAway(g_sink);
  return 0;
}
