# frozenchars

文字列連結・繰り返し・数値フォーマットをなるべくコンパイル時に行う、ヘッダオンリーの小さな C++ ライブラリです。

## クイックスタート

```cpp
#include "frozenchars.hpp"
auto constexpr msg = frozenchars::concat("answer=", 42, ", hex=", frozenchars::Hex(255));
// msg.sv() == "answer=42, hex=0xff"
```

### 最小コンパイル例（1コマンド）

`example.cpp` を用意したら、次の1コマンドでビルド・実行できます。

```bash
g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I. example.cpp && ./a.out
```

## `make_static` 対応型一覧

`make_static` はオーバーロードで入力型ごとに処理されます。主な対応は以下です。

| 分類                        | 対応型                                                                         | 例                                | `constexpr` のまま実現可否                          | 備考                                 |
| --------------------------- | ------------------------------------------------------------------------------ | --------------------------------- | --------------------------------------------------- | ------------------------------------ |
| 文字列リテラル              | `char const (&)[N]`                                                            | `make_static("hello")`            | ✅ 可                                               | `StaticString<N>` を返します         |
| C文字列ポインタ             | `char const*`, `char*`                                                         | `make_static(ptr)`                | ⚠️ 条件付き可（ポインタ値と参照先が定数評価可能） | `nullptr` は空文字                   |
| 符号付き/なしバイトポインタ | `signed char const*`, `signed char*`, `unsigned char const*`, `unsigned char*` | `make_static(uptr)`               | ⚠️ 条件付き可（ポインタ値と参照先が定数評価可能） | 値 `0` を終端として扱います          |
| `span`                      | `std::span<char/signed char/unsigned char/std::byte>`（const 含む）            | `make_static(std::span{buf})`     | ⚠️ 条件付き可（`span` と参照先が定数評価可能）    | 値 `0` までを文字列化                |
| `array`                     | `std::array<char/signed char/unsigned char/std::byte, N>`                      | `make_static(arr)`                | ✅ 可（`constexpr std::array` の場合）              | `span` 版へ委譲                      |
| `vector`                    | `std::vector<char/signed char/unsigned char/std::byte>`                        | `make_static(vec)`                | ❌ ランタイム向け                                   | `span` 版へ委譲                      |
| 数値（10進）                | 整数型 (`Integral`)                                                            | `make_static(42)`                 | ✅ 可                                               | `"42"`                               |
| 数値（16進）                | `Hex`                                                                          | `make_static(Hex(255))`           | ✅ 可                                               | `"0xff"`                             |
| 浮動小数点                  | `Precision`, `FloatingPoint`                                                   | `make_static(Precision(3.14, 2))` | ✅ 可                                               | `FloatingPoint` はデフォルト精度 `2` |
| 文字列ビュー変換可能型      | `std::is_convertible_v<T, std::string_view>` を満たす型                        | `make_static(std::string("abc"))` | ⚠️ 条件付き可（変換元が定数評価可能な場合）       | 上記の専用オーバーロードが優先       |

> 補足: `std::vector` は C++20 以降メンバ関数が `constexpr` 化されていますが、動的確保メモリを同一評価内で解放する制約があるため、実質ランタイム利用になるはずです。

### 早見表（入力型 → 生成される最大文字数）

| 入力カテゴリ                                                                                     | 返却型              | 最大文字数（終端を除く） | 終端規則                     | `constexpr` のまま実現可否                |
| ------------------------------------------------------------------------------------------------ | ------------------- | -----------------------: | ---------------------------- | ----------------------------------------- |
| 文字列リテラル `char const (&)[N]`                                                               | `StaticString<N>`   |                  `N - 1` | 配列長ベース（`N` から決定） | ✅ 可                                      |
| ポインタ / `span` / `array` / `vector`（`char` / `signed char` / `unsigned char` / `std::byte`） | `StaticString<257>` |                    `256` | 先頭から `0` 値まで          | ⚠️ 条件付き可（`vector` は実質ランタイム） |
| 整数型 (`Integral`)                                                                              | `StaticString<21>`  |                     `20` | 数値変換結果の長さ           | ✅ 可                                      |
| `Hex`                                                                                            | `StaticString<19>`  |                     `18` | 数値変換結果の長さ           | ✅ 可                                      |
| `Precision` / `FloatingPoint`                                                                    | `StaticString<49>`  |                     `48` | 数値変換結果の長さ           | ✅ 可                                      |
| `std::string_view` 変換可能型（汎用オーバーロード）                                              | `StaticString<257>` |                    `256` | `string_view::size()` ベース | ⚠️ 条件付き可（変換元が定数評価可能な場合） |

> 補足: 文字列系（ポインタ / `span` / `array` / `vector`）は、先頭から `0` 値までを文字列として扱います。

### サンプル入力 → 出力（ミニ表）

| 入力コード                                              | 出力（`sv()`）         | メモ                          |
| ------------------------------------------------------- | ---------------------- | ----------------------------- |
| `make_static("abc")`                                    | `"abc"`                | 文字列リテラル                |
| `make_static(42)`                                       | `"42"`                 | 整数（10進）                  |
| `make_static(Hex(255))`                                 | `"0xff"`               | 16進タグ                      |
| `make_static(Precision(3.14159, 2))`                    | `"3.14"`               | 精度指定                      |
| `make_static(std::string("hello"))`                     | `"hello"`              | `string_view` 変換可能型      |
| `make_static(std::array<char,5>{'a','b','\0','x','x'})` | `"ab"`                 | 0値で終端                     |
| `make_static(std::vector<char>(300, 'a'))`              | `"aaaa..."`（256文字） | 終端なしは 256 文字で切り詰め |
| `make_static(nullptr)`                                  | コンパイルエラー       |                               |

### 変換ルール（文字列系）

- 変換先は内部的に `StaticString<257>`（最大 256 文字 + 終端）
- ポインタ/`span`/`array`/`vector` 系は **0 値（`'\0'` / `std::byte{0}`）で終端**
- 終端が無い場合は **最大 256 文字で切り詰め**

### 非対応入力

- `make_static(nullptr)` は `= delete` されておりコンパイルエラーです
- 未対応型はフォールバック `= delete` によりコンパイルエラーになります

## よくある落とし穴

- `make_static(nullptr)` は **意図的に禁止** されています。
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

