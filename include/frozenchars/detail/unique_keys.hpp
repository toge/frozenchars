#pragma once

#include <algorithm>
#include <array>
#include <string_view>

#include "../string.hpp"

namespace frozenchars::detail {

/**
 * @brief キー群に重複（同一文字列）がないかをコンパイル時に判定する
 *
 * @tparam Keys キー列（FrozenString NTTP）
 * @return bool 重複があれば true
 */
template <FrozenString... Keys>
[[nodiscard]] consteval auto has_duplicate_keys() -> bool {
  if constexpr (sizeof...(Keys) <= 1) {
    return false;
  } else {
    constexpr std::array key_views{ std::string_view{Keys.buffer.data(), Keys.length}... };
    auto sorted = key_views;
    std::ranges::sort(sorted);
    for (auto i = 1uz; i < sorted.size(); ++i) {
      if (sorted[i - 1] == sorted[i]) {
        return true;
      }
    }
    return false;
  }
}

} // namespace frozenchars::detail
