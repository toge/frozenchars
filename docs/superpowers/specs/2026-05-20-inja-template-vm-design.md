# inja互換コア構文テンプレートエンジン設計（FrozenString NTTP + constexpr Bytecode VM）

## 1. 目的

injaと同系のテンプレート言語（コア構文）を解釈できるテンプレートエンジンを実装する。

- 対象構文: `{{ 式 }}`, `{% if %}...{% else %}...{% endif %}`, `{% for %}...{% endfor %}`, `{# コメント #}`
- 非対象: `include`, `set`, `callbacks`, 継承などの拡張機能
- テンプレートは `FrozenString` で定義し、NTTPで渡す
- パースは `consteval/constexpr` で完了
- 値の受け取りと文字列生成は実行時に行う

## 2. 全体アーキテクチャ

エントリポイントを `template_vm<Src>` とする。

- `Src`: `FrozenString` NTTP
- コンパイル時処理: `Src -> Token列 -> AST -> Bytecode`
- 実行時処理: `template_vm<Src>::render(Value const&) -> std::string`

この構成により、テンプレート構文の妥当性はビルド時に保証し、レンダリングだけを実行時に実行する。

## 3. コンポーネント分割

1. `include/frozenchars/template_value.hpp`  
   - `std::variant` ベースの値モデル（Null/Bool/Number/String/Array/Object）
2. `include/frozenchars/template_lexer.hpp`  
   - `{{ }}`, `{% %}`, `{# #}` の区間抽出とトークン化
3. `include/frozenchars/template_ast.hpp`  
   - ノード定義（Text/Expr/If/For/Block）
4. `include/frozenchars/template_parser.hpp`  
   - トークン列からネスト構造（if/else/for）を構築
5. `include/frozenchars/template_bytecode.hpp`  
   - ASTを命令列へ変換、ジャンプ先解決
6. `include/frozenchars/template_runtime.hpp`  
   - Bytecode実行器、式評価器、文字列出力器
7. `include/frozenchars/template_engine.hpp`  
   - `template_vm<Src>` 公開API

`include/frozenchars.hpp` から `template_engine.hpp` を公開する。

## 4. Bytecode設計

### 4.1 命令セット（最小）

- `EMIT_TEXT const_id`
- `EMIT_VALUE`
- `LOAD_VAR path_id`
- `LOAD_LITERAL const_id`
- `EVAL_UNARY op`
- `EVAL_BINARY op`
- `JUMP addr`
- `JUMP_IF_FALSE addr`
- `ITER_BEGIN`
- `ITER_NEXT addr_end`
- `ITER_END`

### 4.2 制御構造

- `if`: 条件式評価後 `JUMP_IF_FALSE` で else 先頭へ分岐、then末尾に `JUMP` を置いて endif の先へ飛ばす
- `for`: `ITER_BEGIN` で反復子フレーム作成、`ITER_NEXT` で終端判定と本文反復

## 5. 値モデル（実行時）

`template_value` は `std::variant` で表現する。

- `null`
- `bool`
- `std::int64_t`
- `double`
- `std::string`
- `std::vector<template_value>`
- `std::unordered_map<std::string, template_value>`

補助:

- パス解決: `a.b.c` を object チェーンとして解決
- 真偽値判定（inja寄り）:
  - false: null / false / 0 / 0.0 / empty string / empty array / empty object
  - true: 上記以外

## 6. 式サブセット

初期実装でサポートする演算:

- 単項: `not`, unary `-`
- 二項: `+ - * / %`, `== != < <= > >=`, `and or`, `in`
- 括弧: `(...)`
- 変数参照: `name`, `obj.key`
- リテラル: 数値、文字列、真偽値、null

優先順位は inja/Jinja系の一般的優先順に合わせる。

## 7. エラーハンドリング

### 7.1 コンパイル時

- タグ未閉鎖
- `endif` / `endfor` 不整合
- 不正な式構文

これらは `consteval` 中の `static_assert` で停止させる。

### 7.2 実行時

- 未定義変数
- 不正型演算
- `for` 対象が配列/オブジェクトでない

`template_render_error` を送出し、サイレントフォールバックは行わない。

## 8. テスト方針

`test/test_template_vm.cpp` を追加し、以下を検証する。

1. コンパイル時計算
   - 正常: if/for/comment、ネスト、演算子優先順位
   - 異常: 閉じタグ不一致、構文不正
2. 実行時レンダリング
   - 変数展開、if分岐、for展開、コメント除去
3. 実行時エラー
   - 未定義変数、型不一致、反復不能値
4. 互換ケース
   - inja READMEのコア構文代表例を移植

## 9. 公開API案

```cpp
template <auto Src>
struct template_vm {
  static auto render(template_value const& root) -> std::string;
};
```

補助関数:

```cpp
template <auto Src>
auto render_template(template_value const& root) -> std::string;
```

## 10. 受け入れ基準

- `FrozenString` NTTPでテンプレート定義できる
- パースはコンパイル時に完了する
- 実行時に値を渡して `std::string` を生成できる
- injaコア構文（式/if/for/comment）のサンプルが通る
- 失敗時にコンパイル時/実行時のエラー境界が明確
