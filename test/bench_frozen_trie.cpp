#include "frozenchars/literals.hpp"
#include "frozenchars/map.hpp"
#include "frozenchars/trie_map.hpp"

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

auto print_results(std::string_view label, std::vector<bench_result> const& results) -> void {
  std::cout << "\n[" << label << "]\n\n";
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
  std::cout << "\n";
}

} // namespace

int main(int argc, char** argv) {
  auto iterations = std::uint64_t{500'000};
  if (argc > 1) {
    auto const parsed = std::strtoull(argv[1], nullptr, 10);
    if (parsed > 0) iterations = static_cast<std::uint64_t>(parsed);
  }

  // ---- Pattern 1: Short unique-first-char keys (3 keys) ----
  constexpr auto p1_map = frozen_map<int, "a"_fs, "b"_fs, "c"_fs>{
    std::array<int, 3>{1, 2, 3}
  };
  constexpr auto p1_trie = frozen_trie_map<int, "a"_fs, "b"_fs, "c"_fs>{
    std::array<int, 3>{1, 2, 3}
  };

  // ---- Pattern 2: HTTP methods (5 keys, mixed lengths) ----
  constexpr auto p2_map = frozen_map<int, "GET"_fs, "PUT"_fs, "POST"_fs, "DELETE"_fs, "HEAD"_fs>{
    std::array<int, 5>{1, 2, 3, 4, 5}
  };
  constexpr auto p2_trie = frozen_trie_map<int, "GET"_fs, "PUT"_fs, "POST"_fs, "DELETE"_fs, "HEAD"_fs>{
    std::array<int, 5>{1, 2, 3, 4, 5}
  };

  // ---- Pattern 3: Common prefix (4 keys) ----
  constexpr auto p3_map = frozen_map<int, "timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>{
    std::array<int, 4>{1, 2, 3, 4}
  };
  constexpr auto p3_trie = frozen_trie_map<int, "timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>{
    std::array<int, 4>{1, 2, 3, 4}
  };

  // ---- Pattern 4: NATO alphabet (20 keys), same as bench_frozen_map large_map ----
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

  // ---- Pattern 5: Long keys (>30 chars, 5 keys) ----
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

  // ---- Pattern 6: Same-length keys (10 keys, 21 chars each) ----
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

  // ---- Pattern 7: medium keys for round-robin ----
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

  auto map_results = std::vector<bench_result>{};
  auto trie_results = std::vector<bench_result>{};
  map_results.reserve(30);
  trie_results.reserve(30);

  auto iters = iterations;

  // ---- frozen_map measurements ----
  map_results.push_back(measure("P1 short(3) find hit-sum", [&]{
    auto it = p1_map.find("a"); g_sink += (it != p1_map.end());
    it = p1_map.find("b"); g_sink += (it != p1_map.end());
    it = p1_map.find("c"); g_sink += (it != p1_map.end());
  }, iters));
  map_results.push_back(measure("P1 short(3) contains miss", [&]{
    g_sink += p1_map.contains("x");
  }, iters));

  map_results.push_back(measure("P2 http(5) find hit-sum", [&]{
    auto it = p2_map.find("GET"); g_sink += (it != p2_map.end());
    it = p2_map.find("PUT"); g_sink += (it != p2_map.end());
    it = p2_map.find("POST"); g_sink += (it != p2_map.end());
  }, iters));
  map_results.push_back(measure("P2 http(5) contains miss", [&]{
    g_sink += p2_map.contains("PATCH");
  }, iters));

  map_results.push_back(measure("P3 prefix(4) find hit", [&]{
    auto it = p3_map.find("timeout"); g_sink += (it != p3_map.end());
    it = p3_map.find("timeout_ms"); g_sink += (it != p3_map.end());
  }, iters));
  map_results.push_back(measure("P3 prefix(4) contains miss", [&]{
    g_sink += p3_map.contains("timeout_abc");
  }, iters));

  map_results.push_back(measure("P4 nato(20) find hit", [&]{
    auto it = p4_map.find("golf"); g_sink += (it != p4_map.end());
  }, iters));
  map_results.push_back(measure("P4 nato(20) find miss", [&]{
    auto it = p4_map.find("zulu"); g_sink += (it != p4_map.end());
  }, iters));

  map_results.push_back(measure("P5 longkey(5) find hit", [&]{
    auto it = p5_map.find("authentication_token_secret_key"); g_sink += (it != p5_map.end());
  }, iters));
  map_results.push_back(measure("P5 longkey(5) find miss", [&]{
    auto it = p5_map.find("nonexistent_key_that_is_long_enough"); g_sink += (it != p5_map.end());
  }, iters));

  map_results.push_back(measure("P6 samelen(10) find hit", [&]{
    auto it = p6_map.find("configuration_key_fiv"); g_sink += (it != p6_map.end());
  }, iters));
  map_results.push_back(measure("P6 samelen(10) find miss", [&]{
    auto it = p6_map.find("configuration_key_xxx"); g_sink += (it != p6_map.end());
  }, iters));

  {
    constexpr std::string_view keys[] = {"timeout","retry","backoff","endpoint","headers",
                                         "method","path","query","body","status"};
    std::size_t idx = 0;
    map_results.push_back(measure("P7 med(10) find round-robin", [&]{
      auto it = p7_map.find(keys[idx % 10]);
      g_sink += (it != p7_map.end());
      ++idx;
    }, iters));
  }

  print_results("frozen_map benchmark", map_results);

  // ---- frozen_trie_map measurements ----
  trie_results.push_back(measure("P1 short(3) find hit-sum", [&]{
    auto it = p1_trie.find("a"); g_sink += (it != p1_trie.end());
    it = p1_trie.find("b"); g_sink += (it != p1_trie.end());
    it = p1_trie.find("c"); g_sink += (it != p1_trie.end());
  }, iters));
  trie_results.push_back(measure("P1 short(3) contains miss", [&]{
    g_sink += p1_trie.contains("x");
  }, iters));

  trie_results.push_back(measure("P2 http(5) find hit-sum", [&]{
    auto it = p2_trie.find("GET"); g_sink += (it != p2_trie.end());
    it = p2_trie.find("PUT"); g_sink += (it != p2_trie.end());
    it = p2_trie.find("POST"); g_sink += (it != p2_trie.end());
  }, iters));
  trie_results.push_back(measure("P2 http(5) contains miss", [&]{
    g_sink += p2_trie.contains("PATCH");
  }, iters));

  trie_results.push_back(measure("P3 prefix(4) find hit", [&]{
    auto it = p3_trie.find("timeout"); g_sink += (it != p3_trie.end());
    it = p3_trie.find("timeout_ms"); g_sink += (it != p3_trie.end());
  }, iters));
  trie_results.push_back(measure("P3 prefix(4) contains miss", [&]{
    g_sink += p3_trie.contains("timeout_abc");
  }, iters));

  trie_results.push_back(measure("P4 nato(20) find hit", [&]{
    auto it = p4_trie.find("golf"); g_sink += (it != p4_trie.end());
  }, iters));
  trie_results.push_back(measure("P4 nato(20) find miss", [&]{
    auto it = p4_trie.find("zulu"); g_sink += (it != p4_trie.end());
  }, iters));

  trie_results.push_back(measure("P5 longkey(5) find hit", [&]{
    auto it = p5_trie.find("authentication_token_secret_key"); g_sink += (it != p5_trie.end());
  }, iters));
  trie_results.push_back(measure("P5 longkey(5) find miss", [&]{
    auto it = p5_trie.find("nonexistent_key_that_is_long_enough"); g_sink += (it != p5_trie.end());
  }, iters));

  trie_results.push_back(measure("P6 samelen(10) find hit", [&]{
    auto it = p6_trie.find("configuration_key_fiv"); g_sink += (it != p6_trie.end());
  }, iters));
  trie_results.push_back(measure("P6 samelen(10) find miss", [&]{
    auto it = p6_trie.find("configuration_key_xxx"); g_sink += (it != p6_trie.end());
  }, iters));

  {
    constexpr std::string_view keys[] = {"timeout","retry","backoff","endpoint","headers",
                                         "method","path","query","body","status"};
    std::size_t idx = 0;
    trie_results.push_back(measure("P7 med(10) find round-robin", [&]{
      auto it = p7_trie.find(keys[idx % 10]);
      g_sink += (it != p7_trie.end());
      ++idx;
    }, iters));
  }

  print_results("frozen_trie_map benchmark", trie_results);

  std::cout << "[sink] " << g_sink << '\n';
  return 0;
}
