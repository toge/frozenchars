#include "frozenchars.hpp"

using namespace frozenchars::literals;

/**
 * @brief to_sv 経由で std::format に渡した際の
 *        フォーマット指定子と引数型の不一致をコンパイル時に検出する失敗ケース
 */
#if defined(__cpp_lib_format)
auto bad = std::format(frozenchars::to_sv<"{:d}"_fs>(), "not an int");
#else
#  error "format unavailable"
#endif

int main() {}
