# AGENTS.md

`frozenchars` の作業ガイド。将来のセッションが同じ失敗を繰り返さないために、検証済みの事実だけをまとめる。

## プロジェクト概要

- ヘッダオンリー C++ ライブラリ。`FrozenString<N>`（`N=終端 '\0' を含むバッファ長`）が核。`consteval` 主体だが、ランタイムでも動くものは `constexpr`。
- 公開ターゲットは `frozenchars::frozenchars`（CMake `INTERFACE`）。利用時は `#include "frozenchars.hpp"` 1行。
- C++26 優先、なければ C++23 → C++20 へフォールバック（`CMakeLists.txt:10-22`）。
- `ENABLE_INJA=OFF` にすれば glaze 不要でビルドできる（inja 系ヘッダもインストールされない）。
- CI 未整備（`.github/` 無し）。手動でビルド・テスト。
- README は 1000行超の和文ドキュメント（`README.md`、現時点で 1131 行）。

## ビルドとテスト

- 通常ビルド: `bash build.sh`（成果物 `build/`）→ テスト: `bash test.sh`
- 静的リンク版: `bash build.sh static`（`build_static/` に生成）
- vcpkg の実体パスは `~/vm/vcpkg`（`build.sh:30`）。`conanfile.py` が存在すれば conan フローへ分岐。
- 1ファイルだけ手元で動かす:
  ```bash
  g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include example.cpp && ./a.out
  ```
  （README に `-I.` と書いてあるが誤り。include ディレクトリを指定すること）
- テストフレームワーク: Catch2 v3。全テストが `all_test` 1個の実行ファイルにまとまる（`-r junit` で CTest 実行）。
- `bench_inja_runtime` はベンチマークで CTest 未登録（手動実行用）。
- `test_split_(direct|logic|tester|v_debug).cpp` の4ファイルは `all_test` から除外。手動デバッグ用。
- `compile_commands.json` は `build/compile_commands.json` に出力。

## テストの落とし穴

- `static_assert` のエラーメッセージを変更したら `test/compile_fail/` の `EXPECTED_TEXT` を必ず更新（`run_compile_fail.cmake` が部分一致で検証）。
- コンパイル時マップの新たな `static_assert` を追加したら、`compile_fail/` にも失敗ケースを追加するルール。
- `ENABLE_INJA=OFF` でも壊れないこと。inja コードは `#ifdef FROZENCHARS_HAS_GLAZE` でガード。

## コーディング規約

- **命名** (`.clang-tidy`)：クラス・変数・パラメータ・メンバ = `lower_case`、関数 = `camelBack`、定数・constexpr 変数 = `UPPER_CASE`。
- **フォーマット** (`.clang-format`)：LLVM ベース、2 スペース、200 列、Left ポインタ、Attach ブレース。
- **改行** (`.editorconfig`)：LF、UTF-8、ファイル末改行あり、末尾空白トリム。
- コメントは日本語 Doxygen。`@brief` など統一。新規 API には同じスタイルで付ける。
- 数値変換は `std::to_chars` / `std::from_chars` 優先。`memcpy` ではなく `int64_t` 代入。
- 不要になった関数・型・分岐は消し切る文化。

## やってはいけないこと

- `include/` に `.cpp` を置かない（ヘッダオンリー維持）。
- `docs/superpowers/` に触れない（`.gitignore`、個人用シンボリックリンク）。
- `_fs` リテラルの戻り値型は `FrozenString<文字数+1>`（終端 '\0' を含む）。`"abc"_fs` は `FrozenString<4>`。これに依存する変更は全利用箇所を grep。

## 参考リソース

- README: `README.md`（和文、機能網羅）
- アンブレラヘッダ: `include/frozenchars.hpp`
- 最小サンプル: `example.cpp`、`example_split.cpp`、`example_render_with_output_buffer.cpp`
- コンパイル時マップ: `include/frozenchars/map.hpp`、`test/test_frozen_map.cpp`
- テンプレート VM: `include/frozenchars/inja_engine.hpp` → `inja/engine_impl.hpp`
- 失敗系テスト機構: `test/compile_fail/run_compile_fail.cmake`
- 命名・スタイル: `.clang-format`、`.clang-tidy`、`.editorconfig`
- 既存作業ツリー一覧: `git worktree list`（`.worktrees/` 配下、新規作成前に確認）
