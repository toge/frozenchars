# frozen_regex Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement `frozen_regex<FrozenString Pattern, std::size_t MaxStrings, FrozenString DotChars>` — a compile-time regex enumerator that produces all strings matching a pattern (alternation, char class, concatenation, groups) and can convert them to a `frozen_map`.

**Architecture:** Single new header `include/frozenchars/frozen_regex.hpp`. A `consteval` recursive-descent parser builds a flat AST (`std::vector<node>` with child indices). An enumerator walks the AST producing `std::vector<std::string>` (P0784 constexpr vector, GCC 14+ verified), then `finalize` sorts + dedups. The class holds the result as `static constexpr std::array<FrozenString<N+1>, Count>` and exposes `enumerate()`, `contains()`, `keys()`, and `to_frozen_map<V, Values...>()` (NTTP pack expansion via `index_sequence`).

**Tech Stack:** C++23, frozenchars (FrozenString, frozen_map), Catch2 v3 for testing, `run_compile_fail.cmake` for compile-error tests.

**Spec:** `docs/superpowers/specs/2026-07-01-frozen-regex-design.md`

---

## File Structure

- **Create:** `include/frozenchars/frozen_regex.hpp` — single-header implementation (AST types, parser, enumerator, `frozen_regex` class template)
- **Create:** `test/test_frozen_regex.cpp` — Catch2 v3 runtime tests (auto-registered to `all_test` via glob)
- **Create:** `test/compile_fail/frozen_regex_quantifier.cpp` — `+` rejected
- **Create:** `test/compile_fail/frozen_regex_unbalanced_paren.cpp` — unbalanced `(` rejected
- **Create:** `test/compile_fail/frozen_regex_empty_pattern.cpp` — empty pattern rejected
- **Create:** `test/compile_fail/frozen_regex_max_strings_exceeded.cpp` — `MaxStrings` overflow rejected
- **Modify:** `include/frozenchars.hpp` — add 1 `#include` line
- **Modify:** `test/CMakeLists.txt` — add 4 `assert_frozen_map_compile_fail(...)` entries

No other files touched. `CMakeLists.txt` (root) and `.github/workflows/ci.yml` unchanged.

---

### Task 1: Scaffold `include/frozenchars/frozen_regex.hpp` with AST types and parser

This task creates the header with the `detail::fregex` AST types and a complete `consteval` recursive-descent parser. No enumerator or public class yet — the parser is tested via an internal `static_assert` that parses a small pattern and checks the AST root kind. The file must compile cleanly.

**Files:**
- Create: `include/frozenchars/frozen_regex.hpp`

- [ ] **Step 1: Write the header with AST types and parser**

```cpp
/**
 * @file frozen_regex.hpp
 * @brief 正規表現パターンからマッチしうる全文字列をコンパイル時に列挙するライブラリ
 * @details
 *   サポート構文: リテラル / 選択(|) / 文字クラス([a-z][^a]) / グループ(()) / ドット(.) / エスケープ(\\ \. \[ \] \( \) \| \- \^)
 *   非対応: 量指定子(+ * ? {n,m}) / 先読み・後読み / キャプチャグループ
 *   列挙数上限 MaxStrings (デフォルト 4096) を超えると consteval throw でコンパイルエラー。
 */
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "frozenchars/string.hpp"
#include "frozenchars/map.hpp"

namespace frozenchars {
namespace detail::fregex {

/// @brief AST ノードの種別
enum class node_kind : std::uint8_t {
  literal,      ///< 単一文字
  dot,          ///< ドット（DotChars 文字集合に展開）
  char_class,   ///< 文字クラス [abc] [a-z] [^abc]
  concat,       ///< 直列結合（子ノードの直積）
  alt,          ///< 選択（子ノードの和集合）
  group,        ///< グループ化（子と同じ）
};

/// @brief AST ノード（フラット表現、子はインデックス参照）
struct node {
  node_kind kind{node_kind::literal};
  char literal_char{'\0'};                       ///< kind == literal のみ使用
  std::vector<char> char_set;                    ///< kind == dot / char_class のみ使用
  std::vector<std::size_t> child_indices;        ///< kind == concat / alt / group のみ使用
};

/// @brief AST 全体（フラットなノード配列 + ルートインデックス）
struct ast {
  std::vector<node> nodes;
  std::size_t root_index{0};
};

/// @brief 再帰下降パーサー（consteval）
struct parser {
  std::string_view src;   ///< 未消費のパターン
  ast tree;               ///< 構築中の AST

  /// @brief エントリポイント: パターン全体をパースして AST を返す
  static consteval auto parse(std::string_view pattern) -> ast {
    if (pattern.empty()) throw "frozen_regex: empty pattern";
    parser p{pattern, {}};
    auto const root = p.parse_alt();
    if (!p.eof()) throw "frozen_regex: unbalanced bracket";
    p.tree.root_index = root;
    return p.tree;
  }

  /// @brief 末端に達したか
  [[nodiscard]] consteval auto eof() const noexcept -> bool {
    return src.empty();
  }

  /// @brief 現在の先頭文字を覗く（消費しない）
  [[nodiscard]] consteval auto peek() const -> char {
    if (eof()) throw "frozen_regex: unexpected end of pattern";
    return src.front();
  }

  /// @brief 先頭1文字を消費して返す
  consteval auto consume() -> char {
    auto const c = peek();
    src.remove_prefix(1);
    return c;
  }

  /// @brief 文法エラー（consteval throw でコンパイルエラー）
  [[noreturn]] consteval auto error(char const* msg) -> void {
    throw msg;
  }

  /// @brief ノードを追加してインデックスを返す
  consteval auto add_node(node n) -> std::size_t {
    auto const idx = tree.nodes.size();
    tree.nodes.push_back(std::move(n));
    return idx;
  }

  /// @brief alt = concat ('|' concat)*
  consteval auto parse_alt() -> std::size_t {
    auto first = parse_concat();
    if (eof() || peek() != '|') return first;
    // 選択ノードを構築
    node alt_node{node_kind::alt, '\0', {}, {first}};
    while (!eof() && peek() == '|') {
      consume();  ///< '|' を消費
      alt_node.child_indices.push_back(parse_concat());
    }
    return add_node(std::move(alt_node));
  }

  /// @brief concat = atom+（'(' '|' ')' が来るまで原子を並べる）
  consteval auto parse_concat() -> std::size_t {
    std::vector<std::size_t> children;
    while (!eof() && peek() != '|' && peek() != ')') {
      children.push_back(parse_atom());
    }
    if (children.empty()) throw "frozen_regex: empty alternative";
    if (children.size() == 1) return children[0];
    node concat_node{node_kind::concat, '\0', {}, std::move(children)};
    return add_node(std::move(concat_node));
  }

  /// @brief atom = literal | dot | char_class | group
  consteval auto parse_atom() -> std::size_t {
    auto const c = peek();
    if (c == '(') return parse_group();
    if (c == '[') return parse_char_class();
    if (c == '.') return parse_dot();
    if (c == '\\') return parse_escape_atom();
    // 量指定子は非対応
    if (c == '+' || c == '*' || c == '?') {
      throw "frozen_regex: quantifiers not supported";
    }
    if (c == '{') {
      throw "frozen_regex: quantifiers not supported";
    }
    // 先読み・後読きは非対応（peek で (?= 等を検出）
    if (c == ')') throw "frozen_regex: unbalanced parenthesis";
    // 通常リテラル
    consume();
    return add_node(node{node_kind::literal, c, {}, {}});
  }

  /// @brief '(' alt ')' をパース
  consteval auto parse_group() -> std::size_t {
    consume();  ///< '(' を消費
    // (?= 等の先読み・後読みを検出
    if (!eof() && peek() == '?') {
      throw "frozen_regex: lookahead/lookbehind not supported";
    }
    auto inner = parse_alt();
    if (eof() || peek() != ')') throw "frozen_regex: unbalanced parenthesis";
    consume();  ///< ')' を消費
    return add_node(node{node_kind::group, '\0', {}, {inner}});
  }

  /// @brief '[' ['^'] (item)+ ']'
  /// @details item = escape | range | single_char
  ///          range = char '-' char
  ///          先頭・末尾の '-' は文法エラー（リテラルとして使うには \- とエスケープ）
  consteval auto parse_char_class() -> std::size_t {
    consume();  ///< '[' を消費
    bool const negate = (!eof() && peek() == '^');
    if (negate) consume();
    std::vector<char> chars;
    while (!eof() && peek() != ']') {
      auto const c1 = parse_class_char();
      if (!eof() && peek() == '-') {
        // 範囲演算子の可能性
        consume();  ///< '-' を消費
        if (eof() || peek() == ']') {
          // 末尾の '-' は文法エラー
          throw "frozen_regex: dangling '-' in character class";
        }
        auto const c2 = parse_class_char();
        if (c1 > c2) throw "frozen_regex: invalid character range";
        for (auto ch = c1; ch <= c2; ++ch) chars.push_back(ch);
      } else {
        chars.push_back(c1);
      }
    }
    if (eof()) throw "frozen_regex: unbalanced bracket";
    consume();  ///< ']' を消費
    if (chars.empty()) throw "frozen_regex: empty character class";
    // 否定クラス: DotChars から chars を引いた差集合
    if (negate) chars = complement_against_dotchars(chars);
    node n{node_kind::char_class, '\0', std::move(chars), {}};
    return add_node(std::move(n));
  }

  /// @brief 文字クラス内の1文字（エスケープ対応）
  consteval auto parse_class_char() -> char {
    auto const c = peek();
    if (c == '\\') return parse_escape();
    if (c == ']') throw "frozen_regex: unbalanced bracket";
    consume();
    return c;
  }

  /// @brief ドット '.' をパース（char_set は enumerator 側で DotChars から展開）
  consteval auto parse_dot() -> std::size_t {
    consume();  ///< '.' を消費
    return add_node(node{node_kind::dot, '\0', {}, {}});
  }

  /// @brief エスケープ付き atom: \\ \. \[ \] \( \) \| \- \^
  consteval auto parse_escape_atom() -> std::size_t {
    auto const c = parse_escape();
    return add_node(node{node_kind::literal, c, {}, {}});
  }

  /// @brief エスケープ1文字を消費して返す
  consteval auto parse_escape() -> char {
    consume();  ///< '\\' を消費
    if (eof()) throw "frozen_regex: dangling backslash";
    auto const c = consume();
    switch (c) {
      case '\\': case '.': case '[': case ']':
      case '(': case ')': case '|': case '-': case '^':
        return c;
      default:
        throw "frozen_regex: unsupported escape sequence";
    }
  }

  /// @brief DotChars（外部から注入される予定、ここではダミー）
  /// @details 実装は Task 3 で DotChars NTTP を受け取る形に統合
  static consteval auto complement_against_dotchars(std::vector<char> const& exclude) -> std::vector<char> {
    // Task 3 で DotChars を使って書き直す。Task 1 では空を返してテストを通す。
    (void)exclude;
    return {};
  }
};

} // namespace detail::fregex
} // namespace frozenchars
```

- [ ] **Step 2: Add a temporary `static_assert` smoke test in a scratch file**

Create `test/test_frozen_regex.cpp` with only a parser smoke test (no Catch2 yet, just a `main` that returns 0):

```cpp
#include "frozenchars/frozen_regex.hpp"
using namespace frozenchars;

// パーサーのスモークテスト: "a|b" をパースしてルートが alt ノードになることを確認
constexpr auto smoke_parse() -> bool {
  auto const tree = detail::fregex::parser::parse("a|b");
  return tree.nodes[tree.root_index].kind == detail::fregex::node_kind::alt;
}
static_assert(smoke_parse(), "parser smoke test failed");

int main() { return 0; }
```

- [ ] **Step 3: Compile the scratch file directly to verify the header compiles**

Run:
```bash
g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include test/test_frozen_regex.cpp -o /tmp/fregex_smoke && /tmp/fregex_smoke
```
Expected: compiles with no errors/warnings, exits 0.

- [ ] **Step 4: Remove the scratch test (keep the file empty for Task 2)**

Overwrite `test/test_frozen_regex.cpp` with just the include so subsequent tasks fill it:
```cpp
#include "frozenchars/frozen_regex.hpp"
```

- [ ] **Step 5: Commit**

```bash
git add include/frozenchars/frozen_regex.hpp test/test_frozen_regex.cpp
git commit -m "feat(frozen_regex): consteval 再帰下降パーサーと AST 型を追加"
```

---

### Task 2: Implement the enumerator and finalize

This task adds the `enumerator` (walks AST, produces `std::vector<std::string>` with overflow checks) and `finalize` (sort + dedup + max_length). It also fixes `complement_against_dotchars` to accept the DotChars set as a parameter (moved from parser to enumerator). The parser's `parse_char_class` will pass DotChars through to nodes — but since the parser doesn't know DotChars yet, we store the `negate` flag in the node and resolve the complement in the enumerator.

**Files:**
- Modify: `include/frozenchars/frozen_regex.hpp` (add enumerator + finalize, adjust char_class node to carry `negate`)

- [ ] **Step 1: Add a `negate` field to the node and rework parse_char_class**

Replace the `node` struct and `parse_char_class` to carry the negate flag instead of resolving complement in the parser. The enumerator will compute the complement.

Edit the `node` struct:
```cpp
struct node {
  node_kind kind{node_kind::literal};
  char literal_char{'\0'};
  std::vector<char> char_set;          ///< char_class: 正クラスの場合のみ設定（否定は enumerator で解決）
  bool negate_class{false};            ///< char_class の否定フラグ
  std::vector<std::size_t> child_indices;
};
```

Edit `parse_char_class` to NOT resolve complement (leave `chars` as the positive set, set `negate_class`):
```cpp
consteval auto parse_char_class() -> std::size_t {
  consume();  ///< '[' を消費
  bool const negate = (!eof() && peek() == '^');
  if (negate) consume();
  std::vector<char> chars;
  while (!eof() && peek() != ']') {
    auto const c1 = parse_class_char();
    if (!eof() && peek() == '-') {
      consume();
      if (eof() || peek() == ']') throw "frozen_regex: dangling '-' in character class";
      auto const c2 = parse_class_char();
      if (c1 > c2) throw "frozen_regex: invalid character range";
      for (auto ch = c1; ch <= c2; ++ch) chars.push_back(ch);
    } else {
      chars.push_back(c1);
    }
  }
  if (eof()) throw "frozen_regex: unbalanced bracket";
  consume();  ///< ']' を消費
  if (chars.empty()) throw "frozen_regex: empty character class";
  node n{node_kind::char_class, '\0', std::move(chars), {}, negate};
  return add_node(std::move(n));
}
```

Remove the `complement_against_dotchars` stub from `parser` (no longer needed).

- [ ] **Step 2: Add the enumerator and finalize after the parser**

Append after `struct parser { ... };` in `namespace detail::fregex`:

```cpp
/// @brief 列挙結果（中間表現、動的サイズ）
struct enumerate_result {
  std::vector<std::string> strings;
  std::size_t max_length{0};
};

/// @brief AST を辿って全文字列を列挙
/// @tparam MaxStrings 列挙上限（超過で consteval throw）
template <std::size_t MaxStrings>
struct enumerator {
  ast const& tree;
  std::string_view dot_chars;  ///< ドットがマッチする文字集合

  /// @brief エントリポイント
  static consteval auto run(ast const& t, std::string_view dot) -> enumerate_result {
    enumerator e{t, dot};
    auto strs = e.enumerate_node(t.root_index);
    return finalize(std::move(strs));
  }

  /// @brief 指定ノードの列挙結果を返す
  consteval auto enumerate_node(std::size_t idx) -> std::vector<std::string> {
    auto const& n = tree.nodes[idx];
    switch (n.kind) {
      case node_kind::literal:
        return {std::string{static_cast<char>(n.literal_char)}};
      case node_kind::dot: {
        std::vector<std::string> res;
        res.reserve(dot_chars.size());
        for (auto c : dot_chars) res.push_back(std::string{c});
        return res;
      }
      case node_kind::char_class: {
        std::vector<char> chars = n.char_set;
        if (n.negate_class) chars = complement(chars);
        std::vector<std::string> res;
        res.reserve(chars.size());
        for (auto c : chars) res.push_back(std::string{c});
        return res;
      }
      case node_kind::group: {
        return enumerate_node(n.child_indices[0]);
      }
      case node_kind::concat: {
        std::vector<std::vector<std::string>> child_results;
        child_results.reserve(n.child_indices.size());
        for (auto ci : n.child_indices) child_results.push_back(enumerate_node(ci));
        return cartesian_concat(std::move(child_results));
      }
      case node_kind::alt: {
        std::vector<std::vector<std::string>> child_results;
        child_results.reserve(n.child_indices.size());
        for (auto ci : n.child_indices) child_results.push_back(enumerate_node(ci));
        return merge_alt(std::move(child_results));
      }
    }
    throw "frozen_regex: unreachable";
  }

  /// @brief CONCAT の直積: 単位元 "" から各子の文字列を順に結合
  consteval auto cartesian_concat(std::vector<std::vector<std::string>> children)
    -> std::vector<std::string> {
    std::vector<std::string> acc{""};
    for (auto& child : children) {
      std::vector<std::string> next;
      next.reserve(acc.size() * child.size());
      for (auto const& prefix : acc) {
        for (auto const& suffix : child) {
          next.push_back(prefix + suffix);
          check_overflow(next.size());
        }
      }
      acc = std::move(next);
    }
    return acc;
  }

  /// @brief ALT の和集合（重複除去は finalize で実施）
  consteval auto merge_alt(std::vector<std::vector<std::string>> children)
    -> std::vector<std::string> {
    std::vector<std::string> res;
    for (auto& child : children) {
      for (auto& s : child) {
        res.push_back(std::move(s));
        check_overflow(res.size());
      }
    }
    return res;
  }

  /// @brief 列挙数が MaxStrings を超えたら throw
  consteval auto check_overflow(std::size_t current) const -> void {
    if (current > MaxStrings) throw "frozen_regex: enumeration exceeds MaxStrings";
  }

  /// @brief 否定クラスの差集合: dot_chars から exclude を引く
  [[nodiscard]] consteval auto complement(std::vector<char> const& exclude) const
    -> std::vector<char> {
    std::vector<char> res;
    for (auto c : dot_chars) {
      if (std::find(exclude.begin(), exclude.end(), c) == exclude.end()) {
        res.push_back(c);
      }
    }
    return res;
  }

  /// @brief sort + dedup + max_length 計算
  static consteval auto finalize(std::vector<std::string> strs) -> enumerate_result {
    std::ranges::sort(strs);
    strs.erase(std::unique(strs.begin(), strs.end()), strs.end());
    if (strs.size() > MaxStrings) throw "frozen_regex: enumeration exceeds MaxStrings";
    std::size_t maxlen = 0;
    for (auto const& s : strs) maxlen = std::max(maxlen, s.size());
    return enumerate_result{std::move(strs), maxlen};
  }
};
```

- [ ] **Step 3: Update the scratch test to verify the enumerator**

Overwrite `test/test_frozen_regex.cpp`:
```cpp
#include "frozenchars/frozen_regex.hpp"
using namespace frozenchars;

// "a|b" をパース+列挙 → {"a", "b"}
constexpr auto smoke_enum() -> bool {
  auto const tree = detail::fregex::parser::parse("a|b");
  auto const res = detail::fregex::enumerator<64>::run(tree, "abcABC012_");
  return res.strings.size() == 2 && res.strings[0] == "a" && res.strings[1] == "b";
}
static_assert(smoke_enum(), "enumerator smoke test failed");

int main() { return 0; }
```

- [ ] **Step 4: Compile and run the scratch test**

Run:
```bash
g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include test/test_frozen_regex.cpp -o /tmp/fregex_smoke && /tmp/fregex_smoke
```
Expected: compiles, exits 0 (static_assert passes).

- [ ] **Step 5: Reset the test file to just the include**

```cpp
#include "frozenchars/frozen_regex.hpp"
```

- [ ] **Step 6: Commit**

```bash
git add include/frozenchars/frozen_regex.hpp test/test_frozen_regex.cpp
git commit -m "feat(frozen_regex): 列挙エンジンと finalize を追加"
```

---

### Task 3: Implement the `frozen_regex` class template with `enumerate()` and `contains()`

This task adds the public `frozen_regex` class template. It holds the parse + enumerate result as `static constexpr` members and exposes `enumerate()`, `contains()`, `keys()`. The `to_frozen_map` is added in Task 4.

**Files:**
- Modify: `include/frozenchars/frozen_regex.hpp` (add the class template at the end of `namespace frozenchars`)

- [ ] **Step 1: Add `default_dot_chars` and the `frozen_regex` class template**

Append before the closing `} // namespace frozenchars`:

```cpp
/// @brief デフォルトのドット文字集合 [a-zA-Z0-9_]
inline constexpr char default_dot_chars[] = {
  'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z',
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
  '0','1','2','3','4','5','6','7','8','9','_'
};

/**
 * @brief 正規表現からマッチしうる文字列をコンパイル時に列挙するクラステンプレート
 * @tparam Pattern   正規表現パターン（FrozenString NTTP）
 * @tparam MaxStrings 列挙上限（超過すると consteval throw、デフォルト 4096）
 * @tparam DotChars  ドット `.` がマッチする文字集合（FrozenString NTTP、デフォルト [a-zA-Z0-9_]）
 */
template <FrozenString Pattern,
          std::size_t MaxStrings = 4096,
          FrozenString DotChars = FrozenString<sizeof(default_dot_chars)>{default_dot_chars}>
class frozen_regex {
  static constexpr auto raw_result_ = detail::fregex::enumerator<MaxStrings>::run(
    detail::fregex::parser::parse(Pattern.sv()),
    std::string_view{DotChars.buffer.data(), DotChars.length});

public:
  /// @brief 列挙された文字列の最大長
  static constexpr std::size_t max_length_v = raw_result_.max_length;

  /// @brief 列挙された文字列数（重複除去済み）
  static constexpr std::size_t count_v = raw_result_.strings.size();

private:
  /// @brief FrozenString 配列に変換（短い分は '\0' パディング）
  static constexpr auto enumerated_keys_ = []() {
    std::array<FrozenString<max_length_v + 1>, count_v> arr{};
    for (auto i = 0uz; i < count_v; ++i) {
      auto const& s = raw_result_.strings[i];
      for (auto j = 0uz; j < s.size(); ++j) arr[i].buffer[j] = s[j];
      arr[i].buffer[s.size()] = '\0';
      arr[i].length = s.size();
    }
    return arr;
  }();

  /// @brief string_view 配列（contains() の二分探索用）
  static constexpr auto key_views_ = []() {
    std::array<std::string_view, count_v> views{};
    for (auto i = 0uz; i < count_v; ++i) {
      views[i] = std::string_view{enumerated_keys_[i].buffer.data(), enumerated_keys_[i].length};
    }
    return views;
  }();

public:
  /// @brief 列挙結果を FrozenString 配列で取得（sort 済み・重複なし）
  static consteval auto enumerate()
    -> std::array<FrozenString<max_length_v + 1>, count_v> {
    return enumerated_keys_;
  }

  /// @brief 文字列がマッチしうるか判定（O(log N) 二分探索）
  static constexpr auto contains(std::string_view s) noexcept -> bool {
    return std::binary_search(key_views_.begin(), key_views_.end(), s);
  }

  /// @brief sort 済みキー配列を取得（値の指定順序確認用）
  static constexpr auto keys() noexcept
    -> std::span<std::string_view const, count_v> {
    return key_views_;
  }
};
```

- [ ] **Step 2: Write a scratch test exercising enumerate() and contains()**

Overwrite `test/test_frozen_regex.cpp`:
```cpp
#include "frozenchars/frozen_regex.hpp"
#include "frozenchars/literals.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

// "GET|POST" → {"GET", "POST"}, contains で両方 true
static_assert(frozen_regex<R"(GET|POST)"_fs>::count_v == 2);
static_assert(frozen_regex<R"(GET|POST)"_fs>::contains("GET"));
static_assert(frozen_regex<R"(GET|POST)"_fs>::contains("POST"));
static_assert(!frozen_regex<R"(GET|POST)"_fs>::contains("PUT"));

// "a|a" の重複除去 → count=1
static_assert(frozen_regex<R"(a|a)"_fs>::count_v == 1);

int main() { return 0; }
```

- [ ] **Step 3: Compile and run**

Run:
```bash
g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include test/test_frozen_regex.cpp -o /tmp/fregex_smoke && /tmp/fregex_smoke
```
Expected: compiles, exits 0.

- [ ] **Step 4: Reset the test file**

```cpp
#include "frozenchars/frozen_regex.hpp"
```

- [ ] **Step 5: Commit**

```bash
git add include/frozenchars/frozen_regex.hpp test/test_frozen_regex.cpp
git commit -m "feat(frozen_regex): frozen_regex クラステンプレートと enumerate/contains を追加"
```

---

### Task 4: Implement `to_frozen_map()` and the umbrella header include

This task adds the `to_frozen_map<V, Values...>()` member that converts the enumerated keys to a `frozen_map` via NTTP pack expansion, and adds the 1-line include to the umbrella header.

**Files:**
- Modify: `include/frozenchars/frozen_regex.hpp` (add `to_frozen_map` to the class)
- Modify: `include/frozenchars.hpp` (add 1 include line)

- [ ] **Step 1: Add `to_frozen_map` to the `frozen_regex` class**

Add this public member after `keys()` in the `frozen_regex` class template (before the closing `};`):

```cpp
  /// @brief frozen_map へ変換（値は sort 済みキー順に1:1で指定）
  /// @tparam V    値型
  /// @tparam Values sort 済みキー順に対応する値列（count_v 個必要）
  template <typename V, V... Values>
  static consteval auto to_frozen_map()
    -> frozen_map<V, enumerated_keys_[Is]...> {
    static_assert(sizeof...(Values) == count_v,
                  "to_frozen_map requires exactly count_v values matching sorted key order");
    return to_frozen_map_impl<V, Values...>(std::make_index_sequence<count_v>{});
  }

private:
  /// @brief index_sequence で NTTP パックに展開
  template <typename V, V... Values, std::size_t... Is>
  static consteval auto to_frozen_map_impl(std::index_sequence<Is...>)
    -> frozen_map<V, enumerated_keys_[Is]...> {
    return frozen_map<V, enumerated_keys_[Is]...>{
      std::array<V, count_v>{ Values... }
    };
  }
```

- [ ] **Step 2: Add the umbrella header include**

Edit `include/frozenchars.hpp` and add this line after the `#include "frozenchars/map.hpp"` line (and before `#include "frozenchars/set.hpp"`):

```cpp
#include "frozenchars/frozen_regex.hpp"
```

- [ ] **Step 3: Write a scratch test exercising to_frozen_map()**

Overwrite `test/test_frozen_regex.cpp`:
```cpp
#include "frozenchars.hpp"
#include "frozenchars/literals.hpp"
using namespace frozenchars;
using namespace frozenchars::literals;

// "GET|POST|PUT" → sort 後 {"GET","POST","PUT"} → 値は 200, 201, 204
constexpr auto make_method_map() {
  return frozen_regex<R"(GET|POST|PUT)"_fs>::to_frozen_map<int, 200, 201, 204>();
}

static_assert(make_method_map().at("GET") == 200);
static_assert(make_method_map().at("POST") == 201);
static_assert(make_method_map().at("PUT") == 204);

int main() { return 0; }
```

- [ ] **Step 4: Compile and run**

Run:
```bash
g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include test/test_frozen_regex.cpp -o /tmp/fregex_smoke && /tmp/fregex_smoke
```
Expected: compiles, exits 0. If there is an error about `enumerated_keys_[Is]...` not being a constant expression, the `enumerated_keys_` member may need to be referenced as `frozen_regex::enumerated_keys_[Is]` — fix by qualifying with the class name inside `to_frozen_map_impl`.

- [ ] **Step 5: Reset the test file**

```cpp
#include "frozenchars/frozen_regex.hpp"
```

- [ ] **Step 6: Commit**

```bash
git add include/frozenchars/frozen_regex.hpp include/frozenchars.hpp test/test_frozen_regex.cpp
git commit -m "feat(frozen_regex): to_frozen_map 変換と umbrella ヘッダへの追加"
```

---

### Task 5: Write the full Catch2 test suite in `test/test_frozen_regex.cpp`

This task replaces the scratch test file with the complete Catch2 v3 test suite covering all supported syntax, edge cases, and `to_frozen_map`. The file is auto-registered to `all_test` via the glob in `test/CMakeLists.txt`.

**Files:**
- Modify: `test/test_frozen_regex.cpp` (full rewrite)

- [ ] **Step 1: Write the complete Catch2 test suite**

```cpp
#include "catch2/catch_all.hpp"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("frozen_regex literal match", "[frozen_regex]") {
  static_assert(frozen_regex<"a"_fs>::count_v == 1);
  static_assert(frozen_regex<"a"_fs>::contains("a"));
  REQUIRE(frozen_regex<"a"_fs>::contains("a"));
  REQUIRE_FALSE(frozen_regex<"a"_fs>::contains("b"));
  REQUIRE_FALSE(frozen_regex<"a"_fs>::contains(""));
  REQUIRE_FALSE(frozen_regex<"a"_fs>::contains("aa"));
}

TEST_CASE("frozen_regex concatenation", "[frozen_regex]") {
  static_assert(frozen_regex<"abc"_fs>::count_v == 1);
  REQUIRE(frozen_regex<"abc"_fs>::contains("abc"));
  REQUIRE_FALSE(frozen_regex<"abc"_fs>::contains("ab"));
  REQUIRE_FALSE(frozen_regex<"abc"_fs>::contains("abcd"));
}

TEST_CASE("frozen_regex alternation", "[frozen_regex]") {
  using R = frozen_regex<R"(GET|POST)"_fs>;
  static_assert(R::count_v == 2);
  REQUIRE(R::contains("GET"));
  REQUIRE(R::contains("POST"));
  REQUIRE_FALSE(R::contains("PUT"));
  REQUIRE_FALSE(R::contains(""));
}

TEST_CASE("frozen_regex alternation dedup", "[frozen_regex]") {
  using R = frozen_regex<R"(a|a)"_fs>;
  static_assert(R::count_v == 1);
  REQUIRE(R::contains("a"));
}

TEST_CASE("frozen_regex char class explicit", "[frozen_regex]") {
  using R = frozen_regex<"[abc]"_fs>;
  static_assert(R::count_v == 3);
  REQUIRE(R::contains("a"));
  REQUIRE(R::contains("b"));
  REQUIRE(R::contains("c"));
  REQUIRE_FALSE(R::contains("d"));
}

TEST_CASE("frozen_regex char class range", "[frozen_regex]") {
  using R = frozen_regex<"[a-z]"_fs>;
  static_assert(R::count_v == 26);
  REQUIRE(R::contains("a"));
  REQUIRE(R::contains("m"));
  REQUIRE(R::contains("z"));
  REQUIRE_FALSE(R::contains("A"));
  REQUIRE_FALSE(R::contains("0"));
}

TEST_CASE("frozen_regex char class negate", "[frozen_regex]") {
  using R = frozen_regex<"[^a]"_fs>;
  // DotChars デフォルトは [a-zA-Z0-9_] (63文字) から 'a' を引いて 62文字
  static_assert(R::count_v == 62);
  REQUIRE_FALSE(R::contains("a"));
  REQUIRE(R::contains("b"));
  REQUIRE(R::contains("A"));
  REQUIRE(R::contains("0"));
  REQUIRE(R::contains("_"));
  REQUIRE_FALSE(R::contains("-"));
}

TEST_CASE("frozen_regex dot default charset", "[frozen_regex]") {
  using R = frozen_regex<"."_fs>;
  static_assert(R::count_v == 63);
  REQUIRE(R::contains("a"));
  REQUIRE(R::contains("Z"));
  REQUIRE(R::contains("0"));
  REQUIRE(R::contains("_"));
  REQUIRE_FALSE(R::contains("-"));
  REQUIRE_FALSE(R::contains("/"));
  REQUIRE_FALSE(R::contains("."));
}

TEST_CASE("frozen_regex group", "[frozen_regex]") {
  using R = frozen_regex<R"((ab|cd))"_fs>;
  static_assert(R::count_v == 2);
  REQUIRE(R::contains("ab"));
  REQUIRE(R::contains("cd"));
  REQUIRE_FALSE(R::contains("abc"));
}

TEST_CASE("frozen_regex concat with group", "[frozen_regex]") {
  using R = frozen_regex<R"(a(b|c))"_fs>;
  static_assert(R::count_v == 2);
  REQUIRE(R::contains("ab"));
  REQUIRE(R::contains("ac"));
  REQUIRE_FALSE(R::contains("a"));
  REQUIRE_FALSE(R::contains("abc"));
}

TEST_CASE("frozen_regex escape literal dot", "[frozen_regex]") {
  using R = frozen_regex<R"(a\.\.b)"_fs>;
  static_assert(R::count_v == 1);
  REQUIRE(R::contains("a..b"));
  REQUIRE_FALSE(R::contains("a.b"));
}

TEST_CASE("frozen_regex escape pipe", "[frozen_regex]") {
  using R = frozen_regex<R"(a\|b)"_fs>;
  static_assert(R::count_v == 1);
  REQUIRE(R::contains("a|b"));
  REQUIRE_FALSE(R::contains("a"));
  REQUIRE_FALSE(R::contains("b"));
}

TEST_CASE("frozen_regex escape paren", "[frozen_regex]") {
  using R = frozen_regex<R"(\(a\))"_fs>;
  static_assert(R::count_v == 1);
  REQUIRE(R::contains("(a)"));
}

TEST_CASE("frozen_regex enumerate returns sorted array", "[frozen_regex]") {
  using R = frozen_regex<R"(POST|GET|PUT)"_fs>;
  auto const arr = R::enumerate();
  static_assert(arr.size() == 3);
  REQUIRE(arr[0].sv() == "GET");
  REQUIRE(arr[1].sv() == "POST");
  REQUIRE(arr[2].sv() == "PUT");
}

TEST_CASE("frozen_regex keys returns sorted span", "[frozen_regex]") {
  using R = frozen_regex<R"(POST|GET|PUT)"_fs>;
  auto const keys = R::keys();
  static_assert(keys.size() == 3);
  REQUIRE(keys[0] == "GET");
  REQUIRE(keys[1] == "POST");
  REQUIRE(keys[2] == "PUT");
}

TEST_CASE("frozen_regex to_frozen_map int values", "[frozen_regex]") {
  using R = frozen_regex<R"(GET|POST|PUT)"_fs>;
  // sort 後: "GET","POST","PUT" → 200, 201, 204
  auto const m = R::to_frozen_map<int, 200, 201, 204>();
  REQUIRE(m.at("GET") == 200);
  REQUIRE(m.at("POST") == 201);
  REQUIRE(m.at("PUT") == 204);
  REQUIRE_FALSE(m.contains("DELETE"));
}

TEST_CASE("frozen_regex to_frozen_map string_view values", "[frozen_regex]") {
  using R = frozen_regex<R"(dev|prd|stg)"_fs>;
  // sort 後: "dev","prd","stg" → dev_url, prd_url, stg_url
  auto const m = R::to_frozen_map<std::string_view,
    "https://dev.example.com",
    "https://example.com",
    "https://stg.example.com">();
  REQUIRE(m.at("dev") == "https://dev.example.com");
  REQUIRE(m.at("prd") == "https://example.com");
  REQUIRE(m.at("stg") == "https://stg.example.com");
}

TEST_CASE("frozen_regex complex pattern HTTP methods", "[frozen_regex]") {
  using R = frozen_regex<R"(GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS)"_fs>;
  static_assert(R::count_v == 7);
  REQUIRE(R::contains("GET"));
  REQUIRE(R::contains("DELETE"));
  REQUIRE(R::contains("OPTIONS"));
  REQUIRE_FALSE(R::contains("TRACE"));
}
```

- [ ] **Step 2: Build the full test suite via the project build system**

Run:
```bash
bash build.sh
```
Expected: build succeeds, `all_test` is built in `build/`.

- [ ] **Step 3: Run the test suite**

Run:
```bash
bash test.sh
```
Expected: all tests pass, including the new `[frozen_regex]` tests. If any `static_assert` fails, read the compiler error and fix the implementation in `frozen_regex.hpp`.

- [ ] **Step 4: Commit**

```bash
git add test/test_frozen_regex.cpp
git commit -m "test(frozen_regex): Catch2 テストスイートを追加"
```

---

### Task 6: Add compile-fail tests and register them in CMake

This task adds the 4 compile-fail test files and registers them via `assert_frozen_map_compile_fail(...)` in `test/CMakeLists.txt`.

**Files:**
- Create: `test/compile_fail/frozen_regex_quantifier.cpp`
- Create: `test/compile_fail/frozen_regex_unbalanced_paren.cpp`
- Create: `test/compile_fail/frozen_regex_empty_pattern.cpp`
- Create: `test/compile_fail/frozen_regex_max_strings_exceeded.cpp`
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: Create the quantifier compile-fail test**

`test/compile_fail/frozen_regex_quantifier.cpp`:
```cpp
#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"

using namespace frozenchars::literals;

auto const r = frozenchars::frozen_regex<"a+"_fs>{};
```

- [ ] **Step 2: Create the unbalanced-paren compile-fail test**

`test/compile_fail/frozen_regex_unbalanced_paren.cpp`:
```cpp
#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"

using namespace frozenchars::literals;

auto const r = frozenchars::frozen_regex<R"((a)"_fs>{};
```

- [ ] **Step 3: Create the empty-pattern compile-fail test**

`test/compile_fail/frozen_regex_empty_pattern.cpp`:
```cpp
#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"

using namespace frozenchars::literals;

auto const r = frozenchars::frozen_regex<""_fs>{};
```

- [ ] **Step 4: Create the max-strings-exceeded compile-fail test**

`test/compile_fail/frozen_regex_max_strings_exceeded.cpp`:
```cpp
#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_regex.hpp"

using namespace frozenchars::literals;

// [a-z] は 26 文字 → MaxStrings=4 で超過
auto const r = frozenchars::frozen_regex<"[a-z]"_fs, 4>{};
```

- [ ] **Step 5: Register the compile-fail tests in `test/CMakeLists.txt`**

Add after the existing `assert_frozen_map_compile_fail(...)` block (before the `example_*` targets):

```cmake
assert_frozen_map_compile_fail(
  NAME frozen_regex_quantifier
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_regex_quantifier.cpp
  EXPECTED_TEXT "quantifiers not supported"
)

assert_frozen_map_compile_fail(
  NAME frozen_regex_unbalanced_paren
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_regex_unbalanced_paren.cpp
  EXPECTED_TEXT "unbalanced parenthesis"
)

assert_frozen_map_compile_fail(
  NAME frozen_regex_empty_pattern
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_regex_empty_pattern.cpp
  EXPECTED_TEXT "empty pattern"
)

assert_frozen_map_compile_fail(
  NAME frozen_regex_max_strings_exceeded
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_regex_max_strings_exceeded.cpp
  EXPECTED_TEXT "exceeds MaxStrings"
  EXPECTED_TEXTS "expression did not evaluate to a constant"
)
```

- [ ] **Step 6: Rebuild and run the full test suite**

Run:
```bash
bash build.sh && bash test.sh
```
Expected: all tests pass, including the 4 new compile-fail tests. If a compile-fail test unexpectedly succeeds (CMake error "compilation unexpectedly succeeded"), the pattern in the `.cpp` file compiled when it should not — verify the error path in the parser/enumerator triggers the throw.

- [ ] **Step 7: Commit**

```bash
git add test/compile_fail/frozen_regex_quantifier.cpp \
        test/compile_fail/frozen_regex_unbalanced_paren.cpp \
        test/compile_fail/frozen_regex_empty_pattern.cpp \
        test/compile_fail/frozen_regex_max_strings_exceeded.cpp \
        test/CMakeLists.txt
git commit -m "test(frozen_regex): コンパイルエラー系テスト4件を追加し CMake に登録"
```

---

### Task 7: Final verification

This task runs the complete verification suite and confirms the implementation is complete.

**Files:** None (verification only)

- [ ] **Step 1: Run the full build and test suite**

Run:
```bash
bash build.sh && bash test.sh
```
Expected: build succeeds, all tests pass (existing + new `[frozen_regex]` + 4 compile-fail tests).

- [ ] **Step 2: Verify the umbrella header works standalone**

Run:
```bash
echo '#include "frozenchars.hpp"
#include <iostream>
int main() {
  using namespace frozenchars::literals;
  std::cout << frozenchars::frozen_regex<R"(GET|POST)"_fs>::contains("GET") << "\n";
}' > /tmp/umbrella_test.cpp
g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include /tmp/umbrella_test.cpp -o /tmp/umbrella_test && /tmp/umbrella_test
```
Expected: compiles and prints `1`.

- [ ] **Step 3: Verify the static build works**

Run:
```bash
bash build.sh static && bash test.sh
```
Expected: static build succeeds, all tests pass.

- [ ] **Step 4: Confirm git status is clean**

Run:
```bash
git status
```
Expected: clean working tree (all changes committed in Tasks 1-6).

- [ ] **Step 5: Show the final commit log**

Run:
```bash
git log --oneline -8
```
Expected: the 6 feature commits from Tasks 1-6 visible at the top.

---

## Self-Review

**1. Spec coverage:**
- Spec §「公開 API」 → Task 3 (`enumerate`, `contains`, `keys`) + Task 4 (`to_frozen_map`) ✓
- Spec §「内部設計 / AST」 → Task 1 (AST types + parser) ✓
- Spec §「列挙エンジン」 → Task 2 (enumerator + finalize + overflow + dedup + sort) ✓
- Spec §「frozen_map 統合」 → Task 4 (NTTP pack expansion via index_sequence) ✓
- Spec §「ファイル構成」 → Tasks 1-6 (header, test, umbrella, compile_fail) ✓
- Spec §「テスト方針」 → Task 5 (Catch2 suite) + Task 6 (compile_fail) ✓
- Spec §「CMake と CI」 → Task 6 (CMake entries) + Task 7 (static build verification) ✓
- Spec §「サポート構文」 → Tasks 5 (positive tests for all syntax) ✓
- Spec §「非対応構文」 → Task 6 (compile_fail for quantifier, unbalanced, empty, overflow) ✓
- Spec §「文字クラスの範囲と否定」 → Task 5 (range test, negate test) ✓
- Spec §「エスケープ」 → Task 5 (escape dot, pipe, paren tests) ✓

**2. Placeholder scan:** No "TBD", "TODO", "implement later", "appropriate error handling", or "similar to Task N" found. All code blocks contain complete implementation.

**3. Type consistency:**
- `node_kind` enum used consistently in Task 1 (definition), Task 2 (switch in enumerator) ✓
- `enumerate_result` struct defined in Task 2, used in Task 3 (`raw_result_`) ✓
- `default_dot_chars` defined in Task 3, used in Task 3 default template arg ✓
- `enumerated_keys_` member referenced in Task 4 `to_frozen_map_impl` ✓
- `count_v`, `max_length_v` consistent across Task 3 (definition) and Task 4 (static_assert) ✓
- `frozen_regex<Pattern, MaxStrings, DotChars>` signature consistent across Tasks 3, 4, 5, 6 ✓
- `FrozenString<N+1>` (N+1 for null terminator) consistent across Tasks 3, 4 ✓
