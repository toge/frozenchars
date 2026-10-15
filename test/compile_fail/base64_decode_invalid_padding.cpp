#include "frozenchars/encoding.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

// '=' 終端の 4 文字未満入力はデコードサイズ計算で size_t アンダーフローに陥らず、
// Base64 入力不正として診断されなければならない
auto const decoded = frozenchars::base64_decode<"="_fs>();
