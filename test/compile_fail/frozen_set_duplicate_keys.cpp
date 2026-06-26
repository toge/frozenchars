#include "frozenchars/literals.hpp"
#include "frozenchars/set.hpp"

using namespace frozenchars::literals;

auto duplicate_set = frozenchars::frozen_set<"dup"_fs, "dup"_fs>{};
