#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/perfect_map.hpp"

using namespace frozenchars::literals;

auto bad_map = frozenchars::make_perfect_map<int, "timeout"_fs, "retry"_fs>(
  std::pair{"timeout", 30}
);
