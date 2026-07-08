// スタンドアロンスモークテスト: コアモジュールのみでコンパイルできることの確認。
// glaze / json / regex / SIMD を引き込まないことを前提とする。
#include "frozenchars/mod/core.hpp"

#include <cassert>

int main() {
  using namespace frozenchars::literals;

  constexpr auto s = "hello"_fs;
  static_assert(s.length == 5);

  constexpr auto frozen = frozenchars::freeze("world");
  static_assert(frozen.length == 5);

  constexpr auto value = frozenchars::parse_number<int>("12345"_fs);
  static_assert(value == 12345);

  return 0;
}
