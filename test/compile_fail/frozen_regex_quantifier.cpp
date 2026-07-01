#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"

using namespace frozenchars::literals;

auto const r = frozenchars::frozen_regex<"a+"_fs>{};
