# Inja Engine Fixes Design

**Goal:** `include/frozenchars/inja_engine.hpp` の既存公開 API を維持したまま、`else` チェーン判定、値文字列化の失敗処理、glaze 連携、関数抽出、変数パス分割、ホットパス最適化、ドキュメント補強を一括で整理し、貼り付けられた修正仕様を単一実装単位として着地させる。

**Scope:** 変更対象は `include/frozenchars/inja_engine.hpp` と回帰追加先の `test/test_template_vm.cpp` に限定する。`render`, `compile_template`, `extract_template_function_calls` のシグネチャは変更しない。C++26 前提で設計し、`std::inplace_vector` は feature-test macro で有効化する。

**Non-goals:** 旧 glaze 版との後方互換維持、新しい公開 API 追加、追加ベンチマーク基盤の導入、compile-fail ハーネス新設は行わない。

---

## Architecture

実装は 1 ファイル内で次の 3 ブロックに分ける。

1. **consteval パーサ / 関数抽出ブロック**
   - `node` に `is_plain_else` を追加し、`process_statement` の `else` 鎖判定をソース再読込からノード属性参照へ移す。
   - `process_statement` 冒頭に `stmt_offset` を集約し、`0uz` リテラルへ統一する。
   - `extract_template_function_calls` で `{# ... #}` コメントを明示スキップする。
   - 未使用の `always_false_v` を削除する。

2. **ランタイム変換 / lookup ブロック**
   - `append_value` の整数・浮動小数 `std::to_chars` 失敗を `render_error` にする。
   - glaze 連携の `glz::to_tie(const_cast<...>)` を `glz::to_tie(value)` に置き換え、現行 glaze 前提へ揃える。
   - `runtime_options_ref` と `runtime_options` の関連 API に Doxygen コメントを追加し、`std::cref` 利用を明示する。

3. **ホットパス最適化ブロック**
   - `split_variable_path` を小バッファ `path_segments` へ置き換える。
   - `lookup_in_object_root`, `lookup_in_typed_root`, `lookup_typed_object_view_root`, `resolve_local` などの呼び出し側を `path_segments` ベースへ追従させる。
   - `for` ループ内で `inja_array<std::vector<inja_value>>` の要素を再変換せず、そのまま `inja_value` として扱う。

---

## File Map

- **Modify:** `include/frozenchars/inja_engine.hpp`
  - `node`, `parse_program`, `append_value`, glaze lookup 群, `split_variable_path`, `render_program_with_lookup`, `runtime_options_ref` コメント
- **Modify:** `test/test_template_vm.cpp`
  - `else if` / `else` 分岐、コメント誤検出、`append_value` 失敗、`split_variable_path`、既存回帰の確認

---

## Detailed Design

### 1. consteval parser and extractor changes

`process_statement` は `stmt_offset` をラムダ先頭で一度だけ計算する。`else_stmt` ノード生成時に `stmt.starts_with("else if ")` でない場合だけ `is_plain_else = true` を立てる。既存の `else_index` 鎖構造は維持し、二重 `else` 検出だけを `tail_stmt == "else"` から `program.nodes[tail_else].is_plain_else` へ置換する。これにより begin/end インデックスからの再解釈をなくし、パース時に決定した意味情報だけを使う。

`extract_template_function_calls` は `expr_open` / `stmt_open` に加え `comment_open` を探索対象へ加える。`comment_open` が最小位置なら対応する `comment_close` まで `pos` を進め、コメント内部を `extract_calls_from_expr` に渡さない。これで `{# upper(x) #}` のようなコメントを関数呼び出しとして誤登録しない。

このブロックでは同時に `detail` 名前空間内の `std::size_t{0}` 初期化を `0uz` に寄せ、未使用 `always_false_v` を削除する。

### 2. runtime conversion and lookup changes

`append_value` では `std::int64_t` と `double` の両分岐で `std::to_chars` の戻り値 `ec` を必ず検査し、失敗時は `render_error` を投げる。成功時のみ `out.append` し、空文字列を黙って出力する経路をなくす。

glaze 有効時は `convert_reflectable`, `lookup_in_reflectable`, `lookup_typed_object_view_value`, `make_typed_object_view` 内の `const_cast` を削除し、`glz::to_tie(value)` の直接呼び出しへ統一する。今回の仕様では古い glaze との両立は目的外なので、互換レイヤーやフォールバック helper は持たない。

`runtime_options_ref` には `std::cref(opts)` を使う意図を Doxygen で明記する。合わせて `add_function`, `reserve_functions`, `add_include`, `reserve_includes` の利用意図も補強し、ランタイム拡張設定の使い方をヘッダ単体で追えるようにする。

### 3. path splitting and hot-path optimization

`split_variable_path` の戻り値は `std::vector<std::string_view>` から `path_segments` に変える。`path_segments` は最大 16 要素の小バッファ型とし、`__cpp_lib_inplace_vector >= 202406L` のときは `std::inplace_vector<std::string_view, 16>`、それ以外は `std::array<std::string_view, 16>` とサイズカウンタで実装する。17 要素目は `render_error{"variable path too deep (max 16 segments)"}` とする。

`path_segments` は `std::span<std::string_view const>` へ渡せるようにし、`join_segments`, `lookup_in_inja_value`, `lookup_in_typed_value`, `resolve_loop_property`, `resolve_local_binding` などの span ベース API は維持する。呼び出し側だけを `path_segments` 受け取り後に span 化する構成にし、関数境界の変更を最小化する。

`render_program_with_lookup` の `for` ループでは `std::visit` 内の要素型を見て、`inja_value` 要素なら `to_inja_value_or_throw` を通さず直接 `inja_value` を構築する。それ以外の配列要素は従来どおり変換する。

---

## Error Handling

- `else` の重複は consteval パース中に `throw "template parse error: unexpected else"` を維持する。
- `append_value` の数値変換失敗は `render_error` として呼び出し側へ伝播する。
- 変数パスの空セグメントは既存どおり `render_error{"empty identifier"}`、深さ超過は新規に `variable path too deep (max 16 segments)` とする。
- glaze 経由の lookup / object view は現行 API 前提のため、古い依存への救済分岐は持たない。

---

## Validation

回帰は `test/test_template_vm.cpp` に寄せ、次を確認する。

1. `else if` / `else` の正常分岐で `A`, `B`, `C` が期待どおり出る。
2. `{# upper(x) #}{{ x }}` に対して `extract_template_function_calls` が関数を誤検出しない。
3. `append_value` が `std::numeric_limits<double>::quiet_NaN()` で `render_error` を投げる。
4. `split_variable_path("a.b.c.d.e")` が 5 セグメントを返し、17 セグメント超過でエラーになる。
5. 既存の typed root / include / loop 回帰が崩れていない。

`else` 二重定義の compile-fail 確認は通常の実行時テストへ無理に組み込まず、ソース近傍コメントで保持する。

---

## Implementation Notes

- `#include <inplace_vector>` は feature-test macro を満たすときだけ導入する。
- `path_segments` は `detail` 名前空間内に閉じる。
- 公開 API のシグネチャ変更は行わない。
- 変更は `inja_engine.hpp` の責務内で完結させ、別ヘッダへの分割は行わない。
