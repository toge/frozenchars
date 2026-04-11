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

## `parse_to_tuple`（型列文字列 → `std::tuple<...>`）

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

## マルチライン文字列の処理

複数行を含む文字列（`\n` で区切られた文字列）に対して、行単位での加工を行うスタンドアロン関数です。`FrozenString` と文字列リテラルの両方を受け取ります。

- `remove_leading_spaces(str, n)` : 各行の先頭から最大 `n` 個の空白を削除します。
- `remove_comment_lines(str, comment_seq = "#")` : 指定した文字列で始まる行を削除します。
- `remove_comments(str, comment_seq = "#")` : 指定した文字列以降行末までを削除します。直前の空白文字も削除します。
- `join_lines(str)` : すべての行を結合します。結合部分にスペースがない場合は自動的に 1 つ挿入します。
- `trim_trailing_spaces(str)` : 各行の末尾の空白（スペース、タブなど）を削除します。
- `remove_empty_lines(str)` : 空行（改行のみの行）を削除します。

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

// インラインコメント（# 以降）を削除（直前の空白も削除される）
auto constexpr r3 = remove_comments(src, "#");
// "  line1\n##comment\n  line2\n\nline3"

// 行を結合（スペース補完あり）
auto constexpr r4 = join_lines("line1\nline2"_fs);
// "line1 line2"

// 末尾の空白を削除
auto constexpr r5 = trim_trailing_spaces("line1  \nline2 \n"_fs);
// "line1\nline2\n"

// 空行を削除
auto constexpr r6 = remove_empty_lines("line1\n\nline2"_fs);
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

## テスト

このリポジトリでは以下でビルド・テストできます。

- `build.sh`
- `test.sh`

## インストール / パッケージ生成

`CMake` の `install`に対応しています。

```
cmake --install build --prefix ./install
```

インストール先のディレクトリには `frozenchars.hpp` と `find_package(frozenchars CONFIG)` 用の CMake 設定が含まれます。

## ライセンス

MIT License
