# frozen_regex 設計

**日付**: 2026-07-01
**ステータス**: 設計承認済み・実装待ち
**関連**: 既存 `frozenchars/ctre.hpp`（CTRE ベースのコンパイル時マッチング）とは独立モジュール

## 概要

正規表現パターンから「マッチしうる全文字列」をコンパイル時に列挙する `frozen_regex` クラステンプレートを新規追加する。列挙結果は `frozen_map` のキーとして直接利用可能で、文字列→値のルックアップもコンパイル時解決できる。

## 設計方針

1. **既存 frozenchars は最小変更** — 新規追加ファイル `include/frozenchars/frozen_regex.hpp` と `test/test_frozen_regex.cpp`、umbrella `include/frozenchars.hpp` への1行追加のみ
2. **injamm 不使用** — 列挙ロジックは frozenchars の constexpr ユーティリティのみで実装
3. **frozen_map のキーとして直接使える配列を生成** — `to_frozen_map<V, v0, v1, ...>()` 変換関数を提供
4. **実装範囲は「選択肢＋文字クラス＋連結＋グループ」に限定** — 繰り返し・量指定子は対象外（無限言語になるため）

## 公開 API

```cpp
namespace frozenchars {

/// @brief デフォルトのドット文字集合 [a-zA-Z0-9_]
inline constexpr char default_dot_chars[] = {
  'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
  '0','1','2','3','4','5','6','7','8','9','_'
};

/// @brief 正規表現からマッチしうる文字列をコンパイル時に列挙するクラステンプレート
/// @tparam Pattern   正規表現パターン（FrozenString NTTP）
/// @tparam MaxStrings 列挙上限（超過すると consteval throw でコンパイルエラー、デフォルト 4096）
/// @tparam DotChars  ドット `.` がマッチする文字集合（FrozenString NTTP、デフォルトは [a-zA-Z0-9_]）
template <FrozenString Pattern,
          std::size_t MaxStrings = 4096,
          FrozenString DotChars = FrozenString{default_dot_chars}>
class frozen_regex {
public:
  static constexpr std::size_t max_length_v = /* パース時に決定 */;
  static constexpr std::size_t count_v      = /* パース時に決定 */;

  /// @brief 列挙結果を FrozenString 配列で取得（sort 済み・重複なし）
  static consteval auto enumerate() -> std::array<FrozenString<max_length_v + 1>, count_v>;

  /// @brief 文字列がマッチしうるか判定（O(log N) 二分探索）
  static constexpr auto contains(std::string_view s) noexcept -> bool;

  /// @brief sort 済みキー配列を取得（値の指定順序確認用）
  static constexpr auto keys() noexcept -> std::span<std::string_view const, count_v>;

  /// @brief frozen_map へ変換（値は sort 済みキー順に1:1で指定）
  template <typename V, V... Values>
  static consteval auto to_frozen_map() -> frozen_map<V, /* enumerated_keys_ */...>;
};

} // namespace frozenchars
```

## 利用例

```cpp
using namespace frozenchars::literals;

// 1. HTTPメソッド -> ステータスコード
auto m1 = frozen_regex<R"(GET|POST|PUT|DELETE)"_fs>::to_frozen_map<int, 200, 201, 204, 200>();

// 2. 環境名 -> URL（値の順序は sort 後のキー順: dev, prd, stg）
auto m2 = frozen_regex<R"(dev|stg|prd)"_fs>::to_frozen_map<std::string_view,
  "https://dev.example.com",
  "https://example.com",
  "https://stg.example.com">();

// 3. 単純なマッチング判定（frozen_map なし）
if (frozen_regex<R"(GET|POST)"_fs>::contains(method)) { /* ... */ }

// 4. 列挙結果の直接取得
auto keys = frozen_regex<R"(GET|POST|PUT)"_fs>::enumerate();
// keys = {"GET", "POST", "PUT"} (sort 済み)
```

## 内部設計

### アプローチ: consteval 再帰下降パーサー + std::vector 中間表現 → std::array 変換

AST を `std::vector<node>` で構築し、列挙エンジンが AST を辿って `std::vector<std::string>` に蓄積、最終的に `std::array<FrozenString<N+1>, Count>` に変換する。`std::vector` は P0784 で consteval 内使用可能（GCC 14+ で動作確認済み、CI 環境の Fedora 44/43/41 + GCC 16/15/14 で問題なし）。

### AST ノード型（detail/fregex 名前空間）

```cpp
enum class node_kind : std::uint8_t {
  literal,      ///< 単一文字
  dot,          ///< ドット（文字集合に展開）
  char_class,   ///< 文字クラス [abc] [a-z] [^abc]
  concat,       ///< 直列結合（子ノードの直積）
  alt,          ///< 選択（子ノードの和集合）
  group,        ///< グループ化（子と同じ）
};

struct node {
  node_kind kind;
  char literal_char;
  std::vector<char> char_set;
  std::vector<std::size_t> child_indices;
};

struct ast {
  std::vector<node> nodes;
  std::size_t root_index;
};
```

フラット配列 + 子インデックスで木を表現（ポインタ不要、consteval 内で安全）。

### パーサー（再帰下降、consteval）

```cpp
struct parser {
  std::string_view src;
  ast tree;

  static consteval auto parse(std::string_view pattern) -> ast;
  consteval auto parse_alt() -> std::size_t;         ///< alt = concat ('|' concat)*
  consteval auto parse_concat() -> std::size_t;      ///< concat = atom+
  consteval auto parse_atom() -> std::size_t;        ///< atom = literal | dot | char_class | group
  consteval auto parse_group() -> std::size_t;       ///< '(' alt ')'
  consteval auto parse_char_class() -> std::size_t;  ///< '[' ['^'] (char | char '-' char)+ ']'
  consteval auto parse_escape() -> char;             ///< \\ \. \[ \] \( \) \| \- \^
  consteval auto eof() const noexcept -> bool;
  [[noreturn]] consteval auto error(char const* msg) -> void;
};
```

### サポート構文

| 構文 | 例 | 挙動 |
|------|----|----|
| リテラル | `abc` | 各文字を LITERAL、CONCAT で結合 |
| 選択 | `a\|b` | ALT ノード、子=[a, b] |
| 文字クラス | `[a-z]` `[abc]` `[^a]` | CHAR_CLASS、`char_set` に展開 |
| グループ | `(ab\|cd)` | GROUP ノード、子=内部 ALT |
| ドット | `.` | DOT、`char_set` に DotChars を展開 |
| エスケープ | `\\` `\.` `\|` `\-` `\^` | リテラルとして解釈 |

### 非対応構文（consteval throw でコンパイルエラー）

- 量指定子 `+ * ? {n,m}` → `throw "frozen_regex: quantifiers not supported"`
- 先読み `(?=` `(?!`、後読み `(?<=` `(?<!` → throw
- キャプチャ `(?<name>` `(?P<name>` → throw
- 対応外エスケープ `\d` `\w` `\s` 等 → throw
- 文字クラス略記 `[[:alpha:]]` → throw
- 閉じてない `(` `[` → throw "unbalanced bracket"
- 空パターン `""` → throw "empty pattern"
- 空選択肢 `a|` `|b` `||` → throw "empty alternative"

### 文字クラスの範囲と否定

- `-` は範囲演算子（`[a-z]` は a〜z の26文字）。エスケープ `\-` でリテラル `-`。先頭・末尾の `-`（`[-a]` `[a-]`）は左辺または右辺が無い範囲指定とみなし文法エラー（リテラルとして使うには必ず `\-` とエスケープする）
- 否定クラス `[^abc]` は **DotChars を全文字空間とみなして差集合** を取る（`[^a]` は DotChars - {a}）。これにより否定クラスも有限列挙可能。`^` をリテラルとして使うには `\^` とエスケープする

### 列挙エンジン（consteval）

```cpp
template <std::size_t MaxStrings>
struct enumerator {
  ast const& tree;
  enumerate_result result;  ///< std::vector<std::string> + max_length

  static consteval auto run(ast const& tree) -> enumerate_result;
  consteval auto enumerate_node(std::size_t node_index) -> std::vector<std::string>;
  consteval auto cartesian_concat(std::vector<std::vector<std::string>> const& children)
    -> std::vector<std::string>;  ///< CONCAT の直積、各ステップで overflow check
  consteval auto merge_alt(std::vector<std::vector<std::string>> const& children)
    -> std::vector<std::string>;  ///< ALT の和集合
  consteval auto check_overflow(std::size_t current) const -> void;  ///< 超過で throw
};
```

### AST ノードごとの列挙動

| ASTノード | 列挙動 |
|---|---|
| LITERAL(c) | `{ std::string{c} }` （1要素） |
| DOT | char_set の各文字 c について `{ std::string{c} }` を生成 |
| CHAR_CLASS(set) | char_set の各文字 c について `{ std::string{c} }` を生成 |
| CONCAT(a, b, ...) | 子ノード列挙結果の直積（prefixes × suffixes）。結果数=Π(子の数) |
| ALT(a, b, ...) | 子ノード列挙結果の和集合。結果数=Σ(子の数) |
| GROUP(x) | 子ノードの結果をそのまま返す |

### CONCAT 直積の爆発対策

`check_overflow()` を各結合ステップで呼び出し、`MaxStrings` を超えたら即座に `throw` でコンパイルエラー。

### 重複除去とソート（最終段、finalize）

```cpp
consteval auto finalize(enumerate_result raw) -> enumerate_result {
  std::ranges::sort(raw.strings);
  raw.strings.erase(std::unique(raw.strings.begin(), raw.strings.end()), raw.strings.end());
  raw.max_length = std::ranges::max(raw.strings, {}, [](auto const& s) { return s.size(); }).size();
  if (raw.strings.size() > MaxStrings) throw "frozen_regex: enumeration exceeds MaxStrings";
  return raw;
}
```

### FrozenString 配列への変換

```cpp
template <std::size_t N, std::size_t Count>
consteval auto to_frozen_string_array(enumerate_result const& res)
  -> std::array<FrozenString<N + 1>, Count> {
  // 短い文字列は '\0' パディング、length メンバに実際の長さを設定
}
```

`FrozenString<N+1>` の `+1` は終端 `\0` 分（frozenchars 規約）。短い文字列は `buffer` 残りが `\0` で埋まる。

### frozen_map 統合（値→型昇格）

```cpp
template <FrozenString Pattern, std::size_t MaxStrings, FrozenString DotChars>
class frozen_regex {
  static constexpr auto raw_result_ = detail::fregex::finalize<MaxStrings>(...);
  static constexpr std::size_t max_length_v = raw_result_.max_length;
  static constexpr std::size_t count_v      = raw_result_.strings.size();

  static constexpr auto enumerated_keys_ =
    detail::fregex::to_frozen_string_array<max_length_v, count_v>(raw_result_);

  static constexpr auto key_views_ = [] {
    std::array<std::string_view, count_v> views{};
    for (auto i = 0uz; i < count_v; ++i) views[i] = enumerated_keys_[i].sv();
    return views;
  }();

public:
  static consteval auto enumerate() -> std::array<FrozenString<max_length_v + 1>, count_v>;
  static constexpr auto contains(std::string_view s) noexcept -> bool;
  static constexpr auto keys() noexcept -> std::span<std::string_view const, count_v>;

  template <typename V, V... Values>
  static consteval auto to_frozen_map() -> frozen_map<V, enumerated_keys_[Is]...> {
    static_assert(sizeof...(Values) == count_v,
                  "to_frozen_map requires exactly count_v values matching sorted key order");
    return to_frozen_map_impl<V, Values...>(std::make_index_sequence<count_v>{});
  }
};
```

`enumerated_keys_` が `static constexpr std::array` として保持されることで、`enumerated_keys_[Is]`（`Is` はテンプレートパラメータ）が定数式として評価可能。これを `frozen_map<V, Keys...>` の NTTP パックに展開する。

### 値の指定順序に関する制約

`finalize()` で `std::sort` を実行するため、列挙結果は**辞書順**。`to_frozen_map<V, v0, v1, ...>()` の `Values...` は sort 済み順序に1:1で対応。ユーザは `keys()` を呼び出して sort 済み順序を確認してから値を指定する。

```cpp
// パターン "POST|GET|PUT"
// 出現順: "POST", "GET", "PUT"
// sort 後: "GET", "POST", "PUT"
//                      ↑ この順序で値を指定
auto m = frozen_regex<R"(POST|GET|PUT)"_fs>::to_frozen_map<int, 201, 200, 204>();
```

## ファイル構成

```
include/frozenchars/frozen_regex.hpp   # 本体（新規）
test/test_frozen_regex.cpp             # テスト（新規）
include/frozenchars.hpp                # umbrella に1行追加（既存、1行のみ変更）
test/compile_fail/frozen_regex_*.cpp   # コンパイルエラー系テスト（新規、4ファイル予定）
test/CMakeLists.txt                    # compile_fail エントリ追加（既存、数行）
```

### `include/frozenchars/frozen_regex.hpp` の依存

- `frozenchars/string.hpp`（FrozenString）
- `frozenchars/map.hpp`（frozen_map）
- 他モジュール（ops, ctre 等）には依存しない

### umbrella ヘッダへの追加

`include/frozenchars.hpp` に1行追加（map.hpp と set.hpp の間など適切な位置）:

```cpp
#include "frozenchars/frozen_regex.hpp"
```

## テスト方針

### `test/test_frozen_regex.cpp`（Catch2 v3）

`test/CMakeLists.txt` の glob で自動的に `all_test` に組み込まれる（ファイル新設だけで登録完了）。

| カテゴリ | テスト内容 |
|---|---|
| 基本マッチング | `frozen_regex<"a"_fs>::contains("a")` が true、`"b"` が false |
| 選択 | `"GET\|POST"_fs` で両方マッチ、未知は false |
| 文字クラス | `"[abc]"_fs` で a/b/c マッチ、d は false |
| 文字クラス範囲 | `"[a-z]"_fs` で a-z 全マッチ、A は false |
| 否定クラス | `"[^a]"_fs` で b マッチ、a は false（DotChars 差集合） |
| ドット | `"."_fs` で a/A/0/_ マッチ、`-` は false |
| ドット文字集合変更 | DotChars NTTP 指定で挙動変化確認 |
| グループ | `"(ab\|cd)"_fs` で ab/cd マッチ |
| 連結 | `"a(b\|c)"_fs` で ab/ac マッチ |
| 重複除去 | `"a\|a"_fs` で count=1、contains("a")=true |
| 列挙数 | `"[a-z]"_fs::count_v == 26` |
| 列挙配列取得 | `enumerate()` の内容が期待通り |
| frozen_map 変換 | `to_frozen_map<int, 1, 2>()` でキー→値ルックアップ |
| エスケープ | `"a\.\.b"_fs` で `a..b` マッチ、`a.b` は false |

### `test/compile_fail/`（コンパイルエラー系）

既存の `run_compile_fail.cmake` 機構を利用。`test/CMakeLists.txt` に `assert_frozen_map_compile_fail(...)` エントリを追加。

追加ファイル予定:
- `frozen_regex_quantifier.cpp`（`"a+"_fs` で `EXPECTED_TEXT "quantifiers not supported"`）
- `frozen_regex_unbalanced_paren.cpp`
- `frozen_regex_empty_pattern.cpp`
- `frozen_regex_max_strings_exceeded.cpp`（小さい MaxStrings 指定で超過）

## CMake と CI

- **ルート `CMakeLists.txt`**: 変更不要。`cxx_std_23` 既存設定のまま。`frozen_regex.hpp` は INTERFACE ライブラリの一部として自動公開
- **`test/CMakeLists.txt`**: glob で `test_frozen_regex.cpp` が自動追加。compile_fail エントリを数行追加
- **CI（`.github/workflows/ci.yml`）**: 変更不要。Fedora 44/43/41 + GCC 16/15/14 で `std::vector` の consteval 内使用は動作する

## ビルド・テスト検証

```bash
bash build.sh && bash test.sh
```

## 制限事項（ドキュメント化対象）

- 量指定子（`+ * ? {n,m}`）は非対応（無限言語になるため）
- 先読み・後読みは非対応
- キャプチャグループは非対応（グループ化のみ）
- 列挙数上限 `MaxStrings`（デフォルト4096）→ 超過でコンパイルエラー
- 否定クラス `[^abc]` は DotChars を全文字空間とみなす（DotChars に含まれない文字はマッチしない）
- パターン内の空白はリテラル（標準 regex 互換）。拡張モードが必要なら利用者側で `uncomment` アダプタを適用してから渡す
- `to_frozen_map` の値は sort 済みキー順に1:1で指定する必要がある

## 決定履歴

| # | 決定事項 | 理由 |
|---|---------|------|
| 1 | ctre とは独立モジュール | ctre はマッチング、frozen_regex は列挙（逆操作） |
| 2 | `frozenchars/` 直下 + umbrella に1行追加 | プロジェクト慣例（1語・アンダースコア無し）に従う |
| 3 | ドット `[a-zA-Z0-9_]` + テンプレート変更可 | 仕様書通り、キー文字列列挙の用途に適する |
| 4 | `to_frozen_map<V, v0, ...>()` 呼び出し側が値1:1指定 | 柔軟性が最高、frozen_map の値型を限定しない |
| 5 | 列挙時の重複は除去して一意化 | frozen_map のキー一意制約に合致 |
| 6 | `max_strings` 超過は consteval 内 throw | 標準的な C++ 定数式エラー扱い |
| 7 | `FrozenString<N>` 配列 + `contains()` + `to_frozen_map()` | 仕様書通り、frozenchars の型体系に一貫 |
| 8 | `frozen_regex<R"(...)"_fs>` FrozenString NTTP | frozenchars の他モジュールと同じスタイル |
| 9 | 空白はリテラル（標準 regex 互換） | 拡張モードは `uncomment` アダプタに任せる |
| 10 | `-` は範囲演算子 | 標準 regex 互換、`[a-z]` で26文字展開 |
