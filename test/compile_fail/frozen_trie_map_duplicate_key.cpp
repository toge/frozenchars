#include "frozenchars/literals.hpp"
#include "frozenchars/trie_map.hpp"

#include <utility>

using namespace frozenchars;
using namespace frozenchars::literals;

// 同じキーが2回渡された場合はエラーになること（片方の値が黙って捨てられない）
constexpr auto map = make_frozen_trie_map<int, "alpha"_fs, "beta"_fs>(
  std::pair{"alpha", 11},
  std::pair{"alpha", 22}
);