#include "frozenchars.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace frozenchars;
using namespace frozenchars::inja;
using namespace frozenchars::literals;

namespace {

/**
 * @brief ベンチ 1 ケース分の計測結果。
 */
struct bench_result {
  std::string_view name{};
  std::uint64_t iterations{};
  double total_ms{};
  double ns_per_iter{};
  double mib_per_sec{};
  std::size_t bytes{};
};

/**
 * @brief 最適化で消されないように出力サイズを蓄積するシンク。
 */
volatile std::size_t g_sink = 0;

/**
 * @brief inja レンダリングを繰り返し実行して計測する。
 *
 * @tparam Src コンパイル時計算されるテンプレート文字列。
 * @param name ケース名。
 * @param root ルートコンテキスト。
 * @param options ランタイムオプション（必要な場合のみ）。
 * @param iterations 計測反復回数。
 */
template <auto Src>
auto run_case(std::string_view name,
              inja_value const& root,
              runtime_options const* options,
              std::uint64_t iterations) -> bench_result {
  // ウォームアップ（初回割り当てや命令キャッシュの偏りを緩和）。
  for (auto i = std::uint64_t{0}; i < 500; ++i) {
    auto const out = options ? render<Src>(root, std::cref(*options)) : render<Src>(root);
    g_sink += out.size();
  }

  auto bytes = std::size_t{0};
  auto const begin = std::chrono::steady_clock::now();
  for (auto i = std::uint64_t{0}; i < iterations; ++i) {
    auto const out = options ? render<Src>(root, std::cref(*options)) : render<Src>(root);
    bytes += out.size();
    g_sink += out.size();
  }
  auto const end = std::chrono::steady_clock::now();

  auto const elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
  auto const elapsed_ms = static_cast<double>(elapsed_ns) / 1'000'000.0;
  auto const ns_per_iter = static_cast<double>(elapsed_ns) / static_cast<double>(iterations);
  auto const sec = static_cast<double>(elapsed_ns) / 1'000'000'000.0;
  auto const mib_per_sec = sec > 0.0
                             ? (static_cast<double>(bytes) / (1024.0 * 1024.0)) / sec
                             : 0.0;

  return bench_result{
    .name = name,
    .iterations = iterations,
    .total_ms = elapsed_ms,
    .ns_per_iter = ns_per_iter,
    .mib_per_sec = mib_per_sec,
    .bytes = bytes,
  };
}

/**
 * @brief 結果テーブルを表示する。
 */
auto print_results(std::vector<bench_result> const& results) -> void {
  std::cout << "\n[inja runtime benchmark]\n";
  std::cout << "steady_clock.is_steady = " << std::chrono::steady_clock::is_steady << "\n\n";

  std::cout << std::left
            << std::setw(24) << "case"
            << std::right
            << std::setw(12) << "iters"
            << std::setw(14) << "total[ms]"
            << std::setw(14) << "ns/iter"
            << std::setw(14) << "MiB/s"
            << std::setw(14) << "bytes"
            << "\n";

  std::cout << std::string(24 + 12 + 14 + 14 + 14 + 14, '-') << "\n";

  for (auto const& r : results) {
    std::cout << std::left
              << std::setw(24) << r.name
              << std::right
              << std::setw(12) << r.iterations
              << std::setw(14) << std::fixed << std::setprecision(3) << r.total_ms
              << std::setw(14) << std::fixed << std::setprecision(1) << r.ns_per_iter
              << std::setw(14) << std::fixed << std::setprecision(2) << r.mib_per_sec
              << std::setw(14) << r.bytes
              << "\n";
  }

  std::cout << "\n[sink] " << g_sink << '\n';
}

} // namespace

int main(int argc, char** argv) {
  auto iterations = std::uint64_t{20'000};
  if (argc > 1) {
    auto const parsed = std::strtoull(argv[1], nullptr, 10);
    if (parsed > 0) {
      iterations = static_cast<std::uint64_t>(parsed);
    }
  }

  auto const common_items = make_array({
    make_object({{"name", "alpha"}, {"value", 11}}),
    make_object({{"name", "beta"}, {"value", 22}}),
    make_object({{"name", "gamma"}, {"value", 33}}),
    make_object({{"name", "delta"}, {"value", 44}}),
    make_object({{"name", "epsilon"}, {"value", 55}}),
  });

  auto const root_lookup = make_object({
    {"user", make_object({{"profile", make_object({{"city", "Tokyo"}})}})},
    {"constv", 777},
    {"items", common_items},
  });

  auto const root_numeric = make_object({
    {"scale", 1.125},
    {"nums", make_array({1, 2, 3, 4, 5, 6, 7, 8})},
  });

  auto const root_control = make_object({
    {"show", true},
    {"items", common_items},
  });

  auto options = runtime_options{};
  options.reserve_functions(8);
  options.reserve_includes(8);
  options.add_include("part", "|P|");
  options.add_function("sum3", [](std::vector<inja_value> const& args) -> inja_value {
    if (args.size() != 3) {
      throw render_error{"sum3 expects 3 arguments"};
    }
    return inja_value{as_int(args[0]) + as_int(args[1]) + as_int(args[2])};
  });

  constexpr auto tmpl_lookup =
    "{% for item in items %}{{ item.name }}-{{ user.profile.city }}-{{ constv }};{% endfor %}"_fs;

  constexpr auto tmpl_numeric =
    "{{ 123456789 + 987654321 }}|{{ 3.1415926535 * scale }}|{% for n in nums %}{{ n + 0.25 }}{% endfor %}"_fs;

  constexpr auto tmpl_control =
    "{% if show %}{% for x in items %}{% set t = upper(x.name) %}{{ t }}{{ sum3(1,2,3) }}{% include \"part\" %}{% endfor %}{% endif %}"_fs;

  auto results = std::vector<bench_result>{};
  results.reserve(3);
  results.push_back(run_case<tmpl_lookup>("lookup-heavy", root_lookup, nullptr, iterations));
  results.push_back(run_case<tmpl_numeric>("numeric-literal", root_numeric, nullptr, iterations));
  results.push_back(run_case<tmpl_control>("control-flow+include", root_control, &options, iterations));

  print_results(results);
  return 0;
}
