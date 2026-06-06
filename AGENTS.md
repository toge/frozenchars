# AGENTS.md

`frozenchars` の作業ガイド。将来の OpenCode セッションが同じ失敗を繰り返さないために、リポジトリ固有の事実だけをまとめる。

## プロジェクト概要

- ヘッダオンリーの C++ ライブラリ。文字列連結・繰り返し・数値フォーマット・コンパイル時マップ・inja 風テンプレート VM などを `consteval` で提供する。
- 核となる型は `frozenchars::FrozenString<N>`（`N` は終端 `'\0'` を含むバッファ長）。
- 公開ターゲットは `frozenchars::frozenchars`（CMake `INTERFACE`）。利用側からは単一のインクルード `#include "frozenchars.hpp"` だけで全機能を使える。
- C++26 を最優先（C++23 / C++20 にフォールバック）。`-std=c++26` 未対応のコンパイラでは `STD_CPP` が `cxx_std_23`/`cxx_std_20` に落ちる。
- 詳細は `README.md`（1015 行、和文）を参照。クイックスタートは `example.cpp`。

## ビルドとテスト

`build.sh` は vcpkg を使う前提。`~/vm/vcpkg` が実体パス（`build.sh:30`）。リポジトリ内に `conanfile.py` があれば conan フローへ分岐する。

- 通常ビルド: `bash build.sh`（成果物は `build/`）
- 静的リンク版: `bash build.sh static`（`build_static/` に生成）
- テスト実行: `bash test.sh` ≡ `cd build && ctest -V`
- 1 ファイルだけ手元で回したい場合:
  ```bash
  g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include example.cpp && ./a.out
  ```
- CMake オプション:
  - `ENABLE_TESTS` 既定 ON。`test/CMakeLists.txt` を `add_subdirectory`。
  - `ENABLE_INJA` 既定 ON。`find_package(glaze CONFIG REQUIRED)` を呼び、`FROZENCHARS_ENABLE_INJA=1` を定義。OFF にすると `glaze` 不要で inja 系ヘッダがインストールされない。
- `ctest -j` の並列度は `build.sh` が `--parallel 4` を指定しているが、テストの構成上もっと上げても問題ない。
- `compile_commands.json` は `build/compile_commands.json` に出力される（`.vscode/c_cpp_properties.json` もそこを参照）。

## リポジトリ構成の要点

- `include/frozenchars.hpp` … アンブレラヘッダ。個別ヘッダを使わずこれを 1 行インクルードするのが慣習。
- `include/frozenchars/` … 機能別ヘッダ（`string.hpp`, `freeze.hpp`, `string_ops.hpp`, `map.hpp`, `inja_engine.hpp`, `inja_value.hpp`, `inja_function.hpp` など）。
- `include/frozenchars/inja/` … inja テンプレ VM の内部実装（`types.hpp`, `engine_impl.hpp`, `api.hpp`, `detail/`）。`inja_engine.hpp` 経由でのみ公開。
- `include/frozenchars/inja_types.hpp` … inja の新型定義（`value_view`, `temp_value`, `render_error`, `fixed_string`, `expr_kind`, `precomputed_path`）。`inja_value.hpp` を置き換える予定。
- `include/frozenchars/inja_access.hpp` … コンパイル時フィールドアクセサ（`accessor<T, "name">::resolve(obj)`）。glaze 反射型に対して NTTP ベースの O(1) フィールドアクセスを提供。
- `include/frozenchars/detail/` … `pipe.hpp`, `freeze_impl.hpp`, `number_conv.hpp`, `char_utils.hpp` などの実装詳細。外部公開は非想定。
- `test/` … Catch2 ベースの単体テスト。`test/CMakeLists.txt` は `file(GLOB test_src test_*.cpp)` で自動収集。
- `test/compile_fail/` … **失敗すべき**コンパイルのスナップショット。`assert_frozen_map_compile_fail(NAME SOURCE EXPECTED_TEXT)` が `try_compile` でビルド失敗とエラーメッセージの含有を検証する。新種の `static_assert` を追加したら、ここにもケースを追加する。
- `cmake/frozencharsConfig.cmake.in` … `find_dependency(glaze)` は `ENABLE_INJA` 時のみ。
- `vcpkg.json` … `catch2`, `ctre`, `glaze`, `simde` の 4 つ。`ENABLE_INJA=OFF` でも `ctre` は単体テストで必須。
- `CMakeUserPresets.json` は conan 用の薄いラッパ（`build/CMakePresets.json` を取り込む）。vcpkg フローのみで使うなら無視してよい。
- `.worktrees/` 配下に既存作業ツリーがある（`inja-engine-fixes`, `enum-map-impl`, `join-pad-replace-local`, `trim-helpers` など）。`git worktree list` で確認してから新規 worktree を作成する。
- `build*`, `vcpkg_installed/`, `.worktrees/`, `docs/superpowers` は `.gitignore` 済み。

## テストの細かい決まり

- `test_split_(direct|logic|tester|v_debug)\.cpp` の 4 ファイルは `test/CMakeLists.txt:5` でメインビルドから除外されている。これらは手動デバッグ用なので `all_test` には混ぜない。
- 1 個の実行ファイル `all_test` に全テストをまとめてリンク（`-r junit` で CTest 実行）。
- 個別 example はそれぞれ別実行ファイル: `example_smoke`（`example.cpp`）, `example_render_with_output_buffer`, `example_split`。
- `bench_inja_runtime` はベンチマークで CTest には登録されていない（手動実行用）。
- `format` 機能を使うには C++23 の `__cpp_lib_format`（`<format>`）が必要。`include/frozenchars/format.hpp` はそれがないとき `std::formatter` 特殊化と `to_sv` を提供しない。`to_sv<"..."_fs>()` は `consteval` で NTTP 文字列を取り出すユーティリティ。

## コーディング規約

- フォーマット: `.clang-format` 準拠（LLVM ベース、2 スペース、200 列、Left ポインタ、Attach ブレース）。
- 静的解析: `.clang-tidy` の命名規則がリポジトリ標準。クラス・変数・パラメータ・メンバは `lower_case`、関数は `camelBack`、定数・constexpr 変数は `UPPER_CASE`。
- 改行コード: LF、UTF-8、ファイル末改行あり、末尾空白トリム（`.editorconfig`）。
- 既存コメントは日本語 Doxygen 形式。API を追加・変更したら同じスタイルで `@brief` などを付ける。
- `consteval` を乱用しない方針。ランタイムでも動く実装は `constexpr`、本当にコンパイル時強制のみ `consteval`。
- 数値は整数なら `std::to_chars` / `std::from_chars` を優先（constexpr 対応済み）。`memcpy` ではなく `int64_t` の代入を使う慣習（コミット履歴参照）。
- 不要な関数・型は積極的に削除する文化。不要になった `static_assert` 用分岐や `find` 系のデッドコードは消し切る。

## 作業前に必ず確認すること

- 変更が `FrozenString<N>` の `N` 推定に影響する場合、テンプレ特殊化と `static_assert` のエラーメッセージを両方更新する。
- `static_assert` の文言を変更したら `test/compile_fail/` 配下の `EXPECTED_TEXT` を必ず更新する（`run_compile_fail.cmake` が部分一致検索する）。
- inja 関連の変更は `ENABLE_INJA=OFF` でもライブラリが壊れないこと（`FROZENCHARS_HAS_GLAZE` ガード）。
- 公開 API を追加・削除したら `README.md` の対応する節も更新する（手厚い和文ドキュメント）。
- 既存の `_worktrees` ブランチで進行中の作業と衝突しないか `git worktree list` と `git branch -a` で確認する。

## やってはいけないこと

- ヘッダオンリーの性質を壊す `.cpp` を `include/` 配下に置かない（実装は `detail/` 以下のヘッダへ）。
- `glaze` を `ENABLE_INJA=OFF` の構成で必須にしない。
- `_fs` リテラル（`frozenchars::literals::operator""_fs`）の戻り値型は `FrozenString<N+1>`（`'\0'` ぶん 1 多い）。これに依存したコードを変更する場合は全利用箇所を grep する。
- `CMakeLists.txt` の `ENABLE_INJA` 既定値は現在 ON（コミット `e28f828` で切替）。既定値を変える場合は README の最小ビルド例も合わせて確認する。
- `docs/superpowers/` には触れない（`.gitignore` され、シンボリックリンク経由で読み込まれる個人用ノート）。

## 参考リンク

- README: `README.md`（1015 行、和文、機能の網羅的リファレンス）
- アンブレラヘッダ: `include/frozenchars.hpp`
- 最小サンプル: `example.cpp`, `example_split.cpp`, `example_render_with_output_buffer.cpp`
- コンパイル時マップ仕様: `include/frozenchars/map.hpp`, `test/test_frozen_map.cpp`
- テンプレート VM: `include/frozenchars/inja_engine.hpp` → `inja/engine_impl.hpp`
- 失敗系テストの仕組み: `test/compile_fail/run_compile_fail.cmake`
- 命名・スタイル: `.clang-format`, `.clang-tidy`, `.editorconfig`
