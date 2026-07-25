#include <catch2/catch_test_macros.hpp>
#include "frozenchars/string.hpp"
#include "frozenchars/detail/split_impl.hpp"
#include "frozenchars/detail/char_utils.hpp"

using namespace frozenchars;

TEST_CASE("split direct", "[split]") {
  static constexpr auto fs = FrozenString<20>("apple,banana,cherry");
  constexpr auto count = detail::split_count_impl<detail::is_char<','>>(fs);
  CHECK(count == 3);
}