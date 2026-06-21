# AGENTS.md

`frozenchars` の作業ガイド。将来のセッションが同じ失敗を繰り返さないために、検証済みの事実だけをまとめる。

## プロジェクト概要

- ヘッダオンリー C++ ライブラリ。`FrozenString<N>`（`N=終端 '\0' を含むバッファ長`）が核。`consteval` 主体だが、ランタイムでも動くものは `constexpr`。
- 公開ターゲットは `frozenchars::frozenchars`（CMake `INTERFACE`）。利用時は `#include "frozenchars.hpp"` 1行。
- 現在は `cxx_std_23` 固定（`CMakeLists.txt:41`）。C++26 フォールバックは未実装（将来追加予定）。
- `ENABLE_INJA=OFF` にすれば glaze 不要でビルドでき、inja 系ヘッダはインストールされない（`CMakeLists.txt:62-78` にて `PATTERN "inja*" EXCLUDE` で制御）。
- CI は `.github/workflows/ci.yml` に存在（Fedora 44/43/41、GCC 16/15/14）。
- README は 1000行超の和文ドキュメント（`README.md`、現時点で 1300 行超）。

## ビルドとテスト

- 通常ビルド: `bash build.sh`（成果物 `build/`）→ テスト: `bash test.sh`
- 静的リンク版: `bash build.sh static`（`build_static/` に生成）
- vcpkg の実体パスは `~/vm/vcpkg`（`build.sh:30`）。`conanfile.py` は存在しない（conan 分岐は build.sh 内の死コード）。
- 1ファイルだけ手元で動かす:
  ```bash
  g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include example.cpp && ./a.out
  ```
- テストフレームワーク: Catch2 v3。全テストが `all_test` 1個の実行ファイルにまとまる（`-r junit` で CTest 実行）。
- `bench_inja_runtime` はベンチマークで CTest 未登録（手動実行用）。`ENABLE_INJA=ON` 時のみビルド。
- `test_split_(direct|logic|tester|v_debug).cpp` の4ファイルは `all_test` から除外。手動デバッグ用。
- `compile_commands.json` は `build/compile_commands.json` に出力。
- `partition` の戻り値型は `std::tuple<FrozenString<N>, FrozenString<K>, FrozenString<N>>`（入力サイズ N 固定）。GCC 16 の NTTP + ユーザー定義比較演算子に関する制約のため、`before_len/after_len` を NTTP として使わない設計。`test_string_ops.cpp` では C++26 の `constexpr auto [...]` を使わず C++23 互換の `auto const [...]` + 直接 `static_assert` を使用。

## テストの落とし穴

- `static_assert` のエラーメッセージを変更したら `test/compile_fail/` の `EXPECTED_TEXT` を必ず更新（`run_compile_fail.cmake` が部分一致で検証）。
- コンパイル時マップの新たな `static_assert` を追加したら、`compile_fail/` にも失敗ケースを追加するルール。
- `ENABLE_INJA=OFF` でも壊れないこと。inja コードは `#ifdef FROZENCHARS_HAS_GLAZE` でガード（`detail/glaze_detect.hpp` を経由する）。
- `ENABLE_INJA=OFF` 時、test/CMakeLists.txt の `list(FILTER ... EXCLUDE)` で inja 系 7 ファイルが自動除外される。

## コーディング規約

- **命名** (`.clang-tidy`)：クラス・変数・パラメータ・メンバ = `lower_case`、関数 = `camelBack`、定数・constexpr 変数 = `UPPER_CASE`。
- **フォーマット** (`.clang-format`)：LLVM ベース、2 スペース、200 列、Left ポインタ、Attach ブレース。
- **改行** (`.editorconfig`)：LF、UTF-8、ファイル末改行あり、末尾空白トリム、`[*.{hpp,cpp}]` の `indent_size = 2`。
- コメントは日本語 Doxygen。`@brief` など統一。新規 API には同じスタイルで付ける。
- 数値変換は `std::to_chars` / `std::from_chars` 優先。`memcpy` ではなく `int64_t` 代入。
- 不要になった関数・型・分岐は消し切る文化。

## やってはいけないこと

- `include/` に `.cpp` を置かない（ヘッダオンリー維持）。
- `docs/superpowers/` は通常ディレクトリ（シンボリックリンクではない）。`plans/` と `specs/` 配下の 2 ファイルが git 追跡済み。`.gitignore:9` で ignore 指定だが追跡済みなので注意。
- `_fs` リテラルの戻り値型は `FrozenString<文字数+1>`（終端 '\0' を含む）。`"abc"_fs` は `FrozenString<4>`。これに依存する変更は全利用箇所を grep。

## 参考リソース

- README: `README.md`（和文、機能網羅）
- アンブレラヘッダ: `include/frozenchars.hpp`
- 最小サンプル: `example.cpp`、`example_split.cpp`、`example_render_with_output_buffer.cpp`（inja 使用）、`example_frozen_map_use_cases.cpp`、`example_parse_to_tuple_use_cases.cpp`
- コンパイル時マップ: `include/frozenchars/map.hpp`、`test/test_frozen_map.cpp`
- テンプレート VM: `include/frozenchars/inja_engine.hpp` → `inja/engine_impl.hpp`
- 失敗系テスト機構: `test/compile_fail/run_compile_fail.cmake`
- 命名・スタイル: `.clang-format`、`.clang-tidy`、`.editorconfig`
- glaze 検出マクロ: `include/frozenchars/detail/glaze_detect.hpp`（`FROZENCHARS_HAS_GLAZE` を定義）
- 既存作業ツリー一覧: `git worktree list`（`.worktrees/` 配下、新規作成前に確認）
