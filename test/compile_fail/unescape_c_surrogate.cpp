#include "frozenchars/encoding.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

constexpr auto invalid_codepoint = frozenchars::unescape_c<"\\uD800"_fs>();
