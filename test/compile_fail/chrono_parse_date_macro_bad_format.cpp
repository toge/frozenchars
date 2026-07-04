#include "frozenchars.hpp"
#include "frozenchars/chrono.hpp"
using namespace frozenchars::literals;

auto constexpr result = frozenchars::parse_date_macro<"Foo 99 2026"_fs>();
int main() {}
