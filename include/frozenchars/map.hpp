#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <concepts>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <ranges>
#include <span>

#include "string.hpp"

#if defined(__SSE4_2__)
#include <nmmintrin.h>
#elif defined(__aarch64__) && defined(__ARM_FEATURE_CRC32)
#include <arm_acle.h>
#endif

namespace frozenchars {

/**
 * @brief frozen_map の 1 要素（キーと値のペア）を表す構造体
 *
 * @tparam T 値の型
 */
template <typename T>
struct frozen_map_entry {
  std::string_view key;  // キー文字列（FrozenString のビュー）
  T value;               // 対応する値
};

namespace detail {

/**
 * @brief std::forward_like の互換実装
 * @details std::forward_like と同名だと ADL 経由で衝突するため、異なる名前を使用する。
 *          __cpp_lib_forward_like が定義されている場合は std::forward_like に委譲し、
 *          未サポートの場合はフォールバック実装を返す。
 * @tparam Self 価値カテゴリと const 性の基準となる型
 * @tparam T 変換対象の値の型
 * @param x 変換対象の値への参照
 * @return Self の価値カテゴリ・const 性を反映した x への参照
 */
template <typename Self, typename T>
[[nodiscard]] constexpr auto&& forward_like_dispatch(T&& x) noexcept {
  return std::forward_like<Self>(std::forward<T>(x));
}

/**
 * @brief forward_like_dispatch の戻り値型を得るエイリアス
 */
template <typename Self, typename T>
using forward_like_t = decltype(forward_like_dispatch<Self>(std::declval<T&>()));

/**
 * @brief ソフトウェア版 CRC32C (Castagnoli) 実装 (Constexpr用)
 */
namespace crc32c {

static constexpr std::uint32_t k_polynomial = 0x82F63B78;  // CRC32C (Castagnoli) の生成多項式（リフレクト済み）

/**
 * @brief CRC32C の単一バイト処理用テーブルをコンパイル時に生成する
 *
 * @return std::array<std::uint32_t, 256> 256 エントリのルックアップテーブル
 */
[[nodiscard]] consteval auto make_table() {
  std::array<std::uint32_t, 256> table{};
  for (std::uint32_t i = 0; i < 256; ++i) {
    std::uint32_t res = i;
    for (int j = 0; j < 8; ++j) res = (res >> 1) ^ (res & 1 ? k_polynomial : 0);
    table[i] = res;
  }
  return table;
}

static constexpr auto k_table = make_table();  // 生成済み CRC32C テーブル

/**
 * @brief 1 バイト分の CRC32C 更新を行う
 *
 * @param crc これまでの CRC 値
 * @param byte 入力バイト
 * @return std::uint32_t 更新後の CRC 値
 */
constexpr auto update_byte(std::uint32_t crc, std::uint8_t byte) noexcept -> std::uint32_t {
  return (crc >> 8) ^ k_table[(crc ^ byte) & 0xFF];  // 下位 8 ビットでテーブルを索引
}

/**
 * @brief ソフトウェア版 CRC32C ハッシュ（シード付き）
 *
 * @param key ハッシュ対象のキー
 * @param seed ハッシュシード
 * @return std::uint32_t ハッシュ値
 */
constexpr auto hash_software(std::string_view key, std::uint32_t seed) noexcept -> std::uint32_t {
  std::uint32_t crc = seed;
  for (auto const c : key) crc = update_byte(crc, static_cast<std::uint8_t>(c));
  return crc;
}

} // namespace crc32c

/**
 * @brief ハッシュアルゴリズム: ハードウェア命令（SSE4.2 / ARM CRC32）を使用、そうでなければソフトウェア実装
 */
constexpr auto hash_impl(std::string_view key, std::uint32_t seed) noexcept -> std::uint64_t {
  auto h = [key, seed] {
    if (!std::is_constant_evaluated()) {
#if defined(__SSE4_2__)
      std::uint64_t crc = seed;
      auto const n = key.size();
      auto const d = key.data();
      std::size_t i = 0;
      for (; i + 8 <= n; i += 8) {
          std::uint64_t chunk = 0;
          for (std::size_t j = 0; j < 8; ++j) {
            chunk |= static_cast<std::uint64_t>(static_cast<unsigned char>(d[i + j])) << (j * 8);
          }
          crc = _mm_crc32_u64(crc, chunk);
      }
      for (; i < n; ++i) crc = _mm_crc32_u8(static_cast<std::uint32_t>(crc), static_cast<std::uint8_t>(d[i]));
      return crc;
#elif defined(__aarch64__) && defined(__ARM_FEATURE_CRC32)
      std::uint32_t crc = seed;
      auto const n = key.size();
      auto const d = reinterpret_cast<const std::uint8_t*>(key.data());
      std::size_t i = 0;
      for (; i + 8 <= n; i += 8) {
          std::uint64_t chunk = 0;
          for (std::size_t j = 0; j < 8; ++j) {
            chunk |= static_cast<std::uint64_t>(d[i + j]) << (j * 8);
          }
          crc = __crc32cd(crc, chunk);
      }
      for (; i < n; ++i) crc = __crc32cb(crc, d[i]);
      return static_cast<std::uint64_t>(crc);
#endif
      // SSE4.2 / ARM CRC32 のどちらも使えない環境では if ブロックを抜けてソフトウェア実装へフォールバックする
    }
    return static_cast<std::uint64_t>(crc32c::hash_software(key, seed));
  }();
  // ファイナライザ（SplitMix64 と同等の定数）: CRC32 の線形性を破り、同長文字列でもシードが効くようにする
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdull;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53ull;
  h ^= h >> 33;
  return h;
}

/**
 * @brief 引数以上の最小の 2 のべき乗を求める
 *
 * @param n 基準値
 * @return std::size_t n 以上の最小の 2 のべき乗
 */
[[nodiscard]] consteval auto next_pow2(std::size_t n) -> std::size_t {
  std::size_t res = 1; while (res < n) res <<= 1; return res;
}

/**
 * @brief キー群に重複（同一文字列）がないかをコンパイル時に判定する
 *
 * @tparam Keys キー列（FrozenString NTTP）
 * @return bool 重複があれば true
 */
template <FrozenString... Keys>
[[nodiscard]] consteval auto has_duplicate_keys() -> bool {
  if constexpr (sizeof...(Keys) <= 1) {
    return false;
  } else {
    constexpr std::array key_views{ std::string_view{Keys.buffer.data(), Keys.length}... };
    auto sorted = key_views;
    std::ranges::sort(sorted);
    for (auto i = 1uz; i < sorted.size(); ++i) {
      if (sorted[i - 1] == sorted[i]) {
        return true;
      }
    }
    return false;
  }
}

/**
 * @brief テーブルサイズに応じた適切なインデックス型を選択する
 */
template <std::size_t TableSize>
using index_type_t = std::conditional_t<(TableSize < 127), std::int8_t,
                    std::conditional_t<(TableSize < 32767), std::int16_t, std::int32_t>>;

/**
 * @brief ルックアップテーブルのメタデータとシード値の探索結果
 */
template <std::size_t TableSize, std::size_t KeyCount>
struct lookup_seed_result {
  using index_t = index_type_t<KeyCount>;
  std::uint32_t seed;                     ///< 衝突を回避するためのハッシュシード
  std::array<index_t, TableSize> table;   ///< インデックス値を格納するルックアップテーブル
};

/**
 * @brief 完全衝突ゼロのルックアップテーブルを構築するためのシード値を探索する
 * キー数が多い場合、コンパイル時の計算量制限を回避するためにこの関数の呼び出しはスキップされる必要がある。
 * シードは 16 刻みの粗探索で当たりをつけた後、密探索に切り替える 2 段階戦略で収束を速める。
 */
template <std::size_t TableSize, FrozenString... Keys>
[[nodiscard]] consteval auto find_lookup_seed() {
  using result_t = lookup_seed_result<TableSize, sizeof...(Keys)>;
  using index_t = typename result_t::index_t;
  constexpr std::array key_views{ std::string_view{Keys.buffer.data(), Keys.length}... };
  constexpr auto mask = TableSize - 1;
  // シードは衝突が見つかった周辺だけ 1 刻みで再探索する 2 段階戦略で
  // 100 万線形より高速に収束する。上限は 100 万を維持しつつ無駄走査を抑える。
  constexpr auto kCoarseStep = 16u;
  constexpr auto kSeedLimit = 1'000'000u;
  auto best_seed = 0u;
  for (auto seed = 0u; seed < kSeedLimit; seed += kCoarseStep) {
    std::array<index_t, TableSize> table; table.fill(static_cast<index_t>(-1));
    auto collision = false;
    for (auto i = 0uz; i < sizeof...(Keys); ++i) {
      auto const slot = static_cast<std::size_t>(hash_impl(key_views[i], seed) & mask);
      if (table[slot] != static_cast<index_t>(-1)) { collision = true; break; }
      table[slot] = static_cast<index_t>(i);
    }
    if (!collision) { return result_t{seed, table}; }
    best_seed = seed;
  }
  // 粗探索で衝突が見つかった最後のシードの前後を密探索
  for (auto delta = 1u; delta < kCoarseStep && best_seed + delta < kSeedLimit; ++delta) {
    auto const seed = best_seed + delta;
    std::array<index_t, TableSize> table; table.fill(static_cast<index_t>(-1));
    auto collision = false;
    for (auto i = 0uz; i < sizeof...(Keys); ++i) {
      auto const slot = static_cast<std::size_t>(hash_impl(key_views[i], seed) & mask);
      if (table[slot] != static_cast<index_t>(-1)) { collision = true; break; }
      table[slot] = static_cast<index_t>(i);
    }
    if (!collision) { return result_t{seed, table}; }
  }
  throw "frozen_map seed search exhausted";
}

/**
 * @brief CHD（bucket displacement）方式の 2 段ハッシュテーブル
 *
 * 1 段目: hash(key, 0) でバケットを決定。
 * 2 段目: バケットごとのシード d を使い hash(key, d) でスロットを決定。
 * 構築時に大きいバケットから順に衝突しないシードを探索するため、任意のキー数で O(1) 探索が可能。
 */
template <std::size_t BucketCount, std::size_t TableSize, std::size_t KeyCount>
struct chd_result {
  using index_t = index_type_t<KeyCount>;
  std::array<std::uint32_t, BucketCount> bucket_seeds{};  ///< バケットごとの 2 段目シード
  std::array<index_t, TableSize> table{};                 ///< スロット → 要素インデックス（-1 = 空）
};

/**
 * @brief CHD テーブルをコンパイル時に構築する
 *
 * @tparam BucketCount バケット数（2 冪）
 * @tparam TableSize スロット数（2 冪、KeyCount 以上）
 * @param key_views キービューの配列
 * @return chd_result 構築されたテーブル
 */
template <std::size_t BucketCount, std::size_t TableSize, std::size_t KeyCount>
[[nodiscard]] consteval auto build_chd(std::array<std::string_view, KeyCount> const& key_views)
    -> chd_result<BucketCount, TableSize, KeyCount> {
  using result_t = chd_result<BucketCount, TableSize, KeyCount>;
  using index_t = typename result_t::index_t;
  constexpr auto bucket_mask = BucketCount - 1;
  constexpr auto slot_mask = TableSize - 1;

  auto result = result_t{};
  result.table.fill(static_cast<index_t>(-1));

  // 各キーの 1 段目バケット割り当て
  std::array<std::size_t, KeyCount> bucket_of{};
  for (auto i = 0uz; i < KeyCount; ++i) {
    bucket_of[i] = static_cast<std::size_t>(hash_impl(key_views[i], 0) & bucket_mask);
  }

  // カウンティングソートでバケットごとのキーを連続領域に並べる
  std::array<std::uint32_t, BucketCount + 1> starts{};
  for (auto const b : bucket_of) ++starts[b + 1];
  for (auto b = 0uz; b < BucketCount; ++b) starts[b + 1] += starts[b];
  std::array<std::uint32_t, KeyCount> keys_by_bucket{};
  {
    auto cursor = starts;
    for (auto i = 0uz; i < KeyCount; ++i) {
      keys_by_bucket[cursor[bucket_of[i]]++] = static_cast<std::uint32_t>(i);
    }
  }

  // 大きいバケットから処理する（空きスロットが多いうちに配置してシード探索を高速化）
  std::array<std::uint32_t, BucketCount> order{};
  for (auto b = 0uz; b < BucketCount; ++b) order[b] = static_cast<std::uint32_t>(b);
  std::ranges::sort(order, [&](auto const a, auto const b) {
    return (starts[a + 1] - starts[a]) > (starts[b + 1] - starts[b]);
  });

  std::array<std::size_t, KeyCount> cand{};  // 現在のバケットの暫定スロット
  for (auto const b : order) {
    auto const first = starts[b];
    auto const last = starts[b + 1];
    if (first == last) break;  // サイズ降順なので以降はすべて空バケット
    auto found = false;
    // 2 段階シード探索: 16 刻みの粗探索で見つけた候補の前後を密探索する。
    // バケット内のキーが衝突しない候補 d を総当たりで探す。
    constexpr auto kCoarseStep = 16u;
    constexpr auto kSeedLimit = 1'000'000u;
    auto best_d = 1u;
    for (auto d = 1u; d < kSeedLimit && !found; d += kCoarseStep) {
      auto ok = true;
      for (auto m = first; m < last; ++m) {
        auto const slot = static_cast<std::size_t>(hash_impl(key_views[keys_by_bucket[m]], d) & slot_mask);
        if (result.table[slot] != static_cast<index_t>(-1)) { ok = false; break; }
        auto dup = false;
        for (auto p = first; p < m; ++p) {
          if (cand[p - first] == slot) { dup = true; break; }
        }
        if (dup) { ok = false; break; }
        cand[m - first] = slot;
      }
      if (ok) {
        for (auto m = first; m < last; ++m) {
          result.table[cand[m - first]] = static_cast<index_t>(keys_by_bucket[m]);
        }
        result.bucket_seeds[b] = d;
        found = true;
        break;
      }
      best_d = d;
    }
    // 粗探索で衝突した候補の前後 1..15 を密探索
    for (auto delta = 1u; delta < kCoarseStep && best_d + delta < kSeedLimit && !found; ++delta) {
      auto const d = best_d + delta;
      auto ok = true;
      for (auto m = first; m < last; ++m) {
        auto const slot = static_cast<std::size_t>(hash_impl(key_views[keys_by_bucket[m]], d) & slot_mask);
        if (result.table[slot] != static_cast<index_t>(-1)) { ok = false; break; }
        auto dup = false;
        for (auto p = first; p < m; ++p) {
          if (cand[p - first] == slot) { dup = true; break; }
        }
        if (dup) { ok = false; break; }
        cand[m - first] = slot;
      }
      if (ok) {
        for (auto m = first; m < last; ++m) {
          result.table[cand[m - first]] = static_cast<index_t>(keys_by_bucket[m]);
        }
        result.bucket_seeds[b] = d;
        found = true;
        break;
      }
    }
    if (!found) throw "frozen_map CHD seed search exhausted";
  }
  return result;
}

/**
 * @brief CHD テーブルからキー候補のインデックスを引く
 *
 * @return index_t 候補インデックス（-1 = 候補なし）。最終的なキー一致確認は呼び出し側で行う。
 */
template <std::size_t BucketCount, std::size_t TableSize, std::size_t KeyCount>
[[nodiscard]] constexpr auto chd_find(chd_result<BucketCount, TableSize, KeyCount> const& chd,
                                      std::string_view key) noexcept {
  auto const bucket = static_cast<std::size_t>(hash_impl(key, 0) & (BucketCount - 1));
  auto const slot = static_cast<std::size_t>(hash_impl(key, chd.bucket_seeds[bucket]) & (TableSize - 1));
  return chd.table[slot];
}

/**
 * @brief キー集合に対する高速ルックアップ用の静的メタデータを保持する
 *
 * @tparam Keys キー列（FrozenString NTTP）
 */
template <FrozenString... Keys>
struct lookup_index {
  static constexpr auto size() noexcept -> std::size_t { return sizeof...(Keys); }
  using size_type = std::size_t;
  using index_t = detail::index_type_t<size()>;  // 要素数に応じた最小の符号付きインデックス型

  // 宣言順のキービュー
  static constexpr std::array<std::string_view, size()> key_views_{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };

  static constexpr auto k_max_key_len_ = std::max({Keys.length...});  // キーの最大長

  // 存在するキー長の集合（長さによる迅速な除外用）
  static constexpr auto valid_lengths_ = [] {
    std::array<bool, k_max_key_len_ + 1> table{};
    ((table[Keys.length] = true), ...);
    return table;
  }();

  // 全キー長が一意（長さだけで特定可能）か
  static constexpr auto all_lengths_unique_ = [] {
    std::array<std::size_t, size()> lens{Keys.length...};
    std::ranges::sort(lens);
    return std::ranges::adjacent_find(lens) == lens.end();
  }();

  // キー長 → 要素インデックスのマップ（長さ一意の場合にのみ有効なスロットを保持）
  static constexpr auto length_to_index_ = [] {
    std::array<index_t, k_max_key_len_ + 1> table{};
    table.fill(static_cast<index_t>(-1));
    std::size_t idx = 0;
    ((table[Keys.length] = static_cast<index_t>(idx++)), ...);
    return table;
  }();

  // 辞書順にソートしたキービュー（二分探索用）
  static constexpr std::array<std::string_view, size()> sorted_key_views_ = [] {
    auto res = key_views_;
    std::ranges::sort(res);
    return res;
  }();

  static constexpr auto sorted_indices_ = [] {
    // インデックス付きソート: O(N log N) で sorted_key_views_ と対応する元インデックスを求める
    std::array<index_t, size()> res{};
    std::array<std::size_t, size()> order{};
    for (auto i = 0uz; i < size(); ++i) {
      order[i] = i;
    }
    std::ranges::sort(order, [](auto a, auto b) { return key_views_[a] < key_views_[b]; });
    for (auto i = 0uz; i < size(); ++i) {
      res[i] = static_cast<index_t>(order[i]);
    }
    return res;
  }();

  static constexpr auto k_lookup_threshold = 64uz;  // ルックアップテーブルを使う要素数の上限（これ超えは二分探索へ）
  static constexpr auto k_chd_threshold = 256uz;  // 二分探索を使う要素数の上限（これ超えは CHD 方式）
  static constexpr auto use_lookup_table_ = (size() <= k_lookup_threshold);  // ルックアップテーブル方式か
  static constexpr auto use_binary_search_ = (size() > k_lookup_threshold && size() <= k_chd_threshold);  // 中規模: ソート済みキーへの二分探索
  static constexpr auto use_chd_ = (size() > k_chd_threshold);  // 大規模: CHD 2 段ハッシュ
  static constexpr auto table_size_ = use_lookup_table_ ? detail::next_pow2(size() * 4) : 1uz;  // 負荷率 1/4 を見込んだ 2 冪サイズ
  static constexpr auto mask_ = table_size_ - 1;  // ビットマスク（table_size_ は 2 冪）

  // ルックアップテーブル方式なら衝突ゼロのシードを探索、それ以外は空メタデータ
  static consteval auto make_lookup_metadata() {
    if constexpr (use_lookup_table_) {
      return detail::find_lookup_seed<table_size_, Keys...>();
    } else {
      return detail::lookup_seed_result<table_size_, size()>{0, {static_cast<index_t>(-1)}};
    }
  }
  static constexpr auto metadata_ = make_lookup_metadata();  // ルックアップテーブル方式のメタデータ（シード + テーブル）
  static constexpr auto k_seed = metadata_.seed;        // ハッシュシード
  static constexpr auto lookup_table_ = metadata_.table;  // スロット → 要素インデックス

  // 大規模（k_lookup_threshold 超）: CHD 2 段ハッシュで O(1) を維持する
  static constexpr auto chd_bucket_count_ = use_lookup_table_ ? 1uz : detail::next_pow2(size());
  static constexpr auto chd_table_size_ = use_lookup_table_ ? 1uz : detail::next_pow2(size() * 2);  // 負荷率 1/2
  static consteval auto make_chd_metadata() {
    if constexpr (use_lookup_table_) {
      return detail::chd_result<1, 1, size()>{};  // 未使用のダミー
    } else {
      return detail::build_chd<chd_bucket_count_, chd_table_size_>(key_views_);
    }
  }
  static constexpr auto chd_ = make_chd_metadata();  // CHD 方式のメタデータ（バケットシード + テーブル）

  static constexpr bool all_keys_short = ((Keys.length <= 16) && ...);  // 全キーが 16 バイト以下（128bit 比較可能）

  // 16 バイト以下のキーを 128bit（2×uint64）にパックして一括比較可能にする
  struct alignas(16) padded_key {
    std::uint64_t data[2]{0, 0};  // キー文字列をバイト列として詰めた 16 バイト
    std::uint8_t len = 0;         // 実際のキー長
  };

  // 全キーを padded_key 配列に変換する（短いキーの高速比較用）
  static consteval auto make_padded_keys() {
    std::array<padded_key, size()> res{};
    std::size_t idx = 0;
    ([&] {
      res[idx].len = static_cast<std::uint8_t>(Keys.length);
      for (std::size_t i = 0; i < Keys.length; ++i)
        res[idx].data[i / 8] |=
          (static_cast<std::uint64_t>(static_cast<unsigned char>(Keys.buffer[i])) << ((i % 8) * 8));
      ++idx;
    }(), ...);
    return res;
  }

  // 短いキーのみパディング版を保持、さもなければ空配列（key_equals で通常比較にフォールバック）
  static constexpr auto padded_keys_ = [] {
    if constexpr (all_keys_short) {
      return make_padded_keys();
    } else {
      return std::array<padded_key, size()>{};
    }
  }();

  /**
   * @brief 指定インデックスのキーと入力キーが等しいかを判定する
   *
   * @param key 比較する入力キー
   * @param index 比較対象の要素インデックス
   * @return bool 等しければ true
   */
  [[nodiscard]] static constexpr auto key_equals(std::string_view key, size_type index) noexcept -> bool {
    if constexpr (all_keys_short) {
      // 短いキー: 128bit パックと長さ比較で一括照合
      if (key.size() > 16) return false;
      auto const& pk = padded_keys_[index];
      if (key.size() != pk.len) return false;
      std::uint64_t low = 0, high = 0;
      auto const d = key.data();
      auto const n = key.size();
      for (std::size_t i = 0; i < n; ++i) {
        if (i < 8) low |= (static_cast<std::uint64_t>(static_cast<unsigned char>(d[i])) << (i * 8));
        else high |= (static_cast<std::uint64_t>(static_cast<unsigned char>(d[i])) << ((i - 8) * 8));
      }
      return (low == pk.data[0] && high == pk.data[1]);
    }
    // 長いキー: 通常の string_view 比較
    return key_views_[index] == key;
  }

  /**
   * @brief キーから要素インデックスを探索する
   *
   * @param key 探索するキー
   * @return size_type 見つかったインデックス、未検出は size()（番兵）
   */
  static constexpr auto find_index_raw(std::string_view key) noexcept -> size_type {
    auto const len = key.size();
    // 長さが一意なら長さだけでインデックスを一意特定（ハッシュ不要）
    if constexpr (all_lengths_unique_) {
      if (len > k_max_key_len_) return size();
      auto const index = length_to_index_[len];
      if (index == static_cast<decltype(index)>(-1)) return size();
      if (key_equals(key, static_cast<size_type>(index))) [[likely]]
        return static_cast<size_type>(index);
      return size();
    }
    // 中規模（小）: シード付きハッシュでスロットを引き、衝突時は key_equals で確定
    if constexpr (use_lookup_table_) {
      if (len > k_max_key_len_ || !valid_lengths_[len]) return size();
      auto const h = detail::hash_impl(key, k_seed);
      auto const slot = static_cast<std::size_t>(h & mask_);
      auto const index = lookup_table_[slot];
      if (index == static_cast<decltype(index)>(-1)) return size();
      if (key_equals(key, static_cast<size_type>(index))) [[likely]]
        return static_cast<size_type>(index);
      return size();
    }
    // 中規模（中）: ソート済みキービューへ二分探索
    if constexpr (use_binary_search_) {
      if (len > k_max_key_len_ || !valid_lengths_[len]) return size();
      auto const it = std::ranges::lower_bound(sorted_key_views_, key);
      if (it == sorted_key_views_.end() || *it != key) return size();
      auto const sorted_pos = static_cast<size_type>(it - sorted_key_views_.begin());
      return static_cast<size_type>(sorted_indices_[sorted_pos]);
    }
    // 大規模: CHD 2 段ハッシュでスロットを引き、key_equals で確定
    if constexpr (use_chd_) {
      if (len > k_max_key_len_ || !valid_lengths_[len]) return size();
      auto const index = detail::chd_find(chd_, key);
      if (index == static_cast<decltype(index)>(-1)) return size();
      if (key_equals(key, static_cast<size_type>(index))) [[likely]]
        return static_cast<size_type>(index);
      return size();
    }
    return size();
  }
};

/**
 * @brief 要素が tuple_size==2 の pair-like かを判定するコンセプト
 */
template <typename EntryLike>
concept PairLikeEntry = requires {
  typename std::tuple_size<std::remove_cvref_t<EntryLike>>::type;
  requires std::tuple_size_v<std::remove_cvref_t<EntryLike>> == 2;
};

/**
 * @brief pair-like 要素から指定位置の値を取得する（ADL 経由の get を使用）
 *
 * @tparam Index 取得する位置（0=キー, 1=値）
 * @tparam EntryLike 要素の型
 * @param entry 要素
 * @return decltype(auto) 該当位置の値への参照
 */
template <std::size_t Index, typename EntryLike>
constexpr decltype(auto) pair_like_get(EntryLike&& entry) {
  using std::get; return get<Index>(std::forward<EntryLike>(entry));
}

// std::map / std::unordered_map / std::array を判定する型特性
template <typename T> struct is_std_map : std::false_type {};
template <typename K, typename V, typename C, typename A> struct is_std_map<std::map<K, V, C, A>> : std::true_type {};
template <typename T> inline constexpr bool is_std_map_v = is_std_map<std::remove_cvref_t<T>>::value;
template <typename T> struct is_std_unordered_map : std::false_type {};
template <typename K, typename V, typename H, typename P, typename A> struct is_std_unordered_map<std::unordered_map<K, V, H, P, A>> : std::true_type {};
template <typename T> inline constexpr bool is_std_unordered_map_v = is_std_unordered_map<std::remove_cvref_t<T>>::value;
template <typename T> struct is_std_array : std::false_type {};
template <typename T, std::size_t N> struct is_std_array<std::array<T, N>> : std::true_type {};
template <typename T> inline constexpr bool is_std_array_v = is_std_array<std::remove_cvref_t<T>>::value;
template <typename T> inline constexpr bool always_false_v = false;  // static_assert の文脈で常に false を与える（展開抑止）

/**
 * @brief to() の変換先が連想コンテナ（std::map / std::unordered_map）かを判定する
 */
template <typename Result, typename ValueArg>
concept is_frozen_map_associative_result = (is_std_map_v<Result> || is_std_unordered_map_v<Result>) &&
    std::default_initializable<std::remove_cvref_t<Result>> &&
    std::constructible_from<typename std::remove_cvref_t<Result>::key_type, std::string_view> &&
    std::constructible_from<typename std::remove_cvref_t<Result>::mapped_type, ValueArg> &&
    requires(std::remove_cvref_t<Result>& result, typename std::remove_cvref_t<Result>::key_type key, typename std::remove_cvref_t<Result>::mapped_type value) {
      result.emplace(std::move(key), std::move(value));
    };

/**
 * @brief is_frozen_map_associative_result のエイリアスコンセプト（公開名）
 */
template <typename Result, typename ValueArg>
concept frozen_map_associative_result = is_frozen_map_associative_result<Result, ValueArg>;

/**
 * @brief to() の変換先が固定長配列（std::array<pair-like, 要素数>）かを判定する
 */
template <typename Result, std::size_t ExpectedSize, typename ValueArg>
concept is_frozen_map_array_result = is_std_array_v<Result> && std::tuple_size_v<std::remove_cvref_t<Result>> == ExpectedSize &&
    PairLikeEntry<typename std::remove_cvref_t<Result>::value_type> &&
    std::constructible_from<typename std::remove_cvref_t<Result>::value_type, std::string_view, ValueArg>;

/**
 * @brief is_frozen_map_array_result のエイリアスコンセプト（公開名）
 */
template <typename Result, std::size_t ExpectedSize, typename ValueArg>
concept frozen_map_array_result = is_frozen_map_array_result<Result, ExpectedSize, ValueArg>;

/**
 * @brief to() が受け付ける変換先結果型（連想コンテナまたは固定長配列）の総合コンセプト
 */
template <typename Result, std::size_t ExpectedSize, typename ValueArg>
concept frozen_map_result = frozen_map_associative_result<Result, ValueArg> || frozen_map_array_result<Result, ExpectedSize, ValueArg>;

} // namespace detail

/**
 * @brief コンパイル時に構築される、キー固定の連想コンテナ（frozen map）
 *
 * @tparam T 値の型
 * @tparam Keys キー列（FrozenString NTTP）
 */
template <typename T, FrozenString... Keys>
class frozen_map {
public:
  using key_type = std::string_view;
  using mapped_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using value_type = std::pair<std::string_view const, T>;
  using reference = std::pair<std::string_view, T&>;
  using const_reference = std::pair<std::string_view, T const&>;

  /**
   * @brief frozen_map のイテレータ基底（ランダムアクセス、キーはビューで固定）
   *
   * @tparam Owner 所有側の frozen_map 型
   * @tparam Ref 参照の値型（pair-like）
   */
  template <typename Owner, typename Ref>
  class iterator_base {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = frozen_map::value_type;
    using difference_type = std::ptrdiff_t;

    // operator-> の戻り値用プロキシ（キー・値へのメンバアクセスを提供）
    struct arrow_proxy_impl {
        Ref ref_v;
        std::string_view& key;
        decltype(std::declval<Ref>().second)& value;
        constexpr arrow_proxy_impl(Ref r) : ref_v(r), key(ref_v.first), value(ref_v.second) {}
        constexpr auto operator->() noexcept -> arrow_proxy_impl* { return this; }
        constexpr auto operator->() const noexcept -> const arrow_proxy_impl* { return this; }
    };
    using arrow_proxy = arrow_proxy_impl;
    using pointer = arrow_proxy;
    using reference = Ref;

    constexpr iterator_base() noexcept = default;
    constexpr iterator_base(Owner* owner, size_type index) noexcept : owner_{owner}, index_{index} {}
    constexpr auto operator*() const noexcept -> reference { return reference{detail::lookup_index<Keys...>::key_views_[index_], owner_->values_[index_]}; }
    constexpr auto operator->() const noexcept -> pointer { return pointer{operator*()}; }
    constexpr auto operator++() noexcept -> iterator_base& { ++index_; return *this; }
    constexpr auto operator++(int) noexcept -> iterator_base { auto tmp = *this; ++index_; return tmp; }
    constexpr auto operator--() noexcept -> iterator_base& { --index_; return *this; }
    constexpr auto operator--(int) noexcept -> iterator_base { auto tmp = *this; --index_; return tmp; }
    constexpr auto operator+=(difference_type n) noexcept -> iterator_base& { index_ += n; return *this; }
    constexpr auto operator-=(difference_type n) noexcept -> iterator_base& { index_ -= n; return *this; }
    friend constexpr auto operator+(iterator_base a, difference_type n) noexcept -> iterator_base { return a += n; }
    friend constexpr auto operator+(difference_type n, iterator_base a) noexcept -> iterator_base { return a += n; }
    friend constexpr auto operator-(iterator_base a, difference_type n) noexcept -> iterator_base { return a -= n; }
    friend constexpr auto operator-(iterator_base a, iterator_base b) noexcept -> difference_type { return static_cast<difference_type>(a.index_) - static_cast<difference_type>(b.index_); }
    friend constexpr bool operator==(iterator_base const& a, iterator_base const& b) noexcept { return a.index_ == b.index_; }
    friend constexpr auto operator<=>(iterator_base const& a, iterator_base const& b) noexcept { return a.index_ <=> b.index_; }
  private:
    Owner* owner_{nullptr}; size_type index_{0};
  };

  using iterator = iterator_base<frozen_map, reference>;
  using const_iterator = iterator_base<frozen_map const, const_reference>;

  static_assert(sizeof...(Keys) > 0, "frozen_map requires at least one key");
  static_assert(!detail::has_duplicate_keys<Keys...>(), "frozen_map keys must be unique");
  /** @brief キーの総数を返す */
  static constexpr auto size() noexcept -> size_type { return sizeof...(Keys); }
  /** @brief 格納可能な最大要素数（size() と同じ） */
  static constexpr auto max_size() noexcept -> size_type { return size(); }
  /** @brief 常に false（frozen_map は少なくとも1つのキーを要求する） */
  [[nodiscard]] static constexpr auto empty() noexcept -> bool { return false; }
  /**
   * @brief 同じキー集合を持つ 2 つの frozen_map の全値が等しいかを判定する
   * @details キー集合（Keys...）が一致する場合にのみ ADL で見つかる hidden friend として定義される。
   *          値型 T に operator== が定義されている必要がある。
   */
  [[nodiscard]] friend constexpr auto operator==(frozen_map const& lhs, frozen_map const& rhs) noexcept -> bool {
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return ((lhs.values_[I] == rhs.values_[I]) && ... && true);
    }(std::make_index_sequence<size()>{});
  }
  /**
   * @brief 辞書順にソートされたキー配列を取得する
   */
  [[nodiscard]] static constexpr auto keys() noexcept -> std::span<const std::string_view, size()> {
    return lookup_::sorted_key_views_;
  }
  /**
   * @brief 宣言順のキー配列を取得する
   */
  [[nodiscard]] static constexpr auto keys_in_declaration_order() noexcept
    -> std::span<std::string_view const, size()> {
    return lookup_::key_views_;
  }
  /**
   * @brief キーに対応する要素を探索する
   *
   * @param key 探索するキー
   * @return iterator 見つかった要素、未検出は end()
   */
  [[nodiscard]] constexpr auto find(std::string_view key) noexcept -> iterator {
    auto const i = lookup_::find_index_raw(key);
    return i != size() ? iterator{this, i} : end();
  }
  /**
   * @brief キーに対応する要素を探索する（const 版）
   *
   * @param key 探索するキー
   * @return const_iterator 見つかった要素、未検出は end()
   */
  [[nodiscard]] constexpr auto find(std::string_view key) const noexcept -> const_iterator {
    auto const i = lookup_::find_index_raw(key);
    return i != size() ? const_iterator{this, i} : end();
  }
  /**
   * @brief キーが存在するかを個数で返す（0 または 1）
   *
   * @param key 調べるキー
   * @return size_type 存在すれば 1、さもなければ 0
   */
  [[nodiscard]] constexpr auto count(std::string_view key) const noexcept -> size_type {
    return lookup_::find_index_raw(key) != size() ? 1uz : 0uz;
  }
  /// @brief 先頭イテレータを返す
  constexpr auto begin() noexcept -> iterator { return iterator{this, 0}; }
  /// @brief 末尾イテレータを返す
  constexpr auto end() noexcept -> iterator { return iterator{this, size()}; }
  /// @brief 先頭イテレータを返す（const 版）
  constexpr auto begin() const noexcept -> const_iterator { return const_iterator{this, 0}; }
  /// @brief 末尾イテレータを返す（const 版）
  constexpr auto end() const noexcept -> const_iterator { return const_iterator{this, size()}; }
  /// @brief 先頭イテレータを返す（const_iterator）
  constexpr auto cbegin() const noexcept -> const_iterator { return begin(); }
  /// @brief 末尾イテレータを返す（const_iterator）
  constexpr auto cend() const noexcept -> const_iterator { return end(); }
  /**
   * @brief デフォルトコンストラクタ（値型がデフォルト構築可能な場合のみ）
   */
  constexpr frozen_map() noexcept requires std::default_initializable<T> = default;
  /**
   * @brief 値配列から構築する（キー順に対応）
   * @param values キー順に対応する値の配列
   */
  constexpr explicit frozen_map(std::array<T, size()> values) noexcept(std::is_nothrow_move_constructible_v<T>) : values_{std::move(values)} {}
  /**
   * @brief 初期化リストから構築する（要素数はキー数と一致が必要）
   * @param values キー順に対応する値の初期化リスト
   */
  constexpr explicit frozen_map(std::initializer_list<T> values) requires std::constructible_from<T, T const&> : values_{copy_initializer_list(values)} {}
  /**
   * @brief キー・値エントリ配列から構築する（キー一致で配置、欠落キーは例外）
   * @param entries キー・値ペアの配列
   */
  constexpr explicit frozen_map(std::array<frozen_map_entry<T>, size()> entries) : values_{reorder_entries(std::move(entries))} {}
  /**
   * @brief キーが存在するかを判定する
   *
   * @param key 調べるキー
   * @return bool 存在すれば true
   */
  [[nodiscard]] constexpr auto contains(std::string_view key) const noexcept -> bool {
    return lookup_::find_index_raw(key) != size();
  }
  /**
   * @brief 複数のキーが全て存在するかを一括判定する (consteval)
   * @details 空パックに対しては true (vacuous truth) を返す。
   */
  template <FrozenString... QueryKeys>
  [[nodiscard]] static consteval auto contains_all() noexcept -> bool {
    return ((contains_impl<QueryKeys>()) && ... && true);
  }
  /**
   * @brief キーに対応する値への参照を取得する（未検出は例外）
   *
   * @param key 探索するキー
   * @return T& 値への参照
   * @throw std::out_of_range キーが存在しない場合
   */
  [[nodiscard]] constexpr auto at(std::string_view key) -> T& {
    auto const i = lookup_::find_index_raw(key);
    if (i != size()) [[likely]] return values_[i];
    throw std::out_of_range(
      std::string{"frozen_map key not found: "} + std::string{key});
  }
  /**
   * @brief キーに対応する値への参照を取得する（const 版、未検出は例外）
   *
   * @param key 探索するキー
   * @return T const& 値への参照
   * @throw std::out_of_range キーが存在しない場合
   */
  [[nodiscard]] constexpr auto at(std::string_view key) const -> T const& {
    auto const i = lookup_::find_index_raw(key);
    if (i != size()) [[likely]] return values_[i];
    throw std::out_of_range(
      std::string{"frozen_map key not found: "} + std::string{key});
  }
  /**
   * @brief キーに対応する値を optional で取得する（未検出は nullopt）
   *
   * @param key 探索するキー
   * @return std::optional<std::reference_wrapper<T>> 値への参照、または空
   */
  [[nodiscard]] constexpr auto get(std::string_view key) noexcept -> std::optional<std::reference_wrapper<T>> { if (auto const index = find_index_opt(key); index) [[likely]] return values_[*index]; return std::nullopt; }
  /**
   * @brief キーに対応する値を optional で取得する（const 版）
   *
   * @param key 探索するキー
   * @return std::optional<std::reference_wrapper<T const>> 値への参照、または空
   */
  [[nodiscard]] constexpr auto get(std::string_view key) const noexcept -> std::optional<std::reference_wrapper<T const>> { if (auto const index = find_index_opt(key); index) [[likely]] return values_[*index]; return std::nullopt; }
  /**
   * @brief キーがあればその値、なければデフォルト値を返す
   *
   * @param key 探索するキー
   * @param default_value 未検出時に返す値
   * @return T 取得した値またはデフォルト値
   */
  [[nodiscard]] constexpr auto get_value_or(std::string_view key, T const& default_value) const noexcept -> T {
    if (auto const slot = find_index_opt(key); slot) [[likely]] return values_[*slot];
    return default_value;
  }
  /**
   * @brief キーに対応する値への参照を取得する（未検出は at() が例外を送出）
   * @param key 探索するキー
   * @return T& / T const& 値への参照
   */
  constexpr auto operator[](std::string_view key) -> T& { return at(key); }
  constexpr auto operator[](std::string_view key) const -> T const& { return at(key); }
  /**
   * @brief lvalue のマップを指定した結果型へ変換する
   * @tparam Result 変換先（std::map / std::unordered_map / std::array<pair-like, size()>）
   * @return Result 変換結果
   */
  template <typename Result> requires detail::frozen_map_result<Result, size(), detail::forward_like_t<frozen_map const&, mapped_type>>
  [[nodiscard]] constexpr auto to() const& -> Result { return to_result<Result>(*this); }
  /**
   * @brief rvalue のマップを指定した結果型へ変換する（値はムーブされる）
   * @tparam Result 変換先（std::map / std::unordered_map / std::array<pair-like, size()>）
   * @return Result 変換結果
   */
  template <typename Result> requires detail::frozen_map_result<Result, size(), detail::forward_like_t<frozen_map&&, mapped_type>>
  [[nodiscard]] constexpr auto to() && -> Result { return to_result<Result>(std::move(*this)); }
private:
  using lookup_ = detail::lookup_index<Keys...>;

  /**
   * @brief 単一 NTTP キーがマップ内に存在するか (contains_all 内部用)
   */
  template <FrozenString Key>
  [[nodiscard]] static consteval auto contains_impl() noexcept -> bool {
    return ((Key.sv() == std::string_view{Keys.buffer.data(), Keys.length}) || ... || false);
  }

  /**
   * @brief キーから要素インデックスを optional で取得する
   *
   * @param key 探索するキー
   * @return std::optional<size_type> インデックス、または空
   */
  [[nodiscard]] static constexpr auto find_index_opt(std::string_view key) noexcept
      -> std::optional<size_type> {
    auto const i = lookup_::find_index_raw(key);
    return i != size() ? std::optional<size_type>{i} : std::nullopt;
  }
  // 初期化リストから値配列を構築（要素数検証付き）
  static constexpr auto copy_initializer_list(std::initializer_list<T> values) -> std::array<T, size()> requires std::constructible_from<T, T const&> {
    if (values.size() != size()) throw std::invalid_argument("frozen_map size mismatch: expected " + std::to_string(size()) + " values (one per key), got " + std::to_string(values.size()));
    return [&]<std::size_t... I>(std::index_sequence<I...>) { return std::array<T, size()>{ *(values.begin() + I)... }; }(std::make_index_sequence<size()>{});
  }
  // エントリ配列をキー順の値配列へ並べ替え（欠落キーは例外）
  static constexpr auto reorder_entries(std::array<frozen_map_entry<T>, size()> entries) -> std::array<T, size()> {
    auto values = std::array<std::optional<T>, size()>{};
    for (auto& entry : entries) {
      auto const index = lookup_::find_index_raw(entry.key);
      if (index == size()) {
        throw std::invalid_argument("frozen_map unknown key");
      }
      if (values[index].has_value()) {
        throw std::invalid_argument("frozen_map duplicate key");
      }
      values[index].emplace(std::move(entry.value));
    }
    for (auto const& slot : values) {
      if (!slot.has_value()) {
        throw std::invalid_argument("missing key");
      }
    }
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return std::array<T, size()>{ std::move(*values[I])... };
    }(std::make_index_sequence<size()>{});
  }
  // 値を Self の価値カテゴリ・const 性に合わせて転送する
  template <typename Self> static constexpr decltype(auto) forward_mapped(Self&& self, size_type index) { return detail::forward_like_dispatch<Self>(self.values_[index]); }
  // to() の配列（std::array<pair-like>）への変換
  template <typename Result, typename Self, std::size_t... Index> static constexpr auto to_array_result(Self&& self, std::index_sequence<Index...>) -> Result { return Result{ typename Result::value_type{ lookup_::key_views_[Index], forward_mapped<Self>(std::forward<Self>(self), Index) }... }; }
  // to() の連想コンテナ（std::map / std::unordered_map）への変換
  template <typename Result, typename Self> static constexpr auto to_associative_result(Self&& self) -> Result {
    Result result{};
    if constexpr (requires(Result& r) { r.reserve(size()); }) {
      result.reserve(size());
    }
    for (auto index = 0uz; index < size(); ++index) {
      result.emplace(typename Result::key_type{lookup_::key_views_[index]},
                     typename Result::mapped_type{forward_mapped<Self>(std::forward<Self>(self), index)});
    }
    return result;
  }
  /**
   * @brief to() の変換先に応じて配列または連想コンテナへ変換する
   *
   * @tparam Result 変換先の型
   * @tparam Self 自身の価値カテゴリ
   * @param self 変換元のマップ
   * @return Result 変換結果
   */
  template <typename Result, typename Self> static constexpr auto to_result(Self&& self) -> Result {
    using value_arg = detail::forward_like_t<Self, mapped_type>;
    if constexpr (detail::frozen_map_array_result<Result, size(), value_arg>) return to_array_result<Result>(std::forward<Self>(self), std::make_index_sequence<size()>{});
    else if constexpr (detail::frozen_map_associative_result<Result, value_arg>) return to_associative_result<Result>(std::forward<Self>(self));
    else static_assert(detail::always_false_v<Result>, "frozen_map::to<Result>() supports std::map, std::unordered_map, and std::array<pair-like, size()> results only");
  }
  std::array<T, size()> values_{};  // キー順に対応する値の格納（インデックスは lookup_index と一致）
};

/**
 * @brief pair-like 要素群から frozen_map を構築する
 *
 * @tparam T 値の型
 * @tparam Keys キー列（FrozenString NTTP）
 * @tparam Entries 要素の型パラメータパック
 * @param entries キー・値ペアの要素群
 * @return frozen_map<T, Keys...> 構築されたマップ
 */
template <typename T, FrozenString... Keys, typename... Entries>
  requires((detail::PairLikeEntry<Entries> && ...) &&
           (std::convertible_to<decltype(detail::pair_like_get<0>(std::declval<Entries>())), std::string_view> && ...) &&
           (std::convertible_to<decltype(detail::pair_like_get<1>(std::declval<Entries>())), T> && ...))
constexpr auto make_frozen_map(Entries&&... entries) -> frozen_map<T, Keys...> {
  static_assert(sizeof...(Keys) == sizeof...(Entries), "make_frozen_map requires exactly one entry per key");
  auto arr = std::array<frozen_map_entry<T>, sizeof...(Keys)>{
    frozen_map_entry<T>{detail::pair_like_get<0>(std::forward<Entries>(entries)), detail::pair_like_get<1>(std::forward<Entries>(entries))}...
  };
  return frozen_map<T, Keys...>{std::move(arr)};
}

/**
 * @brief 値配列から frozen_map を構築する
 *
 * @tparam T 値の型
 * @tparam Keys キー列（FrozenString NTTP）
 * @param values キー順に対応する値の配列
 * @return frozen_map<T, Keys...> 構築されたマップ
 */
template <typename T, FrozenString... Keys>
constexpr auto make_frozen_map(std::array<T, sizeof...(Keys)> values) -> frozen_map<T, Keys...> {
  return frozen_map<T, Keys...>{std::move(values)};
}

/**
 * @brief キー・値リテラルを保持する補助構造体（make_frozen_map_kv 用）
 *
 * @tparam N キー文字列長（終端含む）
 * @tparam V 値の型
 */
template <std::size_t N, typename V>
struct kv {
  FrozenString<N> key; V value;  // キーと値
  constexpr kv(char const (&k)[N], V v) : key{k}, value{std::move(v)} {}
};
template <std::size_t N, typename V> kv(char const (&)[N], V) -> kv<N, V>;  // 型推論ガイド

/**
 * @brief kv リテラル群から frozen_map を構築する（コンパイル時、キーは型から抽出）
 *
 * @tparam T 値の型
 * @tparam KVs kv リテラルのパラメータパック
 * @return frozen_map<T, ...> 構築されたマップ
 */
template <typename T, kv... KVs>
constexpr auto make_frozen_map_kv() -> frozen_map<T, KVs.key...> {
  return frozen_map<T, KVs.key...>{ std::array<T, sizeof...(KVs)>{ KVs.value... } };
}

} // namespace frozenchars

namespace std {

/// @brief frozen_map_entry を 2 要素の tuple-like として扱うための特殊化
template <typename T>
struct tuple_size<frozenchars::frozen_map_entry<T>> : std::integral_constant<std::size_t, 2> {};

/// @brief frozen_map_entry の指定位置の型（0=キー, 1=値）
template <std::size_t I, typename T>
struct tuple_element<I, frozenchars::frozen_map_entry<T>> { using type = std::conditional_t<I == 0, std::string_view, T>; };

/// @brief kv を 2 要素の tuple-like として扱うための特殊化
template <std::size_t N, typename V>
struct tuple_size<frozenchars::kv<N, V>> : std::integral_constant<std::size_t, 2> {};

/// @brief kv の指定位置の型（0=キー, 1=値）
template <std::size_t I, std::size_t N, typename V>
struct tuple_element<I, frozenchars::kv<N, V>> { using type = std::conditional_t<I == 0, std::string_view, V>; };

} // namespace std
