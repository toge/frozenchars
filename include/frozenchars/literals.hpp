#pragma once

#include "frozen_string.hpp"
#include <cstddef>

namespace frozenchars::literals {

/**
 * @brief 文字列リテラルを FrozenString に変換する
 *
 * @tparam FS 固定長文字列
 * @return auto 変換文字列
 */
template <FixedString FS>
auto consteval operator""_fs() noexcept {
  auto res = FrozenString<FS.sv().size() + 1>{};
  auto const s = FS.sv();
  for (auto i = 0uz; i < s.size(); ++i) {
    res.buffer[i] = s[i];
  }
  res.buffer[s.size()] = '\0';
  return res;
}

} // namespace frozenchars::literals
