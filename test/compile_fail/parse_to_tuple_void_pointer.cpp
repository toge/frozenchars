#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

// 未対応の型名は "Unknown type name" でコンパイルエラーになる
using V = typename decltype(parse_to_tuple<"long double"_fs>())::type;
