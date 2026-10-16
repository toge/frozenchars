# Changelog

このプロジェクトの主要な変更点を記録します。
形式は [Keep a Changelog](https://keepachangelog.com/ja/1.1.0/) に準拠し、
バージョニングは [Semantic Versioning](https://semver.org/lang/ja/) に従います。

## [Unreleased]

### Changed

- CI: vcpkg をキャッシュから復元した場合は `git fetch` + `git reset --hard origin/master` と再 bootstrap で最新に同期するようにした。キャッシュされた古いクローンには `vcpkg.json` の `builtin-baseline` が指すコミットやバージョン DB が無く、また未追跡ファイルとの衝突で `pull` が失敗していたため（Windows は `|| exit /b 1` で失敗を検知する）。
- `.clang-format`: 既存コードの体裁に合わせて設定し直した（名前空間はフラット、縦位置揃えなし、行末コメント1スペース、`Standard: c++20`、`AccessModifierOffset: -2`）。リポジトリ全体は一括整形されていないため、CI の clang-format ゲートは変更された行のみを検査する。検査には clang-format 22.1.8 を使う。
- `.clang-tidy`: `readability-identifier-naming.FunctionCase` を `camelBack` から `lower_case` に変更（既存コードの大半が snake_case）。
- `AGENTS.md`: README の行数・サンプルパス・命名規約を現状に合わせて更新。
- `CMakeLists.txt`: バージョンを最新のタグに合わせて 0.8.1 に更新。警告オプションを `$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:...>` の複数ID形式に集約して重複を解消（-Wall/-Wextra/-pedantic は GCC・Clang・AppleClang 共通、-fconstexpr-steps は Clang 系共通）。
- `README.md`: 目次124エントリのアンカーを再生成（死んでいたリンク63件）。コード例88件がすべてコンパイルできるように修正。`vcpkg` の記述・単一ヘッダのサイズ表記・要件のコンパイラ一覧を更新。「よくある落とし穴」に `FrozenString::front()` / `back()` / `operator[]` の範囲外アクセス（チェックなし・未定義動作）と回避方法を追記。
- CI: README の一貫性を検証する `docs and style` ジョブを追加（目次アンカー / コード例のコンパイル / clang-format 22.1.8 による変更行の書式検査）。
- CI: `docs and style` の README コード例は対応下限の `g++-14` でコンパイル検証する。Ubuntu 既定の g++ 13 は `std::forward_like` など C++23 の一部が無く全ブロックが失敗するため。`tools/check_readme_examples.py` は環境変数 `CXX` でコンパイラを差し替えできるようにした。

- `CMakeLists.txt`: GCC に `-fconstexpr-ops-limit` / `-fconstexpr-loop-limit` を追加。`json::crush` / `json::compress` は 180 バイト程度の JSON で GCC の既定の constexpr 上限を超え、`crush` がコンパイルエラーになっていた（Clang 系は既に `-fconstexpr-steps` を指定していた）。`json::crush` を使う場合は GCC でも同じフラグが必要。
- `test/CMakeLists.txt`: ベンチマーク用の `wildcards` 依存を `master` ではなくコミット（5662507f）で固定し再現可能にした。パッチ対象が見つからない場合は `FATAL_ERROR` で検出するようにした（従来は `string(REPLACE)` が静かに空振りし、壊れたベンチのビルドが通っていた）。`nanobench` がある構成時のみ取得するようにした。
- `vcpkg.json`: `builtin-baseline` を追加し、ポートの解決を再現可能にした。

- `wildcard_match` / `wildcard_find`: `*` の後ろに `?` / `[` / `(` / `\` が続くパターンでバックトレッキングが `*` の個数に対して指数的に増えていた（コンパイル時のステップ数超過や実行時間の爆発）問題を修正。失敗した (テキスト位置, パターン位置) をメモ化して O(テキスト長 × パターン長) に収めた。
- `wildcard_match(FrozenString const&, std::string_view)`: コンパイルできない（一度もコンパイルが通らない）オーバーロードを削除。パターンを NTTP で渡す `wildcard_match<PAT>(text)` を使用すること。
- `wildcard_match(char const (&)[N], std::string_view)`: 他のオーバーロードに合わせて `constexpr` を付与。

- `make_frozen_trie_map`: エントリをキー名で対応づけるようになり、並び順がキー順と違っても正しく格納される。`frozen_map` / `frozen_multimap` / `frozen_bimap` と同じく未知キー・重複キー・欠落キーをコンパイル時エラーにする（従来は位置で代入していたため、キーの取り違えが静かに通っていた）。

- `test/CMakeLists.txt`: compile-fail ケース（33件）を CMake 設定時ではなく `ctest` のテストとして実行するようにした。静的_assert の診断メッセージを変更しても `bash test.sh` が検出する。
- `test/CMakeLists.txt`: `test_*.cpp` の glob に `CONFIGURE_DEPENDS` を付け、テストファイルを追加・改名しただけで再 configure されるようにした（これまで追加したテストが実行されないまま緑になることがあった）。
- `test.sh`: ビルドディレクトリを第1引数で指定できるようになった（`bash build.sh static && bash test.sh build_static`）。
- `build.sh`: 何も読み取られていなかった `-DSTATIC=ON` を廃止。`bash build.sh static` は従来どおり `build_static` に(static) トリプレットの構成を作る。

- **例外なしを既定化（glaze追従、破壊的変更）**: 実行時APIは一律 `std::expected<T, std::errc>` を返すようになり、`-fno-exceptions` でビルドできるようになった。例外を送出しうる API は `try_make` ファクトリに分離し、`at()` は `expected<reference_wrapper<T>, errc>` を返す。`operator[]` と `FrozenString::front/back/operator[]` はチェックなし（事前条件）。`FROZENCHARS_THROW` / `FROZENCHARS_WASI_MINIMAL` / `ENABLE_WASI_MINIMAL` / `single_include/frozenchars_wasi_minimal.hpp` / `test/smoke_wasi_minimal.cpp` は廃止。コンパイル時（NTTP）の不正入力は従来どおりコンパイルエラー。
- `minify_html`: インライン要素（`<b>` / `<span>` / `<a>` 等）とテキストの間の空白を 1 個残すようになった（従来は削除して単語が連結していた）。`{{name}}` はインラインテキスト扱い、`{{#sec}}` / `{{/sec}}` は構造タグ扱い。
- `minify_html`: `</p>` の省略を、直後がブロック要素・親の終了・入力終端のときに限定した（インライン要素やテキストが続くと DOM が変わるため）。
- `tools/amalgamate.py`: 出力を決定的にした（生成時刻を削除）。単一ヘッダのリポジトリ URL を修正。
- `CMakeLists.txt` のバージョンを git タグと一致する 0.7.0 に更新。

### Fixed

- `README.md`: `partition` の例が C++26 の constexpr 構造化束縛を使っており GCC 14 でコンパイルできなかったため、C++23 互換の `std::get` + `static_assert` に変更。`remove_comments` / `remove_comment_lines` の例は一時オブジェクトを直接渡すと GCC 14 の constexpr 評価で失敗するため、変数に束縛する形に変更。
- `test_frozen_map.cpp`: 実プロセスの `fork()` で中止を検証するテストが Emscripten（`__unix__` を定義するが `fork()` 非対応）でもビルド・実行され、`REQUIRE(pid >= 0)` の失敗後に `memory access out of bounds` で異常終了していた問題を修正。Emscripten をテスト対象から除外した。
- `test/CMakeLists.txt` / `json` の深度テスト: JSON パーサの再帰は `max_depth`=512 段まで進むが、Emscripten の既定スタック 64 KiB では深度 300 程度で枯渇し `validate_json` の深度制限テストが `memory access out of bounds` で落ちていた問題を修正。`all_test` のスタックを 1 MiB に拡張した。
- `.clang-format`: `Standard: c++23` は clang-format 22 以前では解釈できず設定ファイル全体が無効になっていたため `Standard: c++20` に修正。`public:` の字下げを既存コードに合わせるため `AccessModifierOffset` を -2 に修正。
- `frozen_format`: 浮動小数点数の作業バッファが `char[128]` 固定で、`{:.124}f` 以上の精度指定でバッファ外へ書き込んでいた問題を修正。書式文字列中の最大精度に追従するサイズに変更した。
- `frozen_format`: `{1}` のような明示的フィールドインデックスを幅指定として黙って解釈し、`std::format` と異なる結果していた（`"{1} {1}"` が `1 2` になる）問題を修正。`frozen_format: explicit field index is not supported` としてコンパイル時エラーにする。

- `frozen_trie_index`: `label_length` が `uint8_t` のため、共通接頭辞が 255 文字を超えるキーが静かに切り詰められ探索不能になっていた問題を修正。255 文字を超えるキーをコンパイル時エラーにする。
- `json::validate_json`: 不正な数値（`[-]` / `[1.]` / `[1e]` / `[01]` 等）と不正な文字列エスケープ（`["a\qb"]` / `["a\u12"]` / 単独サロゲート等）、文字列中の生の制御文字を `true` として受理していた問題を修正。RFC 8259 の number / string Production に厳密に従う。
- `glaze_frozen_map`: JSON 側のキーがエスケープされている場合（`{"\u0061":10}`）に NTTP キーと一致せず既定値のまま返っていた問題を修正。キーをアンクエスケープ（`\uXXXX` とサロゲートペアを含む）してから突き合わせる。

- `frozen_map::operator[]` / `frozen_trie_map::operator[]`: 未検出キーのガードが `assert` ベースだったため、`NDEBUG`（Release ビルド、既定の `windows-msvc` CI を含む）では無効化され范围外アクセスがそのまま残っていた。`FROZENCHARS_PRECONDITION` を追加し、定数評価ではコンパイルエラー、実行時は診断メッセージを出して `std::abort()` するようにした（`NDEBUG` でも有効）。
- `json::decode_utf8`: 入力末尾でシーケンスが切れている場合、継続バイトを境界チェックせずに読んで範囲外アクセスしていた問題を修正。`index + シーケンス長 > 入力長` を先に判定するようにした。
- `json::validate_json` / `json::compress` / `json::crush`: 入れ子深度の上限がなく、深くネストした入力で再帰がスタック枯渇していた問題を修正。`frozenchars::json::max_depth`（既定 512）を超える JSON は不正として扱う。
- `frozen_map::operator[]` / `frozen_trie_map::operator[]`: 存在しないキーで番兵インデックス（= `size()`）による `std::array` の範囲外アクセス（未定義動作）となる問題を修正。`FROZENCHARS_ASSERT` によるチェックを追加し、定数評価ではメッセージ付きのコンパイルエラー、実行時（assert 有効ビルド）は `frozenchars: frozen_map::operator[]: key not found` で中止する。
- `base64_decode`（NTTP 版）: `'=' 終端の 4 文字未満入力（`"="`・`"=="`・`"AB="` 等）でデコードサイズ計算が `size_t` アンダーフローし、`FrozenString requires N > 0` という無関係なエラーになっていた問題を修正。`frozenchars: base64_decode: invalid Base64 input (padding exceeds data)` として診断される。
- `base64_decode`（NTTP 版）: 入力長が 4 の倍数でない場合（`"Zg"`・`"Zg="`・`"QUJDRA"` 等、RFC 4648 の末尾グループ）にデコードバッファサイズを 1〜2 バイト少なく計算し、`FrozenString` の範囲外へ書き込んでいた問題を修正。デコード本体と同じ走査でサイズを算出するように変更したので、これらの入力は正しくデコードされる。
- `base64_decode`（NTTP 版）: パディングの後にデータ文字が続く入力（`"AB=CD"` 等）を `frozenchars: base64_decode: invalid Base64 input (data after padding)` として診断するようにした。
- CMake: `cmake_policy(SET CMP0169 OLD)`（CMake 3.30 で追加されたポリシー）を `cmake_minimum_required(VERSION 3.25)` のまま設定していたため、CMake 3.25〜3.29 では `cmake_minimum_required` 直後に hard error となり configure できない問題を修正（`if(POLICY CMP0169)` で保護）。
- `freeze(char)` / `concat(..., 'a')` が文字をコードポイント（`"97"`）に変換していた。
- `wildcard_match`: `[!...]` 否定クラスが英数字以外にマッチしなかった（frozen_regex 委譲の補集合が `DotChars` 限定のため）。`{` `}` `+` `^` `$` を含む `*` なしパターンがコンパイルエラーになっていた。
- `wildcard_match`: サフィックス早期判定が中間部と同じ文字を二重に使い、`[ab]x*x` が `"ax"` にマッチしていた。
- `minify_sql` / `minify_lua`: `a - -b` を `a--b`（行コメント）に融合していた。`minify_lua` は `1 .. y` を不正な数値リテラル `1..y` にしていた。
- `minify_html`: テキスト中の `'` / `"` で引用符状態に入り、以降の空白・コメント・タグが未処理で素通しになっていた。
- `minify_html`: `<pre>` / `<textarea>` の内容の空白を畳んでいた。

- `json::crush` の辞書領域が後続の置換で破壊される問題を修正（データ領域限定の置換に変更）。
- `json::crush` が置換を行わない（圧縮が機能しない）問題を修正。

### Added

- `FROZENCHARS_PRECONDITION(cond, msg)`: `NDEBUG` でも無効化されない前提条件チェック。定数評価ではコンパイルエラー、実行時は診断メッセージを出して `std::abort()` する。
- `frozenchars::json::max_depth`: JSON の入れ子深度の上限（既定 512）。`validate_json` / `compress` / `crush` はこれを超える入力を不正として扱う。
- `hash`（`frozenchars/hash.hpp`）: FNV-1a ハッシュ `fnv1a`、`hash_v<"...">`、`"..."_hash` リテラル、`hash_combine`、`constexpr_hash<T>`。コンパイル時定数として文字列 `switch` の `case` ラベルに使える。
- `json_escape` / `json_quoted`（`encoding.hpp`）: JSON 文字列リテラルのエスケープ（RFC 8259）をコンパイル時に計算。NTTP 版はバッファ長が確定する。パイプ演算子 `ops::json_escape` / `ops::json_quoted` も利用可能。
- `json::decompress` / `json::uncrush`: コンパイル時 JSON 圧縮（`compress` / `crush`）の復元 API。
- `json::compress` の出力を有効な JSON に変更。数値の原文（小数・指数）と型情報を保持。

## [0.8.0] – [0.8.1]

タグのみ付与され、本ファイルへの記載がありません。差分は各タグ間の `git log` を参照してください。

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

[Unreleased]: https://github.com/toge/frozenchars/compare/v0.8.1...HEAD
[0.8.0]: https://github.com/toge/frozenchars/compare/v0.7.0...v0.8.0
[0.7.0]: https://github.com/toge/frozenchars/compare/v0.2.0...v0.7.0
[0.2.0]: https://github.com/toge/frozenchars/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/toge/frozenchars/releases/tag/v0.1.0
