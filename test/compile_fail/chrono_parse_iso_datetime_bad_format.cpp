#include "frozenchars.hpp"
#include "frozenchars/chrono.hpp"
using namespace frozenchars::literals;

auto constexpr result = frozenchars::parse_iso_datetime<"2026-07-04 14:30:00"_fs>();
int main() {}
