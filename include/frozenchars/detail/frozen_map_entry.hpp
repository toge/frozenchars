#pragma once

#include <string_view>

namespace frozenchars {

/**
 * @brief 固定長マップ系がエントリを受け取る際に使う 1 要素（キーと値のペア）
 *
 * frozen_map / frozen_multimap / frozen_bimap / frozen_trie_map が共通して使う。
 *
 * @tparam T 値の型
 */
template <typename T>
struct frozen_map_entry {
  std::string_view key;  // キー文字列（FrozenString のビュー）
  T value;               // 対応する値
};

} // namespace frozenchars