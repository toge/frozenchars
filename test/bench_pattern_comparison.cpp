#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"
#include "frozenchars/wildcard.hpp"
#include <ctre.hpp>
#include "wildcards.hpp"

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

/** @brief frozen_regex, CTRE, wildcard_match (frozenchars), wildcards (runtime) の
    パターンマッチング性能比較ベンチマーク。
    @details リテラル・小規模選択・中規模選択・パス・文字集合の各ケースで実行時間を計測する。*/

namespace {

/** @brief ベンチマーク結果を格納する構造体。*/
struct bench_result {
  std::string_view name{};
  std::uint64_t iterations{};
  double total_ms{};
  double ns_per_iter{};
};

/** @brief 最適化防止用シンク変数。*/
volatile std::size_t g_sink = 0;

/** @brief 任意の関数を複数回実行し実行時間を計測する。
    @tparam Func 計測対象の関数型
    @param name 結果表示ラベル
    @param fn 計測対象関数
    @param iterations 反復回数
    @return 計測結果 */
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

/** @brief ベンチマーク結果をテーブル形式で標準出力に表示する。
    @param label セクションラベル
    @param results 表示する結果の配列 */
auto print_results(std::string_view label, std::vector<bench_result> const& results) -> void {
  std::cout << "\n[" << label << "]\n\n";
  std::cout << std::left << std::setw(48) << "case"
            << std::right << std::setw(12) << "iters"
            << std::setw(14) << "total[ms]"
            << std::setw(14) << "ns/iter" << "\n";
  std::cout << std::string(48 + 12 + 14 + 14, '-') << "\n";
  for (auto const& r : results) {
    std::cout << std::left << std::setw(48) << r.name
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

  // ---- frozen_regex 型定義 ----
  using FR_literal   = frozen_regex<"endpoint"_fs>;
  using FR_small_alt = frozen_regex<"GET|POST|PUT|DELETE"_fs>;
  using FR_med_alt   = frozen_regex<"GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE"_fs>;
  using FR_path      = frozen_regex<"/api/v1/users|/api/v1/posts|/api/v1/comments|/api/v1/tags"_fs>;
  using FR_cls       = frozen_regex<"[abc]"_fs>;
  using FR_cls_wide  = frozen_regex<"[a-m]"_fs>;

  // ---- CTRE 用 fixed_string ----
  static constexpr auto ctre_literal   = ctll::fixed_string{"endpoint"};
  static constexpr auto ctre_small_alt = ctll::fixed_string{"GET|POST|PUT|DELETE"};
  static constexpr auto ctre_med_alt   = ctll::fixed_string{"GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE"};
  static constexpr auto ctre_path      = ctll::fixed_string{"/api/v1/users|/api/v1/posts|/api/v1/comments|/api/v1/tags"};
  static constexpr auto ctre_cls       = ctll::fixed_string{"[abc]"};
  static constexpr auto ctre_cls_wide  = ctll::fixed_string{"[a-m]"};

  // ---- wildcard_match 用 ----
  constexpr auto wc_literal   = FrozenString{"endpoint"};
  constexpr auto wc_small_alt = FrozenString{"(GET|POST|PUT|DELETE)"};
  constexpr auto wc_med_alt   = FrozenString{"(GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE)"};
  constexpr auto wc_path      = FrozenString{"(/api/v1/users|/api/v1/posts|/api/v1/comments|/api/v1/tags)"};
  constexpr auto wc_cls       = FrozenString{"[abc]"};
  constexpr auto wc_cls_wide  = FrozenString{"[a-m]"};

  auto fr_results   = std::vector<bench_result>{};
  auto ctre_results = std::vector<bench_result>{};
  auto wc_results   = std::vector<bench_result>{};
  auto rt_results   = std::vector<bench_result>{};
  fr_results.reserve(20);
  ctre_results.reserve(20);
  wc_results.reserve(20);
  rt_results.reserve(20);

  auto iters = iterations;

  // ========== frozen_regex ==========
  fr_results.push_back(measure("fr: lit('endpoint') hit",        [&]{ g_sink += FR_literal::contains("endpoint"); }, iters));
  fr_results.push_back(measure("fr: lit('endpoint') miss",       [&]{ g_sink += FR_literal::contains("other"); }, iters));
  fr_results.push_back(measure("fr: small(4) 'GET' hit",         [&]{ g_sink += FR_small_alt::contains("GET"); }, iters));
  fr_results.push_back(measure("fr: small(4) miss",              [&]{ g_sink += FR_small_alt::contains("PATCH"); }, iters));
  fr_results.push_back(measure("fr: med(8) 'OPTIONS' hit",       [&]{ g_sink += FR_med_alt::contains("OPTIONS"); }, iters));
  fr_results.push_back(measure("fr: med(8) miss",                [&]{ g_sink += FR_med_alt::contains("CONNECT"); }, iters));
  fr_results.push_back(measure("fr: path(4) '/api/v1/users'",    [&]{ g_sink += FR_path::contains("/api/v1/users"); }, iters));
  fr_results.push_back(measure("fr: path(4) miss",               [&]{ g_sink += FR_path::contains("/api/v1/other"); }, iters));
  fr_results.push_back(measure("fr: cls[abc] 'a' hit",           [&]{ g_sink += FR_cls::contains("a"); }, iters));
  fr_results.push_back(measure("fr: cls[abc] miss",              [&]{ g_sink += FR_cls::contains("d"); }, iters));
  fr_results.push_back(measure("fr: cls[a-m] 'a' hit",           [&]{ g_sink += FR_cls_wide::contains("a"); }, iters));
  fr_results.push_back(measure("fr: cls[a-m] miss",              [&]{ g_sink += FR_cls_wide::contains("z"); }, iters));

  // ========== CTRE ==========
  ctre_results.push_back(measure("ctre: lit('endpoint') hit",    [&]{ g_sink += static_cast<bool>(ctre::match<ctre_literal>("endpoint")); }, iters));
  ctre_results.push_back(measure("ctre: lit('endpoint') miss",   [&]{ g_sink += static_cast<bool>(ctre::match<ctre_literal>("other")); }, iters));
  ctre_results.push_back(measure("ctre: small(4) 'GET' hit",     [&]{ g_sink += static_cast<bool>(ctre::match<ctre_small_alt>("GET")); }, iters));
  ctre_results.push_back(measure("ctre: small(4) miss",          [&]{ g_sink += static_cast<bool>(ctre::match<ctre_small_alt>("PATCH")); }, iters));
  ctre_results.push_back(measure("ctre: med(8) 'OPTIONS' hit",   [&]{ g_sink += static_cast<bool>(ctre::match<ctre_med_alt>("OPTIONS")); }, iters));
  ctre_results.push_back(measure("ctre: med(8) miss",            [&]{ g_sink += static_cast<bool>(ctre::match<ctre_med_alt>("CONNECT")); }, iters));
  ctre_results.push_back(measure("ctre: path(4) '/api/v1/users'", [&]{ g_sink += static_cast<bool>(ctre::match<ctre_path>("/api/v1/users")); }, iters));
  ctre_results.push_back(measure("ctre: path(4) miss",           [&]{ g_sink += static_cast<bool>(ctre::match<ctre_path>("/api/v1/other")); }, iters));
  ctre_results.push_back(measure("ctre: cls[abc] 'a' hit",       [&]{ g_sink += static_cast<bool>(ctre::match<ctre_cls>("a")); }, iters));
  ctre_results.push_back(measure("ctre: cls[abc] miss",          [&]{ g_sink += static_cast<bool>(ctre::match<ctre_cls>("d")); }, iters));
  ctre_results.push_back(measure("ctre: cls[a-m] 'a' hit",       [&]{ g_sink += static_cast<bool>(ctre::match<ctre_cls_wide>("a")); }, iters));
  ctre_results.push_back(measure("ctre: cls[a-m] miss",          [&]{ g_sink += static_cast<bool>(ctre::match<ctre_cls_wide>("z")); }, iters));

  // ========== wildcard_match (frozenchars) ==========
  wc_results.push_back(measure("wc: lit('endpoint') hit",        [&]{ g_sink += wildcard_match<wc_literal>("endpoint"); }, iters));
  wc_results.push_back(measure("wc: lit('endpoint') miss",       [&]{ g_sink += wildcard_match<wc_literal>("other"); }, iters));
  wc_results.push_back(measure("wc: small(4) 'GET' hit",         [&]{ g_sink += wildcard_match<wc_small_alt>("GET"); }, iters));
  wc_results.push_back(measure("wc: small(4) miss",              [&]{ g_sink += wildcard_match<wc_small_alt>("PATCH"); }, iters));
  wc_results.push_back(measure("wc: med(8) 'OPTIONS' hit",       [&]{ g_sink += wildcard_match<wc_med_alt>("OPTIONS"); }, iters));
  wc_results.push_back(measure("wc: med(8) miss",                [&]{ g_sink += wildcard_match<wc_med_alt>("CONNECT"); }, iters));
  wc_results.push_back(measure("wc: path(4) '/api/v1/users'",    [&]{ g_sink += wildcard_match<wc_path>("/api/v1/users"); }, iters));
  wc_results.push_back(measure("wc: path(4) miss",               [&]{ g_sink += wildcard_match<wc_path>("/api/v1/other"); }, iters));
  wc_results.push_back(measure("wc: cls[abc] 'a' hit",           [&]{ g_sink += wildcard_match<wc_cls>("a"); }, iters));
  wc_results.push_back(measure("wc: cls[abc] miss",              [&]{ g_sink += wildcard_match<wc_cls>("d"); }, iters));
  wc_results.push_back(measure("wc: cls[a-m] 'a' hit",           [&]{ g_sink += wildcard_match<wc_cls_wide>("a"); }, iters));
  wc_results.push_back(measure("wc: cls[a-m] miss",              [&]{ g_sink += wildcard_match<wc_cls_wide>("z"); }, iters));

  // ========== wildcards (runtime) ==========
  rt_results.push_back(measure("rt: lit('endpoint') hit",        [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"endpoint"}, std::string{"endpoint"})); }, iters));
  rt_results.push_back(measure("rt: lit('endpoint') miss",       [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"other"}, std::string{"endpoint"})); }, iters));
  rt_results.push_back(measure("rt: small(4) 'GET' hit",         [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"GET"}, std::string{"GET|POST|PUT|DELETE"})); }, iters));
  rt_results.push_back(measure("rt: small(4) miss",              [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"PATCH"}, std::string{"GET|POST|PUT|DELETE"})); }, iters));
  rt_results.push_back(measure("rt: med(8) 'OPTIONS' hit",       [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"OPTIONS"}, std::string{"GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE"})); }, iters));
  rt_results.push_back(measure("rt: med(8) miss",                [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"CONNECT"}, std::string{"GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE"})); }, iters));
  rt_results.push_back(measure("rt: path(4) '/api/v1/users'",    [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"/api/v1/users"}, std::string{"/api/v1/users|/api/v1/posts|/api/v1/comments|/api/v1/tags"})); }, iters));
  rt_results.push_back(measure("rt: path(4) miss",               [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"/api/v1/other"}, std::string{"/api/v1/users|/api/v1/posts|/api/v1/comments|/api/v1/tags"})); }, iters));
  rt_results.push_back(measure("rt: cls[abc] 'a' hit",           [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"a"}, std::string{"[abc]"})); }, iters));
  rt_results.push_back(measure("rt: cls[abc] miss",              [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"d"}, std::string{"[abc]"})); }, iters));
  rt_results.push_back(measure("rt: cls[a-m] 'a' hit",           [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"a"}, std::string{"[a-m]"})); }, iters));
  rt_results.push_back(measure("rt: cls[a-m] miss",              [&]{ g_sink += static_cast<std::size_t>(wildcards::match(std::string{"z"}, std::string{"[a-m]"})); }, iters));

  print_results("frozen_regex", fr_results);
  print_results("CTRE", ctre_results);
  print_results("wildcard_match (frozenchars)", wc_results);
  print_results("wildcards (runtime)", rt_results);

  std::cout << "[sink] " << g_sink << '\n';
  return 0;
}
