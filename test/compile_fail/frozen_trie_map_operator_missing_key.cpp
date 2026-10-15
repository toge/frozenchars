#include "frozenchars/literals.hpp"
#include "frozenchars/trie_map.hpp"

using namespace frozenchars::literals;

// frozen_trie_map::operator[] は存在しないキーに対して
// 診断メッセージ付きで失敗しなければならない（番兵インデックス参照は UB）
auto const missing = []() consteval {
  frozenchars::frozen_trie_map<int, "a"_fs, "b"_fs> map{std::array<int, 2>{1, 2}};
  return map["missing"];
}();
