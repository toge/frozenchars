#pragma once

#include <cstddef>
#include <functional>

#include "string.hpp"

namespace frozenchars::literals {

/**
 * @brief 文字列リテラルを FrozenString に変換する
 *
 * @tparam FS 固定長文字列
 * @return auto 変換文字列
 */
template <FrozenString FS>
[[nodiscard]] auto consteval operator""_fs() noexcept {
  auto res = FrozenString<FS.sv().size() + 1>{};
  auto const s = FS.sv();
  for (auto i = 0uz; i < s.size(); ++i) {
    res.buffer[i] = s[i];
  }
  res.buffer[s.size()] = '\0';
  res.length = s.size();
  return res;
}

} // namespace frozenchars::literals

namespace std {

/**
 * @brief FrozenString の std::hash 特殊化
 *
 * std::unordered_map 等の連想コンテナのキーとして使用可能にする。
 * literals.hpp を include することで利用可能になる。
 */
template <std::size_t N>
struct hash<frozenchars::FrozenString<N>> {
  [[nodiscard]] auto operator()(frozenchars::FrozenString<N> const& s) const noexcept -> std::size_t {
    return hash<std::string_view>{}(s.sv());
  }
};

} // namespace std
