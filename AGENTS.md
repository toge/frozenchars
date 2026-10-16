# AGENTS.md

`frozenchars` の作業ガイド。将来のセッションが同じ失敗を繰り返さないために、検証済みの事実だけをまとめる。

## プロジェクト概要

- ヘッダオンリー C++ ライブラリ。`FrozenString<N>`（`N=終端 '\0' を含むバッファ長`）が核。`consteval` 主体だが、ランタイムでも動くものは `constexpr`。
- 公開ターゲットは `frozenchars::frozenchars`（CMake `INTERFACE`）。利用時は必要なモジュールのみ include（例: `#include "frozenchars/mod/core.hpp"`）も可能。`frozenchars.hpp` は glaze / json を除く全機能を含む便利ヘッダ（非推奨ではない）。
- 現在は `cxx_std_23` 固定（`CMakeLists.txt:41`）。C++26 フォールバックは未実装（将来追加予定）。
- CI は `.github/workflows/ci.yml` に存在（Fedora 44/43/41、GCC 16/15/14）。
- README は 2500 行超の和文ドキュメント（`README.md`、現時点で 2565 行）。コード例は `tools/check_readme_examples.py`、目次は `tools/check_readme_anchors.py` で CI が検証する。

## ビルドとテスト

- 通常ビルド: `bash build.sh`（成果物 `build/`）→ テスト: `bash test.sh`
- 静的リンク版: `bash build.sh static`（`build_static/` に生成）→ テストは `bash test.sh build_static` とビルドディレクトリを渡す
- vcpkg の実体パスは `~/vm/vcpkg`（`build.sh:30`）。`conanfile.py` は存在しない（conan 分岐は build.sh 内の死コード）。
- 1ファイルだけ手元で動かす:
  ```bash
  g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include example/example.cpp && ./a.out
  ```
- テストフレームワーク: Catch2 v3。全テストが `all_test` 1個の実行ファイルにまとまる（`-r junit` で CTest 実行）。

- `compile_commands.json` は `build/compile_commands.json` に出力。
- `partition` の戻り値型は `std::tuple<FrozenString<N>, FrozenString<K>, FrozenString<N>>`（入力サイズ N 固定）。GCC 16 の NTTP + ユーザー定義比較演算子に関する制約のため、`before_len/after_len` を NTTP として使わない設計。`test_string_ops.cpp` では C++26 の `constexpr auto [...]` を使わず C++23 互換の `auto const [...]` + 直接 `static_assert` を使用。

## テストの落とし穴

- `static_assert` のエラーメッセージを変更したら `test/compile_fail/` の `EXPECTED_TEXT` を必ず更新（`run_compile_fail.cmake` が部分一致で検証）。
- コンパイル時マップの新たな `static_assert` を追加したら、`compile_fail/` にも失敗ケースを追加するルール。
- compile-fail ケースは `ctest` のテスト（`compile_fail_*`）として登録されている。`bash build.sh` だけでは検証されないので、必ず `bash test.sh` まで実行する。
- `test/CMakeLists.txt` の compile-fail 追加は `add_compile_fail_test()` で行う（設定時実行の `assert_compile_fail` は廃止）。

## コーディング規約

- **命名** (`.clang-tidy`)：クラス・変数・パラメータ・メンバ・関数 = `lower_case`、定数・constexpr 変数 = `UPPER_CASE`。
- **フォーマット** (`.clang-format`)：LLVM ベース、2 スペース、200 列、Left ポインタ、Attach ブレース。
- **改行** (`.editorconfig`)：LF、UTF-8、ファイル末改行あり、末尾空白トリム、`[*.{hpp,cpp}]` の `indent_size = 2`。
- コメントは日本語 Doxygen。`@brief` など統一。新規 API には同じスタイルで付ける。
- 数値変換は `std::to_chars` / `std::from_chars` 優先。`memcpy` ではなく `int64_t` 代入。
- 不要になった関数・型・分岐は消し切る文化。

## やってはいけないこと

- `include/` に `.cpp` を置かない（ヘッダオンリー維持）。
- `docs/superpowers/` は通常ディレクトリ（シンボリックリンクではない）。
- `_fs` リテラルの戻り値型は `FrozenString<文字数+1>`（終端 '\0' を含む）。`"abc"_fs` は `FrozenString<4>`。これに依存する変更は全利用箇所を grep。
- リポジトリ全体を `clang-format --dry-run --Werror` で検査する前提にしない（既存コードは一括整形されていない）。CI の書式ゲート（`docs and style`）は clang-format 22.1.8 で変更された行のみを検査する。ローカル確認も同じバージョンで行う。

## 参考リソース

- README: `README.md`（和文、機能網羅）
- アンブレラヘッダ: `include/frozenchars.hpp`
- 最小サンプル: `example/example.cpp`、`example/example_split.cpp`、`example/example_frozen_map_use_cases.cpp`、`example/snippets/type_parser/example_parse_to_tuple.cpp`
- コンパイル時マップ: `include/frozenchars/map.hpp`、`test/test_frozen_map.cpp`
- 失敗系テスト機構: `test/compile_fail/run_compile_fail.cmake`
- 命名・スタイル: `.clang-format`、`.clang-tidy`、`.editorconfig`
- glaze 検出マクロ: `include/frozenchars/detail/glaze_detect.hpp`（`FROZENCHARS_HAS_GLAZE` を定義）
- 既存作業ツリー一覧: `git worktree list`（`.worktrees/` 配下、新規作成前に確認）
