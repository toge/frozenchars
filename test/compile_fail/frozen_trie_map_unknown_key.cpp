#include "frozenchars/literals.hpp"
#include "frozenchars/trie_map.hpp"

#include <utility>

using namespace frozenchars;
using namespace frozenchars::literals;

// make_frozen_trie_map はキー名でエントリを対応づけるため、未知のキーが
// 静かに値を取り違えるのではなくコンパイル時エラーになること
constexpr auto map = make_frozen_trie_map<int, "alpha"_fs, "beta"_fs>(
  std::pair{"alpha", 11},
  std::pair{"gamma", 22}
);