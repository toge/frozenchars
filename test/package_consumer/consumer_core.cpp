#include "frozenchars/json/crush.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

namespace {
constexpr auto INPUT =
    R"({"k0":"value-0","k1":"value-1","k2":"value-2","k3":"value-3","k4":"value-4","k5":"value-5","k6":"value-6","k7":"value-0","k8":"value-1","k9":"value-2","k10":"value-3","k11":"value-4"})"_fs;
constexpr auto CRUSHED = frozenchars::json::crush<INPUT>();
static_assert(CRUSHED.length < INPUT.length);
} // namespace
