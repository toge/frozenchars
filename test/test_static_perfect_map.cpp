#include "catch2/catch_all.hpp"

#include "frozenchars/literals.hpp"
#include "frozenchars/static_perfect_map.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("StaticPerfectMap basic shape", "[static_perfect_map]") {
  StaticPerfectMap<int, "timeout"_fs, "retry"_fs> map{};

  static_assert(StaticPerfectMap<int, "timeout"_fs, "retry"_fs>::size() == 2);
  REQUIRE(map.contains("timeout"));
  REQUIRE(map.contains("retry"));
  REQUIRE_FALSE(map.contains("other"));
}
