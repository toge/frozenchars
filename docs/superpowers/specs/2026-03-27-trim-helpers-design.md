# trim helpers design

## Problem

`frozenchars` に、文字列の両端または片端から指定文字を削る `trim` / `ltrim` / `rtrim` がまだ存在しない。

今回の追加対象は次の2系統とする。

- `FrozenString<N>` を受け取るオーバーロード
- `const char*` を受け取るオーバーロード

削除対象文字はテンプレート引数で指定できるようにし、デフォルトは `' '` とする。

## Goals

- 既存のスタンドアロン関数 API に合わせる
- `trim` / `ltrim` / `rtrim` を `constexpr` で使えるようにする
- `FrozenString<N>` と `const char*` の両方で同じ使い勝手を提供する
- 空文字列、全削除、削除不要のケースを安全に扱う

## Non-goals

- 複数文字を一度に削る API
- 判定関数を受け取る汎用 predicate API
- Unicode whitespace や locale-aware な trim
- `std::string` や `std::string_view` 専用オーバーロードの追加

## Chosen approach

採用案は、単一文字をテンプレート引数で指定する API である。

```cpp
template <char TrimChar = ' ', size_t N>
constexpr auto trim(FrozenString<N> const& str) noexcept;

template <char TrimChar = ' ', size_t N>
constexpr auto ltrim(FrozenString<N> const& str) noexcept;

template <char TrimChar = ' ', size_t N>
constexpr auto rtrim(FrozenString<N> const& str) noexcept;

template <char TrimChar = ' ', size_t N>
constexpr auto trim(char const (&str)[N]) noexcept;

template <char TrimChar = ' ', size_t N>
constexpr auto ltrim(char const (&str)[N]) noexcept;

template <char TrimChar = ' ', size_t N>
constexpr auto rtrim(char const (&str)[N]) noexcept;

template <char TrimChar = ' '>
constexpr auto trim(char const* str) noexcept;

template <char TrimChar = ' '>
constexpr auto ltrim(char const* str) noexcept;

template <char TrimChar = ' '>
constexpr auto rtrim(char const* str) noexcept;
```

## API behavior

### `FrozenString<N>` overloads

- 戻り値は `FrozenString<N>` とする
- 文字列内容だけを詰め直し、容量は維持する
- `trim` は両端、`ltrim` は左端、`rtrim` は右端だけを削る
- 途中にある `TrimChar` は保持する
- すべての文字が `TrimChar` の場合は空文字列を返す

### literal overloads

- 既存の `toupper` / `tolower` / `substr` と同様に、`FrozenString{str}` へ委譲する
- 戻り値は `FrozenString<N>` になる

### `const char*` overloads

- 既存 `freeze(char const*)` と同じ方針で最大 256 文字まで読む
- 戻り値は `FrozenString<257>` とする
- `nullptr` は空文字列として扱う
- 実装は `freeze(str)` に委譲してから trim する

## Implementation details

- 先頭インデックス `start` と末尾インデックス `end` を走査して削る範囲を決める
- `ltrim` は先頭側だけ、`rtrim` は末尾側だけ、`trim` は両方を使う
- 切り出した範囲を新しい `FrozenString` の先頭へコピーし、`length` と終端 `'\0'` を更新する
- 長さ 0 の場合も常に `buffer[0] = '\0'` を保証する
- 共通化は `detail` 名前空間に小さな helper を置いて重複を避ける

## Edge cases

- `""` -> 常に `""`
- `"   "` に対する `trim<>` -> `""`
- `"   a"` に対する `ltrim<>` -> `"a"`
- `"a   "` に対する `rtrim<>` -> `"a"`
- `" a b "` に対する `trim<>` -> `"a b"`
- `trim<'-'>("---a-b---")` -> `"a-b"`
- `const char*` が `nullptr` -> `""`

## Testing strategy

`test/test_simple.cpp` に compile-time 寄りのケース、`test/test_runtime.cpp` に `const char*` runtime ケースを追加する。

最低限含めるべき確認は次のとおり。

- `FrozenString` 入力での `trim` / `ltrim` / `rtrim`
- 文字列リテラル入力での `trim` / `ltrim` / `rtrim`
- デフォルト `' '` の動作
- カスタム文字例として `'-'`
- 空文字列
- 全文字が trim 対象のケース
- `const char*` 入力
- `const char* == nullptr`

## Why this design

この設計は、既存の `right`, `center`, `toupper`, `tolower`, `substr` と同じ「`FrozenString` 本体 + 文字列リテラル委譲 + 必要に応じた pointer overload」という流れに揃う。

また、今回の要件は「空白とみなす文字」をテンプレート引数で指定したい、という単一文字前提のため、predicate 化や複数文字対応を先回りで入れないほうが API が素直で実装も追いやすい。
