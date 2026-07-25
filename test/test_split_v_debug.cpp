#include <catch2/catch_test_macros.hpp>
#include "frozenchars/map.hpp"
#include "frozenchars/split.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("split_v", "[split]") {
  constexpr auto keys = split_v<"apple,banana,cherry"_fs, detail::is_char<','>>;
  CHECK(keys.size() == 3);
  CHECK(keys[0].sv() == "apple");
}