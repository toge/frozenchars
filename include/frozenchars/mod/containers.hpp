#pragma once

/**
 * @file frozenchars/mod/containers.hpp
 * @brief コンテナモジュール: コンパイル時 map / set / trie / bimap / multimap をまとめる。
 *
 * x86 環境では SIMD 組込み(intrinsic)を間接的に含む。
 */
#include "frozenchars/bimap.hpp"
#include "frozenchars/map.hpp"
#include "frozenchars/multimap.hpp"
#include "frozenchars/set.hpp"
#include "frozenchars/trie_index.hpp"
#include "frozenchars/trie_map.hpp"
#include "frozenchars/trie_set.hpp"
