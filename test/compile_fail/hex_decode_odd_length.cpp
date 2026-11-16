#include "frozenchars/literals.hpp"
#include "frozenchars/encoding.hpp"
using namespace frozenchars::literals;

// 16 進文字列の長さが奇数
auto constexpr result = frozenchars::hex_decode("41424"_fs);
int main() {}
