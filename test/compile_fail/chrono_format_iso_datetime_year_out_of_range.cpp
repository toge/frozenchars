#include "frozenchars/chrono.hpp"
#include "frozenchars/literals.hpp"

// 西暦 10000 年は "YYYY-MM-DDTHH:MM:SSZ" の 4 桁年に収まらないため診断される
auto constexpr result = frozenchars::format_iso_datetime(std::chrono::sys_seconds{std::chrono::sys_days{std::chrono::year{10000} / 1 / 1}});
