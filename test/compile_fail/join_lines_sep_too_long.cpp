#include "frozenchars.hpp"
using namespace frozenchars::literals;

// セパレータが33文字（MAX_SEP_LEN=32超）
auto constexpr result = frozenchars::join_lines(
  "a\nb"_fs,
  "123456789012345678901234567890123"  // 33文字
);

int main() {}
