#include <catch2/catch_test_macros.hpp>
#include "frozenchars/string.hpp"
#include "frozenchars/detail/split_impl.hpp"
#include "frozenchars/detail/char_utils.hpp"
#include <array>

using namespace frozenchars;

template <auto Str, auto IsDelim>
struct split_tester {
  static constexpr auto token_count = detail::split_count_impl<IsDelim>(Str);
  static constexpr auto max_len     = detail::max_token_len_impl<IsDelim>(Str);

  static constexpr auto get_value() {
    std::array<FrozenString<max_len + 1>, token_count> res{};
    std::size_t src = 0;
    std::size_t dst = 0;
    while (src < Str.length && dst < token_count) {
      while (src < Str.length && IsDelim(Str.buffer[src])) ++src;
      if (src >= Str.length) break;
      std::size_t token_len = 0;
      while (src < Str.length && !IsDelim(Str.buffer[src])) {
        res[dst].buffer[token_len++] = Str.buffer[src++];
      }
      res[dst].buffer[token_len] = '\0';
      res[dst].length = token_len;
      ++dst;
    }
    return res;
  }
  static constexpr auto value = get_value();
};

struct comma_delim {
  constexpr bool operator()(char c) const noexcept { return c == ','; }
};

TEST_CASE("split tester", "[split]") {
  static constexpr auto fs = FrozenString<20>("apple,banana,cherry");
  using Tester = split_tester<fs, comma_delim{}>;

  CHECK(Tester::token_count == 3);
  CHECK(Tester::max_len == 6);
  CHECK(Tester::value.size() == 3);
  CHECK(Tester::value[0].sv() == "apple");
}