# PerfectMap keyed initialization design

## 背景

`frozenchars::PerfectMap` は現在、以下の2通りで値を初期化できる。

1. `PerfectMap()` による値のデフォルト構築
2. `PerfectMap(std::array<T, N>)` による宣言順の値配列からの初期化

この API だと、呼び出し側は「どの値がどのキーに対応するか」を宣言順で覚えておく必要がある。キーを明示して初期値を与えたい需要に対し、既存 API を壊さずに keyed initialization を追加する。

## 目標

- 既存の `PerfectMap()` と `PerfectMap(std::array<T, N>)` を維持する
- キーを明示しながら `PerfectMap` を初期化できるようにする
- 指定順に依存せず、全キーをちょうど1回ずつ与えれば初期化できるようにする
- 既存の compile-time perfect hash metadata と slot 配置ロジックを再利用する

## 非目標

- キーの部分指定初期化
- 宣言されていないキーの動的追加
- `PerfectMap` の lookup / iterator / storage layout の変更

## 提案 API

### 1. 既存 constructor の維持

以下は変更しない。

```cpp
constexpr PerfectMap() noexcept
  requires std::default_initializable<T>;

constexpr explicit PerfectMap(std::array<T, size()> values) noexcept(/* existing */);
```

### 2. 内部用 keyed constructor の追加

`PerfectMap` に、キー付き初期値配列を受ける constructor を追加する。

```cpp
template <typename T>
struct PerfectMapEntry {
  std::string_view key;
  T value;
};

constexpr explicit PerfectMap(std::array<PerfectMapEntry<T>, size()> entries);
```

この constructor は公開 API だが、主な役割は helper からの受け口とする。利用者が直接使ってもよい。

### 3. helper 関数の追加

利用側の主 API として、pair-like な引数列から `PerfectMap` を組み立てる helper を追加する。

```cpp
template <typename T, FrozenString... Keys, typename... EntryLike>
constexpr auto make_perfect_map(EntryLike&&... entries) -> PerfectMap<T, Keys...>;
```

受理する `EntryLike` は 2 要素の pair-like とし、以下のような呼び出しを想定する。

```cpp
using namespace frozenchars::literals;

auto map = frozenchars::make_perfect_map<int, "timeout"_fs, "retry"_fs>(
  std::pair{"retry", 5},
  std::pair{"timeout", 30}
);
```

helper は各 entry のキーを `std::string_view` へ正規化し、値を `T` へ構築して `PerfectMapEntry<T>` 配列へまとめ、keyed constructor へ渡す。

## 振る舞い

### keyed initialization の成立条件

- entry 数はキー数と一致しなければならない
- すべての宣言済みキーがちょうど1回ずつ現れなければならない
- entry の順序は任意とする

### エラー条件

以下は失敗として扱う。

- 未知キーが含まれる
- 同じキーが複数回指定される
- 宣言済みキーの一部が欠けている

失敗時は `std::invalid_argument` を送出する。`constexpr` 文脈で評価された場合、この `throw` はコンパイルエラーになる。

## 実装方針

### constructor 側

- 既存の `key_views_`, `slot_for()`, `slot_key_views_` を流用する
- `std::array<T, size()>` と `std::array<bool, size()>` をローカルに確保し、entry を1件ずつ検証しながら slot 順へ配置する
- 最後に全 slot が埋まったことを確認して `values_` を初期化する

### helper 側

- `sizeof...(EntryLike) == size()` を要求する
- `get<0>` / `get<1>` または `std::get<0>` / `std::get<1>` で pair-like を読み出せる型を対象にする
- `std::array<PerfectMapEntry<T>, size()>` を構築して keyed constructor に委譲する

### 互換性

- 既存のデフォルト構築版と宣言順 `std::array<T, N>` 版のオーバーロード解決は維持する
- 既存テストと example の利用形は壊さない

## テスト方針

以下を `test/test_perfect_map.cpp` に追加する。

1. `make_perfect_map(...)` で keyed initialization できる
2. 宣言順と異なる順序でも正しく初期化される
3. 未知キーで `std::invalid_argument`
4. 重複キーで `std::invalid_argument`
5. 不足キーで `std::invalid_argument`
6. 既存の `PerfectMap()` と `PerfectMap(std::array<T, N>)` が引き続き動作する

必要なら compile-fail ではなく runtime/`constexpr` テストで扱う。今回の不正ケースは runtime entry に依存するため、主に通常テストで検証する。
