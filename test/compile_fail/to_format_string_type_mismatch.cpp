#include "frozenchars.hpp"
using namespace frozenchars::literals;

/**
 * @brief to_format_string の型不一致をコンパイル時に検出する失敗ケース
 */
#if defined(__cpp_lib_format)
constexpr auto bad = frozenchars::to_format_string<"{:d}"_fs, char const*>();
#else
#  error "format unavailable"
#endif

int main() {}
