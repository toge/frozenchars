#pragma once

#include "fast_inja/detail/chunk.hpp"
#include "fast_inja/detail/parse.hpp"
#include "fast_inja/detail/render.hpp"
#include "fast_inja/detail/types.hpp"
#include <expected>
#include <string>
#include <string_view>
#include <utility>

namespace frozenchars::fast_inja {

template <class T>
using expected = std::expected<T, error_ctx>;

/// @brief ランタイムAPIでテンプレートをレンダリングする
/// @tparam Mode stencil_tag（デフォルト）または mustache_tag
/// @param tmpl テンプレート文字列
/// @param value コンテキスト値
/// @return レンダリング結果または error_ctx
template <class Mode = stencil_tag, class T>
[[nodiscard]] inline expected<std::string> render_runtime(std::string_view tmpl, T const& value) {
  auto chunks = detail::parse(tmpl);
  parsed_template pt;
  pt.chunks = std::move(chunks);

  std::string out;
  out.reserve(tmpl.size() * 2);

  auto r = detail::render_chunks<Mode>(out, pt, value, nullptr);
  if (!r) {
    return std::unexpected(r.error());
  }
  return out;
}

/// @brief プリパースド API でテンプレートをレンダリングする
/// @param p 事前にパースされたテンプレート
/// @param value コンテキスト値
/// @return レンダリング結果または error_ctx
template <class Mode = stencil_tag, class T>
[[nodiscard]] inline expected<std::string> render_parsed(parsed_template const& p, T const& value) {
  std::string out;
  auto r = detail::render_chunks<Mode>(out, p, value, nullptr);
  if (!r) {
    return std::unexpected(r.error());
  }
  return out;
}

/// @brief テンプレート文字列をパースして parsed_template を返す
/// @param tmpl テンプレート文字列
/// @return パース済みテンプレート
[[nodiscard]] inline parsed_template parse_template(std::string_view tmpl) {
  parsed_template pt;
  pt.chunks = detail::parse(tmpl);
  return pt;
}

} // namespace frozenchars::fast_inja
