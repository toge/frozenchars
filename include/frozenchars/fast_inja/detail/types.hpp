#pragma once

#include <cstddef>
#include <ostream>
#include <string_view>

namespace frozenchars::fast_inja {

/// @brief エラーコード
enum class error_code : int {
  none = 0,
  no_read_input = 1,
  unexpected_end = 2,
  unknown_key = 3,
  syntax_error = 4,
  type_mismatch = 5,
  invalid_utf8 = 6,
};

/// @brief error_code をストリームに出力するためのオーバーロード
inline std::ostream& operator<<(std::ostream& os, error_code ec) {
  return os << static_cast<int>(ec);
}

/// @brief エラーコンテキスト
struct error_ctx {
  std::size_t position{};
  error_code ec{error_code::none};
  std::string_view custom_error_message;
};

/// @brief レンダリングモードタグ: エスケープなし（デフォルト）
struct stencil_tag {};

/// @brief レンダリングモードタグ: HTMLエスケープ有効
struct mustache_tag {};

inline constexpr stencil_tag STENCIL_V{};
inline constexpr mustache_tag MUSTACHE_V{};

} // namespace frozenchars::fast_inja
