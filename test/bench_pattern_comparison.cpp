#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"
#include "frozenchars/wildcard.hpp"
#include <ctre.hpp>
#include "wildcards.hpp"

#include <nanobench.h>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>

using namespace frozenchars;
using namespace frozenchars::literals;

/** @brief frozen_regex, CTRE, wildcard_match (frozenchars), wildcards (runtime) の
    パターンマッチング性能比較ベンチマーク。
    @details リテラル・小規模選択・中規模選択・パス・文字集合の各ケースで実行時間を計測する。*/

namespace {

/** @brief 最適化防止用シンク変数。*/
std::size_t g_sink = 0;

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
  constexpr auto wc_nset_abc_de = FrozenString{"[!abc]de"};
  constexpr auto wc_prefix_alt  = FrozenString{"prefix_(ab|cde)_suffix"};
  constexpr auto wc_cls       = FrozenString{"[abc]"};
  constexpr auto wc_cls_wide  = FrozenString{"[a-m]"};

  // ========== frozen_regex ==========
  {
    ankerl::nanobench::Bench bench;
    bench.title("frozen_regex").unit("op").warmup(100).minEpochIterations(iterations);
    bench.run("fr: lit('endpoint') hit",        [&]{ auto r = FR_literal::contains("endpoint"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: lit('endpoint') miss",       [&]{ auto r = FR_literal::contains("other"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: small(4) 'GET' hit",         [&]{ auto r = FR_small_alt::contains("GET"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: small(4) miss",              [&]{ auto r = FR_small_alt::contains("PATCH"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: med(8) 'OPTIONS' hit",       [&]{ auto r = FR_med_alt::contains("OPTIONS"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: med(8) miss",                [&]{ auto r = FR_med_alt::contains("CONNECT"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: path(4) '/api/v1/users'",    [&]{ auto r = FR_path::contains("/api/v1/users"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: path(4) miss",               [&]{ auto r = FR_path::contains("/api/v1/other"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: cls[abc] 'a' hit",           [&]{ auto r = FR_cls::contains("a"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: cls[abc] miss",              [&]{ auto r = FR_cls::contains("d"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: cls[a-m] 'a' hit",           [&]{ auto r = FR_cls_wide::contains("a"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("fr: cls[a-m] miss",              [&]{ auto r = FR_cls_wide::contains("z"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  }

  // ========== CTRE ==========
  {
    ankerl::nanobench::Bench bench;
    bench.title("CTRE").unit("op").warmup(100).minEpochIterations(iterations);
    bench.run("ctre: lit('endpoint') hit",    [&]{ auto r = static_cast<bool>(ctre::match<ctre_literal>("endpoint")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: lit('endpoint') miss",   [&]{ auto r = static_cast<bool>(ctre::match<ctre_literal>("other")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: small(4) 'GET' hit",     [&]{ auto r = static_cast<bool>(ctre::match<ctre_small_alt>("GET")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: small(4) miss",          [&]{ auto r = static_cast<bool>(ctre::match<ctre_small_alt>("PATCH")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: med(8) 'OPTIONS' hit",   [&]{ auto r = static_cast<bool>(ctre::match<ctre_med_alt>("OPTIONS")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: med(8) miss",            [&]{ auto r = static_cast<bool>(ctre::match<ctre_med_alt>("CONNECT")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: path(4) '/api/v1/users'", [&]{ auto r = static_cast<bool>(ctre::match<ctre_path>("/api/v1/users")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: path(4) miss",           [&]{ auto r = static_cast<bool>(ctre::match<ctre_path>("/api/v1/other")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: cls[abc] 'a' hit",       [&]{ auto r = static_cast<bool>(ctre::match<ctre_cls>("a")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: cls[abc] miss",          [&]{ auto r = static_cast<bool>(ctre::match<ctre_cls>("d")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: cls[a-m] 'a' hit",       [&]{ auto r = static_cast<bool>(ctre::match<ctre_cls_wide>("a")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("ctre: cls[a-m] miss",          [&]{ auto r = static_cast<bool>(ctre::match<ctre_cls_wide>("z")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  }

  // ========== wildcard_match (frozenchars) ==========
  {
    ankerl::nanobench::Bench bench;
    bench.title("wildcard_match (frozenchars)").unit("op").warmup(100).minEpochIterations(iterations);
    bench.run("wc: lit('endpoint') hit",        [&]{ auto r = wildcard_match<wc_literal>("endpoint"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: lit('endpoint') miss",       [&]{ auto r = wildcard_match<wc_literal>("other"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: small(4) 'GET' hit",         [&]{ auto r = wildcard_match<wc_small_alt>("GET"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: small(4) miss",              [&]{ auto r = wildcard_match<wc_small_alt>("PATCH"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: med(8) 'OPTIONS' hit",       [&]{ auto r = wildcard_match<wc_med_alt>("OPTIONS"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: med(8) miss",                [&]{ auto r = wildcard_match<wc_med_alt>("CONNECT"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: path(4) '/api/v1/users'",    [&]{ auto r = wildcard_match<wc_path>("/api/v1/users"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: path(4) miss",               [&]{ auto r = wildcard_match<wc_path>("/api/v1/other"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: cls[!abc] 'x' hit",          [&]{ auto r = wildcard_match<wc_nset_abc_de>("xde"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: cls[!abc] miss",             [&]{ auto r = wildcard_match<wc_nset_abc_de>("cde"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: prefix_(ab|cde)_suffix hit", [&]{ auto r = wildcard_match<wc_prefix_alt>("prefix_ab_suffix"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: prefix_(ab|cde)_suffix miss",[&]{ auto r = wildcard_match<wc_prefix_alt>("prefix_ef_suffix"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: cls[abc] 'a' hit",           [&]{ auto r = wildcard_match<wc_cls>("a"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: cls[abc] miss",              [&]{ auto r = wildcard_match<wc_cls>("d"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: cls[a-m] 'a' hit",           [&]{ auto r = wildcard_match<wc_cls_wide>("a"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("wc: cls[a-m] miss",              [&]{ auto r = wildcard_match<wc_cls_wide>("z"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  }

  // ========== wildcards (runtime) ==========
  {
    ankerl::nanobench::Bench bench;
    bench.title("wildcards (runtime)").unit("op").warmup(100).minEpochIterations(iterations);
    bench.run("rt: lit('endpoint') hit",        [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"endpoint"}, std::string{"endpoint"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: lit('endpoint') miss",       [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"other"}, std::string{"endpoint"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: small(4) 'GET' hit",         [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"GET"}, std::string{"GET|POST|PUT|DELETE"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: small(4) miss",              [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"PATCH"}, std::string{"GET|POST|PUT|DELETE"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: med(8) 'OPTIONS' hit",       [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"OPTIONS"}, std::string{"GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: med(8) miss",                [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"CONNECT"}, std::string{"GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS|TRACE"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: path(4) '/api/v1/users'",    [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"/api/v1/users"}, std::string{"/api/v1/users|/api/v1/posts|/api/v1/comments|/api/v1/tags"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: path(4) miss",               [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"/api/v1/other"}, std::string{"/api/v1/users|/api/v1/posts|/api/v1/comments|/api/v1/tags"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: cls[abc] 'a' hit",           [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"a"}, std::string{"[abc]"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: cls[abc] miss",              [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"d"}, std::string{"[abc]"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: cls[a-m] 'a' hit",           [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"a"}, std::string{"[a-m]"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
    bench.run("rt: cls[a-m] miss",              [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"z"}, std::string{"[a-m]"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  }

  ankerl::nanobench::doNotOptimizeAway(g_sink);
  return 0;
}
