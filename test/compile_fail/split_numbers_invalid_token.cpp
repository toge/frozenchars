#include "frozenchars/literals.hpp"
#include "frozenchars/split.hpp"
using namespace frozenchars::literals;

// "12 x 34" の 2 番目のトークンが数値ではない
auto constexpr result = frozenchars::split_numbers("12 x 34"_fs);
int main() {}
