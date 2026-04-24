#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/perfect_map.hpp"

using namespace frozenchars::literals;

struct Target {
  Target() = delete;
  explicit Target(int) = delete;
};

auto bad_map = frozenchars::make_perfect_map<Target, "timeout"_fs>(
  std::pair{"timeout", 30}
);
