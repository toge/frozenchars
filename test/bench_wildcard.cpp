#include "frozenchars/wildcard.hpp"

#include "wildcards.hpp"

#include <nanobench.h>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

using namespace frozenchars;

/** @brief frozenchars wildcard_match と wildcards ライブラリのパフォーマンス比較ベンチマーク。
    @details `*`, `?`, 文字集合, 代替パターンの各ケースでコンパイル時 vs 実行時の実行時間を計測する。
    @note 最初に全パターンの結果一致を検証してからベンチマークを実行する。*/

namespace {

/** @brief 最適化防止用シンク変数。
    @details volatile 宣言によりコンパイラのループ最適化を抑制する。*/
std::size_t g_sink = 0;

/** @brief frozenchars の wildcard_match 結果を検証する。
    @tparam PAT ワイルドカードパターン (NTTP)
    @param text マッチ対象文字列
    @param expected 期待される結果
    @return 検証成功時に true */
template <FrozenString PAT>
[[nodiscard]] bool verify_frozen(std::string_view text, bool expected) {
  auto result = wildcard_match<PAT>(text);
  if (result != expected) {
    std::cerr << "VERIFY FAIL: frozenchars wildcard_match<\""
              << PAT.data() << "\">(\"" << text << "\") = "
              << result << ", expected " << expected << "\n";
    return false;
  }
  return true;
}

/** @brief frozenchars の wildcard_find 結果を検証する。
    @tparam PAT ワイルドカードパターン (NTTP)
    @param text 検索対象文字列
    @param expected 期待されるマッチ部分
    @return 検証成功時に true */
template <FrozenString PAT>
[[nodiscard]] bool verify_find(std::string_view text, std::optional<std::string_view> expected) {
  auto const result = wildcard_find<PAT>(text);
  if (result != expected) {
    std::cerr << "VERIFY FAIL: frozenchars wildcard_find<\""
              << PAT.data() << "\">(\"" << text << "\") = ";
    if (result) {
      std::cerr << '"' << *result << '"';
    } else {
      std::cerr << "nullopt";
    }
    std::cerr << ", expected ";
    if (expected) {
      std::cerr << '"' << *expected << '"';
    } else {
      std::cerr << "nullopt";
    }
    std::cerr << "\n";
    return false;
  }
  return true;
}

/** @brief frozenchars の wildcard_find_all 結果を検証する。
    @tparam PAT ワイルドカードパターン (NTTP)
    @param text 検索対象文字列
    @param expected 期待されるマッチ列
    @return 検証成功時に true */
template <FrozenString PAT>
[[nodiscard]] bool verify_find_all(std::string_view text, std::initializer_list<std::string_view> expected) {
  auto index = std::size_t{0};
  for (auto const sv : wildcard_find_all<PAT>(text)) {
    if (index >= expected.size()) {
      std::cerr << "VERIFY FAIL: frozenchars wildcard_find_all<\""
                << PAT.data() << "\">(\"" << text << "\") produced extra match \""
                << sv << "\"\n";
      return false;
    }

    auto const expected_it = expected.begin() + static_cast<std::ptrdiff_t>(index);
    if (sv != *expected_it) {
      std::cerr << "VERIFY FAIL: frozenchars wildcard_find_all<\""
                << PAT.data() << "\">(\"" << text << "\") match[" << index << "] = \""
                << sv << "\", expected \"" << *expected_it << "\"\n";
      return false;
    }
    ++index;
  }

  if (index != expected.size()) {
    auto const expected_it = expected.begin() + static_cast<std::ptrdiff_t>(index);
    std::cerr << "VERIFY FAIL: frozenchars wildcard_find_all<\""
              << PAT.data() << "\">(\"" << text << "\") missing expected match \""
              << *expected_it << "\"\n";
    return false;
  }

  return true;
}

/** @brief wildcards ライブラリのマッチ結果を検証する。
    @param text マッチ対象文字列
    @param pattern ワイルドカードパターン文字列
    @param expected 期待される結果
    @return 検証成功時に true */
[[nodiscard]] bool verify_wildcards(std::string_view text, std::string_view pattern, bool expected) {
  auto result = static_cast<bool>(wildcards::match(std::string{text}, std::string{pattern}));
  if (result != expected) {
    std::cerr << "VERIFY FAIL: wildcards::match(\"" << text << "\", \""
              << pattern << "\") = " << result << ", expected " << expected << "\n";
    return false;
  }
  return true;
}

} // namespace

int main(int argc, char** argv) {
  auto iterations = std::uint64_t{50'000};
  if (argc > 1) {
    auto const parsed = std::strtoull(argv[1], nullptr, 10);
    if (parsed > 0) {
      iterations = static_cast<std::uint64_t>(parsed);
    }
  }

  // テストパターンとテキストの準備
  std::string long_text = "a";
  for (auto i = 0; i < 100; ++i) long_text += "x";
  long_text += "b";

  std::string long_no_match = "a";
  for (auto i = 0; i < 100; ++i) long_no_match += "x";
  long_no_match += "c";

  std::string long_text_500 = "a";
  for (auto i = 0; i < 500; ++i) long_text_500 += "x";
  long_text_500 += "b";

  std::string file_pattern = "*.[hc](pp|)";
  std::string file_cpp = "main.cpp";
  std::string file_c = "main.c";
  std::string file_hpp = "main.hpp";
  std::string file_h = "main.h";
  std::string file_cc = "main.cc";
  std::string file_xyz = "main.xyz";

  // ベンチマーク前に正しさを検証
  bool all_ok = true;

  constexpr auto pat_a_star_b = FrozenString{"a*b"};
  constexpr auto pat_a_star = FrozenString{"a*"};
  constexpr auto pat_a_q_b = FrozenString{"a?b"};
  constexpr auto pat_set_abc_de = FrozenString{"[abc]de"};
  constexpr auto pat_nset_abc_de = FrozenString{"[!abc]de"};
  constexpr auto pat_source_file = FrozenString{"*.[hc](pp|)"};
  constexpr auto pat_hello_world = FrozenString{"(hello|world)"};
  constexpr auto pat_prefix_alt = FrozenString{"prefix_(ab|cde)_suffix"};
  constexpr auto pat_star = FrozenString{"*"};
  constexpr auto pat_empty = FrozenString{""};

  all_ok &= verify_frozen<pat_a_star_b>(long_text, true);
  all_ok &= verify_frozen<pat_a_star_b>(long_no_match, false);
  all_ok &= verify_frozen<pat_a_q_b>("axb", true);
  all_ok &= verify_frozen<pat_a_q_b>("ab", false);
  all_ok &= verify_frozen<pat_set_abc_de>("cde", true);
  all_ok &= verify_frozen<pat_set_abc_de>("xde", false);
  all_ok &= verify_frozen<pat_nset_abc_de>("xde", true);
  all_ok &= verify_frozen<pat_nset_abc_de>("cde", false);
  all_ok &= verify_frozen<pat_source_file>(file_cpp, true);
  all_ok &= verify_frozen<pat_source_file>(file_c, true);
  all_ok &= verify_frozen<pat_source_file>(file_hpp, true);
  all_ok &= verify_frozen<pat_source_file>(file_h, true);
  all_ok &= verify_frozen<pat_source_file>(file_cc, false);
  all_ok &= verify_frozen<pat_source_file>(file_xyz, false);
  all_ok &= verify_frozen<pat_hello_world>("hello", true);
  all_ok &= verify_frozen<pat_hello_world>("world", true);
  all_ok &= verify_frozen<pat_hello_world>("earth", false);
  all_ok &= verify_frozen<pat_prefix_alt>("prefix_ab_suffix", true);
  all_ok &= verify_frozen<pat_prefix_alt>("prefix_cde_suffix", true);
  all_ok &= verify_frozen<pat_prefix_alt>("prefix_ef_suffix", false);
  all_ok &= verify_frozen<pat_star>("anything", true);
  all_ok &= verify_frozen<pat_star>("", true);
  all_ok &= verify_frozen<pat_empty>("", true);
  all_ok &= verify_frozen<pat_empty>("a", false);
  all_ok &= verify_find<pat_a_star_b>(long_text, std::optional<std::string_view>{std::string_view{long_text}});
  all_ok &= verify_find_all<pat_a_star>("aaa", {"a", "a", "a"});

  // wildcards ライブラリの検証
  all_ok &= verify_wildcards(long_text, "a*b", true);
  all_ok &= verify_wildcards(long_no_match, "a*b", false);
  all_ok &= verify_wildcards("axb", "a?b", true);
  all_ok &= verify_wildcards("ab", "a?b", false);
  all_ok &= verify_wildcards("cde", "[abc]de", true);
  all_ok &= verify_wildcards("xde", "[abc]de", false);
  all_ok &= verify_wildcards("xde", "[!abc]de", true);
  all_ok &= verify_wildcards("cde", "[!abc]de", false);
  all_ok &= verify_wildcards(file_cpp, file_pattern, true);
  all_ok &= verify_wildcards(file_c, file_pattern, true);
  all_ok &= verify_wildcards(file_hpp, file_pattern, true);
  all_ok &= verify_wildcards(file_h, file_pattern, true);
  all_ok &= verify_wildcards(file_cc, file_pattern, false);
  all_ok &= verify_wildcards(file_xyz, file_pattern, false);
  all_ok &= verify_wildcards("hello", "(hello|world)", true);
  all_ok &= verify_wildcards("world", "(hello|world)", true);
  all_ok &= verify_wildcards("earth", "(hello|world)", false);
  all_ok &= verify_wildcards("prefix_ab_suffix", "prefix_(ab|cde)_suffix", true);
  all_ok &= verify_wildcards("prefix_cde_suffix", "prefix_(ab|cde)_suffix", true);
  all_ok &= verify_wildcards("prefix_ef_suffix", "prefix_(ab|cde)_suffix", false);
  all_ok &= verify_wildcards("anything", "*", true);
  all_ok &= verify_wildcards("", "*", true);
  all_ok &= verify_wildcards("", "", true);
  all_ok &= verify_wildcards("a", "", false);

  if (!all_ok) {
    std::cerr << "Verification FAILED. Aborting benchmark.\n";
    return 1;
  }

  std::cout << "All verifications passed.\n";

  // ベンチマーク実行
  ankerl::nanobench::Bench bench;
  bench.title("wildcard benchmark").unit("op").warmup(100).minEpochIterations(iterations);

  // frozenchars のケース
  bench.run("fc: a*b (100c, match)", [&]{ auto r = wildcard_match<pat_a_star_b>(long_text); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: a*b (100c, nomatch)", [&]{ auto r = wildcard_match<pat_a_star_b>(long_no_match); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: a*b (500c, match)", [&]{ auto r = wildcard_match<pat_a_star_b>(long_text_500); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: a?b (match)", [&]{ auto r = wildcard_match<pat_a_q_b>("axb"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: a?b (nomatch)", [&]{ auto r = wildcard_match<pat_a_q_b>("ab"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: [abc]de (match)", [&]{ auto r = wildcard_match<pat_set_abc_de>("cde"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: [abc]de (nomatch)", [&]{ auto r = wildcard_match<pat_set_abc_de>("xde"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: [!abc]de (match)", [&]{ auto r = wildcard_match<pat_nset_abc_de>("xde"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: *.[hc](pp|) .cpp", [&]{ auto r = wildcard_match<pat_source_file>(file_cpp); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: *.[hc](pp|) .cc", [&]{ auto r = wildcard_match<pat_source_file>(file_cc); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: (hello|world) match", [&]{ auto r = wildcard_match<pat_hello_world>("hello"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: (hello|world) nomatch", [&]{ auto r = wildcard_match<pat_hello_world>("earth"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: alt+literal match", [&]{ auto r = wildcard_match<pat_prefix_alt>("prefix_ab_suffix"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: alt+literal nomatch", [&]{ auto r = wildcard_match<pat_prefix_alt>("prefix_ef_suffix"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: * (match)", [&]{ auto r = wildcard_match<pat_star>("anything"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: * (empty)", [&]{ auto r = wildcard_match<pat_star>(""); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: empty (match)", [&]{ auto r = wildcard_match<pat_empty>(""); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc: empty (nomatch)", [&]{ auto r = wildcard_match<pat_empty>("a"); ankerl::nanobench::doNotOptimizeAway(r); g_sink += static_cast<std::size_t>(r); });
  bench.run("fc-find: a*b", [&]{ auto result = wildcard_find<pat_a_star_b>(long_text); ankerl::nanobench::doNotOptimizeAway(result); g_sink += result ? result->size() : 0; });
  bench.run("fc-find-all: a*", [&]{
    std::size_t matched_size = 0;
    for (auto const sv : wildcard_find_all<pat_a_star>("aaa")) {
      matched_size += sv.size();
    }
    ankerl::nanobench::doNotOptimizeAway(matched_size);
    g_sink += matched_size;
  });

  // wildcards ライブラリのケース
  bench.run("wc: a*b (100c, match)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{long_text}, std::string{"a*b"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: a*b (100c, nomatch)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{long_no_match}, std::string{"a*b"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: a*b (500c, match)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{long_text_500}, std::string{"a*b"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: a?b (match)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"axb"}, std::string{"a?b"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: a?b (nomatch)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"ab"}, std::string{"a?b"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: [abc]de (match)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"cde"}, std::string{"[abc]de"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: [abc]de (nomatch)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"xde"}, std::string{"[abc]de"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: [!abc]de (match)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"xde"}, std::string{"[!abc]de"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: *.[hc](pp|) .cpp", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{file_cpp}, std::string{file_pattern})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: *.[hc](pp|) .cc", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{file_cc}, std::string{file_pattern})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: (hello|world) match", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"hello"}, std::string{"(hello|world)"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: (hello|world) nomatch", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"earth"}, std::string{"(hello|world)"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: alt+literal match", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"prefix_ab_suffix"}, std::string{"prefix_(ab|cde)_suffix"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: alt+literal nomatch", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"prefix_ef_suffix"}, std::string{"prefix_(ab|cde)_suffix"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: * (match)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"anything"}, std::string{"*"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: * (empty)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{""}, std::string{"*"})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: empty (match)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{""}, std::string{""})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });
  bench.run("wc: empty (nomatch)", [&]{ auto r = static_cast<std::size_t>(wildcards::match(std::string{"a"}, std::string{""})); ankerl::nanobench::doNotOptimizeAway(r); g_sink += r; });

  ankerl::nanobench::doNotOptimizeAway(g_sink);
  return 0;
}
