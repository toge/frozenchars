#pragma once

/**
 * @file frozenchars/mod/core.hpp
 * @brief コアモジュール: FrozenString 型と最小限の周辺機能をまとめる。
 *
 * リテラル(_fs)、freeze、数値変換、std::formatter 特殊化を含む。
 * glaze / json / regex / コンテナは含まない。
 */
#include "frozenchars/concepts.hpp"
#include "frozenchars/char_pred.hpp"
#include "frozenchars/string.hpp"
#include "frozenchars/literals.hpp"
#include "frozenchars/freeze.hpp"
#include "frozenchars/number_conv.hpp"
#include "frozenchars/format.hpp"
