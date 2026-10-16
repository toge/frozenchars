#include "frozenchars/encoding.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

// パディングが2文字あってもデータ文字が存在しない入力はデコードサイズ計算に
// 陥らないよう、不正な Base64 として診断されなければならない
auto const decoded = frozenchars::base64_decode<"=="_fs>();