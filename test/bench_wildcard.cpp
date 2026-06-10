#include "frozenchars/wildcard.hpp"

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

namespace {

struct bench_result {
  std::string_view name{};
  std::uint64_t iterations{};
  double total_ms{};
  double ns_per_iter{};
};

volatile std::size_t g_sink = 0;

template <FrozenString PAT>
auto run_frozenchars(std::string_view text, std::string_view name,
                     std::uint64_t iterations) -> bench_result {
  // warmup
  for (auto i = std::uint64_t{0}; i < 500; ++i) {
    g_sink += static_cast<std::size_t>(wildcard_match<PAT>(text));
  }

  auto const begin = std::chrono::steady_clock::now();
  for (auto i = std::uint64_t{0}; i < iterations; ++i) {
    g_sink += static_cast<std::size_t>(wildcard_match<PAT>(text));
  }
  auto const end = std::chrono::steady_clock::now();

  auto const elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
  auto const elapsed_ms = static_cast<double>(elapsed_ns) / 1'000'000.0;
  auto const ns_per_iter = static_cast<double>(elapsed_ns) / static_cast<double>(iterations);

  return bench_result{
    .name = name,
    .iterations = iterations,
    .total_ms = elapsed_ms,
    .ns_per_iter = ns_per_iter,
  };
}

auto run_wildcards(std::string_view text, std::string_view pattern, std::string_view name,
                   std::uint64_t iterations) -> bench_result {
  // warmup
  for (auto i = std::uint64_t{0}; i < 500; ++i) {
    g_sink += static_cast<std::size_t>(wildcards::match(
      std::string{text}, std::string{pattern}
    ));
  }

  auto const begin = std::chrono::steady_clock::now();
  for (auto i = std::uint64_t{0}; i < iterations; ++i) {
    g_sink += static_cast<std::size_t>(wildcards::match(
      std::string{text}, std::string{pattern}
    ));
  }
  auto const end = std::chrono::steady_clock::now();

  auto const elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
  auto const elapsed_ms = static_cast<double>(elapsed_ns) / 1'000'000.0;
  auto const ns_per_iter = static_cast<double>(elapsed_ns) / static_cast<double>(iterations);

  return bench_result{
    .name = name,
    .iterations = iterations,
    .total_ms = elapsed_ms,
    .ns_per_iter = ns_per_iter,
  };
}

auto print_results(std::vector<bench_result> const& results) -> void {
  std::cout << "\n[wildcard benchmark]\n";
  std::cout << "steady_clock.is_steady = " << std::chrono::steady_clock::is_steady << "\n\n";

  std::cout << std::left
            << std::setw(36) << "case"
            << std::right
            << std::setw(12) << "iters"
            << std::setw(14) << "total[ms]"
            << std::setw(14) << "ns/iter"
            << "\n";

  std::cout << std::string(36 + 12 + 14 + 14, '-') << "\n";

  for (auto const& r : results) {
    std::cout << std::left
              << std::setw(36) << r.name
              << std::right
              << std::setw(12) << r.iterations
              << std::setw(14) << std::fixed << std::setprecision(3) << r.total_ms
              << std::setw(14) << std::fixed << std::setprecision(1) << r.ns_per_iter
              << "\n";
  }

  std::cout << "\n[sink] " << g_sink << '\n';
}

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

  // Test patterns and texts
  struct bench_case {
    std::string_view name;
    std::string_view pattern_str;   // for wildcards
    std::string_view text;
    bool expected;
  };

  std::string long_text = "a";
  for (auto i = 0; i < 100; ++i) long_text += "x";
  long_text += "b";

  std::string long_no_match = "a";
  for (auto i = 0; i < 100; ++i) long_no_match += "x";
  long_no_match += "c";

  std::string long_text_500 = "a";
  for (auto i = 0; i < 500; ++i) long_text_500 += "x";
  long_text_500 += "b";

  std::string set_text_wildcards = "file.cpp";
  std::string set_text_no = "file.cc";

  std::string file_pattern = "*.[hc](pp|)";
  std::string file_cpp = "main.cpp";
  std::string file_c = "main.c";
  std::string file_hpp = "main.hpp";
  std::string file_h = "main.h";
  std::string file_cc = "main.cc";
  std::string file_xyz = "main.xyz";

  // Verify correctness before benchmarking
  bool all_ok = true;

  constexpr auto pat_a_star_b = FrozenString{"a*b"};
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

  // wildcards verification
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

  // Run benchmarks
  auto results = std::vector<bench_result>{};
  results.reserve(24);

  // frozenchars cases
  results.push_back(run_frozenchars<pat_a_star_b>(long_text, "fc: a*b (100c, match)", iterations));
  results.push_back(run_frozenchars<pat_a_star_b>(long_no_match, "fc: a*b (100c, nomatch)", iterations));
  results.push_back(run_frozenchars<pat_a_star_b>(long_text_500, "fc: a*b (500c, match)", iterations));
  results.push_back(run_frozenchars<pat_a_q_b>("axb", "fc: a?b (match)", iterations));
  results.push_back(run_frozenchars<pat_a_q_b>("ab", "fc: a?b (nomatch)", iterations));
  results.push_back(run_frozenchars<pat_set_abc_de>("cde", "fc: [abc]de (match)", iterations));
  results.push_back(run_frozenchars<pat_set_abc_de>("xde", "fc: [abc]de (nomatch)", iterations));
  results.push_back(run_frozenchars<pat_nset_abc_de>("xde", "fc: [!abc]de (match)", iterations));
  results.push_back(run_frozenchars<pat_source_file>(file_cpp, "fc: *.[hc](pp|) .cpp", iterations));
  results.push_back(run_frozenchars<pat_source_file>(file_cc, "fc: *.[hc](pp|) .cc", iterations));
  results.push_back(run_frozenchars<pat_hello_world>("hello", "fc: (hello|world) match", iterations));
  results.push_back(run_frozenchars<pat_hello_world>("earth", "fc: (hello|world) nomatch", iterations));
  results.push_back(run_frozenchars<pat_prefix_alt>("prefix_ab_suffix", "fc: alt+literal match", iterations));
  results.push_back(run_frozenchars<pat_prefix_alt>("prefix_ef_suffix", "fc: alt+literal nomatch", iterations));
  results.push_back(run_frozenchars<pat_star>("anything", "fc: * (match)", iterations));
  results.push_back(run_frozenchars<pat_star>("", "fc: * (empty)", iterations));
  results.push_back(run_frozenchars<pat_empty>("", "fc: empty (match)", iterations));
  results.push_back(run_frozenchars<pat_empty>("a", "fc: empty (nomatch)", iterations));

  // wildcards cases
  results.push_back(run_wildcards(long_text, "a*b", "wc: a*b (100c, match)", iterations));
  results.push_back(run_wildcards(long_no_match, "a*b", "wc: a*b (100c, nomatch)", iterations));
  results.push_back(run_wildcards(long_text_500, "a*b", "wc: a*b (500c, match)", iterations));
  results.push_back(run_wildcards("axb", "a?b", "wc: a?b (match)", iterations));
  results.push_back(run_wildcards("ab", "a?b", "wc: a?b (nomatch)", iterations));
  results.push_back(run_wildcards("cde", "[abc]de", "wc: [abc]de (match)", iterations));
  results.push_back(run_wildcards("xde", "[abc]de", "wc: [abc]de (nomatch)", iterations));
  results.push_back(run_wildcards("xde", "[!abc]de", "wc: [!abc]de (match)", iterations));
  results.push_back(run_wildcards(file_cpp, file_pattern, "wc: *.[hc](pp|) .cpp", iterations));
  results.push_back(run_wildcards(file_cc, file_pattern, "wc: *.[hc](pp|) .cc", iterations));
  results.push_back(run_wildcards("hello", "(hello|world)", "wc: (hello|world) match", iterations));
  results.push_back(run_wildcards("earth", "(hello|world)", "wc: (hello|world) nomatch", iterations));
  results.push_back(run_wildcards("prefix_ab_suffix", "prefix_(ab|cde)_suffix", "wc: alt+literal match", iterations));
  results.push_back(run_wildcards("prefix_ef_suffix", "prefix_(ab|cde)_suffix", "wc: alt+literal nomatch", iterations));
  results.push_back(run_wildcards("anything", "*", "wc: * (match)", iterations));
  results.push_back(run_wildcards("", "*", "wc: * (empty)", iterations));
  results.push_back(run_wildcards("", "", "wc: empty (match)", iterations));
  results.push_back(run_wildcards("a", "", "wc: empty (nomatch)", iterations));

  print_results(results);
  return 0;
}
