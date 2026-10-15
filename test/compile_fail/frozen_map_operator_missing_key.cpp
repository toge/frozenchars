#include "frozenchars/literals.hpp"
#include "frozenchars/map.hpp"

using namespace frozenchars::literals;

// frozen_map::operator[] は存在しないキーに対して
// 診断メッセージ付きで失敗しなければならない（番兵インデックス参照は UB）
auto const missing = []() consteval {
  auto map = frozenchars::frozen_map<int, "a"_fs, "b"_fs>{};
  return map["missing"];
}();
