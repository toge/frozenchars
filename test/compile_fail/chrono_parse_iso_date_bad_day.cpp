#include "frozenchars.hpp"
#include "frozenchars/chrono.hpp"
using namespace frozenchars::literals;

auto constexpr result = frozenchars::parse_iso_date<"2026-07-00"_fs>();
int main() {}
