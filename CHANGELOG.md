# Changelog

このプロジェクトの主要な変更点を記録します。
形式は [Keep a Changelog](https://keepachangelog.com/ja/1.1.0/) に準拠し、
バージョニングは [Semantic Versioning](https://semver.org/lang/ja/) に従います。

## [Unreleased]

### Changed

- Glaze 連携 (`glaze_frozen_map.hpp`): `frozen_map` の JSON 書き出しが一時 `std::map` を確保しなくなった。キーは宣言順で出力され（0.9.0 はアルファベット順）、`glz::opts::skip_null_members`（既定 `true`）に従って null 値のキーを省略する。ムーブオンリーの値型も書き出せる。
- ビルド設定 (`CMakeLists.txt`): `BUILD_TEST` の既定値はトップレベルビルド時のみ `ON`。警告オプション（`-Wall` など）はトップレベルビルド時のみ追加し、`add_subdirectory` で取り込む利用側には伝播しない。

### Fixed

- `frozen_format`: `long long` の最小値を `{}` でフォーマットすると符号付き整数オーバーフロー（未定義動作）になっていた。空文字列に `:c` を指定すると範囲外読み出しになっていた。`__cpp_lib_constexpr_format` の判定前に `<version>` を include する。
- `frozen_map`: ルックアップテーブル／CHD のシード探索が 16 刻みで候補を飛ばしていたため、衝突ゼロのシードが存在しても見つからずコンパイルに失敗することがあった。キー長がすべて一意な場合はテーブル構築を省略する。
- Glaze 連携 (`glaze_frozen_map.hpp`): JSON キーの不正なエスケープ（`"\x61"` など）を `syntax_error` として拒否する。書き出し時にキーの書き込みが失敗すると区切り文字を範囲外へ書いていた。
- `hash_combine`: 64bit 環境では 64bit の黄金比定数を使用する（ハッシュ値は変わる）。

## [0.9.0] - 2026-11-19

### Changed

- ビルド設定 (`CMakeLists.txt`, `cmake/frozencharsConfig.cmake.in`): **ソース非互換**。`frozenchars::frozenchars` が `glaze::glaze` を自動でリンクしなくなった。Glaze 連携（`glaze_frozen_map.hpp` など）を使う場合は `find_package(frozenchars COMPONENTS glaze)` とし、`frozenchars::glaze` をリンクする。`find_package(frozenchars)` だけの既存プロジェクトはリンクを追加するまでビルドできない。
- ビルド設定 (`CMakeLists.txt`): constexpr の上限緩和オプション（`-fconstexpr-ops-limit`、`-fconstexpr-steps`、`/constexpr:steps`）を `add_compile_options` から `target_compile_options(INTERFACE)` に移動した。`find_package` 経由の利用側にも上限が伝わる。
- `frozen_map` / `frozen_multimap` / `frozen_trie_map`: デフォルトコンストラクタ、`frozen_map::get_value_or`、`frozen_map::operator==` の `noexcept` を値型の性質（`is_nothrow_default_constructible` など）に合わせて条件付きにした。値型が `noexcept` でない場合に `noexcept` 関数から例外が送出されていた。
- `utf8_length` / `utf8_substr` / `utf8_reverse`: 不正な UTF-8 バイト列を先頭バイトのパターンで読み飛ばすのをやめ、1 バイトずつ消費するようにした。有効な UTF-8 の結果は変わらない。不正バイトを含む文字列では符号点数が変わる。

### Fixed

- `json::validate_json`: 文字列リテラル内の生の UTF-8 を検証するようになった。不正な UTF-8 を含む JSON は `true` ではなく `false` を返す（`json::compress` は従来どおり不正バイトを保持する）。
- `json::crush` / `json::uncrush` / `json::crush_compress`: 不正な UTF-8 の入力を `static_assert` で拒否する。Crush 内部の UTF-8→UTF-16 変換は孤立した低位サロゲートも拒否する。
- `parse_iso_datetime`: タイムゾーンオフセットの `hh:mm` を範囲検証する（`hh <= 23 && mm <= 59`）。`Z`/`z` は 20 文字のとき、`±hh:mm` は 25 文字のときだけ受け付ける。
- `parse_date_macro`: 月名 3 文字の直後と、日にち・年の数字位置を検証し、存在しない日付（`"Feb 30 2024"`）も拒否する。コンパイル時は `static_assert`、実行時は `std::errc::invalid_argument`。
- `parse_iso_date` / `parse_iso_datetime` / `parse_date_macro`: `FrozenString` の `length` がバッファ長以上（終端 `'\0'` なし）のとき範囲外読み出しになり得た。`std::errc::invalid_argument` で拒否する。

## [0.8.2] - 2026-11-17

### Fixed

- `parse_number` (コンパイル時パス): 指数部の計算で浮動小数点のオーバーフローを定数式エラーにしていた（`"1e300"` などがコンパイル不能）。乗算前に上限を判定し、範囲外は `std::errc::result_out_of_range` を返す。`float`/`double` の `max_exponent10`/`min_exponent10` を超える指数、指数桁あふれ、非ゼロ値が 0 に丸められるアンダーフローも検出する。
- `detail::to_float_chars`: NaN/Infinity や `long long` 範囲を超える値を空文字列として黙って返していた。`FROZENCHARS_PRECONDITION` で診断する。
- `frozen_format` の浮動小数点: 小数部を 1 桁ずつ切り捨てていたため `{:.2f}` が `0.57` を `"0.56"`、`{:.1f}` が `0.999` を `"0.9"` にしていた。最近接偶数丸め（`std::format` と同じ）に変更。`long long` を超える値で `{:.0f}` の符号が反転する問題も修正し、科学計数法で出力する。NaN/Infinity は `nan`/`inf`、汎用形式は指数と長さで固定/科学を選び末尾 0 を除去する。
- `split_numbers`: 数値でないトークンで `std::expected` を `*` で剥がして未定義動作になっていた。コンパイルエラー（`-fno-exceptions` ビルドでは `abort`）にする。
- `dirname`: 末尾の `/` を無視せず `dirname("foo/")` が `"foo"` になっていた。`"."` を返す。
- `word_wrap`: 改行の直前や折り返しの直前で末尾の空白を 1 個しか除去せず、`"aa   b"` の折り返しが `"aa \nb"` になっていた。すべて除去する。
- `detail::count_occurrences`（空の検索文字列）: `pos += 0` で無限ループしていた。0 回を返す。
- `hex_decode`: 奇数長の 16 進文字列を前提条件違反として拒否する（従来は末尾 1 桁を読み飛ばしていた）。
- `utf8_reverse`: 不正な UTF-8 入力で再エンコード後の長さが元バッファを超え、範囲外書き込みになっていた。`FROZENCHARS_PRECONDITION` で検出する。
- `utf8_substr` / `utf8_reverse`（NTTP 版）: 戻り値型が文字列長（終端 `'\0'` を含まない）になっていた。バッファ長を保つ。
- `unescape_c<...>`: `"\\\\uD800"` のようなエスケープされたバックスラッシュの後ろの `u` を `\u` エスケープと誤認してコンパイルエラーにしていた。
- `br_to_target`: 出力バッファが `max(N*4, 2048)` 固定で、長い置換文字列と多数の `<br>` で溢れていた。入力長から上限を算出する。
- `minify_js`: 正規表現リテラル内の `//` や `/*` をコメントとして削除していた。式の先頭に現れる `/` をリテラルとして扱い、エスケープと文字クラスを考慮して閉じ `/` まで保持する（`shortcut:` のとおり `)` 直後のリテラルは除算とみなす）。
- `wildcard_match` / `wildcard_find`: 空の代替（`()`、`(|a)`、`(a|)`、`(a||b)`）、空の文字クラス `[]`、先頭/末尾 `-` のクラスを `frozen_regex` に委譲してコンパイルエラーにしていた。ネイティブ経路に回す。
- `parse_iso_date` / `parse_iso_datetime`: 数字以外の文字や存在しない日付（`2023-02-30`）を受理していた。コンパイル時は `static_assert`、実行時は `std::errc::invalid_argument` で拒否する。`format_iso_date` は年が `0..9999` の範囲外なら診断する。
- `detail::find_impl`: 実装が `detail::find_substring` と重複していた。前方参照する薄い転送関数にし、空の検索文字列で `pos > length` のとき `npos` を返す `find_substring` の規則に統一する。
- `parse_number`（コンパイル時パス）: 小数部が 18 桁を超えると余った桁数だけ位取りがずれていた（`"3.14159265358979323846"` → `3.0014159265358979`）。先頭の 0 は位取りのみ進め、実際に格納した有効桁だけスケールする。
- `unescape_c`: `\U` 表記のサロゲート（U+D800〜U+DFFF）を UTF-8 として出力し不正なバイト列になっていた。`\u` と同じく不正コードポイントとして保持（NTTP 版はコンパイルエラー）する。
- `frozen_regex`: 文字クラス範囲の終端が `CHAR_MAX` のとき（例 `[a-\x7f]`）符号付き `char` の `++ch` がオーバーフローし、無限ループ（constexpr 操作数の上限超過）になっていた。`int` で走査する。
- `frozen_format`: 明示精度の汎用形式（`{:.3g}` / `{:.3}`）を有効桁数ではなく固定小数点の小数桁として扱い、`std::format` と異なる結果（`1234.0` → `"1234"`）になっていた。指数が `-4 <= E < 精度` なら固定小数点、それ以外は科学計数法にする。
- `frozen_format` / `freeze(Precision(v, p))`: 負のゼロの符号を落としていた（`{:.2f}` が `"0.00"`）。`std::signbit` は libstdc++ 14 で constexpr でないため符号ビットで判定し、`"-0.00"` / `"-0"` を出力する。
- `make_querystring`: 非 const または右辺値の `std::tuple` ではタプル版オーバーロードが選ばれずコンパイル不能だった。非 const 参照・右辺値参照のオーバーロードを追加する。MSVC の consteval で `std::apply` が失敗するため `index_sequence` で展開する。
- `frozen_map`: 256 キー超で `std::array` の CTAD と pack 展開の fold 式が Clang 19/20 の入れ子式上限（256）に達してコンパイルできなかった。配列の型を明示し、キー長の表とパディング生成をループに置き換える。
- `format_iso_datetime`: 年が `0..9999` の範囲外でも 4 桁に切り詰めて出力していた（`year{10000}` → `":000-01-01..."`）。`format_iso_date` と同じ前提条件で診断する。
- `pad_left` / `pad_right` / `center`: `Width == SIZE_MAX` で `Width + 1` が 0 に折り返し、範囲外書き込みになっていた。静的アサートで拒否する。

### Changed

- `detail::pipe`: `std::string_view` 用の `operator|` は、どのパイプアダプタも `std::string_view` を受け取らないため成立しない死コードだった。削除する。

- README: コンパイル時パスの浮動小数点変換は正しい丸めを保証せず、実行時パスと 1〜2 ulp 差が出ることを明記。`split_numbers` の失敗はランタイムの例外ではなくコンパイルエラーであることを修正。

- `cmake/frozencharsConfig.cmake.in`: glaze なしでビルド・インストールした frozenchars を `find_package` すると、glaze が無い環境では失敗していた。ビルド時に glaze を検出した場合のみ `find_dependency(glaze)` するようにした。
- `freeze(Precision(v, p))`: 小数部を 1 桁ずつ切り捨てていたため二進誤差がそのまま出ていた（`Precision(0.57, 2)` → `"0.56"`）。四捨五入するようにした（ちょうど中間の値は 0 から離れる方向に丸める。`printf` の偶数丸めとは異なる）。
- `frozen_trie_index` / `frozen_trie_set` / `frozen_trie_map`: 長さ 256 のキーが static_assert を通り、ラベル長（`uint8_t`）が 0 に切り詰められて誤検索していた。上限を 255 文字にした。
- `wildcard_match` / `wildcard_find`: 最後の `*` がグループ・文字集合の内側にあると `)` 以降を純リテラルのサフィックスとして早期リジェクトしていた（`"(a*)b"` が `"ab"` に不一致）。また、グループの分岐を最短一致の 1 通りしか試さず、後続が失敗しても分岐内の `*` や入れ子の分岐で後戻りしなかった（`"(a|ab)c"` が `"abc"` に不一致）。
- `minify_js`: 改行を単なる空白として扱い ASI の意味が変わっていた（`return\nx` → `return x`）。改行が ASI に影響し得る箇所では改行を残す。また制御構文・`else` / `do` の直後の空文 `;` を削除して構文エラーにしていた（`while(x);}` → `while(x)}`）。
- `minify_sql` の `remove_as`: `CAST(x AS INT)` の AS や `CREATE VIEW v AS SELECT` の AS まで削除していた。CAST / TRY_CAST / SAFE_CAST の括弧内と、`SELECT` / `WITH` / `VALUES` / `TABLE` が続く AS は残す。
- `minify_yaml`: 直前が空白でない `#`（`http://x/#frag` など）をコメントとして削除していた。ブロックスカラー（`|` / `>`）本文の `#` や空行も削除していた。本文はそのまま保持する。巻き戻した書き込みが `length` 以降に残り `operator==` が壊れる問題も修正。
- `path::join`: 途中で絶対パス要素が来たときに先行要素の文字が `length` 以降に残り、`operator==` が誤って false になっていた。
- `frozen_trie_index`: GCC 16 で `-Warray-bounds` / `-Wmaybe-uninitialized` の誤検知が出ていた（短いキーや小さなトライで、到達しない 16 バイト SIMD ロードを GCC が範囲外と判定）。16 文字以上のラベル比較を手書きの SSE2 / NEON から `memcmp` に置き換えた。ベンチマークでは同等以上の速度。
- `test/bench_map_comparison.cpp`: 計測に使われていなかったパターン 7 を削除（`-Wunused-but-set-variable`）。
- `constexpr_log`: バッファが満杯で行を切り詰めても `true` を返し、件数も増やしていた。行全体が収まらない場合は何も書き込まず `false` を返すようにした（`message(i)` が途中で切れた行を返さない）。

### Changed

- CI: vcpkg をキャッシュから復元した場合は `git fetch` + `git reset --hard origin/master` と再 bootstrap で最新に同期するようにした。キャッシュされた古いクローンには `vcpkg.json` の `builtin-baseline` が指すコミットやバージョン DB が無く、また未追跡ファイルとの衝突で `pull` が失敗していたため（Windows は `|| exit /b 1` で失敗を検知する）。
- CI (windows-msvc): `ctest` を MSVC 開発者環境（`vcvars64.bat`）で実行するようにした。compile-fail テストは `cl.exe` を直接起動するため、素の環境では `INCLUDE` が無く標準ヘッダ（`cassert` など）が見つからず全件失敗する。
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

[Unreleased]: https://github.com/toge/frozenchars/compare/v0.9.0...HEAD
[0.9.0]: https://github.com/toge/frozenchars/compare/v0.8.2...v0.9.0
[0.8.2]: https://github.com/toge/frozenchars/compare/v0.8.1...v0.8.2
[0.8.0]: https://github.com/toge/frozenchars/compare/v0.7.0...v0.8.0
[0.7.0]: https://github.com/toge/frozenchars/compare/v0.2.0...v0.7.0
[0.2.0]: https://github.com/toge/frozenchars/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/toge/frozenchars/releases/tag/v0.1.0
