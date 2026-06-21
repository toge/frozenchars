#pragma once

#include "map.hpp"
#include "detail/glaze_detect.hpp"

#if FROZENCHARS_HAS_GLAZE
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
    if (skip_ws<Opts>(ctx, it, end)) {
      return;
    }
    if (it == end) {
      ctx.error = error_code::unexpected_end;
      return;
    }
    if (*it != '{') {
      ctx.error = error_code::syntax_error;
      return;
    }
    ++it;
    if (skip_ws<Opts>(ctx, it, end)) {
      return;
    }
    if (it == end) {
      ctx.error = error_code::unexpected_end;
      return;
    }
    if (*it == '}') {
      ++it;
      return;
    }

    while (true) {
      if (*it != '"') {
        ctx.error = error_code::syntax_error;
        return;
      }
      ++it;
      auto const key_begin = it;
      skip_string_view(ctx, it, end);
      if (ctx.error != error_code::none) {
        return;
      }
      auto const key = std::string_view{key_begin, static_cast<std::size_t>(it - key_begin)};
      ++it;
      if (skip_ws<Opts>(ctx, it, end)) {
        return;
      }
      if (parse_ws_colon<Opts>(ctx, it, end)) {
        return;
      }
      if (auto const slot = value.get(key); slot) {
        parse<JSON>::op<Opts>(slot->get(), ctx, it, end);
      } else {
        skip_value<JSON>::op<Opts>(ctx, it, end);
      }
      if (ctx.error != error_code::none) {
        return;
      }
      if (skip_ws<Opts>(ctx, it, end)) {
        return;
      }
      if (it == end) {
        ctx.error = error_code::unexpected_end;
        return;
      }
      if (*it == '}') {
        ++it;
        return;
      }
      if (*it != ',') {
        ctx.error = error_code::syntax_error;
        return;
      }
      ++it;
      if (skip_ws<Opts>(ctx, it, end)) {
        return;
      }
      if (it == end) {
        ctx.error = error_code::unexpected_end;
        return;
      }
    }
  };
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
