#include "frozenchars.hpp"
using namespace frozenchars::literals;

/**
 * @brief frozen_format の引数不足をコンパイル時に検出する失敗ケース
 */
#if defined(__cpp_lib_format)
auto bad = frozenchars::frozen_format<"{} {}"_fs>(42);
#else
#  error "format unavailable"
#endif

int main() {}
