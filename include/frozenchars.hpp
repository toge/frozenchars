#pragma once

/**
 * @file frozenchars.hpp
 * @brief 便利(全部入り)ヘッダ。非推奨。
 *
 * コンパイル負荷削減のため、個別のモジュールヘッダ
 * (frozenchars/mod/core.hpp 等) の利用を推奨する。
 * 本ヘッダは glaze / json を除く全基本機能を集約する。
 * 非推奨メッセージを抑制したい場合は FROZENCHARS_USE_UMBRELLA を定義する。
 */
#include "frozenchars/mod/all_basic.hpp"

#ifndef FROZENCHARS_USE_UMBRELLA
#pragma message("frozenchars.hpp is deprecated; prefer granular includes like frozenchars/mod/core.hpp")
#endif
