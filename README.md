# frozenchars

文字列連結・繰り返し・数値フォーマットをなるべくコンパイル時に行う、ヘッダオンリーの小さな C++ ライブラリです。

## クイックスタート

```cpp
#include "frozenchars.hpp"
auto constexpr msg = frozenchars::concat("answer=", 42, ", hex=0x", frozenchars::Hex(255));
// msg.sv() == "answer=42, hex=0xff"
```

### 最小コンパイル例（1コマンド）

`example.cpp` を用意したら、次の1コマンドでビルド・実行できます。

```bash
g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I. example.cpp && ./a.out
```

## `repeat`（繰り返し）

`repeat<N>(...)` は、指定した回数だけ文字列を繰り返すスタンドアロン関数です。`FrozenString` と文字列リテラルの両方を受け取ります。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;
auto constexpr r1 = repeat<3>("abc"_ss);  // "abcabcabc"
auto constexpr r2 = repeat<2>("xyz");     // "xyzxyz"
```

## `right` / `center`（幅寄せ）

`repeat` と同じく、`FrozenString` と文字列リテラルの両方を受け取るスタンドアロン関数です。

- `right<Width, Fill = ' '>(...)`
- `center<Width, Fill = ' '>(...)`

`Fill` は省略時に半角スペース（`' '`）になります。
`Width` が元文字列長より小さい場合は、切り詰めず元文字列をそのまま返します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr r1 = right<8>("abc"_ss);      // "     abc"
auto constexpr r2 = right<8, '.'>("abc");    // ".....abc"

auto constexpr c1 = center<7>("abc"_ss);     // "  abc  "
auto constexpr c2 = center<8, '-'>("abc");   // "--abc---"

static_assert(r1.sv() == "     abc");
static_assert(r2.sv() == ".....abc");
static_assert(c1.sv() == "  abc  ");
static_assert(c2.sv() == "--abc---");
```

## `freeze` 対応型一覧

`freeze` はオーバーロードで入力型ごとに処理されます。主な対応は以下です。

| 分類                        | 対応型                                                                         | 例                                | `constexpr` のまま実現可否                          | 備考                                 |
| --------------------------- | ------------------------------------------------------------------------------ | --------------------------------- | --------------------------------------------------- | ------------------------------------ |
| 文字列リテラル              | `char const (&)[N]`                                                            | `freeze("hello")`            | ✅ 可                                               | `FrozenString<N>` を返します         |
| C文字列ポインタ             | `char const*`, `char*`                                                         | `freeze(ptr)`                | ⚠️ 条件付き可（ポインタ値と参照先が定数評価可能） | `nullptr` は空文字                   |
| 符号付き/なしバイトポインタ | `signed char const*`, `signed char*`, `unsigned char const*`, `unsigned char*` | `freeze(uptr)`               | ⚠️ 条件付き可（ポインタ値と参照先が定数評価可能） | 値 `0` を終端として扱います          |
| `span`                      | `std::span<char/signed char/unsigned char/std::byte>`（const 含む）            | `freeze(std::span{buf})`     | ⚠️ 条件付き可（`span` と参照先が定数評価可能）    | 値 `0` までを文字列化                |
| `array`                     | `std::array<char/signed char/unsigned char/std::byte, N>`                      | `freeze(arr)`                | ✅ 可（`constexpr std::array` の場合）              | `span` 版へ委譲                      |
| `vector`                    | `std::vector<char/signed char/unsigned char/std::byte>`                        | `freeze(vec)`                | ❌ ランタイム向け                                   | `span` 版へ委譲                      |
| 数値（2進）                 | `Bin`                                                                          | `freeze(Bin(255))`           | ✅ 可                                               | `"11111111"`                             |
| 数値（8進）                 | `Oct`                                                                          | `freeze(Oct(255))`           | ✅ 可                                               | `"377"`                             |
| 数値（10進）                | 整数型 (`Integral`)                                                            | `freeze(42)`                 | ✅ 可                                               | `"42"`                               |
| 数値（16進）                | `Hex`                                                                          | `freeze(Hex(255))`           | ✅ 可                                               | `"ff"`                               |
| 浮動小数点                  | `Precision`, `FloatingPoint`                                                   | `freeze(Precision(3.14, 2))` | ✅ 可                                               | `FloatingPoint` はデフォルト精度 `2` |
| 文字列ビュー変換可能型      | `std::is_convertible_v<T, std::string_view>` を満たす型                        | `freeze(std::string("abc"))` | ⚠️ 条件付き可（変換元が定数評価可能な場合）       | 上記の専用オーバーロードが優先       |

> 補足: `std::vector` は C++20 以降メンバ関数が `constexpr` 化されていますが、動的確保メモリを同一評価内で解放する制約があるため、実質ランタイム利用になるはずです。

### 早見表（入力型 → 生成される最大文字数）

| 入力カテゴリ                                                                                     | 返却型              | 最大文字数（終端を除く） | 終端規則                     | `constexpr` のまま実現可否                    |
| ------------------------------------------------------------------------------------------------ | ------------------- | -----------------------: | ---------------------------- | --------------------------------------------- |
| 文字列リテラル `char const (&)[N]`                                                               | `FrozenString<N>`   |                  `N - 1` | 配列長ベース（`N` から決定） | ✅ 可                                         |
| ポインタ / `span` / `array` / `vector`（`char` / `signed char` / `unsigned char` / `std::byte`） | `FrozenString<257>` |                    `256` | 先頭から `0` 値まで          | ⚠️ 条件付き可（`vector` は実質ランタイム）  |
| 整数型 (`Integral`)                                                                              | `FrozenString<21>`  |                     `20` | 数値変換結果の長さ           | ✅ 可                                         |
| `Bin`                                                                                            | `FrozenString<65>`  |                     `64` | 数値変換結果の長さ           | ✅ 可                                         |
| `Oct`                                                                                            | `FrozenString<23>`  |                     `22` | 数値変換結果の長さ           | ✅ 可                                         |
| `Hex`                                                                                            | `FrozenString<17>`  |                     `16` | 数値変換結果の長さ           | ✅ 可                                         |
| `Precision` / `FloatingPoint`                                                                    | `FrozenString<49>`  |                     `48` | 数値変換結果の長さ           | ✅ 可                                         |
| `std::string_view` 変換可能型（汎用オーバーロード）                                              | `FrozenString<257>` |                    `256` | `string_view::size()` ベース | ⚠️ 条件付き可（変換元が定数評価可能な場合） |

> 補足: 文字列系（ポインタ / `span` / `array` / `vector`）は、先頭から `0` 値までを文字列として扱います。

### サンプル入力 → 出力（ミニ表）

| 入力コード                                              | 出力（`sv()`）         | メモ                          |
| ------------------------------------------------------- | ---------------------- | ----------------------------- |
| `freeze("abc")`                                    | `"abc"`                | 文字列リテラル                |
| `freeze(Bin(255))`                                 | `"11111111"`           | 16進タグ                      |
| `freeze(Oct(255))`                                 | `"377"`                | 16進タグ                      |
| `freeze(42)`                                       | `"42"`                 | 整数（10進）                  |
| `freeze(Hex(255))`                                 | `"ff"`                 | 16進タグ                      |
| `freeze(Precision(3.14159, 2))`                    | `"3.14"`               | 精度指定                      |
| `freeze(std::string("hello"))`                     | `"hello"`              | `string_view` 変換可能型      |
| `freeze(std::array<char,5>{'a','b','\0','x','x'})` | `"ab"`                 | 0値で終端                     |
| `freeze(std::vector<char>(300, 'a'))`              | `"aaaa..."`（256文字） | 終端なしは 256 文字で切り詰め |
| `freeze(nullptr)`                                  | コンパイルエラー       |                               |

### 変換ルール（文字列系）

- 変換先は内部的に `FrozenString<257>`（最大 256 文字 + 終端）
- ポインタ/`span`/`array`/`vector` 系は **0 値（`'\0'` / `std::byte{0}`）で終端**
- 終端が無い場合は **最大 256 文字で切り詰め**

### 非対応入力

- `freeze(nullptr)` は `= delete` されておりコンパイルエラーです
- 未対応型はフォールバック `= delete` によりコンパイルエラーになります

## よくある落とし穴

- `freeze(nullptr)` は **意図的に禁止** されています。
  `nullptr` をそのまま渡すと、削除されたオーバーロードが選ばれてコンパイルエラーになります。

- `char*` / `unsigned char*` / `signed char*` / `std::byte` 系は、**先頭の 0 値まで**を文字列として扱います。
  バッファ途中に `0` があると、そこで文字列が終わります。

- `span` / `array` / `vector` 系に終端が無い場合は、**最大 256 文字で切り詰め** られます。
  257 文字目以降を必要とする用途には向きません。

- `std::string` のような `std::string_view` 変換可能型は汎用オーバーロードで処理されますが、
  より専用のオーバーロード（例: 文字列リテラル、ポインタ系）がある場合はそちらが優先されます。

## テスト

このリポジトリでは以下でビルド・テストできます。

- `build.sh`
- `test.sh`

## ライセンス

MIT License
