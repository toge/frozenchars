#include "frozenchars/literals.hpp"
#include "frozenchars/trie_index.hpp"

using namespace frozenchars::literals;

// 内部ノードの label 長さは uint8_t で持つため、255 文字を超えるキー（境界値 256 文字を含む）では
// 共通接頭辞が切り詰められ、キーが静かに探索不能になる。静かに壊れる前に
// コンパイル時エラーにする。
using index_type = frozenchars::frozen_trie_index<"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                                  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                                  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                                                  "aaaaaaaaaaaaaaaa"_fs>; // 境界値: 256 文字

static_assert(index_type::NPOS == 1);