#include "frozenchars/frozen_map.hpp"
#include "frozenchars/split.hpp"
#include "frozenchars/literals.hpp"
#include <iostream>
#include <tuple>

using namespace frozenchars;
using namespace frozenchars::literals;

/** @brief split_v のデバッグテスト。手動デバッグ用スタンドアロンファイル。
    @details コンパイル時文字列分割とキー配列生成を検証する。*/

int main() {
  // 1. 文字列を分割してキーの配列を作成
  constexpr auto keys = split_v<"apple,banana,cherry"_fs, detail::is_char<','>>;

  std::cout << "Keys size: " << keys.size() << "\n";
  for (auto const& k : keys) {
    std::cout << "Key: [" << k.sv() << "] len=" << k.length << "\n";
  }

  if (keys.size() == 3 && keys[0].sv() == "apple") {
    std::cout << "SUCCESS: Split logic works in main.\n";
    return 0;
  }
  return 1;
}
