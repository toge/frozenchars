#pragma once

/**
 * @file frozenchars/config.hpp
 * @brief ビルドモード設定。
 *
 * FROZENCHARS_FREESTANDING が定義されると、I/O ヘッダ (<ostream> 等) に依存する
 * 機能が無効化される。wasm32-unknown-unknown (freestanding) では自動的に有効になる。
 * 手動で `-DFROZENCHARS_FREESTANDING` を指定しても有効にできる。
 */
#if !defined(FROZENCHARS_FREESTANDING) && defined(__wasm__) && !defined(__wasi__) && !defined(__EMSCRIPTEN__)
#  define FROZENCHARS_FREESTANDING 1
#endif
