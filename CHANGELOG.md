# Changelog

このプロジェクトの主要な変更点を記録します。
形式は [Keep a Changelog](https://keepachangelog.com/ja/1.1.0/) に準拠し、
バージョニングは [Semantic Versioning](https://semver.org/lang/ja/) に従います。

## [Unreleased]

### Changed

- **例外なしを既定化（glaze追従、破壊的変更）**: 実行時APIは一律 `std::expected<T, std::errc>` を返すようになり、`-fno-exceptions` でビルドできるようになった。`FROZENCHARS_THROW` / `FROZENCHARS_WASI_MINIMAL` / `ENABLE_WASI_MINIMAL` / `single_include/frozenchars_wasi_minimal.hpp` / `test/smoke_wasi_minimal.cpp` を廃止。`frozen_map` 等の `at()` は `expected<reference_wrapper<T>, errc>`、`operator[]` と `FrozenString::front/back/operator[]` はチェックなし（事前条件）になった。投げるコンストラクタは `try_make` ファクトリに分離。コンパイル時（NTTP）の不正入力は従来どおりコンパイルエラー。
- **例外なしモードを実装**: `FROZENCHARS_WASI_MINIMAL` 定義時、ライブラリ内の全ての例外送出が `FROZENCHARS_THROW` マクロ経由で `std::abort()` になり、`-fno-exceptions` でビルドできるようになった。従来は `FrozenString::front()/back()/operator[]` の 3 箇所しか無効化されていなかった。
- `minify_html`: インライン要素（`<b>` / `<span>` / `<a>` 等）とテキストの間の空白を 1 個残すようになった（従来は削除して単語が連結していた）。`{{name}}` はインラインテキスト扱い、`{{#sec}}` / `{{/sec}}` は構造タグ扱い。
- `minify_html`: `</p>` の省略を、直後がブロック要素・親の終了・入力終端のときに限定した（インライン要素やテキストが続くと DOM が変わるため）。
- `tools/amalgamate.py`: 出力を決定的にした（生成時刻を削除）。単一ヘッダのリポジトリ URL を修正。
- `CMakeLists.txt` のバージョンを git タグと一致する 0.7.0 に更新。

### Fixed

- `freeze(char)` / `concat(..., 'a')` が文字をコードポイント（`"97"`）に変換していた。
- `wildcard_match`: `[!...]` 否定クラスが英数字以外にマッチしなかった（frozen_regex 委譲の補集合が `DotChars` 限定のため）。`{` `}` `+` `^` `$` を含む `*` なしパターンがコンパイルエラーになっていた。
- `wildcard_match`: サフィックス早期判定が中間部と同じ文字を二重に使い、`[ab]x*x` が `"ax"` にマッチしていた。
- `minify_sql` / `minify_lua`: `a - -b` を `a--b`（行コメント）に融合していた。`minify_lua` は `1 .. y` を不正な数値リテラル `1..y` にしていた。
- `minify_html`: テキスト中の `'` / `"` で引用符状態に入り、以降の空白・コメント・タグが未処理で素通しになっていた。
- `minify_html`: `<pre>` / `<textarea>` の内容の空白を畳んでいた。

- `json::crush` の辞書領域が後続の置換で破壊される問題を修正（データ領域限定の置換に変更）。
- `json::crush` が置換を行わない（圧縮が機能しない）問題を修正。

### Added

- `json::decompress` / `json::uncrush`: コンパイル時 JSON 圧縮（`compress` / `crush`）の復元 API。
- `json::compress` の出力を有効な JSON に変更。数値の原文（小数・指数）と型情報を保持。

## [0.2.0] – [0.7.0]

タグのみ付与され、本ファイルへの記載がありません。差分は各タグ間の `git log` を参照してください。

## [0.1.0]

初回リリース。ヘッダオンリー C++23 コンパイル時文字列ライブラリ。

### Added

- コア型 `FrozenString<N>`（`N` は終端 `'\0'` を含むバッファ長）と `_fs` ユーザー定義リテラル。
- 数値変換: `parse_number` / `freeze`（整数・浮動小数点、Bin/Oct/Hex/精度指定タグ）。
- 文字列操作: `concat` / `partition` / `trim` / surround、`split`、`case_conv`（snake ⇄ camel/Pascal、Capitalize）。
- エンコーディング: hex / base64 / URL / HTML の encode-decode、UTF-8 コードポイント数、`make_querystring`。
- フォーマット: `to_sv`、型安全な `frozen_format`（引数個数の静的検査つき）。
- コンパイル時コンテナ: `frozen_map` / `frozen_set` / `trie_set` / `trie_map` / `trie_index`、配列からのマップ生成。
- 正規表現: `frozen_regex`（コンパイル時エンジン）、CTRE アダプタ、`regex_comment`（コメント除去）、`wildcard`（glob マッチ）。
- テキスト変換: `multiline`（結合・整形・コメント除去）、`minify`（JSON/HTML）、`json/compress`・`json/crush`。
- 日時: `chrono`（`__DATE__` / ISO 8601 / タイムゾーンオフセットのコンパイル時解析）。
- 型生成パーサ: `parse_to_tuple` / `parse_to_variant`。
- その他: `path` ヘルパ、`color`（ANSI カラー）、パイプ演算子アダプタ（`ops`）。
- 任意連携: glaze 検出時に `glaze_frozen_map` ブリッジを有効化。
- ビルド: CMake `INTERFACE` ターゲット `frozenchars::frozenchars`、`install` / `find_package(frozenchars CONFIG)` / export 対応。
- CI: GCC 16/15/14、Clang 22-19、MSVC、macOS、Emscripten (wasm32) でのビルド・テスト。

### Known Issues

- 一部機能に既知の不具合が残存しています（詳細は Issue を参照）。

[Unreleased]: https://github.com/toge/frozenchars/compare/v0.7.0...HEAD
[0.7.0]: https://github.com/toge/frozenchars/compare/v0.2.0...v0.7.0
[0.2.0]: https://github.com/toge/frozenchars/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/toge/frozenchars/releases/tag/v0.1.0
