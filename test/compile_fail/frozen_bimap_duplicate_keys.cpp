#include "frozenchars/bimap.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

auto duplicate_bimap = frozenchars::frozen_bimap<int, "dup"_fs, "dup"_fs>{};
