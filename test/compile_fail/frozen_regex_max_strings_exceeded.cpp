#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"

using namespace frozenchars::literals;

// [a-z] は26文字 → MaxStrings=4 で超過が発生する
auto const r = frozenchars::frozen_regex<"[a-z]"_fs, 4>{};
