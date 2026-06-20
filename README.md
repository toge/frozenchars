# frozenchars

文字列連結・繰り返し・数値フォーマットをなるべくコンパイル時に行うための型とヘルパー関数を提供する、ヘッダオンリーの C++ ライブラリです。

## 特徴

- 内部ではstd::arrayで固定長バッファを持つ `FrozenString` クラスを定義し、文字列リテラルや数値などからコンパイル時に文字列を生成できます。
- 操作関数のほとんどをconstevalで定義しており、コンパイル時に評価されます。
- 文字列操作をチェーンできるパイプ演算子も提供しています。
- コンパイル時の文字列処理を前提にした応用機能もいくつか用意しています。
- HTML エンティティ変換、ワードラップ、UTF-8 コードポイント数計算などの実用関数も提供しています。

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

キーと値のペアからURLクエリ文字列を生成します。`FrozenString` と文字列リテラルの両方を受け取ります。l
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

- std::formatのフォーマット文字列にFrozenStringを直接使うための `to_sv`
- 「keyはコンパイル時に決定し、valueは実行時に更新する」マップ
- 「固定文字列で型リストを書いて、対応するstd::tupleを得る」パーサー
- テンプレート文字列をコンパイル時にパースして、実行時に展開するテンプレートエンジン

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
- 同じコンパイル時APIを短く書きたい場合は `make_kv_map(...)`

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


### inja風テンプレート（constexprパース + 実行時レンダリング）

`FrozenString` をNTTPとして渡すと、テンプレート構造をコンパイル時に解析し、実行時に値を与えてレンダリングできます。
ルートコンテキストは `glz::meta<T>` または Glaze reflection で参照可能な型を渡します。

```cpp
#include "frozenchars.hpp"
#include <glaze/glaze.hpp>
using namespace frozenchars::inja;
using namespace frozenchars::literals;

struct profile {
  std::string city;
};

struct user {
  std::string name;
  int age;
  profile profile;
};

struct root_context {
  user user;
  std::vector<int> items;
  bool ok;
};

auto constexpr src = "Hello {{ user.name }} from {{ user.profile.city }}{% if ok %}:{% for x in items %}{{ loop.index }}={{ x }};{% endfor %}{% endif %}"_fs;
auto const ctx = root_context{
  .user = user{.name = "Tom", .age = 18, .profile = profile{.city = "Tokyo"}},
  .items = {1, 2, 3},
  .ok = true,
};
auto const out = render<src>(ctx); // "Hello Tom from Tokyo:0=1;1=2;2=3;"
```

対応構文（コア）:

- `{{ expr }}`
- `{% if ... %} ... {% else %} ... {% endif %}`
- `{% for item in items %} ... {% endfor %}`
- `{% for key, value in object %} ... {% endfor %}`
- `{# comment #}`

#### 動的な値の扱い (`inja_value.hpp`)

`inja_value` は式評価中の中間値・組み込み/カスタム関数引数・`set` のローカル変数に使われます。
ルートコンテキストの入力は typed context（Glaze reflection 対応型）です。

- `null` / `bool` / `int64_t` / `double` / `std::string`
- `inja_array` (配列)
- `inja_object` (オブジェクト/マップ)

#### 特化型配列による最適化

`inja_array` は、すべての要素が同じ型である場合にメモリ効率と実行速度を向上させるための**特化型配列**をサポートしています。

- `std::vector<int64_t>` (整数特化)
- `std::vector<double>` (浮動小数特化)
- `std::vector<std::string>` (文字列特化)

特化型配列を使用すると、要素ごとの型チェックが省略され、連続したメモリ配置によるキャッシュ効率の向上が期待できます。特に `range()` 関数は最初から整数特化配列を生成します。

明示的に特化型へ変換するための組み込み関数も提供されています：
- `as_int_array(arr)`
- `as_double_array(arr)`
- `as_string_array(arr)`

#### パフォーマンス / 実行モデル

この実装は **constexpr でテンプレート構造を解析**し、ノード列（バイトコード相当）として保持します。  
一方で、**レンダリング自体は実行時**にコンテキストを使って評価されます。  
つまり「毎回テンプレート文字列を再パースしない」ことが主な利点であり、式評価や分岐・反復の判定は実行時に行われます。

#### どのケースが軽量 lookup を使うか

`name` や `user.profile.city` のような単純な識別子/ドットパスは simple-path として扱われ、まず軽量な直接 lookup を試します。  
この経路は `{{ expr }}` だけでなく、simple-path の場合に以下でも使われます。

- `{% if ... %}` の条件式
- `{% for ... in ... %}` の反復対象
- `{% include ... %}` のターゲット式

```jinja
{{ user.profile.city }}             {# simple path: 直接 lookup を先に試す #}
{{ user.age + 1 }}                  {# 複合式: runtime 式パーサで評価 #}
{% if user.active %}...{% endif %} {# simple path 条件なら lookup 優先 #}
```

#### どのケースが runtime 式パーサへフォールバックするか

simple-path でない式は、従来どおり runtime の式パーサで評価されます。  
また simple-path でも、直接 lookup が `not_found` の場合は既存挙動を保つため式パーサへフォールバックします（定数や他の式解決経路を維持するため）。

代表例:

- 四則演算
- 論理演算（`and` / `or` / `not`）
- 比較演算
- 関数呼び出し
- パイプ式
- リテラル
- その他の非 simple-path 式

#### typed context と `inja_value` の役割分担

ルート入力は Glaze reflection（`glz::meta<T>` / reflectable）を前提とした typed context です。  
レンダリング前に typed root 全体を動的 `inja_object` に変換する設計ではなく、メンバー lookup は typed データを直接たどります。  
一方で、式評価の中間値・ローカル変数・関数引数などには `inja_value` が使われます。

#### ループ実行モデル

配列/配列相当の反復は lightweight な local frame を使って実行され、`loop.index` / `loop.index1` / `loop.is_first` / `loop.is_last` はこのループ状態から供給されます（毎回の動的オブジェクトマップ生成ではありません）。  
`{% for k, v in user.profile %}` のような typed reflectable object ループは、反復対象が simple-path のとき typed member を直接反復できます。  
従来どおり `inja_object` の動的オブジェクト反復もサポートされます。

#### 現在の制約

- 複雑な式は runtime 式パーサに依存します。
- 中間値の一部は `inja_value` 表現を使います。
- フォールバック/中間処理の一部ではオブジェクト相当データを動的に実体化する場合があります。
- `set` 文はローカル識別子への代入のみ対応です（`{% set user.name = ... %}` は未対応）。

## テスト

このリポジトリでは以下でビルド・テストできます。

- `build.sh`
- `test.sh`

## インストール / パッケージ生成

`CMake` の `install`に対応しています。

```
cmake --install build --prefix ./install
```

インストール先のディレクトリには `frozenchars.hpp` とその依存先のヘッダファイル、および `find_package(frozenchars CONFIG)` 用の CMake 設定が含まれます。

## ライセンス

MIT License
