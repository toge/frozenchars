#include <catch2/catch_test_macros.hpp>
#include <string_view>
#include "frozenchars/string.hpp"
#include "frozenchars/detail/split_impl.hpp"

using namespace frozenchars;

struct comma_delim {
  constexpr bool operator()(char c) const noexcept { return c == ','; }
};

TEST_CASE("split logic", "[split]") {
  constexpr auto fs = FrozenString<20>("apple,banana,cherry");
  constexpr auto count = detail::split_count_impl<comma_delim{}>(fs);
  constexpr auto max_l = detail::max_token_len_impl<comma_delim{}>(fs);
  CHECK(count == 3);
  CHECK(max_l == 6);
}