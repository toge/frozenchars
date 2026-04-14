# FrozenString shrink_to_fit design

## Problem

`FrozenString<N>` は `buffer` と `length` を持つが、`buffer` 内の最初の `'\0'` に合わせて型サイズを詰め直す API はまだない。
今回追加する `shrink_to_fit` は、`buffer` の先頭から最初の終端文字までを表現できる最小の `FrozenString` を返す。

## Approved API

1. `FrozenString<N>::shrink_to_fit() consteval noexcept`
2. `frozenchars::shrink_to_fit(FrozenString<N> const&) consteval noexcept`
3. `frozenchars::ops::shrink_to_fit`

## Behavior

- 走査対象は `buffer[0..N)` 全体とする。
- 先頭から最初に現れる `'\0'` の位置を `fit_len` とする。
- `'\0'` が見つかった場合は、`FrozenString<fit_len + 1>` を返す。
- `'\0'` が見つからない場合は、現在の内容を表す最小サイズとし、`fit_len = length` として `FrozenString<length + 1>` を返す。
- 返り値の `length` は `fit_len` に更新し、`buffer[fit_len]` は `'\0'` とする。

## Implementation shape

- 実装の本体はメンバ関数に置く。
- 通常関数はメンバ関数への薄い委譲にする。
- pipe adaptor は `frozenchars::shrink_to_fit(str)` を呼ぶ。
- 既存の `freeze(std::array/std::span)` が「最初の 0 値までを文字列とみなす」挙動なので、それと整合する。

## Tests

1. 終端が可視文字列の末尾にある通常ケースで、値が維持されること
2. `buffer` 内に埋め込み `'\0'` があるケースで、そこまでに縮むこと
3. メンバ関数・通常関数・pipe adaptor の 3 経路が同じ結果を返すこと
4. 空文字列で `FrozenString<1>` 相当の結果になること
