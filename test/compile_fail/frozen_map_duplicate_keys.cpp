#include "frozenchars/literals.hpp"
#include "frozenchars/map.hpp"

using namespace frozenchars::literals;

auto duplicate_map = frozenchars::frozen_map<int, "dup"_fs, "dup"_fs>{};
