#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"

using namespace frozenchars::literals;

// [a-z] is 26 chars -> MaxStrings=4 triggers overflow
auto const r = frozenchars::frozen_regex<"[a-z]"_fs, 4>{};
