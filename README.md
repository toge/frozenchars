# frozenchars

文字列連結・繰り返し・数値フォーマットをなるべくコンパイル時に行うための型とヘルパー関数を提供する、ヘッダオンリーの C++ ライブラリです。

## 特徴

- 内部ではstd::arrayで固定長バッファを持つ `FrozenString` クラスを定義し、文字列リテラルや数値などからコンパイル時に文字列を生成できます。
- 操作関数のほとんどをconstevalで定義しており、コンパイル時に評価されます。
- 文字列操作をチェーンできるパイプ演算子も提供しています。
- コンパイル時の文字列処理を前提にした応用機能もいくつか用意しています。
- HTML エンティティ変換、ワードラップ、UTF-8 コードポイント数計算などの実用関数も提供しています。

## 目次

- [特徴](#特徴)
 - [クイックスタート](#クイックスタト)
   - [最小コンパイル例（1コマンド）](#最小コンパイル例（1コマンド）)
 - [モジュール別インクルード（推奨）](#モジュル別インクルド推奨)
- [基本概念](#基本概念)
  - [FrozenString<N>](#frozenstringn)
  - [consteval（常にコンパイル時）](#consteval（常にコンパイル時）)
  - [_fs リテラル](#_fs-リテラル)
  - [パイプ演算子](#パイプ演算子)
- [要件](#要件)
- [サンプルコード](#サンプルコド)
- [repeat（繰り返し）](#repeat（繰り返し）)
- [right / center（幅寄せ）](#right-center（幅寄せ）)
- [toupper / tolower（大文字・小文字変換）](#toupper-tolower（大文字・小文字変換）)
- [trim / ltrim / rtrim（端の文字を削る）](#trim-ltrim-rtrim（端の文字を削る）)
- [substr（部分文字列）](#substr（部分文字列）)
- [contains（部分文字列の有無判定）](#contains（部分文字列の有無判定）)
- [starts_with（接頭辞チェック）](#starts_with（接頭辞チェック）)
- [ends_with（接尾辞チェック）](#ends_with（接尾辞チェック）)
- [partition（区切り文字で3分割）](#partition（区切り文字で3分割）)
- [shrink_to_fit（最初の終端位置に合わせて縮小）](#shrink_to_fit（最初の終端位置に合わせて縮小）)
- [split（区切りで分割）](#split（区切りで分割）)
- [split_numbers（区切りで数値列へ変換）](#split_numbers（区切りで数値列へ変換）)
- [capitalize（先頭大文字化）](#capitalize（先頭大文字化）)
- [to_snake_case（スネークケース変換）](#to_snake_case（スネクケス変換）)
- [to_camel_case（キャメルケース変換）](#to_camel_case（キャメルケス変換）)
- [to_pascal_case（パスカルケース変換）](#to_pascal_case（パスカルケス変換）)
- [url_encode（URLエンコード）](#url_encode（urlエンコド）)
- [url_decode（URLデコード）](#url_decode（urlデコド）)
- [base64_encode（Base64エンコード）](#base64_encode（base64エンコド）)
- [base64_decode（Base64デコード）](#base64_decode（base64デコド）)
- [html_encode / html_decode（HTMLエンティティ変換）](#html_encode-html_decode（htmlエンティティ変換）)
- [minify_html / minify_xml / minify_json / minify_yaml / minify_sql / minify_cypher（マークアップ / データ / クエリ 縮小）](#minify_html-minify_xml-minify_json-minify_yaml-minify_sql-minify_cypher（マクアップ-デタ-クエリ-縮小）)
- [linebreak（改行表現の相互変換）](#linebreak（改行表現の相互変換）)
- [word_wrap（ワードラップ）](#word_wrap（ワドラップ）)
- [文字種判定述語（is_alpha / is_digit / ...）](#文字種判定述語（is_alpha-is_digit-）)
- [utf8_length（UTF-8 コードポイント数）](#utf8_length（utf-8-コドポイント数）)
- [make_querystring（クエリ文字列生成）](#make_querystring（クエリ文字列生成）)
- [マルチライン文字列の処理](#マルチライン文字列の処理)
- [パイプ演算子で文字列ヘルパーをつなぐ](#パイプ演算子で文字列ヘルパをつなぐ)
- [frozen_format（コンパイル時フォーマット）](#frozen_format（コンパイル時フォーマット）)
- [parse_hex_rgb / parse_hex_rgba（hex color → RGB/RGBAタプル）](#parse_hex_rgb-parse_hex_rgba（hex-color-→-rgbrgbaタプル）)
- [freeze 対応型一覧](#freeze-対応型一覧)
  - [早見表（入力型 → 生成される最大文字数）](#早見表（入力型-→-生成される最大文字数）)
  - [サンプル入力 → 出力（ミニ表）](#サンプル入力-→-出力（ミニ表）)
  - [変換ルール（文字列系）](#変換ルル（文字列系）)
  - [非対応入力](#非対応入力)
- [よくある落とし穴](#よくある落とし穴)
- [FrozenStringの応用例](#frozenstringの応用例)
  - [to_sv（std::format 連携）](#to_sv（stdformat-連携）)
  - [frozen_map（固定キーの軽量マップ）](#frozen_map（固定キの軽量マップ）)
    - [宣言順の値を braced-init-list で書く](#宣言順の値を-braced-init-list-で書く)
    - [pair-like エントリから作る基本形](#pair-like-エントリから作る基本形)
    - [to<Result>() で STL コンテナへ変換する](#toresult-で-stl-コンテナへ変換する)
    - [コンパイル時のキー/値列から作る短縮形](#コンパイル時のキ値列から作る短縮形)
    - [パフォーマンスと最適化](#パフォマンスと最適化)
    - [get_value_or — デフォルト付き取得](#get_value_or-—-デフォルト付き取得)
    - [contains_all — 複数キーの一括存在判定](#contains_all-—-複数キの一括存在判定)
    - [keys_in_declaration_order — 宣言順キー配列](#keys_in_declaration_order-—-宣言順キ配列)
    - [operator== / operator!= — 値ごとの等価比較](#operator-operator-—-値ごとの等価比較)
  - [frozen_trie_map（圧縮トライ マップ）](#frozen_trie_map（圧縮トライ-マップ）)
  - [frozen_map / frozen_trie_map / STL コンテナの選び方](#frozen_map--frozen_trie_map--stl-コンテナの選び方)
  - [parse_to_tuple（型列文字列 → std::tuple<...>）](#parse_to_tuple（型列文字列-→-stdtuple）)
    - [parse_to_variant（型列文字列 → std::variant<...>）](#parse_to_variant（型列文字列-→-stdvariant）)
    - [型エイリアス](#型エイリアス)
    - [ポインタ/参照型](#ポインタ参照型)
- [よくある質問 (FAQ)](#よくある質問-faq)
- [wildcard_match（ワイルドカードマッチング）](#wildcard_match（ワイルドカドマッチング）)
  - [frozen_regex / CTRE / wildcard_match / wildcards の選び方](#frozen_regex--ctre--wildcard_match--wildcards-の選び方)
- [frozen_set（コンパイル時集合）](#frozen_set（コンパイル時集合）)
- [remove_comments / remove_comment_lines（コメント行除去）](#remove_comments-remove_comment_lines（コメント行除去）)
- [chrono（日付・時刻のコンパイル時相互変換）](#chrono（日付時刻のコンパイル時相互変換）)
- [テスト](#テスト)
- [インストール / パッケージ生成](#インストル-パッケジ生成)
- [ライセンス](#ライセンス)

## クイックスタート

```cpp
#include "frozenchars.hpp"

auto constexpr msg = frozenchars::concat("answer=", 42, ", hex=0x", frozenchars::Hex(255));
// msg.sv() == "answer=42, hex=0xff"
```

> **注意**: `frozenchars.hpp` は非推奨です。コンパイル負荷削減のため、個別のモジュールヘッダ（`frozenchars/mod/core.hpp` 等）の利用を推奨します。詳しくは[モジュール別インクルード（推奨）](#モジュール別インクルード推奨)を参照してください。非推奨メッセージを抑制したい場合は `FROZENCHARS_USE_UMBRELLA` を定義してください。

### 最小コンパイル例（1コマンド）

`example/example.cpp` を用意したら、次の1コマンドでビルド・実行できます。

```bash
g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include example/example.cpp && ./a.out
```

> **凡例**: 以下の各節のコード例では、`#include "frozenchars.hpp"` と
> `using namespace frozenchars;` `using namespace frozenchars::literals;` を
> あらかじめ記述したものとします。

## モジュール別インクルード（推奨）

`frozenchars.hpp` は glaze / json を除く全機能をまとめた便利ヘッダですが、
必要のない機能（正規表現・SIMD・コンテナ等）までコンパイル時に引き込むため、
コンパイル負荷を抑えたい場合は**個別のモジュールヘッダ**を利用してください。

### 移行ガイド

既存コードで `frozenchars.hpp` を使用している場合、以下の手順でモジュール別インクルードに移行できます：

1. **使用している機能を特定**: コード内で使用している関数・型を確認
2. **対応するモジュールを選択**: 下記の表から必要なモジュールを選択
3. **インクルードを置換**: `#include "frozenchars.hpp"` を必要なモジュールヘッダに置き換え
4. **ビルド確認**: 必要なヘッダがすべて含まれているか確認

**例: 文字列操作とコンテナのみを使用している場合**
```cpp
// 以前
#include "frozenchars.hpp"

// 移行後
#include "frozenchars/mod/core.hpp"
#include "frozenchars/mod/algorithms.hpp"
#include "frozenchars/mod/containers.hpp"
```

モジュールヘッダは `include/frozenchars/mod/` にあり、いずれも `-I include` のもと
`#include "frozenchars/mod/<名>.hpp"` で利用できます。各モジュールは自己完結しており、
重複 include は `#pragma once` により安全です。

| モジュール | 含まれる主な機能 |
|---|---|
| `frozenchars/mod/core.hpp` | `FrozenString` 型・`_fs` リテラル・`freeze`・数値変換・`std::formatter` |
| `frozenchars/mod/algorithms.hpp` | `concat` 等の基本演算・`case_conv`・`split`・`multiline`・`path`・`minify`・`type_parser` |
| `frozenchars/mod/encoding.hpp` | `url_encode`・`make_querystring` |
| `frozenchars/mod/containers.hpp` | `frozen_map`・`frozen_set`・`trie_*` |
| `frozenchars/mod/regex.hpp` | `frozen_regex`・`wildcard_match`・CTRE 連携 |
| `frozenchars/mod/formatting.hpp` | `frozen_format` |
| `frozenchars/mod/chrono.hpp` | 日付・時刻の相互変換 |
| `frozenchars/mod/color.hpp` | ANSI カラー出力 |
| `frozenchars/mod/ops.hpp` | `ops` 名前空間のパイプ演算子アダプタ |
| `frozenchars/mod/all_basic.hpp` | 上記 9 個を集約（glaze / json を除く） |

```cpp
// 例: 文字列操作だけ使いたい場合
#include "frozenchars/mod/algorithms.hpp"
#include "frozenchars/mod/core.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr s = concat("a="_fs, 1);
```

### オプション統合（glaze / json）はオプトイン

glaze 連携と JSON 圧縮はデフォルトでは読み込まれません。必要な場合は個別に
インクルードしてください（該当ヘッダが依存ライブラリを引くため、使わない場合は
コンパイルから除外されます）。

```cpp
#include "frozenchars/glaze_frozen_map.hpp"   // glaze 連携（FROZENCHARS_HAS_GLAZE ガード）
#include "frozenchars/json/compress.hpp"      // JSON 圧縮
#include "frozenchars/json/crush.hpp"         // JSON クラッシュ圧縮
```

### 傘ヘッダの非推奨化

`#include "frozenchars.hpp"` は後方互換のため残されていますが非推奨です。
インクルード時に次のメッセージが出力されます。

```
frozenchars.hpp is deprecated; prefer granular includes like frozenchars/mod/core.hpp
```

CI や無言ビルドでメッセージを抑制したい場合は、コンパイル時に
`FROZENCHARS_USE_UMBRELLA` を定義してください。

```bash
g++ -std=c++23 -DFROZENCHARS_USE_UMBRELLA -I include main.cpp
```

## 基本概念

### `FrozenString<N>`

全機能の核となる型です。`std::array<char, N>` を内部バッファに持ち、
末尾に `'\0'` を保証する固定長文字列です。
テンプレートパラメータ `N` は **終端の '\0' を含めたバッファ長** です。

- `"abc"_fs` の型は `FrozenString<4>`（`'a','b','c','\0'`）
- 連結結果のサイズはコンパイル時に確定するため、動的メモリ確保が起きません

### `consteval`（常にコンパイル時）

このライブラリの操作関数のほとんどは C++23 の `consteval` で定義されています。
`consteval` 関数は必ずコンパイル時に評価されるため、
ランタイムでのオーバーヘッドがゼロであることが保証されます。
実行時の文字列（`std::string` など）を引数に取ることはできません。

### `_fs` リテラル

文字列リテラルから `FrozenString` を生成するユーザ定義リテラルです。
`frozenchars::literals` 名前空間で定義されています。

```cpp
auto constexpr hello = "Hello"_fs;  // FrozenString<6>
// hello.sv() == "Hello"
```

`.sv()` メンバ関数で `std::string_view` を取り出せます。

### パイプ演算子

`frozenchars::ops` 名前空間の関数と `operator|` を使うと、
文字列操作をチェーンして簡潔に書けます。

```cpp
using namespace frozenchars::ops;
auto constexpr r = "  Hello, World!  "_fs
    | trim()
    | toupper()
    | right<20>();
// r.sv() == "       HELLO, WORLD!"
```

左辺は `FrozenString` だけでなく `const char[N]` 文字列リテラルも直接受け付けます
（`_fs` リテラルは省略可能）。

詳細は「[パイプ演算子で文字列ヘルパーをつなぐ](#パイプ演算子で文字列ヘルパをつなぐ)」を参照してください。

## 要件

- C++23（`consteval`, NTTP, `std::to_chars` など）
- 対応コンパイラ: GCC 14 / 15 / 16 以降
- テスト環境: Fedora 41 / 43 / 44（CI で検証）
- ヘッダオンリー — ビルド済みバイナリ不要

## サンプルコード

リポジトリには以下のサンプルファイルが用意されています。まずは `example/example.cpp` を動かしてみてください。

| ファイル | 内容 | 実行コマンド |
|---|---|---|
| `example/example.cpp` | 連結・繰り返し・書式など基本操作 | `g++ -std=c++23 -I include example/example.cpp && ./a.out` |
| `example/example_split.cpp` | 文字列分割の応用 | `g++ -std=c++23 -I include example/example_split.cpp && ./a.out` |
| `example/example_frozen_map_use_cases.cpp` | コンパイル時マップの利用例 | `g++ -std=c++23 -I include example/example_frozen_map_use_cases.cpp && ./a.out` |
| `example/example_parse_to_tuple_use_cases.cpp` | 型列パース→タプルの利用例 | `g++ -std=c++23 -I include example/example_parse_to_tuple_use_cases.cpp && ./a.out` |

## `repeat`（繰り返し）

`repeat<N>(...)` は、指定した回数だけ文字列を繰り返すスタンドアロン関数です。`FrozenString` と文字列リテラルの両方を受け取ります。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr r1 = repeat<3>("abc"_fs);  // "abcabcabc"
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

auto constexpr r1 = right<8>("abc"_fs);      // "     abc"
auto constexpr r2 = right<8, '.'>("abc");    // ".....abc"

auto constexpr c1 = center<7>("abc"_fs);     // "  abc  "
auto constexpr c2 = center<8, '-'>("abc");   // "--abc---"

static_assert(r1.sv() == "     abc");
static_assert(r2.sv() == ".....abc");
static_assert(c1.sv() == "  abc  ");
static_assert(c2.sv() == "--abc---");
```

## `toupper` / `tolower`（大文字・小文字変換）

ASCII の英字を大文字／小文字に変換するスタンドアロン関数です。`FrozenString` と文字列リテラルの両方を受け取ります。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr u = toupper("Hello, World!"_fs);  // "HELLO, WORLD!"
auto constexpr l = tolower("Hello, World!");      // "hello, world!"

static_assert(u.sv() == "HELLO, WORLD!");
static_assert(l.sv() == "hello, world!");
```

## `trim` / `ltrim` / `rtrim`（端の文字を削る）

指定文字を文字列の両端・左端・右端から削るスタンドアロン関数です。`FrozenString`、文字列リテラル、`const char*` を受け取ります。

- `trim<TrimChar = ' '>(...)`
- `ltrim<TrimChar = ' '>(...)`
- `rtrim<TrimChar = ' '>(...)`

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr s1 = trim("  hello  "_fs);      // "hello"
auto constexpr s2 = ltrim<'-'>("---hello");    // "hello"
auto constexpr s3 = rtrim("hello   ");         // "hello"

static_assert(s1.sv() == "hello");
static_assert(s2.sv() == "hello");
static_assert(s3.sv() == "hello");
```

## `substr`（部分文字列）

`substr<Pos, Len>(...)` は部分文字列を取り出します。`FrozenString` と文字列リテラルの両方を受け取ります。

- `Pos >= length` の場合は空文字列を返します
- `Len > 0` の場合は、位置 `Pos` から右へ最大 `Len` 文字を返します
- `Len < 0` の場合は、位置 `Pos` の直前から左へ最大 `abs(Len)` 文字を返します
- 取り出し範囲が文字列長をはみ出す場合は、利用可能な範囲に切り詰めます

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr s1 = substr<7, 5>("Hello, World!"_fs);  // "World"
auto constexpr s2 = substr<0, 5>("Hello, World!");     // "Hello"
auto constexpr s3 = substr<5, -5>("Hello, World!");    // "Hello"

static_assert(s1.sv() == "World");
static_assert(s2.sv() == "Hello");
static_assert(s3.sv() == "Hello");
```

## `contains`（部分文字列の有無判定）

`contains<Substr>(str)` は文字列が部分文字列を含むかを判定します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

// パイプ演算子版
static_assert("hello world"_fs | ops::contains<"world">);
static_assert(!("hello"_fs | ops::contains<"xyz">));

// 直接呼び出し版
constexpr auto str = "hello world"_fs;
static_assert(contains<"world"_fs>(str));

// char[] 版
static_assert(contains<"world">("hello world"));
```

## `starts_with`（接頭辞チェック）

`starts_with<Prefix>(str)` は文字列が指定した接頭辞で始まるかを判定します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

static_assert("hello"_fs | ops::starts_with<"hel">);
static_assert(!("hello"_fs | ops::starts_with<"llo">));
static_assert("abc"_fs | ops::starts_with<"">);  // 空プレフィックスは常に true
```

## `ends_with`（接尾辞チェック）

`ends_with<Suffix>(str)` は文字列が指定した接尾辞で終わるかを判定します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

static_assert("hello"_fs | ops::ends_with<"llo">);
static_assert(!("hello"_fs | ops::ends_with<"hel">));
static_assert("abc"_fs | ops::ends_with<"">);  // 空サフィックスは常に true
```

## `find`（前方検索）

`find<Sub>(str)` は文字列中で部分文字列が最初に出現する位置を返します。
見つからない場合は `std::string_view::npos` を返します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

static_assert(("hello world"_fs | ops::find<"world">) == 6);
static_assert(("hello world"_fs | ops::find<"xyz">) == std::string_view::npos);
static_assert(("abc"_fs | ops::find<"abc">) == 0);
```

## `rfind`（後方検索）

`rfind<Sub>(str)` は文字列中で部分文字列が最後に出現する位置を返します。
見つからない場合は `std::string_view::npos` を返します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

static_assert(("hello world"_fs | ops::rfind<"o">) == 7);
static_assert(("hello world"_fs | ops::rfind<"xyz">) == std::string_view::npos);
static_assert(("abcabc"_fs | ops::rfind<"abc">) == 3);
```

## `find_first_of` / `find_last_of`（文字集合検索）

`find_first_of<Chars>(str)` は文字集合に含まれる文字が最初に出現する位置を、
`find_last_of<Chars>(str)` は最後に出現する位置を返します。
見つからない場合は `std::string_view::npos` を返します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

static_assert(("hello world"_fs | ops::find_first_of<"hw">) == 0);
static_assert(("hello world"_fs | ops::find_first_of<"xyz">) == std::string_view::npos);
static_assert(("hello world"_fs | ops::find_last_of<"ld">) == 10);
```

## `reverse`（文字列反転）

`reverse(str)` は文字列を左右反転した新しい `FrozenString` を返します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

static_assert(("abc"_fs | ops::reverse) == "cba"_fs);
static_assert((""_fs | ops::reverse) == ""_fs);
static_assert(("hello"_fs | ops::reverse) == "olleh"_fs);
```

## `count_substring`（部分文字列出現回数）

`count_substring<Sub>(str)` は文字列中に部分文字列が重なりなしで出現する回数を返します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

static_assert(("hello hello"_fs | ops::count_substring<"hello">) == 2);
static_assert(("aaaa"_fs | ops::count_substring<"aa">) == 2);  // 重なりなし
static_assert(("abc"_fs | ops::count_substring<"xyz">) == 0);
```

## `partition`（区切り文字で3分割）

`partition<Delim>(str)` は文字列を区切り文字で3分割し、`std::tuple` を返します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

// パイプ演算子版
constexpr auto [before, delim, after] = "key=value"_fs | ops::partition<"=">;
static_assert(before.sv() == "key");
static_assert(delim.sv() == "=");
static_assert(after.sv() == "value");

// 区切り文字が見つからない場合
constexpr auto [b2, d2, a2] = "hello"_fs | ops::partition<"=">;
static_assert(b2.sv() == "hello");
static_assert(d2.sv() == "");
static_assert(a2.sv() == "");

// 複数の区切り文字がある場合は最初のもので分割
constexpr auto [b3, d3, a3] = "a=b=c"_fs | ops::partition<"=">;
static_assert(b3.sv() == "a");
static_assert(a3.sv() == "b=c");
```

## `shrink_to_fit`（最初の終端位置に合わせて縮小）

`shrink_to_fit<Str>()` は、`FrozenString` 値 `Str` の `buffer` を先頭から走査し、
最初の `'\0'` までを含む最小サイズの `FrozenString` を返します。
`'\0'` が無い場合は `length + 1` サイズへ縮小します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr raw = [] {
  auto s = FrozenString<6>{};
  s.buffer = {'a', 'b', 'c', '\0', 'x', 'x'};
  s.length = 5;
  return s;
}();

auto constexpr shrunk = shrink_to_fit<raw>();
static_assert(shrunk.sv() == "abc");
```

## `split`（区切りで分割）

`split(...)` は内部で `split_count(...)` を呼び出し、トークン数に合わせた `std::vector<FrozenString<...>>` を返します。1回の呼び出しで分割まで完了します。

従来どおり `split<Count>(...)` も使え、こちらは `std::array<FrozenString<...>, Count>` を返します（`Count` が大きい場合の余りは空文字列）。

区切り文字の判定はデフォルトで ASCII whitespace（半角スペース、タブ、改行など）ですが、`split<is_delim>(...)` のようにテンプレート引数で判定関数を渡せます。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr count = split_count("  alpha  beta\tgamma\n");
static_assert(count == 3);

auto constexpr fixed = split("  alpha  beta\tgamma\n"_fs);
static_assert(fixed[0].sv() == "alpha");
static_assert(fixed[1].sv() == "beta");
static_assert(fixed[2].sv() == "gamma");

constexpr bool is_comma(char c) noexcept { return c == ','; }
auto const values = split<is_comma>("alpha,,beta,gamma");
```

## `split_numbers`（区切りで数値列へ変換）

`split_numbers(...)` は内部で `split_count(...)` を呼び出し、`std::array<数値型, N>` を返します（未使用要素は `0`）。

`Int` には整数型だけでなく `float` / `double` も指定できます。

区切り文字判定関数と数値型はテンプレート引数で指定できます。

- `split_numbers(str)` : 既定（空白区切り + `int`）
- `split_numbers<Int>(str)` : 空白区切り + 指定数値型
- `split_numbers<is_delim>(str)` : 指定区切り + `int`
- `split_numbers<is_delim, Int>(str)` : 指定区切り + 指定数値型

数値でないトークンや、指定した整数型の範囲外の値は、ランタイムでは例外となり、定数評価ではエラーになります。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr count = split_count("10 -20 +30");
static_assert(count == 3);

auto constexpr fixed = split_numbers("10 -20 +30"_fs);
static_assert(fixed[0] == 10);
static_assert(fixed[1] == -20);
static_assert(fixed[2] == 30);

auto constexpr i64_values = split_numbers<long long>("9223372036854775807 -9223372036854775808");
static_assert(i64_values[0] == std::numeric_limits<long long>::max());
static_assert(i64_values[1] == std::numeric_limits<long long>::min());

auto constexpr fvalues = split_numbers<float>("1.5 -2.25 +3.0");
static_assert(fvalues[0] == 1.5f);
static_assert(fvalues[1] == -2.25f);
static_assert(fvalues[2] == 3.0f);

constexpr bool is_semicolon(char c) noexcept { return c == ';'; }
auto const values = split_numbers<is_semicolon>("10;;-20;+30");

constexpr bool is_comma(char c) noexcept { return c == ','; }
auto constexpr uvalues = split_numbers<is_comma, unsigned long>("1,2,3");
static_assert(uvalues[0] == 1ul);
static_assert(uvalues[1] == 2ul);
static_assert(uvalues[2] == 3ul);

auto constexpr dvalues = split_numbers<is_comma, double>("1e2,-2.5e1,3.125");
static_assert(dvalues[0] == 100.0);
static_assert(dvalues[1] == -25.0);
static_assert(dvalues[2] == 3.125);
```

## `capitalize`（先頭大文字化）

先頭の文字を大文字、残りを小文字に変換します。`FrozenString` と文字列リテラルの両方を受け取ります。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr c = capitalize("hELLO wORLD"_fs);  // "Hello world"

static_assert(c.sv() == "Hello world");
```

## `to_snake_case`（スネークケース変換）

camelCase または PascalCase をスネークケース（snake_case）に変換します。各大文字の前に `_` を挿入し、すべての文字を小文字にします。`FrozenString` と文字列リテラルの両方を受け取ります。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr s1 = to_snake_case("helloWorld"_fs);   // "hello_world"
auto constexpr s2 = to_snake_case("HelloWorld");      // "hello_world"

static_assert(s1.sv() == "hello_world");
static_assert(s2.sv() == "hello_world");
```

## `to_camel_case`（キャメルケース変換）

snake_case をキャメルケース（camelCase）に変換します。`_` を除去し、その直後の文字を大文字にします。先頭は小文字のままです。`FrozenString` と文字列リテラルの両方を受け取ります。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr c = to_camel_case("hello_world"_fs);  // "helloWorld"

static_assert(c.sv() == "helloWorld");
```

## `to_pascal_case`（パスカルケース変換）

snake_case をパスカルケース（PascalCase）に変換します。`_` を除去し、その直後の文字および先頭の文字を大文字にします。`FrozenString` と文字列リテラルの両方を受け取ります。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr p = to_pascal_case("hello_world"_fs);  // "HelloWorld"

static_assert(p.sv() == "HelloWorld");
```

## `url_encode`（URLエンコード）

文字列をURLエンコード（パーセントエンコーディング）します。RFC 3986 に準拠し、非予約文字（アルファベット、数字、`-`, `.`, `_`, `~`）以外の文字を `%XX` 形式に変換します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr e1 = url_encode("Hello World!"_fs);  // "Hello%20World%21"
auto constexpr e2 = url_encode<"a b c"_fs>();      // NTTP版（正確なサイズ）

static_assert(e1.sv() == "Hello%20World%21");
static_assert(e2.sv() == "a%20b%20c");
```

## `url_decode`（URLデコード）

URLエンコードされた文字列をデコードします。`%XX` 形式を元の文字に変換し、`+` をスペースに変換します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr d1 = url_decode("Hello%20World%21"_fs);  // "Hello World!"
auto constexpr d2 = url_decode("a+b+c"_fs);            // "a b c"

static_assert(d1.sv() == "Hello World!");
static_assert(d2.sv() == "a b c");
```

## `base64_encode`（Base64エンコード）

文字列を Base64 エンコードします。`FrozenString` と文字列リテラルの両方を受け取ります。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr e1 = base64_encode("Hello"_fs);      // "SGVsbG8="
auto constexpr e2 = base64_encode<"f"_fs>();       // NTTP版（正確なサイズ）

static_assert(e1.sv() == "SGVsbG8=");
static_assert(e2.sv() == "Zg==");
```

## `base64_decode`（Base64デコード）

Base64 エンコードされた文字列をデコードします。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr d1 = base64_decode("SGVsbG8="_fs);    // "Hello"
auto constexpr d2 = base64_decode<"Zg=="_fs>();     // NTTP版（正確なサイズ）

static_assert(d1.sv() == "Hello");
static_assert(d2.sv() == "f");
```

## `html_encode` / `html_decode`（HTMLエンティティ変換）

HTML で特殊な意味を持つ文字をエンティティに変換・復元します。

- `html_encode(...)` : `& < > " '` を `&amp; &lt; &gt; &quot; &#39;` に変換します
- `html_decode(...)` : 上記の名前参照を元の文字に戻します
- 未知のエンティティはそのまま保持されます
- `FrozenString` と文字列リテラルの両方を受け取ります

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr e = html_encode("<hello> & \"world\""_fs);
// "&lt;hello&gt; &amp; &quot;world&quot;"

auto constexpr d = html_decode("&lt;hello&gt; &amp; &quot;world&quot;"_fs);
// "<hello> & \"world\""

// NTTP版
auto constexpr e2 = html_encode<"a<b"_fs>();
static_assert(e2.sv() == "a&lt;b");

auto constexpr d2 = html_decode<"a&lt;b"_fs>();
static_assert(d2.sv() == "a<b");
```

```cpp
// パイプ演算子
namespace fops = frozenchars::ops;
auto constexpr v = "<test>"_fs | fops::html_encode | fops::html_decode;
static_assert(v.sv() == "<test>");
```

## `minify_html` / `minify_xml` / `minify_json` / `minify_yaml` / `minify_sql` / `minify_cypher`（マークアップ / データ / クエリ 縮小）

HTML / XML / JSON / YAML / SQL / Cypher の各ソースをコンパイル時に縮小（minify）します。
すべて `consteval` で実行され、結果は `static_assert` でコンパイル時に検証できます。

- `minify_html(str, options = ...)` / `minify_xml(...)` : HTML/XML のマークアップを縮小します
  - `minify_markup_opt::remove_quotes` を立てると、`class="x"` のような属性値クォートが安全に取り除ける場合に削除されます
  - `minify_markup_opt::remove_end_tags` を立てると、HTML の `</li>` / `</p>` / `</thead>` など省略可能な終了タグが削除されます
  - オプションは `|` で複数指定できます。デフォルトでは両方が有効です
- `minify_json(str)` : JSON の文字列リテラル外から空白と `//` / `/* */` コメントを削除します
- `minify_yaml(str)` : YAML の値内・文字列リテラル内 `#` を残しつつ、行コメントとそれ以降の空白を削除します
- `minify_sql(str, options = ...)` : SQL の `--` / `/* */` コメントと冗長な空白を削除します
  - `minify_sql_opt::shorten_types` : `INTEGER` → `INT`、`BOOLEAN` → `BOOL` など型キーワードを短縮します（デフォルト ON）
  - `minify_sql_opt::remove_as` : `SELECT a AS b` の `AS` を省略します
  - `minify_sql_opt::simplify_join` : `INNER JOIN` を `JOIN` に簡略化します
  - 文字列リテラル（`'...'` / `"..."`）・バッククォート識別子・ブラケット識別子（`[ ... ]`）は内容を保持します
- `minify_cypher(input, output, output_size)` : Cypher クエリを実行時に固定バッファへ縮小します。書き込んだ長さを返します
  - `minify(input)` : 文字列リテラルからコンパイル時に縮小し、`minified_query<N>` 型を返します
  - 文字列リテラル（`'...'` / `"..."`）・バッククォート識別子（`` `...` ``）の内容はそのまま保持されます
  - `--` 行コメントと `/* */` ブロックコメントを削除します
  - 前後の空白や改行、連続する空白を 1 個に圧縮します。`MATCH (n)` のような `(` 直前、`)` 直後のスペースは削除します
  - 末尾の `;` は削除します（後続ステートメントの区切りには新しい空白を挿入します）
- `FrozenString` と文字列リテラルの両方を受け取ります。すべて `N` を含むバッファ長で動作します

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

// HTML: クォート除去 + 省略可能終了タグ除去（デフォルト）
auto constexpr html = minify_html(
  "<div class = \"x\"  >\n"
  "  <span>  hi  </span>\n"
  "  <!-- ignore -->\n"
  "</div>"_fs);
static_assert(html.sv() == "<div class=x><span>hi</span></div>");

// HTML: オプションを無効化（デフォルト動作を維持したいケース）
auto constexpr html_keep = minify_html(
  "<input value=\"x\">"_fs,
  minify_markup_opt::none);
static_assert(html_keep.sv() == "<input value=\"x\">");

// XML
auto constexpr xml = minify_xml(
  "<root>\n"
  "  <!-- comment -->\n"
  "  <item id = \"1\" > value </item>\n"
  "</root>"_fs);
static_assert(xml.sv() == "<root><item id=1>value</item></root>");

// JSON（文字列リテラル内・コメントは別扱い）
auto constexpr minified_json = minify_json(
  "{\n"
  "  \"k\": \"a b\",\n"
  "  \"n\": 1, // comment\n"
  "  \"x\": true\n"
  "}"_fs);
static_assert(minified_json.sv() == "{\"k\":\"a b\",\"n\":1,\"x\":true}");

// YAML（# が行コメント。文字列リテラル内 / 値内の # は残す）
auto constexpr yaml = minify_yaml(
  "key: value   # comment\n"
  "list:\n"
  "  - one  \n"
  "  - \"two # keep\"   # drop\n"
  "\n"_fs);
static_assert(yaml.sv() == "key: value\nlist:\n  - one\n  - \"two # keep\"\n");

// SQL: デフォルト（shorten_types）で型も短縮
auto constexpr sql = minify_sql(
  "SELECT  a,  b  -- comment\n"
  "FROM  tbl\n"
  "WHERE  c = 'x  y'  AND d = 1 ;"_fs);
static_assert(sql.sv() == "SELECT a,b FROM tbl WHERE c='x  y' AND d=1;");

auto constexpr sql_type = minify_sql(
  "SELECT  INTEGER,  BOOLEAN  FROM  tbl\n"
  "WHERE  col :: text = 'x'"_fs);
static_assert(sql_type.sv() == "SELECT INT,BOOL FROM tbl WHERE col::text='x'");

// SQL: AS 省略 + INNER JOIN 簡略化
auto constexpr sql_join = minify_sql(
  "SELECT a.id INNER JOIN users AS a ON a.id = b.id"_fs,
  minify_sql_opt::remove_as | minify_sql_opt::simplify_join);
static_assert(sql_join.sv() == "SELECT a.id JOIN users a ON a.id=b.id");
```

`has_flag(options, flag)` で各フラグの有無を判定できます。

```cpp
static_assert(has_flag(
  minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags,
  minify_markup_opt::remove_quotes));
```

なお、`minify_cypher` は `include/frozenchars/cypher_minifier.hpp` で提供されます（アンブレラ `frozenchars.hpp` には含まれないため個別にインクルードしてください）。

```cpp
#include "frozenchars/cypher_minifier.hpp"
using namespace frozenchars;

auto constexpr q = minify(
  "MATCH (n) // comment\n"
  "RETURN n;");
static_assert(q == "MATCH(n)RETURN n");

// 文字列リテラル内の // や /* */ は削除しない
auto constexpr s = minify("RETURN '//not a comment'");
static_assert(s == "RETURN'//not a comment'");

// バッククォート識別子内の // は削除しない
auto constexpr b = minify("MATCH (`my node`) RETURN `my node`");
static_assert(b == "MATCH(`my node`)RETURN`my node`");

// 実行時バッファ版
char buf[64];
auto const* input = "MATCH (n) RETURN n";
auto const len = minify_cypher(input, buf, sizeof(buf));
// len = 16, buf = "MATCH(n)RETURN n\0"
```

## `linebreak`（改行表現の相互変換）

HTML の `<br>` タグ、リテラル表記の `\n`、実際の改行コード (LF) の3つを相互変換します。

- `linebreak<From, To>(...)` : 指定したフォーマット間で変換します
- `br_to_nl`, `nl_to_br` などの便利エイリアスも提供しています
- `FrozenString` と文字列リテラルの両方を受け取ります

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

// <br> → 実改行
auto constexpr n = br_to_nl("line1<br>line2<br>line3"_fs);
// "line1\nline2\nline3"

// 実改行 → <br>
auto constexpr h = nl_to_br("line1\nline2\nline3"_fs);
// "line1<br>line2<br>line3"

// リテラル \n → 実改行
auto constexpr u = esc_n_to_nl(R"(hello\nworld)"_fs);
// "hello\nworld"
```

```cpp
// パイプ演算子
namespace fops = frozenchars::ops;
auto constexpr v = "a<br>b"_fs | fops::br_to_nl | fops::nl_to_br;
static_assert(v.sv() == "a<br>b");
```


## `word_wrap`（ワードラップ）

文字列を指定された幅で折り返します。スペース区切りで単語を認識し、指定幅を超える前に改行を挿入します。
既存の改行は保持され、各行の先頭と末尾の余分な空白は削除されます。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr w = word_wrap("the quick brown fox jumps over the lazy dog", 10);
// "the quick\nbrown fox\njumps over\nthe lazy\ndog"

static_assert(word_wrap("hello world", 80).sv() == "hello world");

// 既存の改行は保持
auto constexpr w2 = word_wrap("hello\nworld foo bar", 10);
// "hello\nworld foo\nbar"
```

```cpp
// パイプ演算子
namespace fops = frozenchars::ops;
auto constexpr w = "hello world foo"_fs | fops::word_wrap(8);
// "hello\nworld\nfoo"
```

## `indent` / `dedent`（行頭インデント追加・削除）

`indent<Width, Char>(str)` は各行の先頭に指定文字を指定個数付与します。
空行はインデントしません。デフォルトのインデント文字はタブ (`'\t'`) です。

`dedent(str)` は全行の先頭に共通する空白文字を最大限削除します。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

// indent: 各行にタブ1つを付与
static_assert(("a\nb"_fs | ops::indent<1>) == "\ta\n\tb"_fs);

// indent: カスタム文字
static_assert(("a\nb"_fs | ops::indent<1, ' '>) == " a\n b"_fs);

// dedent: 共通先頭空白を削除
static_assert(dedent("  hello\n  world"_fs).sv() == "hello\nworld");
static_assert(dedent("  hello\n    indented"_fs).sv() == "hello\n  indented");
```

## 文字種判定述語（`is_alpha` / `is_digit` / ...）

ASCII 文字の分類を行う constexpr 関数です。パイプ演算子の `Pred` テンプレート引数などに利用できます。

| 関数 | 判定内容 |
|------|----------|
| `is_upper(c)` | 英大文字（`A`-`Z`） |
| `is_lower(c)` | 英小文字（`a`-`z`） |
| `is_alpha(c)` | 英字 |
| `is_digit(c)` | 10進数字（`0`-`9`） |
| `is_alnum(c)` | 英数字 |
| `is_xdigit(c)` | 16進数字（`0`-`9`, `a`-`f`, `A`-`F`） |
| `is_cntrl(c)` | 制御文字（0x00-0x1F, 0x7F） |
| `is_graph(c)` | 表示可能文字（スペース以外） |
| `is_print(c)` | 表示可能文字（スペース含む） |
| `is_punct(c)` | 句読点 |
| `is_blank(c)` | 空白（スペースまたはタブ） |
| `is_space(c)` | ASCII空白文字 |

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;

static_assert(is_alpha('A'));
static_assert(is_digit('5'));
static_assert(is_alnum('z'));
static_assert(is_xdigit('F'));
static_assert(is_space(' '));
static_assert(is_blank('\t'));
static_assert(is_punct('!'));
```

## `utf8_length`（UTF-8 コードポイント数）

UTF-8 エンコードされた文字列のコードポイント数を計算します。不正なバイト列は1バイトを1コードポイントとしてカウントします（フェイルソフト）。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr len1 = utf8_length("Hello"_fs);            // 5
auto constexpr len2 = utf8_length("Hello 世界"_fs);        // 8
auto constexpr len3 = utf8_length("");                     // 0
auto constexpr len4 = utf8_length("\xC3\xA9");            // 1（é）

// NTTP版
auto constexpr len5 = utf8_length<"Hello"_fs>();           // 5
```

## `make_querystring`（クエリ文字列生成）

キーと値のペアからURLクエリ文字列を生成します。`FrozenString` と文字列リテラルの両方を受け取ります。
std::tuple, std::pair, std::array などのタプルライクな型も受け取ります（要素数2である必要があります）。
値はURLエンコードされます。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr q = make_querystring("name", "Alice & Bob");  // "name=Alice%20%26%20Bob"
static_assert(q.sv() == "?name=Alice%20%26%20Bob");

auto constexpr q2 = make_querystring(std::tuple{"name", "Alice & Bob"});
static_assert(q2.sv() == "?name=Alice%20%26%20Bob");
```

## `path::dirname` / `basename` / `extension` / `stem` / `join`（パス文字列操作）

`frozenchars::path` 名前空間に、POSIX パス文字列を操作するコンパイル時関数を提供します。
Windows のバックスラッシュ区切りには対応していません。

- `dirname(path_str)` : 親ディレクトリ部分を返します（`/` が無ければ `.`）。
- `basename(path_str)` : 末尾のファイル名部分を返します。
- `extension(path_str)` : 拡張子（最後の `.` 以降）を返します。ドットファイルの先頭 `.` は無視されます。
- `stem(path_str)` : 拡張子を除いたファイル名を返します。
- `join(parts...)` : 複数のパス要素を `/` で結合します。絶対パスが渡されるとリセットされます。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;
using namespace frozenchars::path;

static_assert(dirname("/usr/bin/gcc"_fs).sv() == "/usr/bin");
static_assert(basename("/usr/bin/gcc"_fs).sv() == "gcc");
static_assert(extension("archive.tar.gz"_fs).sv() == ".gz");
static_assert(stem("archive.tar.gz"_fs).sv() == "archive.tar");
static_assert(join("usr"_fs, "local"_fs, "bin"_fs).sv() == "usr/local/bin");
static_assert(join("/usr"_fs, "bin"_fs).sv() == "/usr/bin");
```

## マルチライン文字列の処理

複数行を含む文字列（`\n` で区切られた文字列）に対して、行単位での加工を行うスタンドアロン関数です。`FrozenString` と文字列リテラルの両方を受け取ります。

- `remove_leading_spaces(str, n)` : 各行の先頭から最大 `n` 個の空白を削除します（`n = 0` ですべて削除）。
- `remove_comment_lines(str, comment_seq = "#")` : 指定した文字列で始まる行を削除します。
- `remove_comments(str, comment_seq = "#")` : 指定した文字列を含めて行末までを削除します（直前の空白は残ります）。
- `remove_trailing_spaces(str, n)` : 各行の末尾から最大 `n` 個の連続した半角スペースを削除します（`n = 0` ですべて削除）。
- `join_lines(str)` : すべての行を結合します。結合部分にスペースがない場合は自動的に 1 つ挿入します。
- `trim_trailing_spaces(str)` : 各行の末尾の空白（スペース、タブなど）を削除します。
- `remove_empty_lines(str)` : 空行（改行のみの行）を削除します。

`remove_trailing_spaces` は半角スペースのみを対象にし、`trim_trailing_spaces` はスペース・タブなどの空白文字全般を対象にします。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr src = "  line1\n##comment\n  line2    # inline comment\n\nline3"_fs;

// 先頭の空白を2つ削除
auto constexpr r1 = remove_leading_spaces(src, 2);
// "line1\n##comment\nline2    # inline comment\n\nline3"

// コメント行（## で始まる行）を削除
auto constexpr r2 = remove_comment_lines(src, "##");
// "  line1\n  line2    # inline comment\n\nline3"

// インラインコメント（# 以降）を削除（直前の空白は残る）
auto constexpr r3 = remove_comments(src, "#");
// "  line1\n##comment\n  line2    \n\nline3"

// 行末の連続スペースを削除
auto constexpr r4 = remove_trailing_spaces(r3);
// "  line1\n##comment\n  line2\n\nline3"

// 行を結合（スペース補完あり）
auto constexpr r5 = join_lines("line1\nline2"_fs);
// "line1 line2"

// 末尾の空白を削除
auto constexpr r6 = trim_trailing_spaces("line1  \nline2 \n"_fs);
// "line1\nline2\n"

// 空行を削除
auto constexpr r7 = remove_empty_lines("line1\n\nline2"_fs);
// "line1\nline2"
```

## パイプ演算子で文字列ヘルパーをつなぐ

パイプ演算子を使うと、文字列ヘルパーを左から右へ読み下せる形で連結できます。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars::literals;
namespace fops = frozenchars::ops;

auto constexpr value = "  abcdef  "_fs | fops::trim | fops::toupper | fops::substr(0, 3);
static_assert(value.sv() == "ABC");
```

スタンドアロン関数 `frozenchars::trim(...)` や `frozenchars::toupper(...)` も引き続き使えます。
`frozenchars::ops::toupper` / `tolower` は、環境によっては C ライブラリの `::toupper` / `::tolower` と
名前が衝突しうるため、README の例では `namespace fops = frozenchars::ops;` のようなエイリアス経由で
アダプタを修飾して使う形を推奨します。

### `_fs` リテラルなしのパイプ

パイプ演算子の左辺は `FrozenString` だけでなく `const char[N]` 文字列リテラルも直接受け付けます。

```cpp
auto constexpr v = "  abcdef  " | fops::trim | fops::toupper | fops::substr(0, 3);
// v.sv() == "ABC"
```

`_fs` リテラルを省略できるため、`#include "frozenchars.hpp"` だけで即座にパイプ処理を記述できます。


## `frozen_format`（コンパイル時フォーマット）

`frozen_format<"format string"_fs>(args...)` は `std::format` 互換の構文でコンパイル時に文字列を生成します。
戻り値は `FrozenString<4096>`（固定最大バッファ、`.sv()` で実際の内容を取得）。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

// 基本置換
constexpr auto msg1 = frozen_format<"The answer is {}."_fs>(42);
// msg1.sv() == "The answer is 42."

// 16進数・ゼロ埋め
constexpr auto msg2 = frozen_format<"Hex: {:08X}"_fs>(0xABCD);
// msg2.sv() == "Hex: 0000ABCD"

// 左寄せ / 右寄せ
constexpr auto msg3 = frozen_format<"{:<10}|{:>10}"_fs>("left", "right");
// msg3.sv() == "left      |     right"

// 浮動小数点・精度指定
constexpr auto msg4 = frozen_format<"Pi = {:.5f}"_fs>(3.1415926535);
// msg4.sv() == "Pi = 3.14159"

// 符号制御
constexpr auto msg5 = frozen_format<"{:+} {:+}"_fs>(1, -1);
// msg5.sv() == "+1 -1"
```

### 対応フォーマット指定

| 指定 | 例 | 説明 |
|------|-----|------|
| `{}` | `frozen_format<"{}"_fs>(42)` | デフォルト形式 |
| `{:d}` | 同上 | 10進整数 |
| `{:x}` / `{:X}` | `frozen_format<"{:x}"_fs>(255)` → `ff` / `FF` | 16進小文字 / 大文字 |
| `{:o}` | `frozen_format<"{:o}"_fs>(8)` → `10` | 8進数 |
| `{:b}` / `{:B}` | `frozen_format<"{:b}"_fs>(3)` → `11` / `11` | 2進数 |
| `{:f}` | `frozen_format<"{:.2f}"_fs>(3.14)` | 固定小数点 |
| `{:e}` / `{:E}` | `frozen_format<"{:e}"_fs>(100.0)` → `1.000000e+02` | 科学計数法 |
| `{:g}` / `{:G}` | 同上 / デフォルト | 自動選択（固定 or 科学） |
| `{:+}` / `{: }` / `{:-}` | 符号制御 | 常時符号 / 空白 / 負のみ |
| `{:<}` / `{:>}` / `{:^}` | `frozen_format<"{:<5}"_fs>(42)` → `42   ` | 左 / 右 / 中央寄せ |
| `{:.5}` | `frozen_format<"{:.5}"_fs>("hello!")` → `hello` | 文字列切り詰め |
| `{:08}` | `frozen_format<"{:08}"_fs>(42)` → `00000042` | ゼロ埋め（整数） |
| `{{` / `}}` | `frozen_format<"{{}}"_fs>()` → `{}` | エスケープ |

コンパイル時にフォーマット文字列の整合性も検証されます（フィールド数不一致・括弧不一致はコンパイルエラー）。


## `parse_hex_rgb` / `parse_hex_rgba`（hex color → RGB/RGBAタプル）

`parse_hex_rgb(...)` は RGB 色文字列を、
`parse_hex_rgba(...)` は RGBA 色文字列を、
それぞれ `std::tuple` に変換します。

- `parse_hex_rgb(...)` は **`#RGB` / `#RRGGBB`**
- `parse_hex_rgba(...)` は **`#RGBA` / `#RRGGBBAA`**
- `to_bgr(...)` は `(r, g, b)` を `(b, g, r)` に並び替えます
- `to_bgra(...)` は `(r, g, b, a)` を `(b, g, r, a)` に並び替えます
- `to_abgr(...)` は `(r, g, b, a)` を `(a, b, g, r)` に並び替えます
- 短縮形は CSS と同じく各 nibble を複製します
  - `#532` → `#553322`
  - `#5a3c` → `#55aa33cc`
- 返り値はそれぞれ `(r, g, b)`、`(r, g, b, a)` の順
- 不正な入力は、ランタイムでは `std::invalid_argument`、定数評価ではエラーになります

```cpp
#include "frozenchars.hpp"
#include <tuple>

using namespace frozenchars;

auto constexpr rgb = parse_hex_rgb("#335577");
auto constexpr short_rgb = parse_hex_rgb("#532");
auto constexpr rgba = parse_hex_rgba("#5a3c");
auto constexpr bgr = to_bgr(rgb);
auto constexpr bgra = to_bgra(rgba);
auto constexpr abgr = to_abgr(rgba);

auto constexpr [r, g, b] = rgb;
auto constexpr [sr, sg, sb] = short_rgb;
auto constexpr [rr, rg, rb, ra] = rgba;
auto constexpr [blue, green, red] = bgr;
auto constexpr [bb, bg, br, ba] = bgra;
auto constexpr [aa, ab, ag, ar] = abgr;

static_assert(r == 0x33);
static_assert(g == 0x55);
static_assert(b == 0x77);
static_assert(sr == 0x55);
static_assert(sg == 0x33);
static_assert(sb == 0x22);
static_assert(rr == 0x55);
static_assert(rg == 0xaa);
static_assert(rb == 0x33);
static_assert(ra == 0xcc);
static_assert(blue == 0x77);
static_assert(green == 0x55);
static_assert(red == 0x33);
static_assert(bb == 0x33);
static_assert(bg == 0xaa);
static_assert(br == 0x55);
static_assert(ba == 0xcc);
static_assert(aa == 0xcc);
static_assert(ab == 0x33);
static_assert(ag == 0xaa);
static_assert(ar == 0x55);
```

集成体や任意のクラスへの初期化にも使えます。

```cpp
#include "frozenchars.hpp"
#include <tuple>

using namespace frozenchars;

struct AggregateColor {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
};

struct AggregateColorA {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
  std::uint8_t a;
};

struct AggregateColorBgr {
  std::uint8_t b;
  std::uint8_t g;
  std::uint8_t r;
};

struct LibraryColor {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;

  constexpr LibraryColor(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
  : r(red), g(green), b(blue)
  {}
};

struct LibraryColorA {
  std::uint8_t r;
  std::uint8_t g;
  std::uint8_t b;
  std::uint8_t a;

  constexpr LibraryColorA(std::uint8_t red, std::uint8_t green, std::uint8_t blue, std::uint8_t alpha)
  : r(red), g(green), b(blue), a(alpha)
  {}
};

struct LibraryColorAbgr {
  std::uint8_t a;
  std::uint8_t b;
  std::uint8_t g;
  std::uint8_t r;

  constexpr LibraryColorAbgr(std::uint8_t alpha, std::uint8_t blue, std::uint8_t green, std::uint8_t red)
  : a(alpha), b(blue), g(green), r(red)
  {}
};

auto constexpr aggregate = std::apply(
  [](auto... channels) constexpr {
    return AggregateColor{channels...};
  },
  parse_hex_rgb("#123")
);

auto constexpr aggregate_a = std::apply(
  [](auto... channels) constexpr {
    return AggregateColorA{channels...};
  },
  parse_hex_rgba("#1234")
);

auto constexpr aggregate_bgr = std::apply(
  [](auto... channels) constexpr {
    return AggregateColorBgr{channels...};
  },
  to_bgr(parse_hex_rgb("#abcdef"))
);

auto constexpr color = std::make_from_tuple<LibraryColor>(parse_hex_rgb("#abcdef"));
auto constexpr color_a = std::make_from_tuple<LibraryColorA>(parse_hex_rgba("#abcdef99"));
auto constexpr color_abgr = std::make_from_tuple<LibraryColorAbgr>(to_abgr(parse_hex_rgba("#12345678")));

static_assert(aggregate.r == 0x11);
static_assert(color.g == 0xcd);
static_assert(aggregate_a.a == 0x44);
static_assert(color_a.a == 0x99);
static_assert(aggregate_bgr.b == 0xef);
static_assert(color_abgr.a == 0x78);
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

## FrozenStringの応用例

FrozenString を活用して「できるだけconstexprで処理させる」を目指した機能をいくつか用意しています。

### `to_sv`（`std::format` 連携）

`<format>` が利用可能な環境では、`FrozenString` を NTTP として使い、
`std::format` のコンパイル時チェックを活かしたフォーマットができます。

- `to_sv<Str>()`
  - `consteval` で NTTP の `FrozenString` を `std::string_view` として取り出す
  - 戻り値は NTTP のバッファを指す定数式となるため、`std::format_string`
    の consteval コンストラクタに直接渡せる
  - フォーマット文字列と引数型の不一致はコンパイル時エラーになる

```cpp
#include "frozenchars.hpp"
#include <string>

using namespace frozenchars;
using namespace frozenchars::literals;

auto s = std::format(frozenchars::to_sv<"id={} name={}"_fs>(), 42, std::string{"alice"});
// s == "id=42 name=alice"
```

> メモ: `std::format_string` はコンパイル時定数として解釈可能な文字列を要求します。
> `to_sv` は NTTP の `FrozenString` から定数式として取り出すことで、
> この要件を満たしつつラッパー関数を介さずに `std::format` を直接呼べるようにします。

`to_sv` 経由で `std::format` ファミリに直接渡せるため、`format_to` / `format_to_n` /
`formatted_size` / ロケール版もすべて同じ書き方で利用できます。

```cpp
#include <iterator>
#include <locale>
#include <string>

auto out = std::string{};
std::format_to(std::back_inserter(out), frozenchars::to_sv<"x={}"_fs>(), 1);
// out == "x=1"

auto n = std::formatted_size(frozenchars::to_sv<"{} {}"_fs>(), "a", 1);
// n == 3

auto const classic = std::locale::classic();
auto s = std::format(classic, frozenchars::to_sv<"{}"_fs>(), 1234);
// s == "1234"
```

### `frozen_map`（固定キーの軽量マップ）

`frozen_map` は、キー集合をコンパイル時に固定しつつ、値を軽量に保持する小さなマップです。

- 宣言順の値をそのまま書きたい場合は `frozen_map{...}`
- キー集合を明示したい場合は `make_frozen_map(...)`
- キーと値の両方をコンパイル時に書き切りたい場合は `make_frozen_map_kv(...)`

#### 宣言順の値を braced-init-list で書く

```cpp
#include "frozenchars/frozen_map.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

frozen_map<std::string, "timeout"_fs, "retry"_fs, "backoff"_fs> map{
  "30", "5", "2"
};

assert(map["timeout"] == "30");
assert(map["retry"] == "5");
assert(map["backoff"] == "2");

frozen_map<std::string_view, "timeout"_fs, "retry"_fs, "backoff"_fs> view_map{
  "30", "5", "2"
};

assert(view_map["timeout"] == "30");
```

要素数がキー数と一致しない場合は、実行時に `std::invalid_argument` を送出します。

`std::string_view` は non-owning なので、保持先の寿命には注意してください。Context7 で確認した `cppreference` の通り、
文字列リテラルを渡すのは安全ですが、一時 `std::string` をそのまま渡すとダングリング参照になります。

#### pair-like エントリから作る基本形

```cpp
#include "frozenchars/frozen_map.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

auto map = make_frozen_map<int, "timeout"_fs, "retry"_fs>(
  std::pair{"retry", 5},
  std::pair{"timeout", 30}
);

static_assert(decltype(map)::size() == 2);
```

#### `to<Result>()` で STL コンテナへ変換する

```cpp
auto map = make_frozen_map<int, "timeout"_fs, "retry"_fs, "backoff"_fs>(
  std::pair{"retry", 5},
  std::pair{"timeout", 30},
  std::pair{"backoff", 2}
);

auto ordered = map.to<std::map<std::string, int>>();
auto hashed = map.to<std::unordered_map<std::string_view, int>>();
auto slots = map.to<std::array<std::pair<std::string_view, int>, 3>>();

auto moved = std::move(map).to<std::unordered_map<std::string, int>>();
```

`to<Result>()` は `std::map` / `std::unordered_map` / `std::array<std::pair<...>, N>` を受け付けます。
`const&` からは値をコピーし、`&&` からは値をムーブします。
`std::array` の並び順は宣言順ではなく、`begin()` / `end()` と同じハッシュスロット順です。

#### コンパイル時のキー/値列から作る短縮形

```cpp
#include "frozenchars.hpp"

using namespace frozenchars;

auto map = make_frozen_map_kv<int,
  kv{"aaa", 5},
  kv{"bbb", 3},
  kv{"ccc", 1},
  kv{"ddd", 0},
  kv{"eee", 3}
>();

static_assert(map["aaa"] == 5);
static_assert(map["ddd"] == 0);
```

`{"aaa", 5}` のような裸の初期化子リストをテンプレート実引数にそのまま置くのは難しいため、
コンパイル時APIでは `kv{"aaa", 5}` 形式を使います。

#### パフォーマンスと最適化

`frozen_map` は、実行時の検索速度を極限まで高めるために以下の最適化を自動的に適用します。

- **ハードウェア加速ハッシュ (CRC32C)**:
  ルックアップ時のハッシュ計算に、CPUのハードウェア命令（x86のSSE4.2命令やARMのCRC32拡張）を使用します。これにより、従来の FNV-1a ハッシュに比べて数倍高速に動作します。
- **128bit 整数比較最適化**:
  マップに登録された全てのキーが16バイト以下の場合、文字列比較を `std::string_view` の汎用的な比較から、レジスタ上の128bit整数比較へと自動的に切り替えます。これにより、キーの検証が1〜2命令で完了します。
- **データローカリティと宣言順反復**:
  実データはメモリ上に連続した配列として保持されます。イテレータによる走査や `to<std::array>()` による変換は、ハッシュ順ではなく**キーの宣言順**に行われるため、キャッシュ効率が高く、予測可能な動作を提供します。

> **依存関係について**:
> 高速なSIMD/ハードウェア命令を利用するため、内部でネイティブのイントリンシック命令（または SIMDe）を使用しています。非x86/ARM環境や古いCPUでは、自動的に `constexpr` 対応のソフトウェア実装にフォールバックされます。

#### `get_value_or` — デフォルト付き取得

キーが存在する場合はその値を、存在しない場合は指定したデフォルト値を返します。

```cpp
auto map = make_frozen_map<int, "timeout"_fs, "retry"_fs>(
  std::pair{"timeout", 30},
  std::pair{"retry", 5}
);

assert(map.get_value_or("timeout", 0) == 30);
assert(map.get_value_or("missing", 99) == 99);
```

`get()` が `std::optional<std::reference_wrapper<...>>` を返すのに対し、`get_value_or` は値をコピーして返すため、デフォルト値との分岐を一行で書けます。

#### `contains_all` — 複数キーの一括存在判定

複数のキーが全てマップ内に存在するかをコンパイル時に判定します。空パックに対しては `true`（vacuous truth）を返します。

```cpp
static_assert(map.contains_all<"timeout"_fs, "retry"_fs>());
static_assert(!map.contains_all<"timeout"_fs, "missing"_fs>());
static_assert(map.contains_all<>()); // 空パック → true
```

`consteval` で評価されるため、不正なキーをコンパイルエラーにできます。ランタイムで複数の `contains` を呼ぶより意図が明確です。

#### `keys_in_declaration_order` — 宣言順キー配列

キーを宣言順で `std::span` として返します。`keys()` がソート済み配列を返すのに対し、宣言時の順序を保ちます。

```cpp
auto map = make_frozen_map<int,
  kv{"aaa", 1}, kv{"bbb", 2}, kv{"ccc", 3}
>();

for (auto key : map.keys_in_declaration_order()) {
  // "aaa", "bbb", "ccc" の順で処理
}
```

要素の配置順と一致するため、走査時にキャッシュ効率が良いです。

#### `operator==` / `operator!=` — 値ごとの等価比較

同じキー集合を持つ 2 つの `frozen_map` の全値が等しいかを判定します。値型 `T` に `operator==` が定義されている必要があります。

```cpp
auto a = make_frozen_map<int, "x"_fs, "y"_fs>(
  std::pair{"x", 1}, std::pair{"y", 2}
);
auto b = make_frozen_map<int, "x"_fs, "y"_fs>(
  std::pair{"x", 1}, std::pair{"y", 2}
);

assert(a == b);
assert(a != make_frozen_map<int, "x"_fs, "y"_fs>(
  std::pair{"x", 1}, std::pair{"y", 99}
));
```

キー集合が異なる型同士では比較できません（コンパイルエラー）。

### `frozen_trie_map`（圧縮トライ マップ）

`frozen_trie_map` は、キー集合をコンパイル時に圧縮トライ（ラディックスツリー）として構築し、実行時に O(キー長) の検索を提供するマップです。

`frozen_map` がハッシュベースであるのに対し、`frozen_trie_map` は共通接頭辞を圧縮するため、**接頭辞の似たキー群**で高い性能を発揮します。

```cpp
#include "frozenchars/trie_map.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

auto map = frozen_trie_map<int, "timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>{
  std::array<int, 4>{1, 2, 3, 4}
};

assert(map["timeout"] == 1);
assert(map["timeout_ms"] == 2);
assert(map.contains("timeout_us"));
```

#### `frozen_trie_map` の特徴

- **圧縮トライ**: 共通接頭辞を1ノードにまとめ、メモリ使用量を削減
- **O(キー長) の検索**: キー数に関係なくキーの文字数だけで検索時間が決まる
- **SIMD ラベル比較**: SSE2 / NEON でラベルの比較を高速化（16文字以上のラベル）
- **子ノード LUT**: 子ノード数が8超のノードでルックアップテーブルを使用
- **ランダムアクセスイテレータ**: `frozen_map` と同じく宣言順の走査が可能

### `frozen_map` / `frozen_trie_map` / STL コンテナの選び方

下記は **find（検索）のみ** のベンチマーク結果です。環境: GCC 16, x86-64, `-O2`。

| パターン | frozen_map | frozen_trie_map | std::map | std::unordered_map |
|---|---:|---:|---:|---:|
| P1: 短キー(3個) | 3.6 ns | **1.6 ns** | 10.5 ns | 12.0 ns |
| P2: HTTPメソッド(5個) | 4.7 ns | **0.3 ns** | 14.6 ns | 9.8 ns |
| P3: 共通接頭辞(4個) | 5.1 ns | **0.3 ns** | 10.7 ns | 11.8 ns |
| P4: NATO字母(20個) | 3.8 ns | **0.3 ns** | 18.1 ns | 23.3 ns |
| P5: 長キー(5個, 25-40文字) | **1.5 ns** | 8.7 ns | 14.1 ns | 4.6 ns |
| P6: 同長キー(10個, 21文字) | **8.1 ns** | 9.3 ns | 18.0 ns | 11.4 ns |

> **单位**: ns/iter（1回の検索にかかるナノ秒）。小さいほど高速。

#### アーキテクチャ比較

| 項目 | `frozen_map` | `frozen_trie_map` | `std::map` | `std::unordered_map` |
|---|---|---|---|---|
| 検索アルゴリズム | CRC32C ハッシュ + LUT | 圧縮トライ | 赤黒木 | ハッシュテーブル |
| 時間計算量 | O(1) 摧定 | O(キー長) | O(log N) | O(1) 摧定 |
| メモリレイアウト | 連続配列 | フラット配列（ノード+ラベル+子） | ポインタベース木 | バケット配列 |
| コンパイル時構築 | ○ | ○ | × | × |
| SIMD 最適化 | CRC32C, 128bit比較 | SSE2/NEON ラベル比較 | × | × |
| キー数上限 | なし（64超は二分探索） | 127 | なし | なし |

#### 用途推奨

| 場面 | 推奨 | 理由 |
|---|---|---|
| キー数 ≤ 64、短いキー | `frozen_map` | CRC32C + LUT で O(1) 近傍の検索 |
| 接頭辞の似たキー群（HTTPヘッダ、設定キー） | `frozen_trie_map` | 共通接頭辞圧縮でメモリ削減 + O(キー長) 検索 |
| 長いキー（30文字以上） | `frozen_map` | ハッシュ計算が効率的 |
| キー数 > 64 | `frozen_map` | 二分探索でも O(log N) で動作 |
| ランタイムでキー追加が必要 | `std::unordered_map` | frozen 系は不変 |
| 順序付き走査が必要 | `std::map` | frozen 系は宣言順 |

#### ベンチマークの再現

```bash
# ビルド
cmake --build build --target bench_map_comparison

# 実行（デフォルト 500,000 イテレーション）
./build/test/bench_map_comparison

# イテレーション数を指定
./build/test/bench_map_comparison 1000000
```

### `parse_to_tuple`（型列文字列 → `std::tuple<...>`）

`parse_to_tuple<Str>()` は、固定文字列で書いた型リストをパースし、
対応する `std::tuple<...>` 型を保持する `type_identity` を返します。
実際の型は `typename decltype(parse_to_tuple<...>())::type` で取り出します。

- `,` で型を区切ります
- `?` を末尾に付けると `std::optional<T>` になります
- `[ ... ]` でネストした `std::tuple<...>` を書けます
- `[ ... ]?` は `std::optional<std::tuple<...>>` になります
- 空要素（例: `"int,,string"`）は `void` として扱われます
- `"void"` と明示的に書いた場合も `void` になります
- 空白は無視されます
- `void?` は `std::optional<void>` になるため非対応です

対応している主な型名:

- `bool`, `char`, `int`, `uint` / `unsigned`, `long`, `ulong`, `float`, `double`
- `string` / `str`, `string_view` / `sv`, `void`, `size_t` / `sz`
- `int8_t` / `int8`, `int16_t` / `int16`, `int32_t` / `int32`, `int64_t` / `int64`
- `uint8_t` / `uint8`, `uint16_t` / `uint16`, `uint32_t` / `uint32`, `uint64_t` / `uint64`

```cpp
#include "frozenchars.hpp"
#include <type_traits>

using namespace frozenchars;
using namespace frozenchars::literals;

using T1 = typename decltype(parse_to_tuple<"int, string?, bool"_fs>())::type;
static_assert(std::is_same_v<T1, std::tuple<int, std::optional<std::string>, bool>>);

using T2 = typename decltype(parse_to_tuple<"[int, bool?], void, sv"_fs>())::type;
static_assert(std::is_same_v<T2, std::tuple<std::tuple<int, std::optional<bool>>, void, std::string_view>>);

using T3 = typename decltype(parse_to_tuple<"int,,[char, double]?"_fs>())::type;
static_assert(std::is_same_v<T3, std::tuple<int, void, std::optional<std::tuple<char, double>>>>);
```

`parse_to_tuple` 自体は型計算用のヘルパーなので、実行時に `std::tuple` オブジェクトを返す関数ではありません。
未知の型名や不正な構文は、ランタイムではなくコンパイル時エラーになります。

#### `parse_to_variant`（型列文字列 → `std::variant<...>`）

`parse_to_variant<Str>()` は、固定文字列で書いた型リストをパースし、
対応する `std::variant<...>` 型を保持する `type_identity` を返します。

```cpp
using V1 = typename decltype(parse_to_variant<"int, string, bool"_fs>())::type;
static_assert(std::is_same_v<V1, std::variant<int, std::string, bool>>);

// _t エイリアスを使った簡潔な書き方
using V2 = parse_to_variant_t<"int, string, bool"_fs>;
static_assert(std::is_same_v<V2, std::variant<int, std::string, bool>>);
```

- `void` 型は `std::monostate` にマッピングされます
- `parse_to_tuple` と同じ構文が使えます（optional, ネスト, サフィックス）

#### 型エイリアス

冗長な `typename decltype(...)::type` パターンを省略するエイリアス:

```cpp
// parse_to_tuple の型エイリアス
template <auto Str>
using parse_to_tuple_t = typename decltype(parse_to_tuple<Str>())::type;

// parse_to_variant の型エイリアス
template <auto Str>
using parse_to_variant_t = typename decltype(parse_to_variant<Str>())::type;

// type_mapping の値エイリアス
template <auto S>
inline constexpr auto type_mapping_v = type_mapping<S>::type;
```

#### ポインタ/参照型

サフィックスでポインタ型・参照型を指定できます:

| 構文 | 型 |
|------|-----|
| `"int*"_fs` | `int*` |
| `"int&"_fs` | `int&` |
| `"int&&"_fs` | `int&&` |

```cpp
using T = parse_to_tuple_t<"int*, string&, bool&&"_fs>;
static_assert(std::is_same_v<T, std::tuple<int*, std::string&, bool&&>>);
```

- `void*`, `void&` は未対応（コンパイルエラー）


## よくある質問 (FAQ)

### Q. `consteval` ですが、実行時にも使えますか？

**いいえ。** `consteval` 関数の引数はすべてコンパイル時定数である必要があります。
実行時の文字列（`std::string`、`const char*` 変数など）は渡せません。
どうしても実行時に使いたい値がある場合は、`#define` マクロや `constexpr` 変数で
定数式として渡す必要があります。

### Q. `FrozenString` の最大サイズは?

`N` はコンパイル時に決まる正の整数であれば制限はありませんが、
スタック上に確保されるため大きな値（数万文字以上）はスタックオーバーフローに注意してください。

### Q. 実行時の文字列と相互変換できますか?

- `FrozenString` → `std::string_view`: `.sv()` メソッド
- `FrozenString` → `std::string`: `std::string(s.sv())`
- `std::string` → `FrozenString`: できません（実行時サイズは NTTP にできないため）

### Q. 他のコンパイル時文字列ライブラリとどう違いますか?

`std::string` ベースの `constexpr` 文字列ライブラリと違い、
`FrozenString` は `consteval` で動作し、動的メモリ確保がありません。
また、パイプ演算子によるチェーン、`split_numbers`、`minify_*` などの
応用ユーティリティが充実しています。

## `wildcard_match`（ワイルドカードマッチング）

`wildcard_match<Pattern>(text)` は、`Pattern` をワイルドカードパターンとして
文字列 `text` がマッチするかをコンパイル時に判定します。

- `*` = 任意の長さの文字列
- `?` = 任意の 1 文字
- `\` = エスケープ
- `[abc]` = セット内の任意の 1 文字
- `(ab|cd)` = 代替（いずれかにマッチ）

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

static_assert(wildcard_match<"*.txt"_fs>("readme.txt"));
static_assert(wildcard_match<"a?c"_fs>("abc"));
static_assert(!wildcard_match<"a?c"_fs>("abcd"));
```

> **自動最適化**: `*` や `?` を含まないパターンはコンパイル時に `frozen_regex` へ委譲され、
> 二分探索（O(log n)）で実行されます。委譲条件は以下のすべてを満たす場合です：
> グループ外に `.` / `|` がない、`\X` エスケープが frozen_regex 互換、`[...]` / `(...)` が
> 正しく閉じている。`(a|b)` や `[abc]` のように frozen_regex と意味が一致するパターンは
> 最大約 65 倍高速化されます。

### `frozen_regex` / `CTRE` / `wildcard_match` / `wildcards` の選び方

下記は **contains / match（マッチ判定）のみ** のベンチマーク結果です。環境: GCC 16, x86-64, `-O2`。

| パターン | frozen_regex | CTRE | wildcard_match | wildcards (RT) |
|---|---:|---:|---:|---:|
| リテラル `'endpoint'` | **0.3 ns** | 1.6 ns | **1.6 ns** (委譲) | 16.2 ns |
| 選択(4) `'GET\|POST\|PUT\|DELETE'` | **0.3 ns** | 1.6 ns | **1.6 ns** (委譲) | 16.2 ns |
| 選択(8) `'GET\|...\|TRACE'` | **0.3 ns** | 1.6 ns | **1.6 ns** (委譲) | 13.8 ns |
| パス(4) `'/api/v1/users\|...'` | **0.3 ns** | 1.7 ns | **1.6 ns** (委譲) | 31.5 ns |
| 文字クラス `[abc]` | 1.1 ns | 1.6 ns | **1.6 ns** (委譲) | 10.7 ns |
| 文字クラス `[a-m]` | 1.7 ns | **1.6 ns** | **1.6 ns** (委譲) | 8.9 ns |

> **单位**: ns/iter（1回のマッチにかかるナノ秒）。小さいほど高速。

#### アーキテクチャ比較

| 項目 | `frozen_regex` | CTRE | `wildcard_match` | `wildcards` |
|---|---|---|---|---|
| 評価タイミング | コンパイル時 | コンパイル時 | コンパイル時 | **実行時** |
| マッチ手法 | 全文字列列挙 + トライ検索 | NFA シミュレーション | 再帰的バックトラッキング（`*?` なしは frozen_regex 委譲） | 再帰的バックトラッキング |
| 対応構文 | リテラル, 選択, 文字クラス, グループ, ドット | **正規表現フルスペック** | ワイルドカード (`*?[]()`) | ワイルドカード (`*?[]()`) |
| 量指定子 (`+*?{n,m}`) | **非対応** | ○ | × | × |
| キャプチャグループ | × | ○ | × | × |
| `std::regex` との互換性 | 部分的 | ○ | × | × |
| ヘッダのみ | ○ | ○ | ○ | × (FetchContent) |

#### 用途推奨

| 場面 | 推奨 | 理由 |
|---|---|---|
| 固定パターンの高速マッチ（HTTPメソッド、路経判定） | `frozen_regex` | 列挙 + トライで O(1) 近傍。選択肢が多くても高速 |
| 正規表現フルスペックが必要（量指定子、キャプチャ） | CTRE | コンパイル時 NFA。`std::regex` より高速 |
| ワイルドカード風パターン（ファイル名マッチ） | `wildcard_match` | `*?` を含まなければ `frozen_regex` 委譲で超高速。構文がシンプル |
| パターンが実行時に決まる | `wildcards` | コンパイル時 NTTP が使えない唯一の選択肢 |

#### ベンチマークの再現

```bash
# ビルド
cmake --build build --target bench_pattern_comparison

# 実行（デフォルト 500,000 イテレーション）
./build/test/bench_pattern_comparison
```

## `frozen_set`（コンパイル時集合）

`frozen_set<Keys...>` は、キーの存在確認をコンパイル時に行える固定サイズの集合です。
`std::set` のコンパイル時代替として使えます。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

using set_t = frozen_set<"red"_fs, "green"_fs, "blue"_fs>;

static_assert(set_t::contains("red"));
static_assert(!set_t::contains("yellow"));
static_assert(set_t::size() == 3);
```

## `remove_comments` / `remove_comment_lines`（コメント行除去）

先頭が指定した文字列で始まる行を除去します（デフォルトは `#`）。

```cpp
#include "frozenchars.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

auto constexpr r = remove_comments("a\n# comment\nb"_fs);
// r.sv() == "a\nb"
```

パイプ演算子でも使用できます。

```cpp
using namespace frozenchars::ops;

auto constexpr r2 = "#header\na=1\n#end"_fs | remove_comments();
// r2.sv() == "a=1"
```


## `chrono`（日付・時刻のコンパイル時相互変換）

`frozenchars/chrono.hpp` は ISO 8601 日付・時刻文字列と `std::chrono` 型の間のコンパイル時相互変換を提供します。
`consteval`（NTTP）と `constexpr`（ランタイム）の両方のオーバーロードがあります。

```cpp
#include "frozenchars.hpp"
#include "frozenchars/chrono.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;
using namespace std::chrono;
```

### パース（文字列 → chrono 型）

| 関数 | 入力例 | 戻り値 |
|------|--------|--------|
| `parse_iso_date<S>()` | `"2026-07-04"_fs` | `year_month_day` |
| `parse_iso_datetime<S>()` | `"2026-07-04T14:30:00Z"_fs` | `sys_seconds` |
| `parse_date_macro<S>()` | `"Jul  4 2026"_fs`（`__DATE__` 形式） | `year_month_day` |
| `compilation_timestamp()` | 自動（`__DATE__` + `__TIME__`） | `sys_seconds` |

```cpp
// ISO 日付
constexpr auto d1 = parse_iso_date<"2026-07-04"_fs>();
// d1 == year{2026}/7/4

// ISO 日時（UTC、タイムゾーンオフセット付きも可）
constexpr auto dt1 = parse_iso_datetime<"2026-07-04T14:30:00Z"_fs>();
constexpr auto dt2 = parse_iso_datetime<"2026-07-04T14:30:00+09:00"_fs>();
// dt1 == sys_days{year{2026}/7/4} + hours{14} + minutes{30}
// dt2 == dt1 - minutes{540} （UTC 換算）

// __DATE__ マクロ形式
constexpr auto d2 = parse_date_macro<"Jul  4 2026"_fs>();
// d2 == year{2026}/7/4

// コンパイルタイムスタンプ
constexpr auto ts = compilation_timestamp();
// ts >= 現在時刻（ビルド時）
constexpr auto ts_jst = compilation_timestamp(minutes{540});
// ts_jst == ts - minutes{540} （JST → UTC 補正）
```

ランタイムオーバーロードも同様に使えます（`static_assert` のかわりに条件分岐でエラーを検出）。

```cpp
constexpr auto dr = parse_iso_date("2026-07-04"_fs);
// dr == year{2026}/7/4
```

### フォーマット（chrono 型 → 文字列）

| 関数 | 入力例 | 出力 |
|------|--------|------|
| `format_iso_date(ymd)` | `year{2026}/7/4` | `"2026-07-04"_fs`（`FrozenString<11>`） |
| `format_iso_datetime(tp)` | `sys_days{...}` + `hours{14}` | `"2026-07-04T14:30:00Z"_fs`（`FrozenString<21>`） |

```cpp
constexpr auto f1 = format_iso_date(year{2026}/7/4);
// f1.sv() == "2026-07-04"

constexpr auto f2 = format_iso_datetime(
  sys_days{year{2026}/7/4} + hours{14} + minutes{30});
// f2.sv() == "2026-07-04T14:30:00Z"
```

### ラウンドトリップ

パース結果を再度フォーマットして元の文字列に戻せることをコンパイル時に検証できます。

```cpp
constexpr auto ymd = year{2026}/7/4;
constexpr auto formatted = format_iso_date(ymd);
constexpr auto parsed = parse_iso_date(formatted);
static_assert(parsed == ymd);
```

> ⚠ `compilation_timestamp()` は `__DATE__` / `__TIME__` マクロに依存するため、日単位の精度です。
> 秒精度のタイムスタンプが必要な場合は `parse_iso_datetime` で明示的に文字列を渡してください。


## テスト

このリポジトリでは以下でビルド・テストできます。

- `build.sh`
- `test.sh`

## インストール / パッケージ生成

### ビルド済み環境に組み込む

```cmake
find_package(frozenchars CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE frozenchars::frozenchars)
```

### インストール

```bash
cmake --install build --prefix ./install
```

インストール先には `frozenchars.hpp`、依存ヘッダ、および `find_package(frozenchars CONFIG)` 用の CMake 設定が配置されます。

### vcpkg（将来対応予定）

> 現時点で vcpkg の公式レジストリには未登録です。将来的にポートを提供する予定です。
> それまでは上記の `cmake --install` によるインストール、またはサブディレクトリ／`FetchContent` での取り込みをご利用ください。

## ライセンス

MIT License

SPDX-License-Identifier: MIT

Copyright (c) 2026 toge
