# FrozenString formatter / parse_number design

## Problem

`FrozenString` は `std::format` / `std::print` に直接渡せず、数値変換は `split_numbers` の内部実装に閉じています。今回の変更では、既存の `FrozenString` API と constexpr 指向を維持しつつ、次の 2 点を追加する。

1. `std::formatter<FrozenString<N>, char>` を追加し、`FrozenString` を `std::format` / `std::print` に直接渡せるようにする。
2. `parse_number<T>(...)` を追加し、`FrozenString` または文字列リテラルから単一数値を constexpr に変換できるようにする。

## Context from current code

- `FrozenString` は `.sv()` で `std::string_view` を返す軽量な固定長文字列型。
- `split_numbers` は `detail::parse_int_token` に整数・浮動小数点の解析を集約している。
- 既存パーサーは空白を数値トークンとしては受理しない。`split_numbers` は先に分割し、各トークンを厳密に解析する。

## Considered approaches

### 1. Shared numeric parser in `detail` (**recommended**)

`split_numbers` の内部パーサーを単一トークン API として整理し、`parse_number<T>` と `split_numbers` の両方から使う。

- **Pros:** 仕様の一貫性が高い。エラー条件・境界判定を一箇所で管理できる。
- **Cons:** 既存 `detail::parse_int_token` の責務整理が必要。

### 2. Separate `parse_number<T>` implementation

`parse_number<T>` を独立実装し、`split_numbers` は現状維持にする。

- **Pros:** 既存コードへの影響が狭い。
- **Cons:** 数値仕様が分岐しやすく、将来の修正でずれやすい。

### 3. Wrap `split_numbers`

単一トークン入力を `split_numbers` 経由で処理する。

- **Pros:** 差分が小さい。
- **Cons:** API と内部責務が不自然で、基数自動検出や厳密なエラー報告を拡張しにくい。

## Chosen design

### `std::formatter<FrozenString<N>, char>`

- `std::formatter<FrozenString<N>, char>` を `frozenchars.hpp` 内で提供する。
- 実装は `std::formatter<std::string_view, char>` を内部メンバとして保持し、`parse` / `format` の両方をそのまま委譲する。
- `format` では `FrozenString::sv()` を渡すだけにして、幅・位置揃え・精度・debug などの文字列 spec は標準実装に任せる。
- これにより `std::format("[{}]", "tag"_fs)` と `std::print("{:>12}\n", "tag"_fs)` がそのまま使える。

### `parse_number<T>(...)`

- 公開 API は `parse_number<T>(FrozenString<N> const&)` と `parse_number<T>(char const (&)[N])` を追加する。
- `T` は `int`, `long`, `long long`, `unsigned` 系, `float`, `double` を含む既存 `Numeric` 制約に従う。`bool` は非対応のままにする。
- 内部実装は `detail` に共通数値パーサーを用意し、`split_numbers` もそれを使うように整理する。
- 整数だけ基数自動検出を追加する。
  - `0x` / `0X` -> 16 進
  - `0b` / `0B` -> 2 進
  - 先頭 `0` -> 8 進
  - それ以外 -> 10 進
- 浮動小数点は 10 進のみを受理し、整数向け接頭辞は無効入力として扱う。
- 先頭末尾の空白は許容しない。

### `consteval` / `constexpr` split

- 実行時例外要件があるため、最終的に例外を送出する数値パース本体は `constexpr` とする。
- ただし、以下のような入力前処理・補助処理は可能な範囲で `consteval` に寄せる。
  - 基数判定
  - 接頭辞の除去位置計算
  - 桁値変換
  - 文字列リテラルから `FrozenString` へのフォワード
- これにより、定数式では最大限コンパイル時評価され、実行時では例外ベースの失敗通知を維持できる。

## Error behavior

- 非数値・不正接頭辞・不正な指数などは `std::invalid_argument`
- 範囲外は `std::out_of_range`
- 定数式評価中は `throw` によってコンパイルエラーになる
- unsigned 型への負値は `std::out_of_range`

## Refactoring boundaries

- `split_numbers` の公開シグネチャや返り値形状は変えない。
- 変更対象は数値解析の内部整理と、新しい単一値 API の追加に限定する。
- 既存の `parse_hex_*` や `parse_to_tuple` など他のパーサーには触れない。

## Test plan

### Formatter tests

- `std::format("[{}] started", "MyApp"_fs)` が期待文字列になる
- `std::format("{:>12}", "MyApp"_fs)` で幅指定が `std::string_view` と同じ結果になる
- `std::print` でも `FrozenString` を直接渡せることを確認する

### `parse_number` success cases

- 10 進整数: `42`, `-100`
- 16 進整数: `0xff`
- 2 進整数: `0b1010`
- 8 進整数: `077`
- 浮動小数点: `3.14`, `-1.5`, `1e2`, `-2.5e1`
- 境界値: `INT_MAX`, `INT_MIN`, `LONG_LONG_MAX`, `ULLONG_MAX`

### `parse_number` failure cases

- 実行時に `abc`, `0x`, `0b102`, `08`, `1.0.0`, `1e`, `-1` to unsigned で期待例外
- 範囲外整数・範囲外浮動小数点で `std::out_of_range`

## Implementation notes for planning

- `std::formatter` 特殊化は `namespace std` に置く必要がある。
- `<format>` と `<print>` を使うテストは現在のツールチェインで有効化し、標準ライブラリ実装依存の差分が出ないか確認する。
- 数値パーサーの共通化では、`split_numbers` が現在期待している厳密パース挙動を崩さないことを優先する。
