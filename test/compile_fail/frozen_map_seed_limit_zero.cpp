#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_map.hpp"

using namespace frozenchars::literals;

constexpr auto forced_failure =
  frozenchars::detail::find_seed<0, "timeout"_fs, "retry"_fs>();
