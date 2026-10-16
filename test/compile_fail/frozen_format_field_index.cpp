#include "frozenchars/frozen_format.hpp"

// std::format の `{1}` のような明示的フィールドインデックスはサポートしていない。
// 黙って幅指定として解釈すると std::format と異なる結果になるため、
// コンパイル時エラーにすること。
constexpr auto formatted = frozenchars::frozen_format<"{1} {1}">(1, 2);