#include "frozenchars/literals.hpp"
#include "frozenchars/perfect_map.hpp"

using namespace frozenchars::literals;

auto duplicate_map = frozenchars::perfect_map<int, "dup"_fs, "dup"_fs>{};
