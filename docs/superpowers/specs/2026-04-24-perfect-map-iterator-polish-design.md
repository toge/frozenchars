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
- 必要なら `size` / `max_size` / `empty` も同様に扱う

## 実装方針

### iterator 側

- `iterator` / `const_iterator` に defaulted default constructor を与える
- `operator->()` は一時オブジェクトのアドレスを返さないよう、iterator 内に軽量 proxy holder を用意してそのポインタを返す
- mutable iterator と const iterator でそれぞれ `PerfectMapReference<T>` / `PerfectMapConstReference<T>` に対応する holder を使う

### helper 制約側

- 既存の `detail::PairLikeEntry` / `detail::PerfectMapEntryLike` を維持する
- runtime test では `std::pair`, `std::tuple`, custom tuple-protocol type の成功系を確認する
- compile-fail では少なくとも以下を固定する:
  - wrong arity
  - key が `std::string_view` 非変換
  - value が `T` 非構築

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
2. default-constructed iterator / const_iterator が比較可能
3. `it->key` / `it->value` が動作する
4. `std::tuple{...}` でも `make_perfect_map(...)` が通る
5. custom tuple-protocol type でも `make_perfect_map(...)` が通る

`test/compile_fail` には以下を追加する。

1. wrong arity
2. key 非変換
3. value 非構築

最終確認では既存の full test path を維持する。
