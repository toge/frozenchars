#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/perfect_map.hpp"

using namespace frozenchars::literals;

struct BadKey {};

auto bad_map = frozenchars::make_perfect_map<int, "timeout"_fs>(
  std::pair{BadKey{}, 30}
);
