#include "frozenchars/literals.hpp"
#include "frozenchars/static_perfect_map.hpp"

using namespace frozenchars::literals;

auto duplicate_map = frozenchars::StaticPerfectMap<int, "dup"_fs, "dup"_fs>{};
