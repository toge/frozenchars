#include <limits>

#include "frozenchars/literals.hpp"
#include "frozenchars/string_ops.hpp"

using namespace frozenchars::literals;

// Width == SIZE_MAX は Width + 1 が 0 に折り返し、バッファ外書き込みになるため診断される
auto constexpr result = frozenchars::pad_left<std::numeric_limits<std::size_t>::max(), 'x'>("ab"_fs);
