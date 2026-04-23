# StaticPerfectMap design

## Problem

`frozenchars::FrozenString` をコンパイル時固定キーとして扱い、値だけを実行時に変更できる
STL 風コンテナを追加したい。
要求される中心仕様は次のとおり。

1. キー集合はテンプレート引数 `FrozenString... Keys` で固定する
2. 完全ハッシュの seed はコンパイル時に導出する
3. ルックアップは `std::string_view` を受けて `O(1)` で行う
4. 値は実行時に更新可能にする

## Constraints and decisions

1. 新規公開ヘッダは `include/frozenchars/static_perfect_map.hpp` とし、`frozenchars.hpp` からは再 export しない
2. クラスは `template <typename T, FrozenString... Keys> class StaticPerfectMap` とする
3. 完全ハッシュは FNV-1a ベースで計算し、`consteval` な seed 探索で `hash(key, seed) % N` の衝突がない seed を求める
4. seed が見つからない場合、またはキーが重複している場合はコンパイルエラーにする
5. `operator[](std::string_view)` の未存在キー時は `std::out_of_range` を送出する
6. 値格納順は宣言順ではなく「完全ハッシュ後のスロット順」とする
7. 値の初期化はデフォルト構築と `std::array<T, N>` 受け取りの両方を提供する

## Approved API

```cpp
template <typename T, FrozenString... Keys>
class StaticPerfectMap {
 public:
  static constexpr auto size() noexcept -> std::size_t;

  constexpr StaticPerfectMap() noexcept(/* T に従う */);
  constexpr explicit StaticPerfectMap(std::array<T, size()> values) noexcept(/* T に従う */);

  constexpr auto contains(std::string_view key) const noexcept -> bool;

  constexpr auto at(std::string_view key) noexcept
      -> std::optional<std::reference_wrapper<T>>;
  constexpr auto at(std::string_view key) const noexcept
      -> std::optional<std::reference_wrapper<T const>>;

  constexpr auto operator[](std::string_view key) -> T&;
  constexpr auto operator[](std::string_view key) const -> T const&;
};
```

## Compile-time design

### Key metadata

- `Keys...` から固定キー配列を型側の `static constexpr` メタ情報として生成する
- 比較用に `std::array<std::string_view, N>` を持つ
- 実ランタイム状態にはキーを持たず、`values_` だけを保持する

### Hash function

- FNV-1a を `constexpr` / `consteval` で実装する
- seed は offset basis への加算または XOR のような単純合成で注入する
- コンパイル時探索では `hash(key, seed) % N` が全キーで一意になる seed を総当たりする

### Validation

- 先に全キー同士の文字列比較で重複キーを検出する
- 次に seed を探索する
- どちらかに失敗した場合は `static_assert` または即時関数内の失敗でコンパイルを止める

## Runtime lookup design

### Storage

- 内部状態は `std::array<T, N> values_` のみ
- `values_[slot]` が、その slot に割り当てられたキーの値を持つ

### Lookup flow

1. `std::string_view` から FNV-1a を計算する
2. `slot = hash(key, seed) % N` を求める
3. その slot に対応する固定キーと `std::string_view` を比較する
4. 一致した場合のみ成功とし、`[[likely]]` を付けた成功分岐で値へ到達する
5. 一致しない場合は未存在キーとして扱う

完全ハッシュは固定キー集合に対してのみ成立するため、未知キーに対しては
「同じ slot に落ちたが一致しない」経路を必ず残す。

## Error handling

- `contains()` は失敗時に `false`
- `at()` は失敗時に空 `std::optional`
- `operator[]` は失敗時に `std::out_of_range`
- 例外メッセージはキーが見つからない旨を含める

## Example and coding requirements

- 利用例は `"timeout"_fs` と `"retry"_fs` を使う `main` 関数を用意する
- サンプル出力には `std::print` を使う
- C++26 の placeholder variables を利用できる箇所では `_` を使う
- Doxygen 日本語コメント、2 スペース、snake_case を守る
- `constexpr` / `noexcept` / `const` は可能な限り付与する

## Tests

1. 基本アクセス: `contains` / `at` / `operator[]` が既知キーで成功する
2. 未存在キー: `contains == false`、`at()` は空、`operator[]` は例外を送出する
3. 値更新: `operator[]` 経由の更新が反映される
4. 初期化: デフォルト構築と `std::array` コンストラクタの両方が動く
5. compile-time guarantees: 重複キーと seed 未発見ケースを個別に検証できる形へ設計する

## Non-goals

- 宣言順イテレーションの保証
- `std::unordered_map` 互換の完全 API
- ランタイムでのキー追加・削除
