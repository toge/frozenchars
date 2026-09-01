#pragma once

/**
 * @file frozenchars/config.hpp
 * @brief ビルドモード設定。
 *
 * FROZENCHARS_WASI_MINIMAL が定義されると、I/O ヘッダ (<ostream> 等) に依存する
 * 機能が無効化される。wasm32-unknown-unknown (bare-metal freestanding) では
 * 自動的に有効になる。wasm32-wasip1 / wasm32-emscripten は WASI/hosted とみなす
 * ため自動では有効にならず、WASI 上で freestanding サブセットを検証する場合は
 * 手動で `-DFROZENCHARS_WASI_MINIMAL` を指定する。本ライブラリの freestanding
 * 対応は wasi-sdk sysroot を用いた wasm32-wasip1 でのビルドを想定（wasm3 等で
 * 実行可能）。真の bare-metal (wasm32-unknown-unknown の -nostdlib) では
 * <string>/<vector>/<map> 等の hosted ヘッダ自体が存在しないため、コア
 * サブセット (string/literals/split 等) に絞る必要がある。
 *
 * 例: clang++ --target=wasm32-wasip1 --sysroot=/opt/wasi-sdk/share/wasi-sysroot
 *       -DFROZENCHARS_WASI_MINIMAL=1 -I include -c src.cpp -o src.o
 */
#if !defined(FROZENCHARS_WASI_MINIMAL) && defined(__wasm__) && !defined(__wasi__) && !defined(__EMSCRIPTEN__)
#  define FROZENCHARS_WASI_MINIMAL 1
#endif
