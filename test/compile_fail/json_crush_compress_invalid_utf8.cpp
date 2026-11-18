#include "frozenchars/json/compress.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

constexpr auto value = frozenchars::json::crush_compress<"[\"\xF0\x80\x80\x80\"]"_fs>();
