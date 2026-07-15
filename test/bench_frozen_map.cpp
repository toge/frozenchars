#include "frozenchars/literals.hpp"
#include "frozenchars/map.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace frozenchars;
using namespace frozenchars::literals;

/**
 * @brief frozen_map のルックアップ性能ベンチマーク。
 *   小（3 キー）・中（10）・大（20）・XL（50）・長キー（5）・同長キー（10）の各パターンで
 *   find / at / contains / get のヒット・ミスを計測する。
 */

namespace {

/** @brief ベンチマーク1件の計測結果を保持する構造体。名前・反復回数・経過時間・1反復あたりの時間を持つ。 */
struct bench_result {
  std::string_view name{};
  std::uint64_t iterations{};
  double total_ms{};
  double ns_per_iter{};
};

/** @brief 最適化防止用の揮発性シンク変数。ベンチマーク結果の書き込み先として使う。 */
volatile std::size_t g_sink = 0;

/** @brief 指定された関数を iterations 回実行し、経過時間を bench_result として返す。 */
template <typename Func>
auto measure(std::string_view name, Func&& fn, std::uint64_t iterations) -> bench_result {
  for (std::uint64_t i = 0; i < 500; ++i) fn();
  auto const begin = std::chrono::steady_clock::now();
  for (std::uint64_t i = 0; i < iterations; ++i) fn();
  auto const end = std::chrono::steady_clock::now();
  auto const elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
  return bench_result{
    .name = name,
    .iterations = iterations,
    .total_ms = static_cast<double>(elapsed_ns) / 1'000'000.0,
    .ns_per_iter = static_cast<double>(elapsed_ns) / static_cast<double>(iterations),
  };
}

/** @brief ベンチマーク結果の一覧を整形して標準出力に表示する。 */
auto print_results(std::vector<bench_result> const& results) -> void {
  std::cout << "\n[frozen_map benchmark]\n\n";
  std::cout << std::left << std::setw(44) << "case"
            << std::right << std::setw(12) << "iters"
            << std::setw(14) << "total[ms]"
            << std::setw(14) << "ns/iter" << "\n";
  std::cout << std::string(44 + 12 + 14 + 14, '-') << "\n";
  for (auto const& r : results) {
    std::cout << std::left << std::setw(44) << r.name
              << std::right << std::setw(12) << r.iterations
              << std::setw(14) << std::fixed << std::setprecision(3) << r.total_ms
              << std::setw(14) << std::fixed << std::setprecision(1) << r.ns_per_iter << "\n";
  }
  std::cout << "\n[sink] " << g_sink << '\n';
}

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

  auto results = std::vector<bench_result>{};
  results.reserve(40);

  auto iters = iterations;
  results.push_back(measure("small(3) find hit", [&]{ auto it = small_map.find("aa"); g_sink += (it != small_map.end()); }, iters));
  results.push_back(measure("small(3) find miss", [&]{ auto it = small_map.find("xx"); g_sink += (it != small_map.end()); }, iters));
  results.push_back(measure("small(3) at hit", [&]{ g_sink += small_map.at("bbbbbb"); }, iters));

  results.push_back(measure("med(10) find hit first", [&]{ auto it = medium_map.find("timeout"); g_sink += (it != medium_map.end()); }, iters));
  results.push_back(measure("med(10) find hit last", [&]{ auto it = medium_map.find("status"); g_sink += (it != medium_map.end()); }, iters));
  results.push_back(measure("med(10) find miss lenOK", [&]{ auto it = medium_map.find("timeoutx"); g_sink += (it != medium_map.end()); }, iters));
  results.push_back(measure("med(10) find miss lenBad", [&]{ auto it = medium_map.find("x"); g_sink += (it != medium_map.end()); }, iters));
  results.push_back(measure("med(10) contains hit", [&]{ g_sink += medium_map.contains("method"); }, iters));
  results.push_back(measure("med(10) contains miss", [&]{ g_sink += medium_map.contains("nothere"); }, iters));
  results.push_back(measure("med(10) get hit", [&]{ auto v = medium_map.get("method"); if (v) g_sink += v->get(); }, iters));
  results.push_back(measure("med(10) get miss", [&]{ auto v = medium_map.get("nothere"); if (v) g_sink += v->get(); }, iters));

  results.push_back(measure("large(20) find hit", [&]{ auto it = large_map.find("golf"); g_sink += (it != large_map.end()); }, iters));
  results.push_back(measure("large(20) find miss", [&]{ auto it = large_map.find("zulu"); g_sink += (it != large_map.end()); }, iters));

  results.push_back(measure("xl(50) find hit first", [&]{ auto it = xl_map.find("k01"); g_sink += (it != xl_map.end()); }, iters));
  results.push_back(measure("xl(50) find hit last", [&]{ auto it = xl_map.find("k50"); g_sink += (it != xl_map.end()); }, iters));
  results.push_back(measure("xl(50) find miss", [&]{ auto it = xl_map.find("k99"); g_sink += (it != xl_map.end()); }, iters));

  // 長キーマップ（5キー、25-40文字）: CRC 8バイトパスの動作確認
  constexpr auto longkey_map = frozen_map<int,
    "configuration_timeout_ms"_fs, "maximum_retry_count_param"_fs,
    "connection_pool_size_setting"_fs, "authentication_token_secret_key"_fs,
    "response_body_encoding_format"_fs>{
    std::array<int, 5>{100, 200, 300, 400, 500}
  };
  results.push_back(measure("longkey(5) find hit", [&]{ auto it = longkey_map.find("authentication_token_secret_key"); g_sink += (it != longkey_map.end()); }, iters));
  results.push_back(measure("longkey(5) find miss", [&]{ auto it = longkey_map.find("nonexistent_key_that_is_long_enough"); g_sink += (it != longkey_map.end()); }, iters));

  // 同長キー（10キー、全21文字）: ハッシュパスを強制（all_lengths_unique_ = false）
  constexpr auto samelen_map = frozen_map<int,
    "configuration_key_one"_fs, "configuration_key_two"_fs,
    "configuration_key_thr"_fs, "configuration_key_fou"_fs,
    "configuration_key_fiv"_fs, "configuration_key_six"_fs,
    "configuration_key_sev"_fs, "configuration_key_eig"_fs,
    "configuration_key_nin"_fs, "configuration_key_ten"_fs>{
    std::array<int, 10>{1,2,3,4,5,6,7,8,9,10}
  };
  results.push_back(measure("samelen(10) find hit", [&]{ auto it = samelen_map.find("configuration_key_fiv"); g_sink += (it != samelen_map.end()); }, iters));
  results.push_back(measure("samelen(10) find miss", [&]{ auto it = samelen_map.find("configuration_key_xxx"); g_sink += (it != samelen_map.end()); }, iters));

  {
    constexpr std::string_view keys[] = {"timeout","retry","backoff","endpoint","headers",
                                         "method","path","query","body","status"};
    std::size_t idx = 0;
    results.push_back(measure("med(10) find round-robin", [&]{
      auto it = medium_map.find(keys[idx % 10]);
      g_sink += (it != medium_map.end());
      ++idx;
    }, iters));
  }

  print_results(results);
  return 0;
}
