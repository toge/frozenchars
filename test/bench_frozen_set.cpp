#include "frozenchars/literals.hpp"
#include "frozenchars/set.hpp"

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

namespace {

struct bench_result {
  std::string_view name{};
  std::uint64_t iterations{};
  double total_ms{};
  double ns_per_iter{};
};

volatile std::size_t g_sink = 0;

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

auto print_results(std::vector<bench_result> const& results) -> void {
  std::cout << "\n[frozen_set benchmark]\n\n";
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

  // ---- Small set: 3 keys, all_lengths_unique ----
  constexpr auto small_set = frozen_set<
    "aa"_fs, "bbbbbb"_fs, "cccccddddddddd"_fs>{};

  // ---- Medium set: 10 keys, mixed lengths ----
  constexpr auto medium_set = frozen_set<
    "timeout"_fs, "retry"_fs, "backoff"_fs, "endpoint"_fs, "headers"_fs,
    "method"_fs, "path"_fs, "query"_fs, "body"_fs, "status"_fs>{};

  // ---- Large set: 20 keys ----
  constexpr auto large_set = frozen_set<
    "alpha"_fs, "bravo"_fs, "charlie"_fs, "delta"_fs, "echo"_fs,
    "foxtrot"_fs, "golf"_fs, "hotel"_fs, "india"_fs, "juliet"_fs,
    "kilo"_fs, "lima"_fs, "mike"_fs, "november"_fs, "oscar"_fs,
    "papa"_fs, "quebec"_fs, "romeo"_fs, "sierra"_fs, "tango"_fs>{};

  // ---- XL set: 50 keys (lookup table path, ≤64) ----
  constexpr auto xl_set = frozen_set<
    "k01"_fs, "k02"_fs, "k03"_fs, "k04"_fs, "k05"_fs,
    "k06"_fs, "k07"_fs, "k08"_fs, "k09"_fs, "k10"_fs,
    "k11"_fs, "k12"_fs, "k13"_fs, "k14"_fs, "k15"_fs,
    "k16"_fs, "k17"_fs, "k18"_fs, "k19"_fs, "k20"_fs,
    "k21"_fs, "k22"_fs, "k23"_fs, "k24"_fs, "k25"_fs,
    "k26"_fs, "k27"_fs, "k28"_fs, "k29"_fs, "k30"_fs,
    "k31"_fs, "k32"_fs, "k33"_fs, "k34"_fs, "k35"_fs,
    "k36"_fs, "k37"_fs, "k38"_fs, "k39"_fs, "k40"_fs,
    "k41"_fs, "k42"_fs, "k43"_fs, "k44"_fs, "k45"_fs,
    "k46"_fs, "k47"_fs, "k48"_fs, "k49"_fs, "k50"_fs>{};

  // ---- XXL 65: linear/binary search path, same-length keys (all_lengths_unique_ = false) ----
  constexpr auto xxl65_set = frozen_set<
    "k00"_fs, "k01"_fs, "k02"_fs, "k03"_fs, "k04"_fs,
    "k05"_fs, "k06"_fs, "k07"_fs, "k08"_fs, "k09"_fs,
    "k10"_fs, "k11"_fs, "k12"_fs, "k13"_fs, "k14"_fs,
    "k15"_fs, "k16"_fs, "k17"_fs, "k18"_fs, "k19"_fs,
    "k20"_fs, "k21"_fs, "k22"_fs, "k23"_fs, "k24"_fs,
    "k25"_fs, "k26"_fs, "k27"_fs, "k28"_fs, "k29"_fs,
    "k30"_fs, "k31"_fs, "k32"_fs, "k33"_fs, "k34"_fs,
    "k35"_fs, "k36"_fs, "k37"_fs, "k38"_fs, "k39"_fs,
    "k40"_fs, "k41"_fs, "k42"_fs, "k43"_fs, "k44"_fs,
    "k45"_fs, "k46"_fs, "k47"_fs, "k48"_fs, "k49"_fs,
    "k50"_fs, "k51"_fs, "k52"_fs, "k53"_fs, "k54"_fs,
    "k55"_fs, "k56"_fs, "k57"_fs, "k58"_fs, "k59"_fs,
    "k60"_fs, "k61"_fs, "k62"_fs, "k63"_fs, "k64"_fs>{};

  // ---- XXL 128: larger >64 set ----
  constexpr auto xxl128_set = frozen_set<
    "k000"_fs, "k001"_fs, "k002"_fs, "k003"_fs, "k004"_fs,
    "k005"_fs, "k006"_fs, "k007"_fs, "k008"_fs, "k009"_fs,
    "k010"_fs, "k011"_fs, "k012"_fs, "k013"_fs, "k014"_fs,
    "k015"_fs, "k016"_fs, "k017"_fs, "k018"_fs, "k019"_fs,
    "k020"_fs, "k021"_fs, "k022"_fs, "k023"_fs, "k024"_fs,
    "k025"_fs, "k026"_fs, "k027"_fs, "k028"_fs, "k029"_fs,
    "k030"_fs, "k031"_fs, "k032"_fs, "k033"_fs, "k034"_fs,
    "k035"_fs, "k036"_fs, "k037"_fs, "k038"_fs, "k039"_fs,
    "k040"_fs, "k041"_fs, "k042"_fs, "k043"_fs, "k044"_fs,
    "k045"_fs, "k046"_fs, "k047"_fs, "k048"_fs, "k049"_fs,
    "k050"_fs, "k051"_fs, "k052"_fs, "k053"_fs, "k054"_fs,
    "k055"_fs, "k056"_fs, "k057"_fs, "k058"_fs, "k059"_fs,
    "k060"_fs, "k061"_fs, "k062"_fs, "k063"_fs, "k064"_fs,
    "k065"_fs, "k066"_fs, "k067"_fs, "k068"_fs, "k069"_fs,
    "k070"_fs, "k071"_fs, "k072"_fs, "k073"_fs, "k074"_fs,
    "k075"_fs, "k076"_fs, "k077"_fs, "k078"_fs, "k079"_fs,
    "k080"_fs, "k081"_fs, "k082"_fs, "k083"_fs, "k084"_fs,
    "k085"_fs, "k086"_fs, "k087"_fs, "k088"_fs, "k089"_fs,
    "k090"_fs, "k091"_fs, "k092"_fs, "k093"_fs, "k094"_fs,
    "k095"_fs, "k096"_fs, "k097"_fs, "k098"_fs, "k099"_fs,
    "k100"_fs, "k101"_fs, "k102"_fs, "k103"_fs, "k104"_fs,
    "k105"_fs, "k106"_fs, "k107"_fs, "k108"_fs, "k109"_fs,
    "k110"_fs, "k111"_fs, "k112"_fs, "k113"_fs, "k114"_fs,
    "k115"_fs, "k116"_fs, "k117"_fs, "k118"_fs, "k119"_fs,
    "k120"_fs, "k121"_fs, "k122"_fs, "k123"_fs, "k124"_fs,
    "k125"_fs, "k126"_fs, "k127"_fs>{};

  // ---- Long-key set (5 keys, >16 bytes): short-key opt off ----
  constexpr auto longkey_set = frozen_set<
    "configuration_timeout_ms"_fs, "maximum_retry_count_param"_fs,
    "connection_pool_size_setting"_fs, "authentication_token_secret_key"_fs,
    "response_body_encoding_format"_fs>{};

  // ---- Same-length set (10 keys, all 21 chars): forces hash path (all_lengths_unique_ = false) ----
  constexpr auto samelen_set = frozen_set<
    "configuration_key_one"_fs, "configuration_key_two"_fs,
    "configuration_key_thr"_fs, "configuration_key_fou"_fs,
    "configuration_key_fiv"_fs, "configuration_key_six"_fs,
    "configuration_key_sev"_fs, "configuration_key_eig"_fs,
    "configuration_key_nin"_fs, "configuration_key_ten"_fs>{};

  auto results = std::vector<bench_result>{};
  results.reserve(40);

  auto iters = iterations;

  results.push_back(measure("small(3) contains hit",    [&]{ g_sink += small_set.contains("aa"); }, iters));
  results.push_back(measure("small(3) contains miss",   [&]{ g_sink += small_set.contains("xx"); }, iters));
  results.push_back(measure("small(3) find hit",        [&]{ auto it = small_set.find("aa"); g_sink += (it != small_set.end()); }, iters));
  results.push_back(measure("small(3) find miss",       [&]{ auto it = small_set.find("xx"); g_sink += (it != small_set.end()); }, iters));

  results.push_back(measure("med(10) contains hit",     [&]{ g_sink += medium_set.contains("method"); }, iters));
  results.push_back(measure("med(10) contains miss",    [&]{ g_sink += medium_set.contains("nothere"); }, iters));
  results.push_back(measure("med(10) find hit first",   [&]{ auto it = medium_set.find("timeout"); g_sink += (it != medium_set.end()); }, iters));
  results.push_back(measure("med(10) find hit last",    [&]{ auto it = medium_set.find("status"); g_sink += (it != medium_set.end()); }, iters));
  results.push_back(measure("med(10) find miss lenOK",  [&]{ auto it = medium_set.find("timeoutx"); g_sink += (it != medium_set.end()); }, iters));
  results.push_back(measure("med(10) find miss lenBad", [&]{ auto it = medium_set.find("x"); g_sink += (it != medium_set.end()); }, iters));
  results.push_back(measure("med(10) count hit",        [&]{ g_sink += medium_set.count("method"); }, iters));

  results.push_back(measure("large(20) contains hit",    [&]{ g_sink += large_set.contains("golf"); }, iters));
  results.push_back(measure("large(20) contains miss",   [&]{ g_sink += large_set.contains("zulu"); }, iters));
  results.push_back(measure("large(20) find hit",        [&]{ auto it = large_set.find("golf"); g_sink += (it != large_set.end()); }, iters));

  // XL (≤64 lookup table path)
  results.push_back(measure("xl(50) contains hit first", [&]{ g_sink += xl_set.contains("k01"); }, iters));
  results.push_back(measure("xl(50) contains hit last",  [&]{ g_sink += xl_set.contains("k50"); }, iters));
  results.push_back(measure("xl(50) contains miss",      [&]{ g_sink += xl_set.contains("k99"); }, iters));
  results.push_back(measure("xl(50) find hit",           [&]{ auto it = xl_set.find("k25"); g_sink += (it != xl_set.end()); }, iters));

  // XXL (>64 path: currently linear, will become binary search)
  results.push_back(measure("xxl(65) contains hit first", [&]{ g_sink += xxl65_set.contains("k00"); }, iters));
  results.push_back(measure("xxl(65) contains hit last",  [&]{ g_sink += xxl65_set.contains("k64"); }, iters));
  results.push_back(measure("xxl(65) contains hit mid",   [&]{ g_sink += xxl65_set.contains("k32"); }, iters));
  results.push_back(measure("xxl(65) contains miss",      [&]{ g_sink += xxl65_set.contains("k99"); }, iters));
  results.push_back(measure("xxl(65) find hit",           [&]{ auto it = xxl65_set.find("k32"); g_sink += (it != xxl65_set.end()); }, iters));

  results.push_back(measure("xxl(128) contains hit first", [&]{ g_sink += xxl128_set.contains("k000"); }, iters));
  results.push_back(measure("xxl(128) contains hit last",  [&]{ g_sink += xxl128_set.contains("k127"); }, iters));
  results.push_back(measure("xxl(128) contains miss",      [&]{ g_sink += xxl128_set.contains("k999"); }, iters));
  results.push_back(measure("xxl(128) find hit",           [&]{ auto it = xxl128_set.find("k064"); g_sink += (it != xxl128_set.end()); }, iters));

  // Long-key (>16 bytes): short-key opt off, CRC 8-byte path
  results.push_back(measure("longkey(5) contains hit", [&]{ g_sink += longkey_set.contains("authentication_token_secret_key"); }, iters));
  results.push_back(measure("longkey(5) contains miss", [&]{ g_sink += longkey_set.contains("nonexistent_key_that_is_long_enough"); }, iters));
  results.push_back(measure("longkey(5) find hit",     [&]{ auto it = longkey_set.find("authentication_token_secret_key"); g_sink += (it != longkey_set.end()); }, iters));

  // Same-length (forces hash path via detail::hash_impl)
  results.push_back(measure("samelen(10) contains hit", [&]{ g_sink += samelen_set.contains("configuration_key_fiv"); }, iters));
  results.push_back(measure("samelen(10) contains miss", [&]{ g_sink += samelen_set.contains("configuration_key_xxx"); }, iters));
  results.push_back(measure("samelen(10) find hit",     [&]{ auto it = samelen_set.find("configuration_key_fiv"); g_sink += (it != samelen_set.end()); }, iters));

  // Round-robin: cycle through medium set keys
  {
    constexpr std::string_view keys[] = {"timeout","retry","backoff","endpoint","headers",
                                         "method","path","query","body","status"};
    std::size_t idx = 0;
    results.push_back(measure("med(10) find round-robin", [&]{
      auto it = medium_set.find(keys[idx % 10]);
      g_sink += (it != medium_set.end());
      ++idx;
    }, iters));
  }

  print_results(results);
  return 0;
}
