# Changelog

このプロジェクトの主要な変更点を記録します。
形式は [Keep a Changelog](https://keepachangelog.com/ja/1.1.0/) に準拠し、
バージョニングは [Semantic Versioning](https://semver.org/lang/ja/) に従います。

## [Unreleased]

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

[Unreleased]: https://github.com/toge/frozenchars/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/toge/frozenchars/releases/tag/v0.1.0
