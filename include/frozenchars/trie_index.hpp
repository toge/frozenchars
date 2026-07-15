#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>

#if defined(__SSE2__)
#include <emmintrin.h>
#elif defined(__ARM_NEON)
#include <arm_neon.h>
#endif

#include "string.hpp"

namespace frozenchars {

namespace detail {

/**
 * @brief 圧縮trie（ラディックスツリー）の1ノード
 * @details フラット配列に格納されるコンパクトなノード表現。
 *          最大256ノード、65535バイトラベル、256子、127値まで対応。
 */
struct trie_flat_node {
  std::uint16_t label_offset;  ///< k_labels へのオフセット
  std::uint8_t  label_length;  ///< このノードのラベル長（ルートは0）
  std::uint8_t  first_child;   ///< k_children へのオフセット
  std::uint8_t  child_count;   ///< 子ノード数（0 = リーフ）
  std::int8_t   value_index;   ///< -1 = 内部ノード, 0..126 = 終端
};

/**
 * @brief コンパイル時trie構築結果のストレージ
 * @tparam NodeCount 最大ノード数
 * @tparam LabelSize ラベルバッファのサイズ
 * @tparam ChildCount 子インデックス配列のサイズ
 */
template <std::size_t NodeCount, std::size_t LabelSize, std::size_t ChildCount>
struct trie_storage {
  static constexpr std::size_t k_node_count = NodeCount;
  static constexpr std::size_t k_label_size = LabelSize;
  static constexpr std::size_t k_child_count = ChildCount;
  std::array<trie_flat_node, NodeCount> nodes{};
  std::array<char, LabelSize> labels{};
  std::array<std::uint8_t, ChildCount> children{};
};

/**
 * @brief キー群に重複があるか判定する
 * @tparam Keys FrozenString キー群
 * @return 重複があれば true
 */
template <FrozenString... Keys>
[[nodiscard]] consteval auto has_dup_trie_keys() -> bool {
  if constexpr (sizeof...(Keys) <= 1) {
    return false;
  } else {
    constexpr std::array views{ std::string_view{Keys.buffer.data(), Keys.length}... };
    auto sorted = views;
    std::ranges::sort(sorted);
    for (auto i = 1uz; i < sorted.size(); ++i) {
      if (sorted[i - 1] == sorted[i]) return true;
    }
    return false;
  }
}

/// @brief trie構築用のキーと値のペア
struct kv_pair {
  std::string_view key;
  int value_index;
};

/**
 * @brief キー群の最長共通接頭辞を計算する
 * @param keys キーと値のペアのリスト
 * @return 共通接頭辞の長さ
 */
[[nodiscard]] consteval auto longest_common_prefix(std::span<const kv_pair> keys) -> std::size_t {
  if (keys.empty()) return 0;
  std::size_t lcp = keys[0].key.size();
  for (auto i = 1uz; i < keys.size(); ++i) {
    auto const& k = keys[i].key;
    std::size_t j = 0;
    while (j < lcp && j < k.size() && keys[0].key[j] == k[j]) {
      ++j;
    }
    lcp = j;
  }
  return lcp;
}

} // namespace detail

/**
 * @brief コンパイル時圧縮trie（ラディックスツリー）索引
 * @tparam Keys 索引化するキー群（FrozenString）
 *
 * FrozenString... からフラット配列ベースの圧縮trieを consteval で構築し、
 * 実行時に O(length) のキー検索を提供する。
 * 内部ノードは prefix を 1 ノードにまとめることでメモリ使用量を削減する。
 */
template <FrozenString... Keys>
struct frozen_trie_index {
  static_assert(sizeof...(Keys) > 0, "frozen_trie_index requires at least one key");
  static_assert(!detail::has_dup_trie_keys<Keys...>(),
                "frozen_trie_index keys must be unique");
  static_assert(sizeof...(Keys) <= 127,
                "frozen_trie_index supports at most 127 keys (int8_t value_index)");

  /// @brief キー数
  static constexpr auto k_key_count = sizeof...(Keys);

  /// @brief 宣言順のキービュー
  static constexpr std::array<std::string_view, k_key_count> k_key_views_{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };

  /// @brief 見つからなかった場合の戻り値
  static constexpr auto NPOS = k_key_count;

  /**
   * @brief キー群を宣言順に取得する
   * @return キーの span
   */
  [[nodiscard]] static constexpr auto keys() noexcept -> std::span<const std::string_view, k_key_count> {
    return {k_key_views_};
  }

private:
  // --- ストレージサイズの見積もり ---
  /// 最大ノード数: キー数 * 2 で十分（各キーに最低1ノード + 分岐）
  static constexpr auto k_max_nodes = k_key_count * 2;

  /// ラベルバッファの最大サイズ: 全キーの文字数の合計
  static constexpr auto k_max_labels = [] {
    std::size_t sum = 0;
    for (auto const& v : k_key_views_) { sum += v.size(); }
    return sum;
  }();

  /// 子インデックス配列の最大サイズ（非ルートノード数が上限）
  static constexpr auto k_max_children = k_key_count * 2;

  using storage_type = detail::trie_storage<k_max_nodes, k_max_labels, k_max_children>;

  /**
   * @brief trieを再帰的に構築する（consteval）
   * @param keys 構築対象のキー群（共通接頭辞は除去済み）
   * @param storage 構築先ストレージ（参照書き換え）
   * @param next_node 次に割り当てるノードインデックス
   * @param next_label_offset 次に割り当てるラベルオフセット
   * @param next_child_offset 次に割り当てる子オフセット
   * @return 作成したノードのインデックス
   */
  static consteval auto build_trie_impl(
    std::span<const detail::kv_pair> keys,
    storage_type& storage,
    int& next_node,
    int& next_label_offset,
    int& next_child_offset
  ) -> int {
    // 1. 最長共通接頭辞を計算
    auto const lcp = detail::longest_common_prefix(keys);

    // 2. ノードを作成
    auto const node_idx = next_node++;
    auto& node = storage.nodes[static_cast<std::size_t>(node_idx)];
    node.label_offset = static_cast<std::uint16_t>(next_label_offset);
    node.label_length = static_cast<std::uint8_t>(lcp);
    for (std::size_t i = 0; i < lcp; ++i) {
      storage.labels[static_cast<std::size_t>(next_label_offset + static_cast<int>(i))] =
        keys[0].key[i];
    }
    next_label_offset += static_cast<int>(lcp);

    // 3. 最初のキーが LCP で完全に消費されるか判定
    auto const first_consumed = (lcp == keys[0].key.size());

    // 4. LCP 以降の残りキーを収集し、空になったキーはこのノードの終端とする
    auto const start = first_consumed ? 1 : 0;
    std::array<detail::kv_pair, k_key_count> remaining{};
    int remaining_count = 0;
    int consumed_value = -1;
    for (auto i = static_cast<std::size_t>(start); i < keys.size(); ++i) {
      auto sub = keys[i].key.substr(lcp);
      if (sub.empty()) {
        consumed_value = keys[i].value_index;
      } else {
        remaining[static_cast<std::size_t>(remaining_count)] = {sub, keys[i].value_index};
        ++remaining_count;
      }
    }

    // 値インデックスを設定
    if (first_consumed) {
      node.value_index = static_cast<std::int8_t>(keys[0].value_index);
    } else if (consumed_value >= 0) {
      node.value_index = static_cast<std::int8_t>(consumed_value);
    } else {
      node.value_index = static_cast<std::int8_t>(-1);
    }

    // 5. 残りがない → リーフノード
    if (remaining_count == 0) {
      return node_idx;
    }

    // 6. 最初の文字でグループ化
    //    ユニークな最初の文字を収集
    std::array<char, k_key_count> unique_first_chars{};
    int unique_char_count = 0;
    for (int ri = 0; ri < remaining_count; ++ri) {
      auto const c = remaining[static_cast<std::size_t>(ri)].key[0];
      bool found = false;
      for (int u = 0; u < unique_char_count; ++u) {
        if (unique_first_chars[static_cast<std::size_t>(u)] == c) {
          found = true;
          break;
        }
      }
      if (!found) {
        unique_first_chars[static_cast<std::size_t>(unique_char_count)] = c;
        ++unique_char_count;
      }
    }

    // 子ノード用の領域を children 配列に割り当てる
    node.first_child = static_cast<std::uint8_t>(next_child_offset);
    node.child_count = static_cast<std::uint8_t>(unique_char_count);
    next_child_offset += unique_char_count;

    // 各グループについて再帰的に子ノードを構築
    for (int g = 0; g < unique_char_count; ++g) {
      auto const c = unique_first_chars[static_cast<std::size_t>(g)];
      std::array<detail::kv_pair, k_key_count> group_keys{};
      int group_count = 0;
      for (int ri = 0; ri < remaining_count; ++ri) {
        auto const& rk = remaining[static_cast<std::size_t>(ri)];
        if (rk.key[0] == c) {
          group_keys[static_cast<std::size_t>(group_count)] = {rk.key, rk.value_index};
          ++group_count;
        }
      }

      auto const child_node_index = build_trie_impl(
        std::span<const detail::kv_pair>{group_keys.data(),
                                         static_cast<std::size_t>(group_count)},
        storage, next_node, next_label_offset, next_child_offset
      );
      storage.children[static_cast<std::size_t>(
        static_cast<std::uint8_t>(node.first_child + static_cast<std::uint8_t>(g))
      )] = static_cast<std::uint8_t>(child_node_index);
    }

    return node_idx;
  }

  /// @brief トリストレージを構築するエントリポイント
  static consteval auto build_trie() -> storage_type {
    storage_type storage{};
    int next_node = 0;
    int next_label_offset = 0;
    int next_child_offset = 0;

    std::array<detail::kv_pair, k_key_count> key_pairs{};
    for (auto i = 0uz; i < k_key_count; ++i) {
      key_pairs[i] = {k_key_views_[i], static_cast<int>(i)};
    }

    build_trie_impl(
      std::span<const detail::kv_pair>{key_pairs.data(), k_key_count},
      storage, next_node, next_label_offset, next_child_offset
    );

    return storage;
  }

public:
  /// @brief コンパイル時に構築されたトリストレージ
  static constexpr auto k_storage = build_trie();
  static constexpr auto const& k_nodes = k_storage.nodes;
  static constexpr auto const& k_labels = k_storage.labels;
  static constexpr auto const& k_children = k_storage.children;

  /// @brief 子ノード数 > 8 のノードがある場合に LUT を使用する
  static constexpr bool k_use_child_lut = [] {
    for (auto const& node : k_nodes) {
      if (node.child_count > 8) return true;
    }
    return false;
  }();

  /// @brief 子ノード LUT: (node_idx * 256 + unsigned char) -> child_idx (0xFF = none)
  static constexpr auto k_child_lut = [] {
    if constexpr (k_use_child_lut) {
      std::array<std::uint8_t, k_max_nodes * 256> lut{};
      lut.fill(0xFF);
      for (auto ni = 0uz; ni < k_max_nodes; ++ni) {
        auto const& node = k_nodes[ni];
        if (node.child_count == 0) continue;
        for (auto ci = 0; ci < static_cast<int>(node.child_count); ++ci) {
          auto const child_idx = k_children[static_cast<std::size_t>(
            static_cast<std::uint8_t>(node.first_child + static_cast<std::uint8_t>(ci)))];
          auto const first_char = k_labels[static_cast<std::size_t>(
            k_nodes[static_cast<std::size_t>(child_idx)].label_offset)];
          lut[ni * 256 + static_cast<unsigned char>(first_char)] =
            static_cast<std::uint8_t>(child_idx);
        }
      }
      return lut;
    } else {
      return std::array<std::uint8_t, 1>{0};
    }
  }();

  /// @brief 高速ラベル比較（SIMD / バイト単位フォールバック）
  [[nodiscard]] static constexpr auto
  compare_label(std::string_view key, std::size_t pos,
                std::uint16_t label_offset, std::uint8_t label_length) noexcept -> bool {
#if defined(__SSE2__)
    if (!std::is_constant_evaluated() && label_length >= 16) {
      auto const* kp = key.data() + pos;
      auto const* lp = k_labels.data() + label_offset;
      auto i = 0;
      for (; i + 16 <= static_cast<int>(label_length); i += 16) {
        auto vk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(kp + i));
        auto vl = _mm_loadu_si128(reinterpret_cast<const __m128i*>(lp + i));
        auto cmp = _mm_cmpeq_epi8(vk, vl);
        if (_mm_movemask_epi8(cmp) != 0xFFFF) return false;
      }
      for (; i < static_cast<int>(label_length); ++i) {
        if (kp[i] != lp[i]) return false;
      }
      return true;
    }
#elif defined(__ARM_NEON)
    if (!std::is_constant_evaluated() && label_length >= 16) {
      auto const* kp = key.data() + pos;
      auto const* lp = k_labels.data() + label_offset;
      auto i = 0;
      for (; i + 16 <= static_cast<int>(label_length); i += 16) {
        auto vk = vld1q_u8(reinterpret_cast<const uint8_t*>(kp + i));
        auto vl = vld1q_u8(reinterpret_cast<const uint8_t*>(lp + i));
        auto cmp = vceqq_u8(vk, vl);
        if (vminvq_u8(cmp) != 0xFF) return false;
      }
      for (; i < static_cast<int>(label_length); ++i) {
        if (kp[i] != lp[i]) return false;
      }
      return true;
    }
#endif
    for (auto i = 0; i < static_cast<int>(label_length); ++i) {
      if (key[pos + static_cast<std::size_t>(i)] !=
          k_labels[static_cast<std::size_t>(label_offset) + static_cast<std::size_t>(i)]) {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief キーを検索する
   * @param key 検索キー
   * @return 見つかったキーの値インデックス、見つからなければ NPOS
   */
  [[nodiscard]] static constexpr auto find(std::string_view key) noexcept -> std::size_t {
    auto pos = 0uz;
    auto node_idx = 0;

    while (true) {
      auto const& node = k_nodes[static_cast<std::size_t>(node_idx)];

      // ラベルをキーの該当部分と比較（SIMD 最適化パス）
      auto const remaining = key.size() - pos;
      if (remaining < static_cast<std::size_t>(node.label_length)) {
        return NPOS;
      }
      if (!compare_label(key, pos, node.label_offset, node.label_length)) {
        return NPOS;
      }
      pos += static_cast<std::size_t>(node.label_length);

      if (pos == key.size()) {
        return node.value_index >= 0
          ? static_cast<std::size_t>(node.value_index)
          : NPOS;
      }

      if (node.child_count == 0) {
        return NPOS;
      }

      // 子ノード探索: LUT（子数 > 8）or 線形走査
      auto const next_char = key[pos];
      if constexpr (k_use_child_lut) {
        auto const child_idx = k_child_lut[
          static_cast<std::size_t>(node_idx) * 256 +
          static_cast<unsigned char>(next_char)];
        if (child_idx != 0xFF) {
          node_idx = static_cast<int>(child_idx);
          ++pos;
          continue;
        }
        return NPOS;
      } else {
        auto const child_begin = k_children.data() + node.first_child;
        auto const child_end = child_begin + node.child_count;
        bool found = false;
        for (auto it = child_begin; it != child_end; ++it) {
          auto const& child = k_nodes[*it];
          if (k_labels[child.label_offset] == next_char) {
            node_idx = static_cast<int>(*it);
            found = true;
            break;
          }
        }
        if (!found) return NPOS;
      }
    }
  }
};

} // namespace frozenchars
