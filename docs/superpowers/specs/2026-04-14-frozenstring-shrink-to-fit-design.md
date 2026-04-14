# FrozenString shrink_to_fit design

## Problem

`FrozenString<N>` は `buffer` と `length` を持つが、`buffer` 内の最初の `'\0'` に合わせて型サイズを詰め直す API はまだない。
今回追加する `shrink_to_fit` は、`buffer` の先頭から最初の終端文字までを表現できる最小の `FrozenString` を返す。

## Approved API

1. `frozenchars::shrink_to_fit<Str>() consteval noexcept`
2. `Str` は `FrozenString` 値を受ける class-type NTTP とする

## Constraint discovered during implementation

`FrozenString<N>` のメンバ関数や通常関数で、オブジェクト内容に応じて
`FrozenString<M>` の `M` を変える実装は、GCC/Clang/MSVC をまたいで移植的には成立しない。
`consteval` であっても `this` や関数引数由来のオブジェクト値を、そのまま型サイズ決定の
テンプレート引数へ使えないため、最小型を返す要件を守るには NTTP ベースの API が必要だった。

## Behavior

- 走査対象は `buffer[0..N)` 全体とする。
- 先頭から最初に現れる `'\0'` の位置を `fit_len` とする。
- `'\0'` が見つかった場合は、`FrozenString<fit_len + 1>` を返す。
- `'\0'` が見つからない場合は、現在の内容を表す最小サイズとし、`fit_len = length` として `FrozenString<length + 1>` を返す。
- 返り値の `length` は `fit_len` に更新し、`buffer[fit_len]` は `'\0'` とする。

## Implementation shape

- 実装本体は `template <auto Str> consteval auto shrink_to_fit() noexcept` に置く。
- `Str` には `FrozenString` 値だけを受ける制約を付ける。
- `fit_len` は `Str.buffer` を走査して求め、`FrozenString<fit_len + 1>` を直接構築する。
- 既存の `freeze(std::array/std::span)` が「最初の 0 値までを文字列とみなす」挙動なので、それと整合する。

## Tests

1. 終端が可視文字列の末尾にある通常ケースで、値が維持されること
2. `buffer` 内に埋め込み `'\0'` があるケースで、そこまでに縮むこと
3. `length` より後ろにある最初の `'\0'` も拾うこと
4. `'\0'` が存在しない場合は `length + 1` サイズになること
5. 空文字列で `FrozenString<1>` 相当の結果になること
