# frozen_trie — コンパイル時圧縮トライ

## 概要

キー集合をコンパイル時に圧縮トライ（Radix Tree / Patricia Trie）に変換するルックアップ機構。
`frozen_map` の完全ハッシュ戦略と比較し、短キー・小集合・共通プレフィックスを持つキー集合で
性能上の優位性があるかを検証する。

## 背景

`frozen_map`（`map.hpp`）は以下のルックアップ戦略を持つ:

1. **≤ 64 keys**: CRC32 完全ハッシュ（シード探索）+ O(1) テーブル参照
2. **全キー長がユニーク**: 長さベースの O(1) ルックアップ
3. **> 64 keys**: 二分探索 O(log N)

ハッシュ計算（CRC32 + finalizer）はキー全体をスキャンする必要があり、キー1つにつき
数十サイクルを要する。一方、トライ木はルックアップ時にキーの必要な部分だけを走査し、
最初の数文字でミスマッチを検出できる。

## 設計

### ストレージ形式: 平坦配列

コンパイル時に全キーから圧縮トライを構築し、3つの `std::array` に格納する:

```cpp
// trie_index.hpp 内部
template <FrozenString... Keys>
struct frozen_trie_index {
  // 圧縮トライの1ノード
  struct node {
    std::uint16_t label_offset;  // k_labels へのオフセット
    std::uint8_t  label_length;  // エッジラベル長 (0 = root)
    std::uint8_t  first_child;   // k_children へのオフセット
    std::uint8_t  child_count;   // 子ノード数
    std::int8_t   value_index;   // -1 = 内部ノード, >=0 = 終端ノード
  };

  static constexpr auto k_labels   = concat_labels();   // char[]
  static constexpr auto k_nodes    = build_nodes();      // node[]
  static constexpr auto k_children = build_children();   // uint8_t[] (子インデックス)
};
```

### トライ構築 (consteval)

```cpp
// 再帰的構築: キー集合 → 圧縮トライ
consteval auto build_trie(auto keys) -> trie_data {
  // 1. 全キーの最長共通プレフィックス (LCP) を計算
  // 2. LCP を1ノードのラベルとする
  // 3. 残りの先頭文字でキーをグループ化
  // 4. 各グループを再帰的に処理
  // 5. 終端ノードに value_index を設定
}
```

構築は `consteval`（コンパイル時のみ）で行い、ランタイムには配列データのみが残る。

### ルックアップ (constexpr)

```cpp
static constexpr auto find(std::string_view key) noexcept -> std::size_t {
  auto n = k_root_index;
  auto pos = 0uz;

  while (pos < key.size()) {
    auto const& nd = k_nodes[n];
    auto label = std::string_view{k_labels.data() + nd.label_offset, nd.label_length};

    if (!key.starts_with(label, pos))
      return npos;  // 不一致

    pos += nd.label_length;

    if (pos == key.size())
      return static_cast<std::size_t>(nd.value_index);  // 完全一致（leaf）

    // 次の文字で子を探索
    auto const c = key[pos];
    auto const children_begin = k_children.data() + nd.first_child;
    auto const children_end = children_begin + nd.child_count;
    auto found = false;
    for (auto it = children_begin; it != children_end; ++it) {
      auto const& child = k_nodes[*it];
      auto const first_char = k_labels[child.label_offset];
      if (first_char == c) {
        n = *it;
        ++pos;
        found = true;
        break;
      }
    }
    if (!found) return npos;
  }
  return npos;
}
```

#### 最適化

子ノードの探索に線形スキャンを使うと子が増えた時に遅くなる。
以下の最適化を段階的に適用する:

1. **子が1つ**: スキャン不要、直接遷移
2. **子 ≤ 8**: アンローリング（コンパイラに任せる）
3. **子 > 8**: 最初の文字でハッシュ（文字→インデックスの256要素テーブル）
   - ただし、`char` の範囲は ASCII  printable に限定されるため、
     `uint8_t[256]` の小さなテーブルで済む

### 型構成

```
frozen_trie_index<Keys...>
  ┃ frozen_trie_set<Keys...>    — trie_set.hpp（set.hpp 相当のAPI）
  ┃ frozen_trie_map<T, Keys...> — trie_map.hpp（map.hpp 相当のAPI）
```

両方とも `frozen_trie_index` の `find()` を内部で使い、`frozen_set`/`frozen_map` と
可能な限り互換のインターフェースを提供する。

#### frozen_trie_set

```cpp
template <FrozenString... Keys>
class frozen_trie_set {
  static constexpr auto size()  noexcept -> std::size_t { return sizeof...(Keys); }
  static constexpr auto contains(std::string_view key) noexcept -> bool;
  static constexpr auto find(std::string_view key) noexcept -> iterator;
  static constexpr auto count(std::string_view key) noexcept -> std::size_t;
  // keys(), keys_in_declaration_order()
  // begin()/end() constexpr イテレータ
};
```

#### frozen_trie_map

```cpp
template <typename T, FrozenString... Keys>
class frozen_trie_map {
  // frozen_map と同一インターフェース
  static constexpr auto size() noexcept -> std::size_t;
  constexpr auto at(std::string_view key) -> T&;
  constexpr auto find(std::string_view key) noexcept -> iterator;
  constexpr auto contains(std::string_view key) noexcept -> bool;
  constexpr auto operator[](std::string_view key) -> T&;
  // ...
private:
  using lookup_ = frozen_trie_index<Keys...>;
  std::array<T, size()> values_{};
};
```

## ベンチマーク計画

`test/bench_frozen_trie.cpp` を新設し、Catch2 の `BENCHMARK` マクロを使用する。

### 計測パターン

| # | パターン | キー例 | キー数 | 想定 |
|---|---------|--------|--------|------|
| 1 | 短キー最小 | `{"a","b","c"}` | 3 | トライ有利 |
| 2 | 短キー小 | `{"get","put","post","delete","head"}` | 5 | トライ有利〜横並び |
| 3 | 共通prefix | `{"timeout","timeout_ms","timeout_us","timeout_ns"}` | 4 | トライ有利 |
| 4 | HTTP status | `{"status","code","message","headers","body"}` | 5 | 横並び |
| 5 | 中規模 (20) | `{"k1".."k20"}` | 20 | 横並び |
| 6 | ハッシュ閾値境界 (64) | `{"k1".."k64"}` | 64 | frozen_map 有利か |
| 7 | 欠損ルックアップ | 存在しないキーで検索 (各ケース) | - | トライ有利（早期失敗） |
| 8 | 長キー (>32 chars) | URL-like キー | 5-10 | frozen_map 有利 |

### 計測方法

各パターンで:
- `BENCHMARK("trie_map lookup hit")`: 全キーを順にルックアップ
- `BENCHMARK("frozen_map lookup hit")`: 同上
- `BENCHMARK("trie_map lookup miss")`: 存在しないキー
- `BENCHMARK("frozen_map lookup miss")`: 同上

### ファイル構成

```
test/bench_frozen_trie.cpp   — ベンチマーク本体
test/test_frozen_trie.cpp    — 正常系テスト
include/frozenchars/
  ├── trie_index.hpp         — frozen_trie_index
  ├── trie_map.hpp           — frozen_trie_map
  └── trie_set.hpp           — frozen_trie_set
```

CMakeLists.txt への追加:
```cmake
add_executable(bench_frozen_trie bench_frozen_trie.cpp)
target_link_libraries(bench_frozen_trie PRIVATE frozenchars::frozenchars)
target_compile_features(bench_frozen_trie PUBLIC cxx_std_23)
add_test(NAME bench_frozen_trie COMMAND bench_frozen_trie)
```

## 非目標

- `frozen_map` 内部へのトライ戦略統合（非依存の独立型として実装）
- ダブル配列トライ（実装コストに見合う性能差が不明）
- ワイルドカード/正規表現マッチング（純粋な完全一致ルックアップ）

## 検討した代替案と棄却理由

### ダブル配列トライ (Double-Array Trie)
理論上最も高速だが、コンパイル時構築が複雑で constexpr との相性が未知数。
最初から導入するとリスクが大きいため、平坦配列圧縮トライから始める。
ベンチマーク結果次第で将来の最適化として検討可能。

### frozen_map への統合
独立した型のほうがベンチマーク比較が明確。frozen_map のルックアップ戦略として
自動選択する方式は、両者の性能差を個別に測定しにくい。

### 各文字1ノードの単純トライ
キー数が少なければ問題ないが、長いキーでノード数が増えキャッシュミスが増加。
圧縮トライならノード数がキー数に比例するため、より優れている。
