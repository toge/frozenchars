# ワイルドカード NTTP マッチング設計

## 概要

`frozenchars` にワイルドカードパターンマッチングを追加。パターンを NTTP として受け取り、テキストがコンパイル時なら `consteval` で全評価、実行時ならコンパイル時前処理済みの高速マッチング関数を実行。

## 構文: `*` と `*` + `?`

| 構文 | 意味 | 例 |
|------|------|-----|
| `*` | 任意の文字列（空含む） | `a*c` → `abc`, `aXXXc` |
| `?` | 任意の1文字 | `a?c` → `abc`, `aXc` |
| `*?*` | 組み合わせ | `a*b?c` → `axbyc` |

エスケープ: 未定義（必要なら後から追加可能）。

## アーキテクチャ

### ファイル構成

```
include/frozenchars/wildcard.hpp     ← 新規（コア実装）
test/test_wildcard.cpp               ← 新規（テスト）
```

### 公開 API

```cpp
namespace frozenchars {

// テキストがコンパイル時定数の場合 → consteval で全評価
template <FrozenStringLike Pattern, FrozenStringLike Text>
consteval bool wildcard_match(Text const& text) → bool;

// テキストが実行時の場合 → コンパイル時前処理済みマッチング
template <FrozenStringLike Pattern>
bool wildcard_match(std::string_view text) → bool;

// pipe アダプター（テキストが実行時の場合）
namespace ops {
  template <FrozenStringLike Pattern>
  struct wildcard_adaptor { ... };

  template <FrozenStringLike Pattern>
  inline constexpr wildcard_adaptor<Pattern> wildcard{};
}

} // namespace frozenchars
```

### コンパイル時テキスト → consteval 全評価

テキストも `FrozenString` の場合、`consteval wildcard_match<Pattern>(text)` で全処理をコンパイル時に行える。

```cpp
auto constexpr result = frozenchars::wildcard_match<"a*c">("abc"_fs);
// result == true（コンパイル時に確定）
```

### 実行時テキスト → コンパイル時前処理 + 実行時マッチング

パターン `"a*c"` をコンパイル時にセグメントに分割:

```
segments = [
    { text: "aaa", length: 3, type: LITERAL },
    { text: "",     length: 0, type: WILDCARD_STAR },
    { text: "bbb", length: 3, type: LITERAL },
]
```

マッチングアルゴリズム:

1. 先頭セグメントが `LITERAL` → `text.starts_with(...)` でチェック
2. 末尾セグメントが `LITERAL` → `text.ends_with(...)` でチェック
3. 残りの `LITERAL` セグメントを順番に `find` で検索
4. `?` は1文字スキップ + セグメント継続

### パターン解析（コンパイル時）

```cpp
enum class seg_type { LITERAL, WILDCARD_STAR, WILDCARD_QUESTION };

struct segment {
    seg_type type;
    size_t offset;   // リテラルセグメントのバッファ内オフセット
    size_t length;   // リテラルの長さ
};

template <FrozenStringLike Pattern>
consteval auto parse_wildcard(Pattern const& p) {
    // パターン文字列を走査して segment 配列を生成
    // "*abc*" → [{STAR,0,0}, {LITERAL,"abc",3}, {STAR,0,0}]
    // "abc*def?ghi" → [{LITERAL,"abc",3}, {STAR}, {LITERAL,"def",3}, {QUESTION}, {LITERAL,"ghi",3}]
}
```

### マッチング実装（実行時テキスト版）

```cpp
template <FrozenStringLike Pattern>
bool wildcard_match(std::string_view text) {
    constexpr auto segments = parse_wildcard(Pattern{});
    constexpr auto NUM_SEGMENTS = segments.size();

    // セグメントが1個で LITERAL のみ → 単純比較
    if constexpr (NUM_SEGMENTS == 1 && segments[0].type == seg_type::LITERAL) {
        return text == Pattern{}.sv();
    }
    // 全体が STAR のみ → 常に true
    else if constexpr (NUM_SEGMENTS == 1 && segments[0].type == seg_type::WILDCARD_STAR) {
        return true;
    }
    else {
        return detail::wildcard_match_impl<segments>(text);
    }
}
```

### Early exit 最適化

- 先頭が `LITERAL` → `starts_with` チェック（不一致なら即 false）
- 末尾が `LITERAL` → `ends_with` チェック（不一致なら即 false）
- 残り文字数 < 残りリテラル文字数 → 即 false
- `?` の数 + `*` の数 = 最小一致長の計算

### `*` のマッチング戦略

`*` は「最短マッチ」ではなく「最長マッチ」（glob 標準）:

```
"a*b" against "axxbxb"
→ 最長マッチ: "axxbxb" の末尾 "b" にマッチ → true
```

実装: 残りのリテラルセグメントを `rfind` で後ろから探す。

## テスト計画

### 基本ケース
- `wildcard_match<"a*c">("abc")` → true
- `wildcard_match<"a*c">("axc")` → true
- `wildcard_match<"a*c">("ac")` → true（* は空も可）
- `wildcard_match<"a*c">("abxc")` → true
- `wildcard_match<"a*c">("ab")` → false

### `?` テスト
- `wildcard_match<"a?c">("abc")` → true
- `wildcard_match<"a?c">("ac")` → false（1文字必須）
- `wildcard_match<"a?c">("abxc")` → false（1文字超出）

### コンパイル時テキスト
- `constexpr auto r = wildcard_match<"a*c">("abc"_fs)` → true
- `static_assert(wildcard_match<"a*c">("abc"_fs))`

### エッジケース
- `wildcard_match<"*">("anything")` → true
- `wildcard_match<"">("")` → true
- `wildcard_match<"">("a")` → false
- `wildcard_match<"a">("a")` → true（ワイルドカードなし = 一致判定）
- パターン `**` → `*` と同等
- パターン `?*` → 最低1文字必須

### pipe アダプター
- `"hello world"_fs | ops::wildcard<"hello*">` → true

## 検証方法

1. `bash build.sh && bash test.sh` でテスト全 pass
2. ベンチマーク: `wildcard_match` vs `fnmatch` vs 手動ループの比較
3. コンパイル時間: パターン複雑度による増加を確認
