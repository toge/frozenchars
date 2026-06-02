#include "frozenchars.hpp"

using namespace frozenchars::literals;

/**
 * @brief to_sv 経由で std::format に渡した際の
 *        引数不足をコンパイル時に検出する失敗ケース
 */
#if defined(__cpp_lib_format)
auto bad = std::format(frozenchars::to_sv<"{} {}"_fs>(), 42);
#else
#  error "format unavailable"
#endif

int main() {}
