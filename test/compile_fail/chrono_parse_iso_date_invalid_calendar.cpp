#include "frozenchars/literals.hpp"
#include "frozenchars/chrono.hpp"
using namespace frozenchars::literals;

// 2023-02-30 は存在しない日付
auto constexpr result = frozenchars::parse_iso_date<"2023-02-30"_fs>();
int main() {}
