#include "frozenchars/literals.hpp"
#include "frozenchars/map.hpp"

#include <nanobench.h>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <string_view>

using namespace frozenchars;
using namespace frozenchars::literals;

/**
 * @brief frozen_map のルックアップ性能ベンチマーク。
 *   小（3 キー）・中（10）・大（20）・XL（50）・長キー（5）・同長キー（10）の各パターンで
 *   find / at / contains / get のヒット・ミスを計測する。
 */

namespace {

/** @brief 最適化防止用の揮発性シンク変数。ベンチマーク結果の書き込み先として使う。 */
volatile std::size_t g_sink = 0;

} // namespace

int main(int argc, char** argv) {
  auto iterations = std::uint64_t{500'000};
  if (argc > 1) {
    auto const parsed = std::strtoull(argv[1], nullptr, 10);
    if (parsed > 0) iterations = static_cast<std::uint64_t>(parsed);
  }

  // ---- 小マップ: 3キー、全長ユニーク ----
  constexpr auto small_map = frozen_map<int, "aa"_fs, "bbbbbb"_fs, "cccccddddddddd"_fs>{
    std::array<int, 3>{10, 20, 30}
  };

  // ---- 中マップ: 10キー、混在長 ----
  constexpr auto medium_map = frozen_map<int,
    "timeout"_fs, "retry"_fs, "backoff"_fs, "endpoint"_fs, "headers"_fs,
    "method"_fs, "path"_fs, "query"_fs, "body"_fs, "status"_fs>{
    std::array<int, 10>{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
  };

  // ---- 大マップ: 20キー ----
  constexpr auto large_map = frozen_map<int,
    "alpha"_fs, "bravo"_fs, "charlie"_fs, "delta"_fs, "echo"_fs,
    "foxtrot"_fs, "golf"_fs, "hotel"_fs, "india"_fs, "juliet"_fs,
    "kilo"_fs, "lima"_fs, "mike"_fs, "november"_fs, "oscar"_fs,
    "papa"_fs, "quebec"_fs, "romeo"_fs, "sierra"_fs, "tango"_fs>{
    std::array<int, 20>{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20}
  };

  // ---- XLマップ: 50キー ----
  constexpr auto xl_map = frozen_map<int,
    "k01"_fs, "k02"_fs, "k03"_fs, "k04"_fs, "k05"_fs,
    "k06"_fs, "k07"_fs, "k08"_fs, "k09"_fs, "k10"_fs,
    "k11"_fs, "k12"_fs, "k13"_fs, "k14"_fs, "k15"_fs,
    "k16"_fs, "k17"_fs, "k18"_fs, "k19"_fs, "k20"_fs,
    "k21"_fs, "k22"_fs, "k23"_fs, "k24"_fs, "k25"_fs,
    "k26"_fs, "k27"_fs, "k28"_fs, "k29"_fs, "k30"_fs,
    "k31"_fs, "k32"_fs, "k33"_fs, "k34"_fs, "k35"_fs,
    "k36"_fs, "k37"_fs, "k38"_fs, "k39"_fs, "k40"_fs,
    "k41"_fs, "k42"_fs, "k43"_fs, "k44"_fs, "k45"_fs,
    "k46"_fs, "k47"_fs, "k48"_fs, "k49"_fs, "k50"_fs>{
    std::array<int, 50>{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,
                         21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
                         41,42,43,44,45,46,47,48,49,50}
  };

  ankerl::nanobench::Bench bench;
  bench.title("frozen_map").unit("op").warmup(100).minEpochIterations(iterations);

  bench.run("small(3) find hit", [&]{ auto it = small_map.find("aa"); ankerl::nanobench::doNotOptimizeAway(it != small_map.end()); g_sink += (it != small_map.end()); });
  bench.run("small(3) find miss", [&]{ auto it = small_map.find("xx"); ankerl::nanobench::doNotOptimizeAway(it != small_map.end()); g_sink += (it != small_map.end()); });
  bench.run("small(3) at hit", [&]{ auto v = small_map.at("bbbbbb"); ankerl::nanobench::doNotOptimizeAway(v); g_sink += v; });

  bench.run("med(10) find hit first", [&]{ auto it = medium_map.find("timeout"); ankerl::nanobench::doNotOptimizeAway(it != medium_map.end()); g_sink += (it != medium_map.end()); });
  bench.run("med(10) find hit last", [&]{ auto it = medium_map.find("status"); ankerl::nanobench::doNotOptimizeAway(it != medium_map.end()); g_sink += (it != medium_map.end()); });
  bench.run("med(10) find miss lenOK", [&]{ auto it = medium_map.find("timeoutx"); ankerl::nanobench::doNotOptimizeAway(it != medium_map.end()); g_sink += (it != medium_map.end()); });
  bench.run("med(10) find miss lenBad", [&]{ auto it = medium_map.find("x"); ankerl::nanobench::doNotOptimizeAway(it != medium_map.end()); g_sink += (it != medium_map.end()); });
  bench.run("med(10) contains hit", [&]{ auto r = medium_map.contains("method"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("med(10) contains miss", [&]{ auto r = medium_map.contains("nothere"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("med(10) get hit", [&]{ auto v = medium_map.get("method"); ankerl::nanobench::doNotOptimizeAway(v); if (v) g_sink += v->get(); });
  bench.run("med(10) get miss", [&]{ auto v = medium_map.get("nothere"); ankerl::nanobench::doNotOptimizeAway(v); if (v) g_sink += v->get(); });

  bench.run("large(20) find hit", [&]{ auto it = large_map.find("golf"); ankerl::nanobench::doNotOptimizeAway(it != large_map.end()); g_sink += (it != large_map.end()); });
  bench.run("large(20) find miss", [&]{ auto it = large_map.find("zulu"); ankerl::nanobench::doNotOptimizeAway(it != large_map.end()); g_sink += (it != large_map.end()); });

  bench.run("xl(50) find hit first", [&]{ auto it = xl_map.find("k01"); ankerl::nanobench::doNotOptimizeAway(it != xl_map.end()); g_sink += (it != xl_map.end()); });
  bench.run("xl(50) find hit last", [&]{ auto it = xl_map.find("k50"); ankerl::nanobench::doNotOptimizeAway(it != xl_map.end()); g_sink += (it != xl_map.end()); });
  bench.run("xl(50) find miss", [&]{ auto it = xl_map.find("k99"); ankerl::nanobench::doNotOptimizeAway(it != xl_map.end()); g_sink += (it != xl_map.end()); });

  // 長キーマップ（5キー、25-40文字）: CRC 8バイトパスの動作確認
  constexpr auto longkey_map = frozen_map<int,
    "configuration_timeout_ms"_fs, "maximum_retry_count_param"_fs,
    "connection_pool_size_setting"_fs, "authentication_token_secret_key"_fs,
    "response_body_encoding_format"_fs>{
    std::array<int, 5>{100, 200, 300, 400, 500}
  };
  bench.run("longkey(5) find hit", [&]{ auto it = longkey_map.find("authentication_token_secret_key"); ankerl::nanobench::doNotOptimizeAway(it != longkey_map.end()); g_sink += (it != longkey_map.end()); });
  bench.run("longkey(5) find miss", [&]{ auto it = longkey_map.find("nonexistent_key_that_is_long_enough"); ankerl::nanobench::doNotOptimizeAway(it != longkey_map.end()); g_sink += (it != longkey_map.end()); });

  // 同長キー（10キー、全21文字）: ハッシュパスを強制（all_lengths_unique_ = false）
  constexpr auto samelen_map = frozen_map<int,
    "configuration_key_one"_fs, "configuration_key_two"_fs,
    "configuration_key_thr"_fs, "configuration_key_fou"_fs,
    "configuration_key_fiv"_fs, "configuration_key_six"_fs,
    "configuration_key_sev"_fs, "configuration_key_eig"_fs,
    "configuration_key_nin"_fs, "configuration_key_ten"_fs>{
    std::array<int, 10>{1,2,3,4,5,6,7,8,9,10}
  };
  bench.run("samelen(10) find hit", [&]{ auto it = samelen_map.find("configuration_key_fiv"); ankerl::nanobench::doNotOptimizeAway(it != samelen_map.end()); g_sink += (it != samelen_map.end()); });
  bench.run("samelen(10) find miss", [&]{ auto it = samelen_map.find("configuration_key_xxx"); ankerl::nanobench::doNotOptimizeAway(it != samelen_map.end()); g_sink += (it != samelen_map.end()); });

  // ---- XXLマップ: 100キー（k_lookup_threshold 超 → CHD 2 段ハッシュパス）----
  constexpr auto xxl_map = frozen_map<int,
    "key001"_fs, "key002"_fs, "key003"_fs, "key004"_fs, "key005"_fs,
    "key006"_fs, "key007"_fs, "key008"_fs, "key009"_fs, "key010"_fs,
    "key011"_fs, "key012"_fs, "key013"_fs, "key014"_fs, "key015"_fs,
    "key016"_fs, "key017"_fs, "key018"_fs, "key019"_fs, "key020"_fs,
    "key021"_fs, "key022"_fs, "key023"_fs, "key024"_fs, "key025"_fs,
    "key026"_fs, "key027"_fs, "key028"_fs, "key029"_fs, "key030"_fs,
    "key031"_fs, "key032"_fs, "key033"_fs, "key034"_fs, "key035"_fs,
    "key036"_fs, "key037"_fs, "key038"_fs, "key039"_fs, "key040"_fs,
    "key041"_fs, "key042"_fs, "key043"_fs, "key044"_fs, "key045"_fs,
    "key046"_fs, "key047"_fs, "key048"_fs, "key049"_fs, "key050"_fs,
    "key051"_fs, "key052"_fs, "key053"_fs, "key054"_fs, "key055"_fs,
    "key056"_fs, "key057"_fs, "key058"_fs, "key059"_fs, "key060"_fs,
    "key061"_fs, "key062"_fs, "key063"_fs, "key064"_fs, "key065"_fs,
    "key066"_fs, "key067"_fs, "key068"_fs, "key069"_fs, "key070"_fs,
    "key071"_fs, "key072"_fs, "key073"_fs, "key074"_fs, "key075"_fs,
    "key076"_fs, "key077"_fs, "key078"_fs, "key079"_fs, "key080"_fs,
    "key081"_fs, "key082"_fs, "key083"_fs, "key084"_fs, "key085"_fs,
    "key086"_fs, "key087"_fs, "key088"_fs, "key089"_fs, "key090"_fs,
    "key091"_fs, "key092"_fs, "key093"_fs, "key094"_fs, "key095"_fs,
    "key096"_fs, "key097"_fs, "key098"_fs, "key099"_fs, "key100"_fs>{
    [] {
      auto v = std::array<int, 100>{};
      for (auto i = 0uz; i < 100; ++i) v[i] = static_cast<int>(i + 1);
      return v;
    }()
  };
  bench.run("xxl(100) find hit first", [&]{ auto it = xxl_map.find("key001"); ankerl::nanobench::doNotOptimizeAway(it != xxl_map.end()); g_sink += (it != xxl_map.end()); });
  bench.run("xxl(100) find hit last", [&]{ auto it = xxl_map.find("key100"); ankerl::nanobench::doNotOptimizeAway(it != xxl_map.end()); g_sink += (it != xxl_map.end()); });
  bench.run("xxl(100) find miss lenOK", [&]{ auto it = xxl_map.find("key999"); ankerl::nanobench::doNotOptimizeAway(it != xxl_map.end()); g_sink += (it != xxl_map.end()); });
  bench.run("xxl(100) find miss lenBad", [&]{ auto it = xxl_map.find("nope"); ankerl::nanobench::doNotOptimizeAway(it != xxl_map.end()); g_sink += (it != xxl_map.end()); });

  {
    constexpr std::string_view keys[] = {"timeout","retry","backoff","endpoint","headers",
                                         "method","path","query","body","status"};
    std::size_t idx = 0;
    bench.run("med(10) find round-robin", [&]{
      auto it = medium_map.find(keys[idx % 10]);
      ankerl::nanobench::doNotOptimizeAway(it != medium_map.end());
      g_sink += (it != medium_map.end());
      ++idx;
    });
  }

  ankerl::nanobench::doNotOptimizeAway(g_sink);
  return 0;
}
