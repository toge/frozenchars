#include "frozenchars/literals.hpp"
#include "frozenchars/chrono.hpp"
using namespace frozenchars::literals;

// 数字以外を含む日付
auto constexpr result = frozenchars::parse_iso_date<"2026-0a-01"_fs>();
int main() {}
