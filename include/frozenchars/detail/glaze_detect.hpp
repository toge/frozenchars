#pragma once

/**
 * @brief glaze ライブラリの存在を検出し FROZENCHARS_HAS_GLAZE を定義する
 *
 * 複数ヘッダから include されても多重定義が起きないよう #ifndef ガードを使用する。
 * 全ての TU で統一されたマクロ名 FROZENCHARS_HAS_GLAZE (0 or 1) を使用すること。
 */
#ifndef FROZENCHARS_HAS_GLAZE
#if defined(__has_include) && __has_include(<glaze/glaze.hpp>)
#include <glaze/glaze.hpp>
#define FROZENCHARS_HAS_GLAZE 1
#else
#define FROZENCHARS_HAS_GLAZE 0
#endif
#endif
