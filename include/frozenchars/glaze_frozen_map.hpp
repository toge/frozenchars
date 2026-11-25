#pragma once

#include "map.hpp"
#include "detail/glaze_detect.hpp"

#if FROZENCHARS_HAS_GLAZE
#include <glaze/json.hpp>

#include <array>
#include <optional>
#include <string>
#include <string_view>
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

enum class unescape_json_key_status {
  ok,
  invalid_escape,
  buffer_too_small,
};

struct unescape_json_key_result {
  unescape_json_key_status status{unescape_json_key_status::ok};
  std::string_view value{};
};

/**
 * @brief JSON のキー文字列（ダブルクォートを除いたエスケープ済みバイト列）を解除する
 *
 * `"\u0061"` のようなエスケープされたキーも frozen_map の NTTP キーと
 * 突き合わせられるようにする。不正なエスケープは invalid_escape、
 * 解除結果がバッファを超える場合は buffer_too_small を返す。
 *
 * @param raw エスケープされたままのキー
 * @param out 解除結果を書き込むバッファ
 * @return unescape_json_key_result 解除結果とステータス
 */
template <std::size_t Capacity>
[[nodiscard]] constexpr auto unescape_json_key(std::string_view raw, std::array<char, Capacity>& out) -> unescape_json_key_result {
  auto const hex = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  std::size_t length = 0;
  auto const emit = [&out, &length](char c) -> bool {
    if (length >= Capacity) return false;
    out[length++] = c;
    return true;
  };
  auto const emit_codepoint = [&emit](unsigned int cp) -> bool {
    if (cp < 0x80) return emit(static_cast<char>(cp));
    if (cp < 0x800) {
      return emit(static_cast<char>(0xC0 | (cp >> 6))) && emit(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    if (cp < 0x10000) {
      return emit(static_cast<char>(0xE0 | (cp >> 12))) &&
             emit(static_cast<char>(0x80 | ((cp >> 6) & 0x3F))) &&
             emit(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return emit(static_cast<char>(0xF0 | (cp >> 18))) &&
           emit(static_cast<char>(0x80 | ((cp >> 12) & 0x3F))) &&
           emit(static_cast<char>(0x80 | ((cp >> 6) & 0x3F))) &&
           emit(static_cast<char>(0x80 | (cp & 0x3F)));
  };

  for (std::size_t i = 0; i < raw.size(); ++i) {
    auto const c = raw[i];
    if (c != '\\') {
      if (!emit(c))
        return {unescape_json_key_status::buffer_too_small, {}};
      continue;
    }
    if (++i >= raw.size())
      return {unescape_json_key_status::invalid_escape, {}};
    switch (raw[i]) {
    case '"':
    case '\\':
    case '/':
      if (!emit(raw[i]))
        return {unescape_json_key_status::buffer_too_small, {}};
      break;
    case 'b':
      if (!emit('\b'))
        return {unescape_json_key_status::buffer_too_small, {}};
      break;
    case 'f':
      if (!emit('\f'))
        return {unescape_json_key_status::buffer_too_small, {}};
      break;
    case 'n':
      if (!emit('\n'))
        return {unescape_json_key_status::buffer_too_small, {}};
      break;
    case 'r':
      if (!emit('\r'))
        return {unescape_json_key_status::buffer_too_small, {}};
      break;
    case 't':
      if (!emit('\t'))
        return {unescape_json_key_status::buffer_too_small, {}};
      break;
    case 'u': {
      auto const read_unit = [&]() -> int {
        if (i + 4 >= raw.size())
          return -1;
        auto value = 0;
        for (auto k = 1uz; k <= 4uz; ++k) {
          auto const digit = hex(raw[i + k]);
          if (digit < 0)
            return -1;
          value = (value << 4) | digit;
        }
        i += 4;
        return value;
      };
      auto unit = read_unit();
      if (unit < 0)
        return {unescape_json_key_status::invalid_escape, {}};
      if (unit >= 0xD800 && unit <= 0xDBFF) { // サロゲートペア
        if (i + 6 >= raw.size() || raw[i + 1] != '\\' || raw[i + 2] != 'u') {
          return {unescape_json_key_status::invalid_escape, {}};
        }
        i += 2;
        auto const low = read_unit();
        if (low < 0xDC00 || low > 0xDFFF)
          return {unescape_json_key_status::invalid_escape, {}};
        unit = 0x10000 + ((unit - 0xD800) << 10) + (low - 0xDC00);
      } else if (unit >= 0xDC00 && unit <= 0xDFFF) {
        return {unescape_json_key_status::invalid_escape, {}};
      }
      if (!emit_codepoint(static_cast<unsigned int>(unit))) {
        return {unescape_json_key_status::buffer_too_small, {}};
      }
      break;
    }
    default:
      return {unescape_json_key_status::invalid_escape, {}};
    }
  }
  return {unescape_json_key_status::ok, std::string_view{out.data(), length}};
}

/**
 * @brief JSON オブジェクトから frozen_map へ読み込む
 *
 * JSON 側に存在するキーのうち、frozen_map に定義されたキーのみ反映し、
 * 未知キーは無視する。キーに JSON エスケープが含まれる場合は解除したうえで
 * 突き合わせる。
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
      auto const raw_key = std::string_view{key_begin, static_cast<std::size_t>(it - key_begin)};
      ++it;
      if (skip_ws<Opts>(ctx, it, end)) {
        return;
      }
      if (parse_ws_colon<Opts>(ctx, it, end)) {
        return;
      }
      // エスケープを含むキーのみ解除したうえで凍結キーと突き合わせる
      auto unescaped = std::array<char, 512>{};
      auto key = std::optional<std::string_view>{raw_key};
      if (raw_key.find('\\') != std::string_view::npos) {
        auto const decoded = unescape_json_key(raw_key, unescaped);
        if (decoded.status == unescape_json_key_status::invalid_escape) {
          ctx.error = error_code::syntax_error;
          return;
        }
        if (decoded.status == unescape_json_key_status::ok) {
          key = decoded.value;
        } else {
          key = std::nullopt;
        }
      }
      if (auto const slot = key ? value.get(*key) : std::nullopt; slot) {
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
 * @brief frozen_map を JSON オブジェクトへ直接書き出す（ヒープ確保・値コピーなし）
 */
template <typename T, frozenchars::FrozenString... Keys>
struct to<JSON, frozenchars::frozen_map<T, Keys...>> {
  template <auto Opts>
  static auto op(frozenchars::frozen_map<T, Keys...> const& value, is_context auto&& ctx, auto&& b, auto& ix) -> void {
    dump('{', b, ix);
    if constexpr (Opts.prettify) {
      ctx.depth += check_indentation_width(Opts);
      dump('\n', b, ix);
      dumpn(check_indentation_char(Opts), ctx.depth, b, ix);
    }
    auto first = true;
    for (auto const& [key, mapped] : value) {
      if (!first) {
        if constexpr (Opts.prettify) {
          dump(",\n", b, ix);
          dumpn(check_indentation_char(Opts), ctx.depth, b, ix);
        } else {
          dump(',', b, ix);
        }
      }
      first = false;
      serialize<JSON>::op<Opts>(key, ctx, b, ix);
      if constexpr (Opts.prettify) {
        dump(": ", b, ix);
      } else {
        dump(':', b, ix);
      }
      serialize<JSON>::op<Opts>(mapped, ctx, b, ix);
      if (bool(ctx.error)) [[unlikely]] {
        return;
      }
    }
    if constexpr (Opts.prettify) {
      ctx.depth -= check_indentation_width(Opts);
      dump('\n', b, ix);
      dumpn(check_indentation_char(Opts), ctx.depth, b, ix);
    }
    dump('}', b, ix);
  }
};

} // namespace glz

#endif
