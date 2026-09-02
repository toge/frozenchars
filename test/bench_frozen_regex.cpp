#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"
#include <ctre.hpp>

#include <nanobench.h>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string_view>

using namespace frozenchars;
using namespace frozenchars::literals;

/** @brief frozen_regex と CTRE の実行時パフォーマンス比較ベンチマーク。500K イテレーションで contains/matches を計測する。 */
namespace {

/** @brief 最適化防止用の volatile sink 変数。 */
std::size_t g_sink = 0;

/** @brief frozen_regex::contains の結果が期待通りかを検証する。不一致時は標準エラーに出力する。
    @param RR       frozen_regex 型
    @param text     検証対象の文字列
    @param expected 期待される結果
    @return 期待通りなら true */
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

/** @brief ctre::match の結果が期待通りかを検証する。不一致時は標準エラーに出力する。
    @param Pattern  CTRE パターン変数
    @param text     検証対象の文字列
    @param expected 期待される結果
    @return 期待通りなら true */
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
  ankerl::nanobench::Bench bench;
  bench.title("frozen_regex vs CTRE benchmark").unit("op").warmup(100).minEpochIterations(iterations);

  // frozen_regex contains()
  bench.run("fr: lit('endpoint') hit",         [&]{ auto r = R_literal::contains("endpoint"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: lit('endpoint') miss",        [&]{ auto r = R_literal::contains("other"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: small(4) 'GET' hit",         [&]{ auto r = R_small_alt::contains("GET"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: small(4) 'DELETE' hit",      [&]{ auto r = R_small_alt::contains("DELETE"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: small(4) miss",              [&]{ auto r = R_small_alt::contains("PATCH"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: med(8) 'OPTIONS' hit",       [&]{ auto r = R_med_alt::contains("OPTIONS"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: med(8) miss",                [&]{ auto r = R_med_alt::contains("CONNECT"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: large(20) 'k01' hit",        [&]{ auto r = R_large_alt::contains("k01"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: large(20) 'k20' hit",        [&]{ auto r = R_large_alt::contains("k20"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: large(20) miss",             [&]{ auto r = R_large_alt::contains("k99"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: path(4) '/api/v1/...' hit",  [&]{ auto r = R_path_alt::contains("/api/v1/users"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: path(4) miss",               [&]{ auto r = R_path_alt::contains("/api/v1/other"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: cls[abc](3) 'a' hit",        [&]{ auto r = R_cls::contains("a"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: cls[abc](3) miss",           [&]{ auto r = R_cls::contains("d"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: cls[a-m](13) 'a' hit",       [&]{ auto r = R_cls_wide::contains("a"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: cls[a-m](13) miss",          [&]{ auto r = R_cls_wide::contains("z"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("fr: dot(63) 'a' hit",            [&]{ auto r = R_dot::contains("a"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });

  // frozen_regex enumerate() / keys()
  bench.run("fr: enumerate() small(4)",       [&]{ auto s = R_small_alt::enumerate(); ankerl::nanobench::doNotOptimizeAway(s); g_sink += s.size(); });
  bench.run("fr: enumerate() large(20)",      [&]{ auto s = R_large_alt::enumerate(); ankerl::nanobench::doNotOptimizeAway(s); g_sink += s.size(); });
  bench.run("fr: keys() med(8)",              [&]{ auto s = R_med_alt::keys(); ankerl::nanobench::doNotOptimizeAway(s); g_sink += s.size(); });

  // frozen_regex to_frozen_map + lookup
  constexpr auto m_small = R_small_alt::template to_frozen_map<int, 1, 2, 3, 4>();
  bench.run("fr: to_map+at 'GET' hit",       [&]{ auto v = m_small.at("GET"); ankerl::nanobench::doNotOptimizeAway(v); g_sink += v; });
  bench.run("fr: to_map+find miss",           [&]{ auto it = m_small.find("PATCH"); ankerl::nanobench::doNotOptimizeAway(it != m_small.end()); g_sink += (it != m_small.end()); });

  // ---- CTRE match() ----
  bench.run("ctre: lit('endpoint') hit",      [&]{ auto r = static_cast<bool>(ctre::match<ctre_literal>("endpoint")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: lit('endpoint') miss",     [&]{ auto r = static_cast<bool>(ctre::match<ctre_literal>("other")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: small(4) 'GET' hit",       [&]{ auto r = static_cast<bool>(ctre::match<ctre_small_alt>("GET")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: small(4) 'DELETE' hit",    [&]{ auto r = static_cast<bool>(ctre::match<ctre_small_alt>("DELETE")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: small(4) miss",            [&]{ auto r = static_cast<bool>(ctre::match<ctre_small_alt>("PATCH")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: med(8) 'OPTIONS' hit",     [&]{ auto r = static_cast<bool>(ctre::match<ctre_med_alt>("OPTIONS")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: med(8) miss",              [&]{ auto r = static_cast<bool>(ctre::match<ctre_med_alt>("CONNECT")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: large(20) 'k01' hit",      [&]{ auto r = static_cast<bool>(ctre::match<ctre_large_alt>("k01")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: large(20) 'k20' hit",      [&]{ auto r = static_cast<bool>(ctre::match<ctre_large_alt>("k20")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: large(20) miss",           [&]{ auto r = static_cast<bool>(ctre::match<ctre_large_alt>("k99")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: path(4) '/api/v1/...' hit",[&]{ auto r = static_cast<bool>(ctre::match<ctre_path_alt>("/api/v1/users")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: path(4) miss",             [&]{ auto r = static_cast<bool>(ctre::match<ctre_path_alt>("/api/v1/other")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: cls[abc](3) 'a' hit",      [&]{ auto r = static_cast<bool>(ctre::match<ctre_cls>("a")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: cls[abc](3) miss",         [&]{ auto r = static_cast<bool>(ctre::match<ctre_cls>("d")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: cls[a-m](13) 'a' hit",     [&]{ auto r = static_cast<bool>(ctre::match<ctre_cls_wide>("a")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: cls[a-m](13) miss",        [&]{ auto r = static_cast<bool>(ctre::match<ctre_cls_wide>("z")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("ctre: dot '.' 'a' hit",          [&]{ auto r = static_cast<bool>(ctre::match<ctre_dot>("a")); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });

  ankerl::nanobench::doNotOptimizeAway(g_sink);
  return 0;
}
