# PerfectMap iterator polish design

## 背景

`frozenchars::PerfectMap` はすでに compile-time perfect hash、keyed initialization、proxy iterator を備えており、range-for や structured bindings でも利用できる。

一方で、STL 風コンテナとして見るとまだ次の余地がある。

- `iterator` / `const_iterator` が concept 上の `std::forward_iterator` を明示的に保証していない
- proxy iterator に `operator->()` がなく、`it->key` / `it->value` が書けない
- `make_perfect_map(...)` の pair-like 制約はあるが、境界テストが不足している
- query 系 API に `[[nodiscard]]` がなく、戻り値を誤って捨てても気づきにくい

この段階では storage layout や perfect hash 導出は変えず、STL 風 surface を仕上げる。

## 目標

- `PerfectMap::iterator` と `PerfectMap::const_iterator` が `std::forward_iterator` を満たすようにする
- `it->key` / `it->value` を安全に使えるようにする
- `make_perfect_map(...)` の pair-like contract をテストで固定する
- query 系 API に `[[nodiscard]]` を追加し、誤用を減らす

## 非目標

- `PerfectMap` の perfect hash アルゴリズム変更
- iteration order の変更
- `first` / `second` 互換メンバの追加
- `keys()` / `values()` のような新 view API 追加

## 提案 API

### 1. iterator / const_iterator の polish

`PerfectMap::iterator` と `PerfectMap::const_iterator` に対して、forward iterator として不足している要素を埋める。

- default constructor を利用可能にする
- 必要な associated types を確認する
- `std::forward_iterator<iterator>` / `std::forward_iterator<const_iterator>` を `static_assert` で固定する

今回の変更で公開 API の名前は増やさず、既存 iterator 型をそのまま強化する。

### 2. proxy iterator の `operator->()`

現在の `operator*()` は `PerfectMapReference<T>` / `PerfectMapConstReference<T>` を返す。これに対応するため、iterator 側に proxy を保持する `operator->()` を追加する。

利用イメージ:

```cpp
for (auto it = map.begin(); it != map.end(); ++it) {
  it->value += 1;
}
```

`operator->()` は `key/value` アクセスだけを目的とし、`first/second` 互換は追加しない。

### 3. query 系 API の `[[nodiscard]]`

以下の戻り値は捨てる意味が薄いため `[[nodiscard]]` を付与する。

- `find`
- `at`
- `contains`
- `count`
- `size`
- `max_size`
- `empty`

## 実装方針

### iterator 側

- `iterator` / `const_iterator` に defaulted default constructor を与える
- `operator->()` は一時オブジェクトのアドレスを返さないよう、iterator 内に軽量 proxy holder を用意してそのポインタを返す
- mutable iterator と const iterator でそれぞれ `PerfectMapReference<T>` / `PerfectMapConstReference<T>` に対応する holder を使う
- `*it` の戻り値型は既存の proxy のまま維持し、その代わり `std::forward_iterator` が要求する `indirectly_readable` / `common_reference` 条件を満たすため、必要な `basic_common_reference` 相当の型特性を追加する
- `std::forward_iterator<iterator>` / `std::forward_iterator<const_iterator>` の `static_assert` が通ることを実装完了条件にする

### helper 制約側

- 既存の `detail::PairLikeEntry` / `detail::PerfectMapEntryLike` を維持する
- runtime test では `std::pair`, `std::tuple`, custom tuple-protocol type の成功系を確認する
- compile-fail では既存の wrong arity ケースを維持しつつ、少なくとも以下を固定する:
  - key が `std::string_view` 非変換
  - value が `T` 非構築
- compile-fail の期待診断は、wrong arity では既存の `"make_perfect_map requires exactly one entry per key"` を維持し、key/value 不適合は cross-compiler で安定する独自 `static_assert` メッセージを追加してそれを検査対象にする

### 互換性

- `*it` の戻り値型は変えない
- keyed initialization の意味論は変えない
- `it->key` / `it->value` が追加されるだけで、既存 call site は壊さない

## エラー処理

- `make_perfect_map(...)` の制約違反は引き続き compile-fail とする
- runtime での unknown key / duplicate key 検証は既存どおり `std::invalid_argument`
- `operator->()` は有効 iterator に対してのみ使う前提で、`end()` での使用防御は新設しない

## テスト方針

`test/test_perfect_map.cpp` に以下を追加する。

1. `static_assert(std::forward_iterator<...>)`
2. default-constructed iterator / const_iterator がそれぞれ同種比較可能
3. `it->key` / `it->value` が動作する
4. `std::tuple{...}` でも `make_perfect_map(...)` が通る
5. custom tuple-protocol type でも `make_perfect_map(...)` が通る

`test/compile_fail` には以下を追加する。

1. wrong arity
2. key 非変換
3. value 非構築

key/value 不適合の compile-fail では、compiler 依存の concept 診断ではなく、helper 側で明示した安定メッセージを検査する。

この spec は、以前の `docs/superpowers/specs/2026-04-24-perfect-map-stl-surface-design.md` で non-goal 扱いだった iterator polish を上書きして追加する。

最終確認では少なくとも以下を通す。

1. `cmake -S . -B build`
2. `cmake --build build --parallel 4`
3. `./build/test/all_test "[perfect_map]"`
4. `ctest --test-dir build --output-on-failure`
5. `./test.sh`
