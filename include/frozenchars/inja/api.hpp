#pragma once

#include <string>
#include <string_view>
#include <expected>
#include "types.hpp"
#include "engine_impl.hpp"

namespace frozenchars::inja {

/// default_function_list を frozenchars::inja 名前空間に公開する
using default_function_list = detail::default_function_list;
using default_environment = detail::default_environment;

/**
 * @brief Collect all function call names from a template at compile time.
 *
 * Used for static_assert validation of unregistered functions.
 * Scans expression blocks, statement blocks, and line statements.
 */
template <auto Src, typename Delims = frozenchars::inja::default_delimiters>
[[nodiscard]] consteval auto extract_template_function_calls()
  -> frozenchars::inja::function_call_set<256> {
  auto constexpr src_sv = Src.sv();
  auto constexpr expr_open = Delims::expression_open.sv();
  auto constexpr expr_close = Delims::expression_close.sv();
  auto constexpr stmt_open = Delims::statement_open.sv();
  auto constexpr stmt_close = Delims::statement_close.sv();
  auto constexpr comment_open = Delims::comment_open.sv();
  auto constexpr comment_close = Delims::comment_close.sv();
  auto constexpr line_prefix = Delims::line_statement_prefix.sv();

  auto result = frozenchars::inja::function_call_set<256>{};
  auto pos = 0uz;

  while (pos < src_sv.size()) {
    // 行文プレフィックスを先頭で確認（行頭のみ）
    auto const at_line_start = (pos == 0) || (src_sv[pos - 1] == '\n');
    if (at_line_start && src_sv.substr(pos, line_prefix.size()) == line_prefix) {
      auto const content_begin = pos + line_prefix.size();
      auto const newline = src_sv.find('\n', content_begin);
      auto const content_end = (newline != std::string_view::npos) ? newline : src_sv.size();
      frozenchars::inja::detail::extract_calls_from_expr(
        src_sv.substr(content_begin, content_end - content_begin), result);
      pos = content_end;
      continue;
    }

    auto const expr_pos = src_sv.find(expr_open, pos);
    auto const stmt_pos = src_sv.find(stmt_open, pos);
    auto const comment_pos = src_sv.find(comment_open, pos);
    auto const next = std::min(
      std::min(
        expr_pos != std::string_view::npos ? expr_pos : std::string_view::npos,
        stmt_pos != std::string_view::npos ? stmt_pos : std::string_view::npos
      ),
      comment_pos != std::string_view::npos ? comment_pos : std::string_view::npos
    );
    if (next == std::string_view::npos) {
      break;
    }

    if (next == comment_pos && (expr_pos == std::string_view::npos || comment_pos < expr_pos) &&
        (stmt_pos == std::string_view::npos || comment_pos < stmt_pos)) {
      auto const content_begin = comment_pos + comment_open.size();
      auto const close = src_sv.find(comment_close, content_begin);
      if (close == std::string_view::npos) {
        break;
      }
      pos = close + comment_close.size();
    } else if (next == expr_pos && (stmt_pos == std::string_view::npos || expr_pos < stmt_pos)) {
      auto const content_begin = expr_pos + expr_open.size();
      auto const close = src_sv.find(expr_close, content_begin);
      if (close == std::string_view::npos) {
        break;
      }
      frozenchars::inja::detail::extract_calls_from_expr(
        src_sv.substr(content_begin, close - content_begin), result);
      pos = close + expr_close.size();
    } else {
      auto const content_begin = stmt_pos + stmt_open.size();
      auto const close = src_sv.find(stmt_close, content_begin);
      if (close == std::string_view::npos) {
        break;
      }
      frozenchars::inja::detail::extract_calls_from_expr(
        src_sv.substr(content_begin, close - content_begin), result);
      pos = close + stmt_close.size();
    }
  }

  return result;
}

/**
 * @brief テンプレートを compile-time で解析し、バイトコードを返す。
 *
 * EnvironmentBinding が function_list の場合は既存互換で扱い、
 * environment の場合はその function_list を取り出して static_assert 検証を行う。
 */
template <auto Src, detail::is_environment_binding EnvironmentBinding = detail::default_environment,
          typename Delims = frozenchars::inja::default_delimiters>
[[nodiscard]] consteval auto compile_template() -> detail::bytecode {
  using function_list_t = typename detail::environment_traits<EnvironmentBinding>::function_list_type;
  auto constexpr calls = extract_template_function_calls<Src, Delims>();
  for (auto i = 0uz; i < calls.count; ++i) {
    if (!function_list_t::contains(calls.get(i))) {
      throw "Template calls function(s) not registered in the FunctionList.";
    }
  }
  return detail::parse_program<Src, Delims>();
}

/**
 * @brief テンプレートをレンダリングする高水準API。
 * @tparam Src テンプレート文字列
 * @param root ルートコンテキスト（frozen_map）
 * @return 出力文字列
 */
template <auto Src, typename Delims = frozenchars::inja::default_delimiters>
  requires (!detail::is_environment_binding<Delims>)
auto render(inja_value const& root, runtime_options_ref runtime_options = std::nullopt) -> std::string {
  if (!std::holds_alternative<inja_object>(root.storage)) {
    throw render_error{"root context must be object"};
  }
  return detail::render_program<Src, detail::default_environment, Delims>(std::get<inja_object>(root.storage), runtime_options);
}

template <auto Src, typename Delims = frozenchars::inja::default_delimiters, typename Context>
  requires (!detail::is_environment_binding<Delims>) && detail::glaze_reflectable<Context>
auto render(Context const& root, runtime_options_ref runtime_options = std::nullopt) -> std::string {
  return detail::render_program<Src, Context, detail::default_environment, Delims>(root, runtime_options);
}

/**
 * @brief テンプレートをレンダリングする高水準API（compile-time FunctionList 指定版）。
 *
 * 指定した FunctionList に含まれない関数名がテンプレート内に存在する場合、
 * static_assert により compile-time エラーになる。
 *
 * @tparam Src          テンプレート文字列
 * @tparam FunctionList compile-time 関数登録テーブル（is_function_list を満たす型）
 * @tparam Delims       テンプレート区切り文字
 *
 * 使用例:
 * @code
 * using my_fns = function_list<fn<"upper", my_upper>, fn<"lower", my_lower>>;
 * constexpr auto src = "{{ upper(name) }}"_fs;
 * auto result = render<src, my_fns>(ctx);
 * @endcode
 */
template <auto Src, detail::is_environment_binding EnvironmentBinding, typename Delims = frozenchars::inja::default_delimiters>
auto render(inja_value const& root, runtime_options_ref runtime_options = std::nullopt) -> std::string {
  using function_list_t = typename detail::environment_traits<EnvironmentBinding>::function_list_type;
  static_assert([]() constexpr -> bool {
    auto constexpr calls = extract_template_function_calls<Src, Delims>();
    for (auto i = 0uz; i < calls.count; ++i) {
      if (!function_list_t::contains(calls.get(i))) {
        return false;
      }
    }
    return true;
  }(),
  "Template calls function(s) not registered in the FunctionList. "
  "Add the missing function(s) to your function_list<...>.");
  if (!std::holds_alternative<inja_object>(root.storage)) {
    throw render_error{"root context must be object"};
  }
  return detail::render_program<Src, EnvironmentBinding, Delims>(std::get<inja_object>(root.storage), runtime_options);
}

template <auto Src, detail::is_environment_binding EnvironmentBinding, typename Delims = frozenchars::inja::default_delimiters, typename Context>
  requires detail::glaze_reflectable<Context>
auto render(Context const& root, runtime_options_ref runtime_options = std::nullopt) -> std::string {
  using function_list_t = typename detail::environment_traits<EnvironmentBinding>::function_list_type;
  static_assert([]() constexpr -> bool {
    auto constexpr calls = extract_template_function_calls<Src, Delims>();
    for (auto i = 0uz; i < calls.count; ++i) {
      if (!function_list_t::contains(calls.get(i))) {
        return false;
      }
    }
    return true;
  }(),
  "Template calls function(s) not registered in the FunctionList. "
  "Add the missing function(s) to your function_list<...>.");
  return detail::render_program<Src, Context, EnvironmentBinding, Delims>(root, runtime_options);
}

/**
 * @brief テンプレートをレンダリングし、カスタム出力バッファへ結果を追加する
 *
 * @tparam Src テンプレート文字列
 * @tparam OutputBuffer append() と result() メソッドを持つクラス
 * @tparam Delims テンプレート区切り文字
 * @param root ルートコンテキスト
 * @param output 出力バッファ（append() メソッドを呼び出される）
 * @return 成功時は void、エラー時は std::string のエラーメッセージ
 */
template <auto Src, typename OutputBuffer, typename Delims = frozenchars::inja::default_delimiters>
  requires requires(OutputBuffer& output, std::string_view chunk) {
    output.append(chunk);
  }
auto render(inja_value const& root, OutputBuffer& output, runtime_options_ref runtime_options = std::nullopt) -> std::expected<void, std::string> {
  try {
    if (!std::holds_alternative<inja_object>(root.storage)) {
      return std::unexpected(std::string{"root context must be object"});
    }
    detail::render_program<Src, OutputBuffer, detail::default_environment, Delims>(std::get<inja_object>(root.storage), output, runtime_options);
    return {};
  } catch (std::exception const& e) {
    return std::unexpected(std::string{e.what()});
  } catch (...) {
    return std::unexpected(std::string{"unknown error during template rendering"});
  }
}

template <auto Src, typename OutputBuffer, typename Delims = frozenchars::inja::default_delimiters, typename Context>
  requires requires(OutputBuffer& output, std::string_view chunk) {
    output.append(chunk);
  } && detail::glaze_reflectable<Context>
auto render(Context const& root, OutputBuffer& output, runtime_options_ref runtime_options = std::nullopt) -> std::expected<void, std::string> {
  try {
    detail::render_program<Src, OutputBuffer, Context, detail::default_environment, Delims>(root, output, runtime_options);
    return {};
  } catch (std::exception const& e) {
    return std::unexpected(std::string{e.what()});
  } catch (...) {
    return std::unexpected(std::string{"unknown error during template rendering"});
  }
}

/**
 * @brief テンプレートをレンダリングし、カスタム出力バッファへ結果を追加する（compile-time FunctionList 指定版）。
 *
 * @tparam Src          テンプレート文字列
 * @tparam FunctionList compile-time 関数登録テーブル
 * @tparam OutputBuffer append() メソッドを持つクラス
 * @tparam Delims       テンプレート区切り文字
 */
template <auto Src, detail::is_environment_binding EnvironmentBinding, typename OutputBuffer, typename Delims = frozenchars::inja::default_delimiters>
  requires requires(OutputBuffer& output, std::string_view chunk) {
    output.append(chunk);
  }
auto render(inja_value const& root, OutputBuffer& output, runtime_options_ref runtime_options = std::nullopt) -> std::expected<void, std::string> {
  using function_list_t = typename detail::environment_traits<EnvironmentBinding>::function_list_type;
  static_assert([]() constexpr -> bool {
    auto constexpr calls = extract_template_function_calls<Src, Delims>();
    for (auto i = 0uz; i < calls.count; ++i) {
      if (!function_list_t::contains(calls.get(i))) {
        return false;
      }
    }
    return true;
  }(),
  "Template calls function(s) not registered in the FunctionList. "
  "Add the missing function(s) to your function_list<...>.");
  try {
    if (!std::holds_alternative<inja_object>(root.storage)) {
      return std::unexpected(std::string{"root context must be object"});
    }
    detail::render_program<Src, OutputBuffer, EnvironmentBinding, Delims>(std::get<inja_object>(root.storage), output, runtime_options);
    return {};
  } catch (std::exception const& e) {
    return std::unexpected(std::string{e.what()});
  } catch (...) {
    return std::unexpected(std::string{"unknown error during template rendering"});
  }
}

template <auto Src, detail::is_environment_binding EnvironmentBinding, typename OutputBuffer, typename Delims = frozenchars::inja::default_delimiters, typename Context>
  requires requires(OutputBuffer& output, std::string_view chunk) {
    output.append(chunk);
  } && detail::glaze_reflectable<Context>
auto render(Context const& root, OutputBuffer& output, runtime_options_ref runtime_options = std::nullopt) -> std::expected<void, std::string> {
  using function_list_t = typename detail::environment_traits<EnvironmentBinding>::function_list_type;
  static_assert([]() constexpr -> bool {
    auto constexpr calls = extract_template_function_calls<Src, Delims>();
    for (auto i = 0uz; i < calls.count; ++i) {
      if (!function_list_t::contains(calls.get(i))) {
        return false;
      }
    }
    return true;
  }(),
  "Template calls function(s) not registered in the FunctionList. "
  "Add the missing function(s) to your function_list<...>.");
  try {
    detail::render_program<Src, OutputBuffer, Context, EnvironmentBinding, Delims>(root, output, runtime_options);
    return {};
  } catch (std::exception const& e) {
    return std::unexpected(std::string{e.what()});
  } catch (...) {
    return std::unexpected(std::string{"unknown error during template rendering"});
  }
}

} // namespace frozenchars::inja
