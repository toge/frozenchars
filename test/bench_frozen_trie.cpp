#include "frozenchars/literals.hpp"
#include "frozenchars/map.hpp"
#include "frozenchars/trie_map.hpp"

#include <nanobench.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <string_view>

using namespace frozenchars;
using namespace frozenchars::literals;

/**
 * @brief frozen_map と frozen_trie_map のルックアップ性能比較ベンチマーク。
 *   7 パターン（短キー・HTTP メソッド・共通プレフィックス・NATO 20 キー・長キー・同長キー・ラウンドロビン）で
 *   両コンテナの find / contains を同一条件で計測する。
 */

namespace {

/** @brief 最適化防止用の揮発性シンク変数。ベンチマーク結果の書き込み先として使う。 */
std::size_t g_sink = 0;

} // namespace

int main(int argc, char** argv) {
  auto iterations = std::uint64_t{500'000};
  if (argc > 1) {
    auto const parsed = std::strtoull(argv[1], nullptr, 10);
    if (parsed > 0) iterations = static_cast<std::uint64_t>(parsed);
  }

  // ---- パターン1: 短い先頭ユニークキー（3キー） ----
  constexpr auto p1_map = frozen_map<int, "a"_fs, "b"_fs, "c"_fs>{
    std::array<int, 3>{1, 2, 3}
  };
  constexpr auto p1_trie = frozen_trie_map<int, "a"_fs, "b"_fs, "c"_fs>{
    std::array<int, 3>{1, 2, 3}
  };

  // ---- パターン2: HTTPメソッド（5キー、混在長） ----
  constexpr auto p2_map = frozen_map<int, "GET"_fs, "PUT"_fs, "POST"_fs, "DELETE"_fs, "HEAD"_fs>{
    std::array<int, 5>{1, 2, 3, 4, 5}
  };
  constexpr auto p2_trie = frozen_trie_map<int, "GET"_fs, "PUT"_fs, "POST"_fs, "DELETE"_fs, "HEAD"_fs>{
    std::array<int, 5>{1, 2, 3, 4, 5}
  };

  // ---- パターン3: 共通プレフィックス（4キー） ----
  constexpr auto p3_map = frozen_map<int, "timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>{
    std::array<int, 4>{1, 2, 3, 4}
  };
  constexpr auto p3_trie = frozen_trie_map<int, "timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>{
    std::array<int, 4>{1, 2, 3, 4}
  };

  // ---- パターン4: NATOアルファベット（20キー）、bench_frozen_map large_map と同じ ----
  constexpr auto p4_map = frozen_map<int,
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

  // ---- パターン5: 長キー（>30文字、5キー） ----
  constexpr auto p5_map = frozen_map<int,
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

  // ---- パターン6: 同長キー（10キー、各21文字） ----
  constexpr auto p6_map = frozen_map<int,
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

  // ---- パターン7: ラウンドロビン用中キー ----
  constexpr auto p7_map = frozen_map<int,
    "timeout"_fs, "retry"_fs, "backoff"_fs, "endpoint"_fs, "headers"_fs,
    "method"_fs, "path"_fs, "query"_fs, "body"_fs, "status"_fs>{
    std::array<int, 10>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
  };
  constexpr auto p7_trie = frozen_trie_map<int,
    "timeout"_fs, "retry"_fs, "backoff"_fs, "endpoint"_fs, "headers"_fs,
    "method"_fs, "path"_fs, "query"_fs, "body"_fs, "status"_fs>{
    std::array<int, 10>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
  };

  // ---- frozen_map 計測 ----
  {
    ankerl::nanobench::Bench bench;
    bench.title("frozen_map benchmark").unit("op").warmup(100).minEpochIterations(iterations);

    bench.run("P1 short(3) find hit-sum", [&]{
      auto it = p1_map.find("a"); ankerl::nanobench::doNotOptimizeAway(it != p1_map.end()); g_sink += (it != p1_map.end());
      it = p1_map.find("b"); ankerl::nanobench::doNotOptimizeAway(it != p1_map.end()); g_sink += (it != p1_map.end());
      it = p1_map.find("c"); ankerl::nanobench::doNotOptimizeAway(it != p1_map.end()); g_sink += (it != p1_map.end());
    });
    bench.run("P1 short(3) contains miss", [&]{
      auto r = p1_map.contains("x"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r;
    });

    bench.run("P2 http(5) find hit-sum", [&]{
      auto it = p2_map.find("GET"); ankerl::nanobench::doNotOptimizeAway(it != p2_map.end()); g_sink += (it != p2_map.end());
      it = p2_map.find("PUT"); ankerl::nanobench::doNotOptimizeAway(it != p2_map.end()); g_sink += (it != p2_map.end());
      it = p2_map.find("POST"); ankerl::nanobench::doNotOptimizeAway(it != p2_map.end()); g_sink += (it != p2_map.end());
    });
    bench.run("P2 http(5) contains miss", [&]{
      auto r = p2_map.contains("PATCH"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r;
    });

    bench.run("P3 prefix(4) find hit", [&]{
      auto it = p3_map.find("timeout"); ankerl::nanobench::doNotOptimizeAway(it != p3_map.end()); g_sink += (it != p3_map.end());
      it = p3_map.find("timeout_ms"); ankerl::nanobench::doNotOptimizeAway(it != p3_map.end()); g_sink += (it != p3_map.end());
    });
    bench.run("P3 prefix(4) contains miss", [&]{
      auto r = p3_map.contains("timeout_abc"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r;
    });

    bench.run("P4 nato(20) find hit", [&]{
      auto it = p4_map.find("golf"); ankerl::nanobench::doNotOptimizeAway(it != p4_map.end()); g_sink += (it != p4_map.end());
    });
    bench.run("P4 nato(20) find miss", [&]{
      auto it = p4_map.find("zulu"); ankerl::nanobench::doNotOptimizeAway(it != p4_map.end()); g_sink += (it != p4_map.end());
    });

    bench.run("P5 longkey(5) find hit", [&]{
      auto it = p5_map.find("authentication_token_secret_key"); ankerl::nanobench::doNotOptimizeAway(it != p5_map.end()); g_sink += (it != p5_map.end());
    });
    bench.run("P5 longkey(5) find miss", [&]{
      auto it = p5_map.find("nonexistent_key_that_is_long_enough"); ankerl::nanobench::doNotOptimizeAway(it != p5_map.end()); g_sink += (it != p5_map.end());
    });

    bench.run("P6 samelen(10) find hit", [&]{
      auto it = p6_map.find("configuration_key_fiv"); ankerl::nanobench::doNotOptimizeAway(it != p6_map.end()); g_sink += (it != p6_map.end());
    });
    bench.run("P6 samelen(10) find miss", [&]{
      auto it = p6_map.find("configuration_key_xxx"); ankerl::nanobench::doNotOptimizeAway(it != p6_map.end()); g_sink += (it != p6_map.end());
    });

    {
      constexpr std::string_view keys[] = {"timeout","retry","backoff","endpoint","headers",
                                           "method","path","query","body","status"};
      std::size_t idx = 0;
      bench.run("P7 med(10) find round-robin", [&]{
        auto it = p7_map.find(keys[idx % 10]);
        ankerl::nanobench::doNotOptimizeAway(it != p7_map.end());
        g_sink += (it != p7_map.end());
        ++idx;
      });
    }
  }

  // ---- frozen_trie_map 計測 ----
  {
    ankerl::nanobench::Bench bench;
    bench.title("frozen_trie_map benchmark").unit("op").warmup(100).minEpochIterations(iterations);

    bench.run("P1 short(3) find hit-sum", [&]{
      auto it = p1_trie.find("a"); ankerl::nanobench::doNotOptimizeAway(it != p1_trie.end()); g_sink += (it != p1_trie.end());
      it = p1_trie.find("b"); ankerl::nanobench::doNotOptimizeAway(it != p1_trie.end()); g_sink += (it != p1_trie.end());
      it = p1_trie.find("c"); ankerl::nanobench::doNotOptimizeAway(it != p1_trie.end()); g_sink += (it != p1_trie.end());
    });
    bench.run("P1 short(3) contains miss", [&]{
      auto r = p1_trie.contains("x"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r;
    });

    bench.run("P2 http(5) find hit-sum", [&]{
      auto it = p2_trie.find("GET"); ankerl::nanobench::doNotOptimizeAway(it != p2_trie.end()); g_sink += (it != p2_trie.end());
      it = p2_trie.find("PUT"); ankerl::nanobench::doNotOptimizeAway(it != p2_trie.end()); g_sink += (it != p2_trie.end());
      it = p2_trie.find("POST"); ankerl::nanobench::doNotOptimizeAway(it != p2_trie.end()); g_sink += (it != p2_trie.end());
    });
    bench.run("P2 http(5) contains miss", [&]{
      auto r = p2_trie.contains("PATCH"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r;
    });

    bench.run("P3 prefix(4) find hit", [&]{
      auto it = p3_trie.find("timeout"); ankerl::nanobench::doNotOptimizeAway(it != p3_trie.end()); g_sink += (it != p3_trie.end());
      it = p3_trie.find("timeout_ms"); ankerl::nanobench::doNotOptimizeAway(it != p3_trie.end()); g_sink += (it != p3_trie.end());
    });
    bench.run("P3 prefix(4) contains miss", [&]{
      auto r = p3_trie.contains("timeout_abc"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r;
    });

    bench.run("P4 nato(20) find hit", [&]{
      auto it = p4_trie.find("golf"); ankerl::nanobench::doNotOptimizeAway(it != p4_trie.end()); g_sink += (it != p4_trie.end());
    });
    bench.run("P4 nato(20) find miss", [&]{
      auto it = p4_trie.find("zulu"); ankerl::nanobench::doNotOptimizeAway(it != p4_trie.end()); g_sink += (it != p4_trie.end());
    });

    bench.run("P5 longkey(5) find hit", [&]{
      auto it = p5_trie.find("authentication_token_secret_key"); ankerl::nanobench::doNotOptimizeAway(it != p5_trie.end()); g_sink += (it != p5_trie.end());
    });
    bench.run("P5 longkey(5) find miss", [&]{
      auto it = p5_trie.find("nonexistent_key_that_is_long_enough"); ankerl::nanobench::doNotOptimizeAway(it != p5_trie.end()); g_sink += (it != p5_trie.end());
    });

    bench.run("P6 samelen(10) find hit", [&]{
      auto it = p6_trie.find("configuration_key_fiv"); ankerl::nanobench::doNotOptimizeAway(it != p6_trie.end()); g_sink += (it != p6_trie.end());
    });
    bench.run("P6 samelen(10) find miss", [&]{
      auto it = p6_trie.find("configuration_key_xxx"); ankerl::nanobench::doNotOptimizeAway(it != p6_trie.end()); g_sink += (it != p6_trie.end());
    });

    {
      constexpr std::string_view keys[] = {"timeout","retry","backoff","endpoint","headers",
                                           "method","path","query","body","status"};
      std::size_t idx = 0;
      bench.run("P7 med(10) find round-robin", [&]{
        auto it = p7_trie.find(keys[idx % 10]);
        ankerl::nanobench::doNotOptimizeAway(it != p7_trie.end());
        g_sink += (it != p7_trie.end());
        ++idx;
      });
    }
  }

  ankerl::nanobench::doNotOptimizeAway(g_sink);
  return 0;
}
