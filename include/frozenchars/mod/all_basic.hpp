#pragma once

/**
 * @file frozenchars/mod/all_basic.hpp
 * @brief 基本モジュール集約: glaze / json を除く全機能をまとめる。
 *
 * frozenchars.hpp 傘ヘッダからも利用される。オプション統合(glaze, json)は
 * 含まれないため、必要な場合は利用者が明示的に include すること。
 */
#include "frozenchars/mod/core.hpp"
#include "frozenchars/mod/algorithms.hpp"
#include "frozenchars/mod/encoding.hpp"
#include "frozenchars/mod/containers.hpp"
#include "frozenchars/mod/regex.hpp"
#include "frozenchars/mod/formatting.hpp"
#include "frozenchars/mod/chrono.hpp"
#include "frozenchars/mod/color.hpp"
#include "frozenchars/mod/ops.hpp"
