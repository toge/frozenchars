#include "frozenchars/json/crush.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

constexpr auto value = frozenchars::json::uncrush<"\xC0\x80_"_fs>();
