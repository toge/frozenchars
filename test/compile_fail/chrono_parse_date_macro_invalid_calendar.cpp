#include "frozenchars/chrono.hpp"
#include "frozenchars/literals.hpp"
using namespace frozenchars::literals;

auto constexpr result = frozenchars::parse_date_macro<"Feb 30 2024"_fs>();
int main() {}
