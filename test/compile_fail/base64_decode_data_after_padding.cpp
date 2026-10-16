#include "frozenchars/encoding.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

// パディングの後にデータ文字が続く入力は Base64 の終端パディング規則に反するため
// 診断されなければならない
auto const decoded = frozenchars::base64_decode<"AB=CD"_fs>();