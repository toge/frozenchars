#include "frozenchars/literals.hpp"
#include "frozenchars/static_perfect_map.hpp"

using namespace frozenchars::literals;

constexpr auto forced_failure =
  frozenchars::detail::find_seed<0, "timeout"_fs, "retry"_fs>();
