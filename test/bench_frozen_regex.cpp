#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"
#include <ctre.hpp>

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
  std::cout << "\n[frozen_regex vs CTRE benchmark]\n\n";
  std::cout << std::left << std::setw(50) << "case"
            << std::right << std::setw(12) << "iters"
            << std::setw(14) << "total[ms]"
            << std::setw(14) << "ns/iter" << "\n";
  std::cout << std::string(50 + 12 + 14 + 14, '-') << "\n";
  for (auto const& r : results) {
    std::cout << std::left << std::setw(50) << r.name
              << std::right << std::setw(12) << r.iterations
              << std::setw(14) << std::fixed << std::setprecision(3) << r.total_ms
              << std::setw(14) << std::fixed << std::setprecision(1) << r.ns_per_iter << "\n";
  }
  std::cout << "\n[sink] " << g_sink << '\n';
}

template <typename RR>
[[nodiscard]] auto verify_fr(std::string_view text, bool expected) -> bool {
  auto const result = RR::contains(text);
  if (result != expected) {
    std::cerr << "VERIFY FAIL: frozen_regex contains(\"" << text << "\") = "
              << result << ", expected " << expected << "\n";
    return false;
  }
  return true;
}

template <auto& Pattern>
[[nodiscard]] auto verify_ctre(std::string_view text, bool expected) -> bool {
  auto const result = static_cast<bool>(ctre::match<Pattern>(text));
  if (result != expected) {
    std::cerr << "VERIFY FAIL: ctre::match<Pattern>(\"" << text << "\") = "
              << result << ", expected " << expected << "\n";
    return false;
  }
  return true;
}

} // namespace

int main(int argc, char** argv) {
  auto iterations = std::uint64_t{500'000};
  if (argc > 1) {
    auto const parsed = std::strtoull(argv[1], nullptr, 10);
    if (parsed > 0) iterations = static_cast<std::uint64_t>(parsed);
  }

  // ---- 正規表現型定義 ----
  using R_literal    = frozen_regex<"endpoint"_fs>;
  using R_small_alt  = frozen_regex<"GET|POST|PUT|DELETE"_fs>;
  using R_med_alt    = frozen_regex<"GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE"_fs>;
  using R_large_alt  = frozen_regex<"k01|k02|k03|k04|k05|k06|k07|k08|k09|k10|k11|k12|k13|k14|k15|k16|k17|k18|k19|k20"_fs>;
  using R_path_alt   = frozen_regex<"/api/v1/users|/api/v1/posts|/api/v1/comments|/api/v1/tags"_fs>;
  using R_cls        = frozen_regex<"[abc]"_fs>;
  using R_cls_wide   = frozen_regex<"[a-m]"_fs>;
  using R_dot        = frozen_regex<"."_fs>;

  // ---- 検証 ----
  bool all_ok = true;

  all_ok &= verify_fr<R_literal>("endpoint",    true);
  all_ok &= verify_fr<R_literal>("other",       false);
  all_ok &= verify_fr<R_small_alt>("GET",       true);
  all_ok &= verify_fr<R_small_alt>("DELETE",    true);
  all_ok &= verify_fr<R_small_alt>("PATCH",     false);
  all_ok &= verify_fr<R_med_alt>("OPTIONS",     true);
  all_ok &= verify_fr<R_med_alt>("CONNECT",     false);
  all_ok &= verify_fr<R_large_alt>("k01",       true);
  all_ok &= verify_fr<R_large_alt>("k20",       true);
  all_ok &= verify_fr<R_large_alt>("k99",       false);
  all_ok &= verify_fr<R_path_alt>("/api/v1/users",    true);
  all_ok &= verify_fr<R_path_alt>("/api/v1/other",    false);
  all_ok &= verify_fr<R_cls>("a",               true);
  all_ok &= verify_fr<R_cls>("d",               false);
  all_ok &= verify_fr<R_cls_wide>("a",          true);
  all_ok &= verify_fr<R_cls_wide>("m",          true);
  all_ok &= verify_fr<R_cls_wide>("z",          false);
  // 注: frozen_regex の dot は default_dot_chars [a-zA-Z0-9_] にのみマッチ
  //      CTRE の dot は全文字にマッチ（改行含む）ため比較は hit のみ
  all_ok &= verify_fr<R_dot>("a",               true);
  all_ok &= verify_fr<R_dot>("Z",               true);
  all_ok &= verify_fr<R_dot>("_",               true);

  // CTRE 用に ctll::fixed_string 変数
  static constexpr auto ctre_literal   = ctll::fixed_string{"endpoint"};
  static constexpr auto ctre_small_alt = ctll::fixed_string{"GET|POST|PUT|DELETE"};
  static constexpr auto ctre_med_alt   = ctll::fixed_string{"GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE"};
  static constexpr auto ctre_large_alt = ctll::fixed_string{"k01|k02|k03|k04|k05|k06|k07|k08|k09|k10|k11|k12|k13|k14|k15|k16|k17|k18|k19|k20"};
  static constexpr auto ctre_path_alt  = ctll::fixed_string{"/api/v1/users|/api/v1/posts|/api/v1/comments|/api/v1/tags"};
  static constexpr auto ctre_cls       = ctll::fixed_string{"[abc]"};
  static constexpr auto ctre_cls_wide  = ctll::fixed_string{"[a-m]"};
  static constexpr auto ctre_dot       = ctll::fixed_string{"."};

  all_ok &= verify_ctre<ctre_literal>("endpoint",    true);
  all_ok &= verify_ctre<ctre_literal>("other",       false);
  all_ok &= verify_ctre<ctre_small_alt>("GET",       true);
  all_ok &= verify_ctre<ctre_small_alt>("DELETE",    true);
  all_ok &= verify_ctre<ctre_small_alt>("PATCH",     false);
  all_ok &= verify_ctre<ctre_med_alt>("OPTIONS",     true);
  all_ok &= verify_ctre<ctre_med_alt>("CONNECT",     false);
  all_ok &= verify_ctre<ctre_large_alt>("k01",       true);
  all_ok &= verify_ctre<ctre_large_alt>("k20",       true);
  all_ok &= verify_ctre<ctre_large_alt>("k99",       false);
  all_ok &= verify_ctre<ctre_path_alt>("/api/v1/users", true);
  all_ok &= verify_ctre<ctre_path_alt>("/api/v1/other", false);
  all_ok &= verify_ctre<ctre_cls>("a",               true);
  all_ok &= verify_ctre<ctre_cls>("d",               false);
  all_ok &= verify_ctre<ctre_cls_wide>("a",          true);
  all_ok &= verify_ctre<ctre_cls_wide>("m",          true);
  all_ok &= verify_ctre<ctre_cls_wide>("z",          false);
  all_ok &= verify_ctre<ctre_dot>("a",               true);
  all_ok &= verify_ctre<ctre_dot>("Z",               true);
  all_ok &= verify_ctre<ctre_dot>("_",               true);

  if (!all_ok) {
    std::cerr << "Verification FAILED. Aborting benchmark.\n";
    return 1;
  }
  std::cout << "All verifications passed.\n";

  // ---- ベンチマーク実行 ----
  auto results = std::vector<bench_result>{};
  results.reserve(48);

  auto iters = iterations;

  // frozen_regex contains()
  results.push_back(measure("fr: lit('endpoint') hit",         [&]{ g_sink += R_literal::contains("endpoint"); }, iters));
  results.push_back(measure("fr: lit('endpoint') miss",        [&]{ g_sink += R_literal::contains("other"); }, iters));
  results.push_back(measure("fr: small(4) 'GET' hit",         [&]{ g_sink += R_small_alt::contains("GET"); }, iters));
  results.push_back(measure("fr: small(4) 'DELETE' hit",      [&]{ g_sink += R_small_alt::contains("DELETE"); }, iters));
  results.push_back(measure("fr: small(4) miss",              [&]{ g_sink += R_small_alt::contains("PATCH"); }, iters));
  results.push_back(measure("fr: med(8) 'OPTIONS' hit",       [&]{ g_sink += R_med_alt::contains("OPTIONS"); }, iters));
  results.push_back(measure("fr: med(8) miss",                [&]{ g_sink += R_med_alt::contains("CONNECT"); }, iters));
  results.push_back(measure("fr: large(20) 'k01' hit",        [&]{ g_sink += R_large_alt::contains("k01"); }, iters));
  results.push_back(measure("fr: large(20) 'k20' hit",        [&]{ g_sink += R_large_alt::contains("k20"); }, iters));
  results.push_back(measure("fr: large(20) miss",             [&]{ g_sink += R_large_alt::contains("k99"); }, iters));
  results.push_back(measure("fr: path(4) '/api/v1/...' hit",  [&]{ g_sink += R_path_alt::contains("/api/v1/users"); }, iters));
  results.push_back(measure("fr: path(4) miss",               [&]{ g_sink += R_path_alt::contains("/api/v1/other"); }, iters));
  results.push_back(measure("fr: cls[abc](3) 'a' hit",        [&]{ g_sink += R_cls::contains("a"); }, iters));
  results.push_back(measure("fr: cls[abc](3) miss",           [&]{ g_sink += R_cls::contains("d"); }, iters));
  results.push_back(measure("fr: cls[a-m](13) 'a' hit",       [&]{ g_sink += R_cls_wide::contains("a"); }, iters));
  results.push_back(measure("fr: cls[a-m](13) miss",          [&]{ g_sink += R_cls_wide::contains("z"); }, iters));
  results.push_back(measure("fr: dot(63) 'a' hit",            [&]{ g_sink += R_dot::contains("a"); }, iters));

  // frozen_regex enumerate() / keys()
  results.push_back(measure("fr: enumerate() small(4)",       [&]{ auto s = R_small_alt::enumerate(); g_sink += s.size(); }, iters));
  results.push_back(measure("fr: enumerate() large(20)",      [&]{ auto s = R_large_alt::enumerate(); g_sink += s.size(); }, iters));
  results.push_back(measure("fr: keys() med(8)",              [&]{ auto s = R_med_alt::keys(); g_sink += s.size(); }, iters));

  // frozen_regex to_frozen_map + lookup
  constexpr auto m_small = R_small_alt::template to_frozen_map<int, 1, 2, 3, 4>();
  results.push_back(measure("fr: to_map+at 'GET' hit",       [&]{ g_sink += m_small.at("GET"); }, iters));
  results.push_back(measure("fr: to_map+find miss",           [&]{ auto it = m_small.find("PATCH"); g_sink += (it != m_small.end()); }, iters));

  // ---- CTRE match() ----
  results.push_back(measure("ctre: lit('endpoint') hit",      [&]{ g_sink += static_cast<bool>(ctre::match<ctre_literal>("endpoint")); }, iters));
  results.push_back(measure("ctre: lit('endpoint') miss",     [&]{ g_sink += static_cast<bool>(ctre::match<ctre_literal>("other")); }, iters));
  results.push_back(measure("ctre: small(4) 'GET' hit",       [&]{ g_sink += static_cast<bool>(ctre::match<ctre_small_alt>("GET")); }, iters));
  results.push_back(measure("ctre: small(4) 'DELETE' hit",    [&]{ g_sink += static_cast<bool>(ctre::match<ctre_small_alt>("DELETE")); }, iters));
  results.push_back(measure("ctre: small(4) miss",            [&]{ g_sink += static_cast<bool>(ctre::match<ctre_small_alt>("PATCH")); }, iters));
  results.push_back(measure("ctre: med(8) 'OPTIONS' hit",     [&]{ g_sink += static_cast<bool>(ctre::match<ctre_med_alt>("OPTIONS")); }, iters));
  results.push_back(measure("ctre: med(8) miss",              [&]{ g_sink += static_cast<bool>(ctre::match<ctre_med_alt>("CONNECT")); }, iters));
  results.push_back(measure("ctre: large(20) 'k01' hit",      [&]{ g_sink += static_cast<bool>(ctre::match<ctre_large_alt>("k01")); }, iters));
  results.push_back(measure("ctre: large(20) 'k20' hit",      [&]{ g_sink += static_cast<bool>(ctre::match<ctre_large_alt>("k20")); }, iters));
  results.push_back(measure("ctre: large(20) miss",           [&]{ g_sink += static_cast<bool>(ctre::match<ctre_large_alt>("k99")); }, iters));
  results.push_back(measure("ctre: path(4) '/api/v1/...' hit",[&]{ g_sink += static_cast<bool>(ctre::match<ctre_path_alt>("/api/v1/users")); }, iters));
  results.push_back(measure("ctre: path(4) miss",             [&]{ g_sink += static_cast<bool>(ctre::match<ctre_path_alt>("/api/v1/other")); }, iters));
  results.push_back(measure("ctre: cls[abc](3) 'a' hit",      [&]{ g_sink += static_cast<bool>(ctre::match<ctre_cls>("a")); }, iters));
  results.push_back(measure("ctre: cls[abc](3) miss",         [&]{ g_sink += static_cast<bool>(ctre::match<ctre_cls>("d")); }, iters));
  results.push_back(measure("ctre: cls[a-m](13) 'a' hit",     [&]{ g_sink += static_cast<bool>(ctre::match<ctre_cls_wide>("a")); }, iters));
  results.push_back(measure("ctre: cls[a-m](13) miss",        [&]{ g_sink += static_cast<bool>(ctre::match<ctre_cls_wide>("z")); }, iters));
  results.push_back(measure("ctre: dot '.' 'a' hit",          [&]{ g_sink += static_cast<bool>(ctre::match<ctre_dot>("a")); }, iters));

  print_results(results);
  return 0;
}
