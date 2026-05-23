#pragma once

#include "map.hpp"

#if defined(__has_include) && __has_include(<glaze/json.hpp>)
#include <glaze/json.hpp>

#include <map>
#include <string>
#include <unordered_map>
#include <utility>

namespace glz {

/**
 * @brief frozenchars::frozen_map を Glaze の custom read/write 対象にする
 */
template <typename T, frozenchars::FrozenString... Keys>
struct meta<frozenchars::frozen_map<T, Keys...>> {
  static constexpr auto custom_read = true;
  static constexpr auto custom_write = true;
};

/**
 * @brief JSON オブジェクトから frozen_map へ読み込む
 *
 * JSON 側に存在するキーのうち、frozen_map に定義されたキーのみ反映し、
 * 未知キーは無視する。
 */
template <typename T, frozenchars::FrozenString... Keys>
struct from<JSON, frozenchars::frozen_map<T, Keys...>> {
  template <auto Opts>
  static auto op(frozenchars::frozen_map<T, Keys...>& value,
                 is_context auto&& ctx,
                 auto&& it,
                 auto end) -> void {
    auto decoded = std::unordered_map<std::string, T>{};
    parse<JSON>::op<Opts>(decoded, ctx, it, end);
    if (ctx.error != error_code::none) {
      return;
    }

    for (auto& [key, mapped] : decoded) {
      if (auto const slot = value.get(key); slot) {
        slot->get() = std::move(mapped);
      }
    }
  }
};

/**
 * @brief frozen_map を JSON オブジェクトへ書き出す
 */
template <typename T, frozenchars::FrozenString... Keys>
struct to<JSON, frozenchars::frozen_map<T, Keys...>> {
  template <auto Opts>
  static auto op(frozenchars::frozen_map<T, Keys...> const& value,
                 is_context auto&& ctx,
                 auto&& b,
                 auto& ix) -> void {
    auto encoded = std::map<std::string, T>{};
    for (auto const& [key, mapped] : value) {
      encoded.emplace(std::string{key}, mapped);
    }
    serialize<JSON>::op<Opts>(encoded, ctx, b, ix);
  }
};

} // namespace glz

#endif
