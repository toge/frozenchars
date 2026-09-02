#pragma once

/**
 * @file frozenchars/config.hpp
 * @brief ビルドモード設定。
 *
 * FROZENCHARS_WASI_MINIMAL が定義されると、例外送出を伴う範囲チェック
 * (front/back/operator[] の out_of_range) が無効化される。wasm32-unknown-unknown
 * (bare-metal freestanding) では自動的に有効になる。wasm32-wasip1 /
 * wasm32-emscripten は WASI/hosted とみなすため自動では有効にならず、WASI 上で
 * 最小構成を検証する場合は手動で `-DFROZENCHARS_WASI_MINIMAL` を指定する。
 * 本ライブラリの WASI 対応は wasi-sdk sysroot を用いた wasm32-wasip1 でのビルドを
 * 想定（wasm3 等で実行可能）。`<iostream>` は wasip1/wasip2 では WASI 経由で
 * 利用可能なため無効化しない。真の bare-metal (wasm32-unknown-unknown の
 * -nostdlib) では <string>/<vector>/<map> 等の hosted ヘッダ自体が存在しないため、
 * コアサブセット (string/literals/split 等) に絞る必要がある。
 *
 * 例: clang++ --target=wasm32-wasip1 --sysroot=/opt/wasi-sdk/share/wasi-sysroot
 *       -DFROZENCHARS_WASI_MINIMAL=1 -I include -c src.cpp -o src.o
 */
#if !defined(FROZENCHARS_WASI_MINIMAL) && defined(__wasm__) && !defined(__wasi__) && !defined(__EMSCRIPTEN__)
#  define FROZENCHARS_WASI_MINIMAL 1
#endif
