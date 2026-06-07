#pragma once

#include <cstdint>

namespace frozenchars::fast_inja::detail {

/// @brief ループコンテキスト
/// @details @index / @first / @last の状態を保持する
struct loop_state {
  std::uint32_t index = 0;
  std::uint32_t count = 1; ///< 1 (not 0) so @last is false at top-level

  /// @brief 最初の要素か判定する
  [[nodiscard]] constexpr bool is_first() const noexcept { return index == 0; }

  /// @brief 最後の要素か判定する
  /// @details count > 1 ガードによりトップレベルコンテキストを "last" にしない
  [[nodiscard]] constexpr bool is_last() const noexcept {
    return count > 1 && index + 1 >= count;
  }
};

} // namespace frozenchars::fast_inja::detail
