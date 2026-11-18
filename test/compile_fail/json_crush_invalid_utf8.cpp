#include "frozenchars/json/crush.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

constexpr auto value = frozenchars::json::crush<"[\"\xF0\x80\x80\x80\"]"_fs>();
