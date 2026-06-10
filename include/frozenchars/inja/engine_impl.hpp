#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <ranges>
#include <span>
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202306L
#include <inplace_vector>
#endif
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "../inja_access.hpp"
#include "../inja_function.hpp"
#include "../inja_value.hpp"
#include "../string.hpp"
#include "types.hpp"

namespace frozenchars::inja {

namespace detail {

  template <typename T>
  using remove_cvref_t = std::remove_cvref_t<T>;

  template <typename T>
  [[nodiscard]] inline auto try_to_inja_value(T const& value) -> std::optional<inja_value>;

  template <typename T>
  [[nodiscard]] inline auto convert_map_like(T const& value) -> std::optional<inja_value> {
    auto out = inja_object{};
    if constexpr (requires { value.size(); }) {
      out.reserve(static_cast<std::size_t>(value.size()));
    }
    for (auto const& [k, v] : value) {
      auto converted = try_to_inja_value(v);
      if (!converted.has_value()) {
        return std::nullopt;
      }
      out.emplace(std::string{std::string_view{k}}, std::move(*converted));
    }
    return inja_value{std::move(out)};
  }

  template <typename T>
  [[nodiscard]] inline auto convert_range_like(T const& value) -> std::optional<inja_value> {
    auto out = std::vector<inja_value>{};
    if constexpr (requires { value.size(); }) {
      out.reserve(static_cast<std::size_t>(value.size()));
    }
    for (auto const& element : value) {
      auto converted = try_to_inja_value(element);
      if (!converted.has_value()) {
        return std::nullopt;
      }
      out.push_back(std::move(*converted));
    }
    return inja_value{inja_array{std::move(out)}};
  }

#if FROZENCHARS_HAS_GLAZE
  template <typename T>
  [[nodiscard]] inline auto convert_reflectable(T const& value) -> std::optional<inja_value> {
    using U              = remove_cvref_t<T>;
    auto           out   = inja_object{};
    constexpr auto count = static_cast<std::size_t>(glz::reflect<U>::size);
    out.reserve(count);
    auto tied = glz::to_tie(value);
    auto ok   = true;
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
      (
          [&] {
            if (!ok) {
              return;
            }
            auto converted = try_to_inja_value(glz::get<Is>(tied));
            if (!converted.has_value()) {
              ok = false;
              return;
            }
            out.emplace(std::string{std::string_view{glz::reflect<U>::keys[Is]}}, std::move(*converted));
          }(),
          ...);
    }(std::make_index_sequence<count>{});
    if (!ok) {
      return std::nullopt;
    }
    return inja_value{std::move(out)};
  }
#endif

  template <typename T>
  [[nodiscard]] inline auto try_to_inja_value(T const& value) -> std::optional<inja_value> {
    using U = remove_cvref_t<T>;
    if constexpr (std::same_as<U, inja_value>) {
      return value;
    } else if constexpr (std::same_as<U, std::nullptr_t>) {
      return inja_value{};
    } else if constexpr (std::same_as<U, bool>) {
      return inja_value{value};
    } else if constexpr (std::integral<U> && !std::same_as<U, bool>) {
      return inja_value{static_cast<std::int64_t>(value)};
    } else if constexpr (std::floating_point<U>) {
      return inja_value{static_cast<double>(value)};
    } else if constexpr (std::same_as<U, std::string>) {
      return inja_value{value};
    } else if constexpr (std::same_as<U, std::string_view>) {
      return inja_value{std::string{value}};
    } else if constexpr (std::is_convertible_v<U, std::string_view>) {
      return inja_value{std::string{std::string_view{value}}};
    } else if constexpr (map_like<U>) {
      return convert_map_like(value);
    } else if constexpr (range_like<U>) {
      return convert_range_like(value);
#if FROZENCHARS_HAS_GLAZE
    } else if constexpr (glaze_reflectable<U>) {
      return convert_reflectable(value);
#endif
    } else {
      return std::nullopt;
    }
  }

  template <typename T>
  [[nodiscard]] inline auto to_inja_value_or_throw(T const& value) -> inja_value {
    auto converted = try_to_inja_value(value);
    if (!converted.has_value()) {
      throw render_error{"value cannot be converted to inja_value"};
    }
    return std::move(*converted);
  }

  /**
   * @brief 空白文字判定（テンプレート用）。
   * @param c 判定対象文字
   * @return 空白なら true
   */
  [[nodiscard]] constexpr auto is_space(char c) -> bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
  }

  /**
   * @brief 文字列ビューの前後空白を除去する。
   * @param s 入力文字列ビュー
   * @return trim 後のビュー
   */
  [[nodiscard]] constexpr auto trim_view(std::string_view s) -> std::string_view {
    auto begin = 0uz;
    auto end   = s.size();
    while (begin < end && is_space(s[begin])) {
      ++begin;
    }
    while (end > begin && is_space(s[end - 1])) {
      --end;
    }
    return s.substr(begin, end - begin);
  }

  [[nodiscard]] constexpr auto is_ident_start_char(char c) -> bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
  }

  [[nodiscard]] constexpr auto is_ident_continue_char(char c) -> bool {
    return is_ident_start_char(c) || (c >= '0' && c <= '9');
  }

  [[nodiscard]] constexpr auto is_simple_path_expression(std::string_view expr) -> bool {
    if (expr.empty() || !is_ident_start_char(expr.front())) {
      return false;
    }
    auto prev_dot = false;
    for (auto i = std::size_t{0}; i < expr.size(); ++i) {
      auto const c = expr[i];
      if (c == '.') {
        if (prev_dot || i == expr.size() - 1) {
          return false;
        }
        prev_dot = true;
        continue;
      }
      if (!is_ident_continue_char(c)) {
        return false;
      }
      prev_dot = false;
    }
    return true;
  }

  enum class open_kind : std::uint8_t {
    expression,      // 式
    statement,       // 文
    comment,         // コメント
    line_statement,  // 行文
    none,            // 該当なし
  };

  struct open_match {
    std::size_t pos{std::string_view::npos};
    open_kind   kind{open_kind::none};
  };

  /**
   * @brief 行文プレフィックスを検索する。
   *
   * @tparam Delims デリミタ型
   * @param src 入力文字列ビュー
   * @param from 検索開始位置
   * @return 見つかった位置、見つからなければ std::string_view::npos
   */
  template <typename Delims>
  [[nodiscard]] constexpr auto find_line_statement(std::string_view src, std::size_t from) -> std::size_t {
    auto const prefix = Delims::line_statement_prefix.sv();
    if (prefix.empty()) {
      return std::string_view::npos;
    }
    auto pos = src.find(prefix, from);
    while (pos != std::string_view::npos) {
      if (pos == 0 || src[pos - 1] == '\n') {
        return pos;
      }
      pos = src.find(prefix, pos + 1);
    }
    return std::string_view::npos;
  }

  /**
   * @brief 次のタグオープンを検索する。
   *
   * @tparam Delims デリミタ型
   * @param src 入力文字列ビュー
   * @param from 検索開始位置
   * @return 見つかったタグオープン情報
   */
  template <typename Delims>
  [[nodiscard]] constexpr auto find_next_open(std::string_view src, std::size_t from) -> open_match {
    auto best = open_match{};

    auto consider = [&](std::size_t pos, open_kind kind) constexpr {
      if (pos == std::string_view::npos) {
        return;
      }
      if (best.pos == std::string_view::npos || pos < best.pos) {
        best = {pos, kind};
        return;
      }
      if (pos == best.pos) {
        auto const priority = [&](open_kind k) constexpr -> int {
          switch (k) {
          case open_kind::expression:
            return 0;
          case open_kind::statement:
            return 1;
          case open_kind::comment:
            return 2;
          case open_kind::line_statement:
            return 3;
          case open_kind::none:
            return 4;
          }
          return 4;
        };
        if (priority(kind) < priority(best.kind)) {
          best = {pos, kind};
        }
      }
    };

    consider(src.find(Delims::expression_open.sv(), from), open_kind::expression);
    consider(src.find(Delims::statement_open.sv(), from), open_kind::statement);
    consider(src.find(Delims::comment_open.sv(), from), open_kind::comment);
    consider(find_line_statement<Delims>(src, from), open_kind::line_statement);
    return best;
  }

  /**
   * @brief テンプレート本体を consteval でパースして内部バイトコードを構築する。
   * @tparam Src FrozenString NTTP
   * @return パース済みバイトコード
   *
   * @throw consteval 文脈で文字列リテラル例外を投げる。
   * 不正構文（未閉鎖タグ、ネスト不整合、未対応文）で失敗する。
   */
  template <auto Src, typename Delims = frozenchars::inja::default_delimiters>
  [[nodiscard]] consteval auto parse_program() -> bytecode {
    auto constexpr src              = Src.sv();
    auto program                    = bytecode{};
    auto constexpr expr_open        = Delims::expression_open.sv();
    auto constexpr expr_close       = Delims::expression_close.sv();
    auto constexpr stmt_open        = Delims::statement_open.sv();
    auto constexpr stmt_close       = Delims::statement_close.sv();
    auto constexpr comment_open     = Delims::comment_open.sv();
    auto constexpr comment_close    = Delims::comment_close.sv();
    auto constexpr line_stmt_prefix = Delims::line_statement_prefix.sv();

    if (expr_open.empty() || expr_close.empty() || stmt_open.empty() || stmt_close.empty() || comment_open.empty() || comment_close.empty()) {
      throw "template parse error: delimiters must not be empty";
    }

    auto stack         = std::array<std::size_t, MAX_NODES>{};
    auto stack_size    = 0uz;
    auto for_depth     = 0uz;
    auto max_for_depth = 0uz;

    auto push_node = [&](node_kind kind, std::size_t begin, std::size_t end) consteval -> std::size_t {
      if (program.count >= MAX_NODES) {
        throw "template parse error: too many nodes";
      }
      auto const index     = program.count++;
      program.nodes[index] = node{kind, begin, end};
      return index;
    };

    // 1. text ブロックを先に切り出す
    // 2. {{ }}, {# #}, {% %} を識別してノード化
    // 3. if/for の対応関係は stack で後方解決
    auto process_statement = [&](std::string_view stmt, std::size_t begin, std::size_t end) consteval {
      auto const stmt_offset = static_cast<std::size_t>(stmt.data() - src.data());
      if (stmt.starts_with("if ")) {
        auto const idx       = push_node(node_kind::if_stmt, begin, end);
        auto const cond_body = stmt.substr(3);
        auto const cond_sv   = trim_view(cond_body);
        if (cond_sv.empty()) {
          throw "template parse error: invalid if statement";
        }
        program.nodes[idx].aux_begin              = stmt_offset + 3 + static_cast<std::size_t>(cond_sv.data() - cond_body.data());
        program.nodes[idx].aux_end                = program.nodes[idx].aux_begin + cond_sv.size();
        program.nodes[idx].if_cond_is_simple_path = is_simple_path_expression(cond_sv);
        stack[stack_size]                         = idx;
        ++stack_size;
        return;
      }
      // else / else if を同じノード種別で連結し、else_index で鎖状に接続する。
      if (stmt == "else" || stmt.starts_with("else if ")) {
        if (stack_size == 0 || program.nodes[stack[stack_size - 1]].kind != node_kind::if_stmt) {
          throw "template parse error: unexpected else";
        }
        auto const if_idx   = stack[stack_size - 1];
        auto const else_idx = push_node(node_kind::else_stmt, begin, end);
        if (!stmt.starts_with("else if ")) {
          program.nodes[else_idx].is_plain_else = true;
        }

        if (program.nodes[if_idx].else_index == std::numeric_limits<std::size_t>::max()) {
          program.nodes[if_idx].else_index = else_idx;
        } else {
          auto tail_else = program.nodes[if_idx].else_index;
          while (program.nodes[tail_else].else_index != std::numeric_limits<std::size_t>::max()) {
            tail_else = program.nodes[tail_else].else_index;
          }
          if (program.nodes[tail_else].is_plain_else) {
            throw "template parse error: unexpected else";
          }
          program.nodes[tail_else].else_index = else_idx;
        }

        if (stmt.starts_with("else if ")) {
          auto const cond_body = stmt.substr(7);
          auto const cond_sv   = trim_view(cond_body);
          if (cond_sv.empty()) {
            throw "template parse error: invalid if statement";
          }
          program.nodes[else_idx].aux_begin              = stmt_offset + 7 + static_cast<std::size_t>(cond_sv.data() - cond_body.data());
          program.nodes[else_idx].aux_end                = program.nodes[else_idx].aux_begin + cond_sv.size();
          program.nodes[else_idx].if_cond_is_simple_path = is_simple_path_expression(cond_sv);
        }
        return;
      }
      if (stmt == "endif") {
        if (stack_size == 0 || program.nodes[stack[stack_size - 1]].kind != node_kind::if_stmt) {
          throw "template parse error: unexpected endif";
        }
        auto const end_idx              = push_node(node_kind::endif_stmt, begin, end);
        auto const if_idx               = stack[stack_size - 1];
        program.nodes[if_idx].end_index = end_idx;
        --stack_size;
        return;
      }
      if (stmt.starts_with("for ")) {
        if (stmt.find(" in ") == std::string_view::npos) {
          throw "template parse error: invalid for statement";
        }
        auto const idx          = push_node(node_kind::for_stmt, begin, end);
        auto       body         = stmt.substr(4);
        auto const in_pos_local = body.find(" in ");
        auto const lhs          = trim_view(body.substr(0, in_pos_local));
        auto const rhs          = trim_view(body.substr(in_pos_local + 4));
        if (rhs.empty()) {
          throw "template parse error: invalid for statement";
        }
        auto const comma = lhs.find(',');
        if (comma == std::string_view::npos) {
          auto const val_sv = trim_view(lhs);
          if (val_sv.empty()) {
            throw "template parse error: invalid for statement";
          }
          program.nodes[idx].aux2_begin  = stmt_offset + 4 + static_cast<std::size_t>(val_sv.data() - body.data());
          program.nodes[idx].aux2_end    = program.nodes[idx].aux2_begin + val_sv.size();
          program.nodes[idx].for_has_key = false;
        } else {
          auto const key_sv = trim_view(lhs.substr(0, comma));
          auto const val_sv = trim_view(lhs.substr(comma + 1));
          if (key_sv.empty() || val_sv.empty()) {
            throw "template parse error: invalid for statement";
          }
          program.nodes[idx].aux3_begin  = stmt_offset + 4 + static_cast<std::size_t>(key_sv.data() - body.data());
          program.nodes[idx].aux3_end    = program.nodes[idx].aux3_begin + key_sv.size();
          program.nodes[idx].aux2_begin  = stmt_offset + 4 + static_cast<std::size_t>(val_sv.data() - body.data());
          program.nodes[idx].aux2_end    = program.nodes[idx].aux2_begin + val_sv.size();
          program.nodes[idx].for_has_key = true;
        }
        program.nodes[idx].aux_begin               = stmt_offset + 4 + static_cast<std::size_t>(rhs.data() - body.data());
        program.nodes[idx].aux_end                 = program.nodes[idx].aux_begin + rhs.size();
        program.nodes[idx].for_iter_is_simple_path = is_simple_path_expression(rhs);
        stack[stack_size]                          = idx;
        ++stack_size;
        ++for_depth;
        if (for_depth > max_for_depth) {
          max_for_depth = for_depth;
        }
        return;
      }
      if (stmt == "endfor") {
        if (stack_size == 0 || program.nodes[stack[stack_size - 1]].kind != node_kind::for_stmt) {
          throw "template parse error: unexpected endfor";
        }
        auto const for_idx               = stack[stack_size - 1];
        auto const end_idx               = push_node(node_kind::endfor_stmt, begin, end);
        program.nodes[for_idx].end_index = end_idx;
        --stack_size;
        --for_depth;
        return;
      }
      // set/include は単独ステートメントノードとして保持し、実行時に評価する。
      if (stmt.starts_with("set ")) {
        auto const idx          = push_node(node_kind::set_stmt, begin, end);
        auto       body         = stmt.substr(4);
        auto const eq_pos_local = body.find('=');
        if (eq_pos_local == std::string_view::npos) {
          throw "template parse error: invalid set statement";
        }
        auto const target_sv = trim_view(body.substr(0, eq_pos_local));
        auto const expr_sv   = trim_view(body.substr(eq_pos_local + 1));
        if (target_sv.empty() || expr_sv.empty()) {
          throw "template parse error: invalid set statement";
        }
        program.nodes[idx].aux2_begin = stmt_offset + 4 + static_cast<std::size_t>(target_sv.data() - body.data());
        program.nodes[idx].aux2_end   = program.nodes[idx].aux2_begin + target_sv.size();
        program.nodes[idx].aux_begin  = stmt_offset + 4 + static_cast<std::size_t>(expr_sv.data() - body.data());
        program.nodes[idx].aux_end    = program.nodes[idx].aux_begin + expr_sv.size();
        return;
      }
      if (stmt.starts_with("include ")) {
        auto const idx       = push_node(node_kind::include_stmt, begin, end);
        auto const name_body = stmt.substr(8);
        auto const name_sv   = trim_view(name_body);
        if (name_sv.empty()) {
          throw "template parse error: invalid include statement";
        }
        program.nodes[idx].aux_begin                   = stmt_offset + 8 + static_cast<std::size_t>(name_sv.data() - name_body.data());
        program.nodes[idx].aux_end                     = program.nodes[idx].aux_begin + name_sv.size();
        program.nodes[idx].include_expr_is_simple_path = is_simple_path_expression(name_sv);
        return;
      }
      throw "template parse error: unsupported statement";
    };

    auto pos = 0uz;
    while (pos < src.size()) {
      auto const next = find_next_open<Delims>(src, pos);
      auto const tag  = next.pos;
      if (tag == std::string_view::npos) {
        if (pos < src.size()) {
          std::ignore = push_node(node_kind::text, pos, src.size());
        }
        break;
      }
      if (tag > pos) {
        std::ignore = push_node(node_kind::text, pos, tag);
      }
      switch (next.kind) {
      // 式
      case open_kind::expression: {
        auto const content_begin = tag + expr_open.size();
        auto const close         = src.find(expr_close, content_begin);
        if (close == std::string_view::npos) {
          throw "template parse error: unclosed expression tag";
        }
        auto const expr_idx                         = push_node(node_kind::expr, content_begin, close);
        auto const expr_body                        = src.substr(content_begin, close - content_begin);
        auto const expr_sv                          = trim_view(expr_body);
        program.nodes[expr_idx].aux_begin           = content_begin + static_cast<std::size_t>(expr_sv.data() - expr_body.data());
        program.nodes[expr_idx].aux_end             = program.nodes[expr_idx].aux_begin + expr_sv.size();
        program.nodes[expr_idx].expr_is_simple_path = is_simple_path_expression(expr_sv);
        pos                                         = close + expr_close.size();
        break;
      }
      // コメント
      case open_kind::comment: {
        auto const content_begin = tag + comment_open.size();
        auto const close         = src.find(comment_close, content_begin);
        if (close == std::string_view::npos) {
          throw "template parse error: unclosed comment tag";
        }
        pos = close + comment_close.size();
        break;
      }
      // 文
      case open_kind::statement: {
        auto const content_begin = tag + stmt_open.size();
        auto const close         = src.find(stmt_close, content_begin);
        if (close == std::string_view::npos) {
          throw "template parse error: unclosed statement tag";
        }
        auto const stmt = trim_view(src.substr(content_begin, close - content_begin));
        process_statement(stmt, content_begin, close);
        pos = close + stmt_close.size();
        break;
      }
      // 行文
      case open_kind::line_statement: {
        auto const content_begin = tag + line_stmt_prefix.size();
        auto const line_end      = src.find('\n', content_begin);
        auto const content_end   = (line_end == std::string_view::npos) ? src.size() : line_end;
        auto const stmt          = trim_view(src.substr(content_begin, content_end - content_begin));
        process_statement(stmt, content_begin, content_end);
        pos = (line_end == std::string_view::npos) ? src.size() : line_end + 1;
        break;
      }
      // 該当なし
      case open_kind::none:
        throw "template parse error: internal opener resolution failure";
      }
    }

    if (stack_size != 0) {
      throw "template parse error: unclosed block";
    }
    program.max_for_depth = max_for_depth;
    return program;
  }

  /**
   * @brief inja_value を数値（double）として扱えるか試す。
   * @param v 対象値
   * @return 数値化できる場合は値、できない場合は std::nullopt
   */
  [[nodiscard]] inline auto try_as_double(inja_value const& v) -> std::optional<double> {
    if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
      return static_cast<double>(*p);
    }
    if (auto const* p = std::get_if<double>(&v.storage)) {
      return *p;
    }
    return std::nullopt;
  }

  /**
   * @brief テンプレート言語の等価比較を行う。
   * @param lhs 左辺
   * @param rhs 右辺
   * @return 等しい場合 true
   */
  [[nodiscard]] inline auto equals_value(inja_value const& lhs, inja_value const& rhs) -> bool {
    if (lhs.storage.index() == rhs.storage.index()) {
      if (std::holds_alternative<inja_null>(lhs.storage)) {
        return true;
      }
      if (auto const* p = std::get_if<bool>(&lhs.storage)) {
        return *p == std::get<bool>(rhs.storage);
      }
      if (auto const* p = std::get_if<std::int64_t>(&lhs.storage)) {
        return *p == std::get<std::int64_t>(rhs.storage);
      }
      if (auto const* p = std::get_if<double>(&lhs.storage)) {
        return *p == std::get<double>(rhs.storage);
      }
      if (auto const* p = std::get_if<std::string>(&lhs.storage)) {
        return *p == std::get<std::string>(rhs.storage);
      }
      if (auto const* p = std::get_if<inja_array>(&lhs.storage)) {
        return *p == std::get<inja_array>(rhs.storage);
      }
      if (auto const* p = std::get_if<inja_object>(&lhs.storage)) {
        return *p == std::get<inja_object>(rhs.storage);
      }
      return false;
    }
    auto const lnum = try_as_double(lhs);
    auto const rnum = try_as_double(rhs);
    return lnum.has_value() && rnum.has_value() && *lnum == *rnum;
  }

  /**
   * @brief テンプレート言語の大小比較（<）を行う。
   * @param lhs 左辺
   * @param rhs 右辺
   * @return lhs < rhs の結果
   * @throws render_error 比較不能な型の組み合わせ
   */
  [[nodiscard]] inline auto less_value(inja_value const& lhs, inja_value const& rhs) -> bool {
    if (auto const lnum = try_as_double(lhs); lnum.has_value()) {
      auto const rnum = try_as_double(rhs);
      if (!rnum.has_value()) {
        throw render_error{"comparison requires same type"};
      }
      return *lnum < *rnum;
    }
    if (std::holds_alternative<std::string>(lhs.storage) && std::holds_alternative<std::string>(rhs.storage)) {
      return std::get<std::string>(lhs.storage) < std::get<std::string>(rhs.storage);
    }
    throw render_error{"comparison requires same type"};
  }

  /**
   * @brief inja_value の文字列表現を出力バッファへ直接書き込む。
   *
   * @param out 追記先バッファ（std::string または append(string_view) を持つ型）
   * @param v 書き込む値
   * @throws render_error 文字列化できない型（array / object）の場合
   */
  template <typename OutputBuffer>
  inline auto append_value(OutputBuffer& out, inja_value const& v) -> void {
    if (auto const* p = std::get_if<std::string>(&v.storage)) {
      out.append(std::string_view{*p});
      return;
    }
    if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
      auto buf             = std::array<char, 24>{};
      auto const [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), *p);
      if (ec != std::errc{}) {
        throw render_error{"integer to string conversion failed"};
      }
      out.append(std::string_view{buf.data(), static_cast<std::size_t>(end - buf.data())});
      return;
    }
    if (auto const* p = std::get_if<double>(&v.storage)) {
      auto buf = std::array<char, 32>{};
      if (!std::isfinite(*p)) {
        throw render_error{"double to string conversion failed"};
      }
      auto const [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), *p);
      if (ec != std::errc{}) {
        throw render_error{"double to string conversion failed"};
      }
      out.append(std::string_view{buf.data(), static_cast<std::size_t>(end - buf.data())});
      return;
    }
    if (auto const* p = std::get_if<bool>(&v.storage)) {
      out.append(*p ? std::string_view{"true"} : std::string_view{"false"});
      return;
    }
    if (std::holds_alternative<inja_null>(v.storage)) {
      out.append(std::string_view{"null"});
      return;
    }
    if (std::holds_alternative<inja_array>(v.storage)) {
      out.append(std::string_view{"[array]"});
      return;
    }
    if (std::holds_alternative<inja_object>(v.storage)) {
      out.append(std::string_view{"{object}"});
      return;
    }
    throw render_error{"cannot convert value to string"};
  }

  // ============================================================
  // field_appender / make_appender_for / fill_appender_table
  // ============================================================

  /**
   * @brief フィールド参照→文字列化のコンパイル時特殊化。
   *
   * `accessor<C, "name">::resolve(ctx)` の戻り型に応じて out へ直接 append する。
   * inja_value を経由しないので、boxing コストがゼロ。
   */
  template <typename Ctx, fixed_string Field>
  struct field_appender {
    using accessor_t = accessor<Ctx, Field>;
    using field_type = typename accessor_t::field_type;

    static auto append(void const* ctx, std::string& out) -> void {
      auto const& c = *static_cast<Ctx const*>(ctx);
      auto const& v = accessor_t::resolve(c);
      using V       = std::remove_cvref_t<decltype(v)>;
      if constexpr (std::is_same_v<V, std::string>) {
        out.append(v);
      } else if constexpr (std::is_same_v<V, std::string_view>) {
        out.append(v);
      } else if constexpr (std::same_as<V, bool>) {
        out.append(v ? std::string_view{"true"} : std::string_view{"false"});
      } else if constexpr (std::integral<V> && !std::same_as<V, bool>) {
        auto buf             = std::array<char, 24>{};
        auto const [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), static_cast<std::int64_t>(v));
        if (ec != std::errc{}) {
          throw render_error{"integer to string conversion failed"};
        }
        out.append(std::string_view{buf.data(), static_cast<std::size_t>(end - buf.data())});
      } else if constexpr (std::floating_point<V>) {
        auto buf = std::array<char, 32>{};
        if (!std::isfinite(v)) {
          throw render_error{"double to string conversion failed"};
        }
        auto const [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), static_cast<double>(v));
        if (ec != std::errc{}) {
          throw render_error{"double to string conversion failed"};
        }
        out.append(std::string_view{buf.data(), static_cast<std::size_t>(end - buf.data())});
      } else {
        auto converted = try_to_inja_value(v);
        if (converted.has_value()) {
          append_value(out, *converted);
        }
      }
    }
  };

  /**
   * @brief glaze 反射型コンテキスト用の appender をセグメント名から検索する。
   */
  template <typename Ctx>
  auto make_appender_for(std::string_view seg) -> void (*)(void const*, std::string&) {
    if constexpr (glaze_reflectable<Ctx>) {
      constexpr auto keys                       = glz::reflect<Ctx>::keys;
      void (*result)(void const*, std::string&) = nullptr;
      auto matched                              = false;
      [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        [[maybe_unused]] auto try_match = [&]<std::size_t I>() {
          if (matched)
            return;
          if (seg != std::string_view{keys[I]})
            return;
          constexpr auto field_name = make_key_fixed_string<I, Ctx>();
          result                    = &field_appender<Ctx, field_name>::append;
          matched                   = true;
        };
        (try_match.template operator()<Is>(), ...);
      }(std::make_index_sequence<keys.size()>{});
      return result;
    } else {
      return nullptr;
    }
  }

  /**
   * @brief bytecode の simple_path ノード全てに appender を埋める。
   */
  template <typename RootContext>
  auto fill_appender_table(bytecode& program) -> void {
    for (std::size_t i = 0; i < program.count; ++i) {
      auto& n = program.nodes[i];
      if (n.expr_e_kind != expr_kind::simple_path)
        continue;
      if (n.path_depth == 0 || n.path_depth > 4)
        continue;
      auto const seg0   = seg_to_sv(n.path_seg_0);
      n.simple_appender = make_appender_for<RootContext>(seg0);
    }
  }

  /// @brief Forward declaration は不要（builtin_function_list + evaluate_function<FunctionList> が担う）

  /**
   * @brief builtin 関数ラッパー群。
   *
   * static unordered_map + std::function による runtime dispatch を排除し、
   * 各関数を独立した named function として定義する。
   * これにより function_list<fn<"upper", bfn_upper>, ...> を用いた
   * compile-time fold expression dispatch が可能になる。
   */

  inline auto bfn_upper(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"upper() expects 1 argument"};
    }
    if (!std::holds_alternative<std::string>(a[0].storage)) {
      throw render_error{"upper() expects string argument"};
    }
    return inja_value{fn_upper(std::get<std::string>(a[0].storage))};
  }

  inline auto bfn_lower(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"lower() expects 1 argument"};
    }
    if (!std::holds_alternative<std::string>(a[0].storage)) {
      throw render_error{"lower() expects string argument"};
    }
    return inja_value{fn_lower(std::get<std::string>(a[0].storage))};
  }

  inline auto bfn_capitalize(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"capitalize() expects 1 argument"};
    }
    if (!std::holds_alternative<std::string>(a[0].storage)) {
      throw render_error{"capitalize() expects string argument"};
    }
    return inja_value{fn_capitalize(std::get<std::string>(a[0].storage))};
  }

  inline auto bfn_replace(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 3) {
      throw render_error{"replace() expects 3 arguments"};
    }
    if (!std::holds_alternative<std::string>(a[0].storage) || !std::holds_alternative<std::string>(a[1].storage) || !std::holds_alternative<std::string>(a[2].storage)) {
      throw render_error{"replace() expects 3 string arguments"};
    }
    return inja_value{fn_replace(std::get<std::string>(a[0].storage), std::get<std::string>(a[1].storage), std::get<std::string>(a[2].storage))};
  }

  inline auto bfn_length(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"length() expects 1 argument"};
    }
    if (!std::holds_alternative<inja_array>(a[0].storage)) {
      throw render_error{"length() expects array argument"};
    }
    return inja_value{fn_length(std::get<inja_array>(a[0].storage))};
  }

  inline auto bfn_first(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"first() expects 1 argument"};
    }
    if (!std::holds_alternative<inja_array>(a[0].storage)) {
      throw render_error{"first() expects array argument"};
    }
    return fn_first(std::get<inja_array>(a[0].storage));
  }

  inline auto bfn_last(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"last() expects 1 argument"};
    }
    if (!std::holds_alternative<inja_array>(a[0].storage)) {
      throw render_error{"last() expects array argument"};
    }
    return fn_last(std::get<inja_array>(a[0].storage));
  }

  inline auto bfn_join(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 2) {
      throw render_error{"join() expects 2 arguments"};
    }
    if (!std::holds_alternative<inja_array>(a[0].storage)) {
      throw render_error{"join() expects first argument to be array"};
    }
    if (!std::holds_alternative<std::string>(a[1].storage)) {
      throw render_error{"join() expects second argument to be string"};
    }
    return inja_value{fn_join(std::get<inja_array>(a[0].storage), std::get<std::string>(a[1].storage))};
  }

  inline auto bfn_sort(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"sort() expects 1 argument"};
    }
    if (!std::holds_alternative<inja_array>(a[0].storage)) {
      throw render_error{"sort() expects array argument"};
    }
    return inja_value{fn_sort(std::get<inja_array>(a[0].storage))};
  }

  inline auto bfn_range(std::vector<inja_value> const& a) -> inja_value {
    switch (a.size()) {
    case 1:
      if (!std::holds_alternative<std::int64_t>(a[0].storage)) {
        throw render_error{"range() expects integer argument"};
      }
      return inja_value{fn_range(std::get<std::int64_t>(a[0].storage))};
    case 2:
      if (!std::holds_alternative<std::int64_t>(a[0].storage) || !std::holds_alternative<std::int64_t>(a[1].storage)) {
        throw render_error{"range() expects integer arguments"};
      }
      return inja_value{fn_range(std::get<std::int64_t>(a[0].storage), std::get<std::int64_t>(a[1].storage))};
    case 3:
      if (!std::holds_alternative<std::int64_t>(a[0].storage) || !std::holds_alternative<std::int64_t>(a[1].storage) || !std::holds_alternative<std::int64_t>(a[2].storage)) {
        throw render_error{"range() expects integer arguments"};
      }
      return inja_value{fn_range(std::get<std::int64_t>(a[0].storage), std::get<std::int64_t>(a[1].storage), std::get<std::int64_t>(a[2].storage))};
    default:
      throw render_error{"range() expects 1, 2, or 3 arguments"};
    }
  }

  inline auto bfn_abs(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"abs() expects 1 argument"};
    }
    return fn_abs(a[0]);
  }

  inline auto bfn_round(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() == 1) {
      return fn_round(a[0]);
    }
    if (a.size() == 2) {
      return fn_round(a[0], a[1]);
    }
    throw render_error{"round() expects 1 or 2 arguments"};
  }

  inline auto bfn_max(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"max() expects 1 argument"};
    }
    if (!std::holds_alternative<inja_array>(a[0].storage)) {
      throw render_error{"max() expects array argument"};
    }
    return fn_max(std::get<inja_array>(a[0].storage));
  }

  inline auto bfn_min(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"min() expects 1 argument"};
    }
    if (!std::holds_alternative<inja_array>(a[0].storage)) {
      throw render_error{"min() expects array argument"};
    }
    return fn_min(std::get<inja_array>(a[0].storage));
  }

  inline auto bfn_even(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"even() expects 1 argument"};
    }
    return fn_even(a[0]);
  }

  inline auto bfn_odd(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"odd() expects 1 argument"};
    }
    return fn_odd(a[0]);
  }

  inline auto bfn_divisibleBy(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 2) {
      throw render_error{"divisibleBy() expects 2 arguments"};
    }
    return fn_divisibleBy(a[0], a[1]);
  }

  inline auto bfn_int(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"int() expects 1 argument"};
    }
    return fn_int(a[0]);
  }

  inline auto bfn_float(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"float() expects 1 argument"};
    }
    return fn_float(a[0]);
  }

  inline auto bfn_isString(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"isString() expects 1 argument"};
    }
    return fn_isString(a[0]);
  }

  inline auto bfn_isArray(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"isArray() expects 1 argument"};
    }
    return fn_isArray(a[0]);
  }

  inline auto bfn_isNumber(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"isNumber() expects 1 argument"};
    }
    return fn_isNumber(a[0]);
  }

  inline auto bfn_isObject(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"isObject() expects 1 argument"};
    }
    return fn_isObject(a[0]);
  }

  inline auto bfn_isBoolean(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"isBoolean() expects 1 argument"};
    }
    return fn_isBoolean(a[0]);
  }

  inline auto bfn_isFloat(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"isFloat() expects 1 argument"};
    }
    return fn_isFloat(a[0]);
  }

  inline auto bfn_isInteger(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"isInteger() expects 1 argument"};
    }
    return fn_isInteger(a[0]);
  }

  inline auto bfn_isNone(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"isNone() expects 1 argument"};
    }
    return fn_isNone(a[0]);
  }

  inline auto bfn_isEmpty(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"isEmpty() expects 1 argument"};
    }
    return fn_isEmpty(a[0]);
  }

  inline auto bfn_default(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 2) {
      throw render_error{"default() expects 2 arguments"};
    }
    return fn_default(a[0], a[1]);
  }

  inline auto bfn_at(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 2) {
      throw render_error{"at() expects 2 arguments"};
    }
    return fn_at(a[0], a[1]);
  }

  inline auto bfn_exists(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"exists() expects 1 argument"};
    }
    return fn_exists(a[0]);
  }

  inline auto bfn_existsIn(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 2) {
      throw render_error{"existsIn() expects 2 arguments"};
    }
    return fn_existsIn(a[0], a[1]);
  }

  inline auto bfn_as_int_array(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"as_int_array() expects 1 argument"};
    }
    return fn_as_int_array(a[0]);
  }

  inline auto bfn_as_double_array(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"as_double_array() expects 1 argument"};
    }
    return fn_as_double_array(a[0]);
  }

  inline auto bfn_as_string_array(std::vector<inja_value> const& a) -> inja_value {
    if (a.size() != 1) {
      throw render_error{"as_string_array() expects 1 argument"};
    }
    return fn_as_string_array(a[0]);
  }

  /// 全 builtin 関数を含む compile-time 関数登録テーブル。
  /// `evaluate_function<default_function_list>` によりデフォルト dispatch に使用する。
  using default_function_list = frozenchars::inja::function_list<
      frozenchars::inja::fn<"upper", bfn_upper>, frozenchars::inja::fn<"lower", bfn_lower>, frozenchars::inja::fn<"capitalize", bfn_capitalize>, frozenchars::inja::fn<"replace", bfn_replace>,
      frozenchars::inja::fn<"length", bfn_length>, frozenchars::inja::fn<"first", bfn_first>, frozenchars::inja::fn<"last", bfn_last>, frozenchars::inja::fn<"join", bfn_join>,
      frozenchars::inja::fn<"sort", bfn_sort>, frozenchars::inja::fn<"range", bfn_range>, frozenchars::inja::fn<"abs", bfn_abs>, frozenchars::inja::fn<"round", bfn_round>,
      frozenchars::inja::fn<"max", bfn_max>, frozenchars::inja::fn<"min", bfn_min>, frozenchars::inja::fn<"even", bfn_even>, frozenchars::inja::fn<"odd", bfn_odd>,
      frozenchars::inja::fn<"divisibleBy", bfn_divisibleBy>, frozenchars::inja::fn<"int", bfn_int>, frozenchars::inja::fn<"float", bfn_float>, frozenchars::inja::fn<"isString", bfn_isString>,
      frozenchars::inja::fn<"isArray", bfn_isArray>, frozenchars::inja::fn<"isNumber", bfn_isNumber>, frozenchars::inja::fn<"isObject", bfn_isObject>,
      frozenchars::inja::fn<"isBoolean", bfn_isBoolean>, frozenchars::inja::fn<"isFloat", bfn_isFloat>, frozenchars::inja::fn<"isInteger", bfn_isInteger>, frozenchars::inja::fn<"isNone", bfn_isNone>,
      frozenchars::inja::fn<"isEmpty", bfn_isEmpty>, frozenchars::inja::fn<"default", bfn_default>, frozenchars::inja::fn<"at", bfn_at>, frozenchars::inja::fn<"exists", bfn_exists>,
      frozenchars::inja::fn<"existsIn", bfn_existsIn>, frozenchars::inja::fn<"as_int_array", bfn_as_int_array>, frozenchars::inja::fn<"as_double_array", bfn_as_double_array>,
      frozenchars::inja::fn<"as_string_array", bfn_as_string_array>>;

  using default_environment = frozenchars::inja::environment<default_function_list, frozenchars::inja::empty_constant_list>;

  /**
   * @brief 関数名と引数から関数を評価する。
   *
   * FunctionList の fold expression dispatch を試みた後、
   * runtime_options のユーザー定義関数にフォールバックする。
   * static unordered_map を使用しないため optimizer がインライン展開可能。
   *
   * @tparam FunctionList  compile-time 関数登録テーブル
   */
  template <is_environment_binding EnvironmentBinding = default_environment>
  [[nodiscard]] inline auto evaluate_function(std::string_view func_name, std::vector<inja_value> const& args, runtime_options_ref runtime_options) -> inja_value {
    using function_list_t = typename environment_traits<EnvironmentBinding>::function_list_type;
    if (auto result = function_list_t::dispatch(func_name, args)) {
      return std::move(*result);
    }
    if (runtime_options.has_value()) {
      auto const& options = runtime_options->get();
      auto const  it      = options.function_call.find(func_name);
      if (it != options.function_call.end()) {
        return it->second(args);
      }
    }
    throw render_error{"unknown function: " + std::string{func_name}};
  }

  struct loop_state {
    std::int64_t index{};
    std::int64_t index1{};
    bool         is_first{};
    bool         is_last{};
  };

  struct local_binding {
    std::string name;
    inja_value  value;
  };

  struct local_frame {
    std::optional<local_binding> key{};
    std::optional<local_binding> value{};
    std::vector<local_binding>   assigned{};
    loop_state                   loop{};
  };

  enum class lookup_status : std::uint8_t {
    not_found,
    not_convertible,
  };

  using lookup_result = std::expected<inja_value, lookup_status>;

  struct path_segments {
    static constexpr auto max_depth = std::size_t{16};

#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202306L
    using storage_type = std::inplace_vector<std::string_view, max_depth>;
#else
    using storage_type = std::array<std::string_view, max_depth>;
#endif

    storage_type storage{};
    std::size_t  size_{};

    auto push_back(std::string_view sv) -> void {
      if (size() >= max_depth) {
        throw render_error{"variable path too deep (max 16 segments)"};
      }
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202306L
      storage.push_back(sv);
#else
      storage[size_++] = sv;
#endif
    }

    [[nodiscard]] auto size() const -> std::size_t {
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202306L
      return storage.size();
#else
      return size_;
#endif
    }

    [[nodiscard]] auto operator[](std::size_t index) const -> std::string_view { return storage[index]; }

    [[nodiscard]] auto front() const -> std::string_view { return storage.front(); }

    [[nodiscard]] auto data() const -> std::string_view const* { return storage.data(); }

    [[nodiscard]] auto span() const -> std::span<std::string_view const> { return {data(), size()}; }
  };

  [[nodiscard]] inline auto split_variable_path(std::string_view name) -> path_segments {
    auto segments = path_segments{};
    auto begin    = std::size_t{0};
    while (begin <= name.size()) {
      auto const end     = name.find('.', begin);
      auto const segment = name.substr(begin, end - begin);
      if (segment.empty()) {
        throw render_error{"empty identifier"};
      }
      segments.push_back(segment);
      if (end == std::string_view::npos) {
        break;
      }
      begin = end + 1;
    }
    return segments;
  }

  [[nodiscard]] inline auto join_segments(std::span<std::string_view const> segments) -> std::string {
    if (segments.empty()) {
      return {};
    }
    auto out   = std::string{};
    auto total = std::size_t{};
    for (auto const segment : segments) {
      total += segment.size();
    }
    total += segments.size() - 1;
    out.reserve(total);
    for (auto i = std::size_t{0}; i < segments.size(); ++i) {
      if (i > 0) {
        out.push_back('.');
      }
      out.append(segments[i]);
    }
    return out;
  }

  [[nodiscard]] inline auto lookup_in_inja_value(inja_value const& value, std::span<std::string_view const> segments, std::size_t index = 0, std::string_view full_path = {}) -> lookup_result {
    if (index >= segments.size()) {
      return value;
    }
    auto const* current = &value;
    for (auto i = index; i < segments.size(); ++i) {
      if (!std::holds_alternative<inja_object>(current->storage)) {
        auto const path = full_path.empty() ? join_segments(segments) : std::string{full_path};
        throw render_error{"cannot resolve path: " + path};
      }
      auto const& obj = std::get<inja_object>(current->storage);
      auto const  it  = obj.find(segments[i]);
      if (it == obj.end()) {
        return std::unexpected(lookup_status::not_found);
      }
      current = &it->second;
    }
    return *current;
  }

  [[nodiscard]] inline auto lookup_in_object_root(inja_object const& root, std::string_view name) -> lookup_result {
    auto const segments = split_variable_path(name);
    auto const it       = root.find(segments.front());
    if (it == root.end()) {
      return std::unexpected(lookup_status::not_found);
    }
    return lookup_in_inja_value(it->second, segments.span(), 1, name);
  }

#if FROZENCHARS_HAS_GLAZE
  template <typename T>
  [[nodiscard]] inline auto lookup_in_typed_value(T const& value, std::span<std::string_view const> segments, std::size_t index = 0, std::string_view full_path = {}) -> lookup_result;

  template <typename T>
  [[nodiscard]] inline auto lookup_in_reflectable(T const& value, std::span<std::string_view const> segments, std::size_t index, std::string_view full_path) -> lookup_result {
    using U                = remove_cvref_t<T>;
    constexpr auto count   = static_cast<std::size_t>(glz::reflect<U>::size);
    auto           tied    = glz::to_tie(value);
    auto const     segment = segments[index];
    auto           found   = false;
    auto           result  = lookup_result{std::unexpected(lookup_status::not_found)};
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
      (
          [&] {
            if (found) {
              return;
            }
            if (segment != std::string_view{glz::reflect<U>::keys[Is]}) {
              return;
            }
            found  = true;
            result = lookup_in_typed_value(glz::get<Is>(tied), segments, index + 1, full_path);
          }(),
          ...);
    }(std::make_index_sequence<count>{});
    if (!found) {
      return std::unexpected(lookup_status::not_found);
    }
    return result;
  }

  template <typename T>
  [[nodiscard]] inline auto lookup_in_typed_value(T const& value, std::span<std::string_view const> segments, std::size_t index, std::string_view full_path) -> lookup_result {
    using U = remove_cvref_t<T>;
    if (index >= segments.size()) {
      auto converted = try_to_inja_value(value);
      if (converted.has_value()) {
        return std::move(*converted);
      }
      return std::unexpected(lookup_status::not_convertible);
    }
    if constexpr (std::same_as<U, inja_value>) {
      return lookup_in_inja_value(value, segments, index, full_path);
    } else if constexpr (map_like<U>) {
      auto const it = value.find(typename U::key_type{segments[index]});
      if (it == value.end()) {
        return std::unexpected(lookup_status::not_found);
      }
      return lookup_in_typed_value(it->second, segments, index + 1, full_path);
    } else if constexpr (glaze_reflectable<U>) {
      return lookup_in_reflectable(value, segments, index, full_path);
    } else {
      auto const path = full_path.empty() ? join_segments(segments) : std::string{full_path};
      throw render_error{"cannot resolve path: " + path};
    }
  }

  template <typename Context>
  [[nodiscard]] inline auto lookup_in_typed_root(Context const& root, std::string_view name) -> lookup_result {
    auto const segments = split_variable_path(name);
    return lookup_in_typed_value(root, segments.span(), 0, name);
  }

  struct typed_object_view {
    void const* object{};
    std::size_t (*size_fn)(void const*){};
    void (*for_each_fn)(void const*, void*, void (*)(void*, std::string_view, inja_value&&)){};

    [[nodiscard]] auto size() const -> std::size_t { return size_fn(object); }

    template <typename Callback>
    auto for_each(Callback&& callback) const -> void {
      struct callback_state {
        Callback* callback_ptr;
      };
      auto state = callback_state{.callback_ptr = &callback};
      for_each_fn(object, &state, [](void* state_ptr, std::string_view key, inja_value&& value) {
        auto* state = static_cast<callback_state*>(state_ptr);
        (*state->callback_ptr)(key, std::move(value));
      });
    }
  };

  template <typename T>
    requires glaze_reflectable<T>
  [[nodiscard]] inline auto make_typed_object_view(T const& object_ref) -> typed_object_view {
    using U = remove_cvref_t<T>;
    return typed_object_view{
        .object      = &object_ref,
        .size_fn     = [](void const*) -> std::size_t { return static_cast<std::size_t>(glz::reflect<U>::size); },
        .for_each_fn = [](void const* object_ptr, void* state_ptr, void (*emit)(void*, std::string_view, inja_value&&)) -> void {
          auto const&    object = *static_cast<U const*>(object_ptr);
          auto           tied   = glz::to_tie(object);
          constexpr auto count  = static_cast<std::size_t>(glz::reflect<U>::size);
          [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            (
                [&] {
                  auto converted = try_to_inja_value(glz::get<Is>(tied));
                  if (!converted.has_value()) {
                    throw render_error{"value cannot be converted to inja_value"};
                  }
                  emit(state_ptr, std::string_view{glz::reflect<U>::keys[Is]}, std::move(*converted));
                }(),
                ...);
          }(std::make_index_sequence<count>{});
        },
    };
  }

  template <typename T>
  [[nodiscard]] inline auto lookup_typed_object_view_value(T const& value, std::span<std::string_view const> segments, std::size_t index) -> std::optional<typed_object_view> {
    using U = remove_cvref_t<T>;
    if (index >= segments.size()) {
      if constexpr (glaze_reflectable<U>) {
        return make_typed_object_view(value);
      } else {
        return std::nullopt;
      }
    }
    if constexpr (map_like<U>) {
      auto const it = value.find(typename U::key_type{segments[index]});
      if (it == value.end()) {
        return std::nullopt;
      }
      return lookup_typed_object_view_value(it->second, segments, index + 1);
    } else if constexpr (glaze_reflectable<U>) {
      constexpr auto count   = static_cast<std::size_t>(glz::reflect<U>::size);
      auto           tied    = glz::to_tie(value);
      auto const     segment = segments[index];
      auto           result  = std::optional<typed_object_view>{};
      [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (
            [&] {
              if (result.has_value()) {
                return;
              }
              if (segment != std::string_view{glz::reflect<U>::keys[Is]}) {
                return;
              }
              result = lookup_typed_object_view_value(glz::get<Is>(tied), segments, index + 1);
            }(),
            ...);
      }(std::make_index_sequence<count>{});
      return result;
    } else {
      return std::nullopt;
    }
  }

  template <typename Context>
  [[nodiscard]] inline auto lookup_typed_object_view_root(Context const& root, std::string_view name) -> std::optional<typed_object_view> {
    auto const segments = split_variable_path(name);
    return lookup_typed_object_view_value(root, segments.span(), 0);
  }
#else
  struct typed_object_view {};

  template <typename Context>
  [[nodiscard]] inline auto lookup_typed_object_view_root(Context const&, std::string_view) -> std::optional<typed_object_view> {
    return std::nullopt;
  }
#endif

  [[nodiscard]] inline auto resolve_local_binding(local_binding const& binding, path_segments const& segments) -> std::optional<inja_value> {
    if (segments.front() != binding.name) {
      return std::nullopt;
    }
    if (segments.size() == 1) {
      return binding.value;
    }
    auto resolved = lookup_in_inja_value(binding.value, segments.span(), 1, join_segments(segments.span()));
    if (!resolved.has_value()) {
      return std::nullopt;
    }
    return std::move(*resolved);
  }

  [[nodiscard]] inline auto resolve_loop_property(local_frame const& frame, path_segments const& segments) -> std::optional<inja_value> {
    if (segments.size() != 2 || segments.front() != "loop") {
      return std::nullopt;
    }
    auto const key = segments[1];
    if (key == "index") {
      return inja_value{frame.loop.index};
    }
    if (key == "index1") {
      return inja_value{frame.loop.index1};
    }
    if (key == "is_first") {
      return inja_value{frame.loop.is_first};
    }
    if (key == "is_last") {
      return inja_value{frame.loop.is_last};
    }
    return std::nullopt;
  }

  [[nodiscard]] inline auto resolve_local(std::vector<local_frame> const& frames, std::string_view name) -> std::optional<inja_value> {
    auto const segments = split_variable_path(name);
    for (auto it = frames.rbegin(); it != frames.rend(); ++it) {
      if (auto value = resolve_loop_property(*it, segments); value.has_value()) {
        return value;
      }
      if (it->key.has_value()) {
        if (auto value = resolve_local_binding(*it->key, segments); value.has_value()) {
          return value;
        }
      }
      if (it->value.has_value()) {
        if (auto value = resolve_local_binding(*it->value, segments); value.has_value()) {
          return value;
        }
      }
      for (auto assigned_it = it->assigned.rbegin(); assigned_it != it->assigned.rend(); ++assigned_it) {
        if (auto value = resolve_local_binding(*assigned_it, segments); value.has_value()) {
          return value;
        }
      }
    }
    return std::nullopt;
  }

  inline auto assign_to_local(std::vector<local_frame>& frames, std::string_view name, inja_value value) -> void {
    if (name.find('.') != std::string_view::npos) {
      throw render_error{"set target must be local identifier"};
    }
    auto const target = std::string{trim_view(name)};
    if (target.empty()) {
      throw render_error{"invalid set target"};
    }
    auto& frame = frames.back();
    if (frame.key.has_value() && frame.key->name == target) {
      frame.key->value = std::move(value);
      return;
    }
    if (frame.value.has_value() && frame.value->name == target) {
      frame.value->value = std::move(value);
      return;
    }
    for (auto it = frame.assigned.rbegin(); it != frame.assigned.rend(); ++it) {
      if (it->name == target) {
        it->value = std::move(value);
        return;
      }
    }
    frame.assigned.push_back(local_binding{.name = target, .value = std::move(value)});
  }

  using builtin_fn = inja_value (*)(std::vector<inja_value> const&);

  /**
   * @brief 実行時式評価器（簡易再帰下降パーサ）。
   * @tparam FunctionList  compile-time 関数登録テーブル
   */
  template <typename Lookup, is_environment_binding EnvironmentBinding = default_environment>
  class expr_parser {
public:
    /**
     * @brief 評価器を初期化する。
     * @param text 式文字列
     * @param lookup 変数解決コールバック
     */
    expr_parser(std::string_view text, Lookup const& lookup, runtime_options_ref runtime_options = std::nullopt) : text_(text), lookup_(lookup), runtime_options_(runtime_options) {}

    /**
     * @brief 式を評価する。
     * @param evaluate false のときは短絡用に副作用なしで走査だけ行う
     * @return 評価結果
     */
    [[nodiscard]] auto parse(bool evaluate = true) -> inja_value {
      auto v = parse_or(evaluate);
      skip_space();
      if (pos_ != text_.size()) {
        throw render_error{"unexpected token in expression"};
      }
      return v;
    }

private:
    std::string_view    text_;
    std::size_t         pos_{};
    Lookup const&       lookup_;
    runtime_options_ref runtime_options_{};

    /**
     * @brief 現在位置から空白を読み飛ばす。
     */
    auto skip_space() -> void {
      while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_])) != 0) {
        ++pos_;
      }
    }

    /**
     * @brief 指定トークンを消費する。
     *
     * 英字トークン（and/or/not/in など）は後続識別子と連結しないよう境界を確認する。
     *
     * @param token 期待トークン
     * @return 消費に成功した場合 true
     */
    [[nodiscard]] auto consume(std::string_view token) -> bool {
      skip_space();
      if (text_.substr(pos_, token.size()) != token) {
        return false;
      }
      auto const next = pos_ + token.size();
      if (!token.empty() && std::isalpha(static_cast<unsigned char>(token.back())) != 0 && next < text_.size()) {
        auto const c = text_[next];
        if (std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_') {
          return false;
        }
      }
      pos_ = next;
      return true;
    }

    /**
     * @brief OR 優先順位レベルを評価する。
     * @param evaluate 短絡時の評価フラグ
     * @return 評価結果
     */
    [[nodiscard]] auto parse_or(bool evaluate) -> inja_value {
      auto lhs = parse_and(evaluate);
      while (consume("or")) {
        auto const lhs_truth = evaluate && frozenchars::inja::truthy(lhs);
        auto       rhs       = parse_and(evaluate && !lhs_truth);
        if (evaluate) {
          lhs = inja_value{lhs_truth || truthy(rhs)};
        }
      }
      return lhs;
    }

    /**
     * @brief AND 優先順位レベルを評価する。
     */
    [[nodiscard]] auto parse_and(bool evaluate) -> inja_value {
      auto lhs = parse_in(evaluate);
      while (consume("and")) {
        auto const lhs_truth = evaluate && frozenchars::inja::truthy(lhs);
        auto       rhs       = parse_in(evaluate && lhs_truth);
        if (evaluate) {
          lhs = inja_value{lhs_truth && frozenchars::inja::truthy(rhs)};
        }
      }
      return lhs;
    }

    /**
     * @brief IN 演算レベルを評価する。
     */
    [[nodiscard]] auto parse_in(bool evaluate) -> inja_value {
      auto lhs = parse_eq(evaluate);
      while (consume("in")) {
        auto rhs = parse_eq(evaluate);
        if (!evaluate) {
          lhs = inja_value{};
          continue;
        }
        if (std::holds_alternative<std::string>(lhs.storage) && std::holds_alternative<std::string>(rhs.storage)) {
          lhs = inja_value{std::get<std::string>(rhs.storage).find(std::get<std::string>(lhs.storage)) != std::string::npos};
        } else if (std::holds_alternative<inja_array>(rhs.storage)) {
          auto const& arr_variant = std::get<inja_array>(rhs.storage);
          auto const& search_val  = lhs;
          lhs                     = inja_value{std::visit(
              [&](auto const& vec) {
                for (auto const& v : vec) {
                  if constexpr (std::is_same_v<std::decay_t<decltype(v)>, inja_value>) {
                    if (equals_value(search_val, v))
                      return true;
                  } else {
                    if (equals_value(search_val, inja_value{v}))
                      return true;
                  }
                }
                return false;
              },
              arr_variant)};
        } else if (std::holds_alternative<inja_object>(rhs.storage) && std::holds_alternative<std::string>(lhs.storage)) {
          auto const& obj = std::get<inja_object>(rhs.storage);
          auto const  key = std::string_view{std::get<std::string>(lhs.storage)};
          lhs             = inja_value{obj.find(key) != obj.end()};
        } else {
          throw render_error{"invalid operands for in"};
        }
      }
      return lhs;
    }

    /**
     * @brief 等価比較レベルを評価する。
     */
    [[nodiscard]] auto parse_eq(bool evaluate) -> inja_value {
      auto lhs = parse_rel(evaluate);
      while (true) {
        if (consume("==")) {
          auto rhs = parse_rel(evaluate);
          if (evaluate) {
            lhs = inja_value{equals_value(lhs, rhs)};
          }
        } else if (consume("!=")) {
          auto rhs = parse_rel(evaluate);
          if (evaluate) {
            lhs = inja_value{!equals_value(lhs, rhs)};
          }
        } else {
          break;
        }
      }
      return lhs;
    }

    /**
     * @brief 関係比較レベルを評価する。
     */
    [[nodiscard]] auto parse_rel(bool evaluate) -> inja_value {
      auto lhs = parse_add(evaluate);
      while (true) {
        if (consume("<=")) {
          auto rhs = parse_add(evaluate);
          if (evaluate) {
            lhs = inja_value{!less_value(rhs, lhs)};
          }
        } else if (consume(">=")) {
          auto rhs = parse_add(evaluate);
          if (evaluate) {
            lhs = inja_value{!less_value(lhs, rhs)};
          }
        } else if (consume("<")) {
          auto rhs = parse_add(evaluate);
          if (evaluate) {
            lhs = inja_value{less_value(lhs, rhs)};
          }
        } else if (consume(">")) {
          auto rhs = parse_add(evaluate);
          if (evaluate) {
            lhs = inja_value{less_value(rhs, lhs)};
          }
        } else {
          break;
        }
      }
      return lhs;
    }

    /**
     * @brief 加減算レベルを評価する。
     */
    [[nodiscard]] auto parse_add(bool evaluate) -> inja_value {
      auto lhs = parse_mul(evaluate);
      while (true) {
        if (consume("+")) {
          auto rhs = parse_mul(evaluate);
          if (!evaluate) {
            lhs = inja_value{};
            continue;
          }
          if (std::holds_alternative<std::string>(lhs.storage) && std::holds_alternative<std::string>(rhs.storage)) {
            lhs = inja_value{std::get<std::string>(lhs.storage) + std::get<std::string>(rhs.storage)};
          } else {
            auto const lnum = try_as_double(lhs);
            auto const rnum = try_as_double(rhs);
            if (!lnum.has_value() || !rnum.has_value()) {
              throw render_error{"invalid operands for +"};
            }
            lhs = inja_value{*lnum + *rnum};
          }
        } else if (consume("-")) {
          auto rhs = parse_mul(evaluate);
          if (evaluate) {
            auto const lnum = try_as_double(lhs);
            auto const rnum = try_as_double(rhs);
            if (!lnum.has_value() || !rnum.has_value()) {
              throw render_error{"invalid operands for -"};
            }
            lhs = inja_value{*lnum - *rnum};
          }
        } else {
          break;
        }
      }
      return lhs;
    }

    /**
     * @brief 乗除余演算レベルを評価する。
     */
    [[nodiscard]] auto parse_mul(bool evaluate) -> inja_value {
      auto lhs = parse_postfix(evaluate);
      while (true) {
        if (consume("*")) {
          auto rhs = parse_postfix(evaluate);
          if (evaluate) {
            auto const lnum = try_as_double(lhs);
            auto const rnum = try_as_double(rhs);
            if (!lnum.has_value() || !rnum.has_value()) {
              throw render_error{"invalid operands for *"};
            }
            lhs = inja_value{*lnum * *rnum};
          }
        } else if (consume("/")) {
          auto rhs = parse_postfix(evaluate);
          if (evaluate) {
            auto const lnum = try_as_double(lhs);
            auto const rnum = try_as_double(rhs);
            if (!lnum.has_value() || !rnum.has_value()) {
              throw render_error{"invalid operands for /"};
            }
            if (*rnum == 0.0) {
              throw render_error{"division by zero"};
            }
            lhs = inja_value{*lnum / *rnum};
          }
        } else if (consume("%")) {
          auto rhs = parse_postfix(evaluate);
          if (evaluate) {
            if (!std::holds_alternative<std::int64_t>(lhs.storage) || !std::holds_alternative<std::int64_t>(rhs.storage)) {
              throw render_error{"invalid operands for %"};
            }
            auto const rval = std::get<std::int64_t>(rhs.storage);
            if (rval == 0) {
              throw render_error{"modulo by zero"};
            }
            lhs = inja_value{std::get<std::int64_t>(lhs.storage) % rval};
          }
        } else {
          break;
        }
      }
      return lhs;
    }

    [[nodiscard]] auto parse_unary(bool evaluate) -> inja_value {
      if (consume("not")) {
        auto v = parse_unary(evaluate);
        return evaluate ? inja_value{!truthy(v)} : inja_value{};
      }
      if (consume("-")) {
        auto v = parse_unary(evaluate);
        if (!evaluate) {
          return inja_value{};
        }
        if (std::holds_alternative<std::int64_t>(v.storage)) {
          return inja_value{-std::get<std::int64_t>(v.storage)};
        }
        auto const num = try_as_double(v);
        if (!num.has_value()) {
          throw render_error{"invalid operand for unary -"};
        }
        return inja_value{-*num};
      }
      return parse_primary(evaluate);
    }

    [[nodiscard]] auto parse_postfix(bool evaluate) -> inja_value {
      auto v = parse_unary(evaluate);
      skip_space();
      while (pos_ < text_.size() && text_[pos_] == '|') {
        if (!evaluate) {
          ++pos_;
          while (pos_ < text_.size()) {
            auto const c = text_[pos_];
            if ((std::isalnum(static_cast<unsigned char>(c)) != 0) || c == '_') {
              ++pos_;
              continue;
            }
            break;
          }
          skip_space();
          if (pos_ < text_.size() && text_[pos_] == '(') {
            auto paren_depth = 1;
            ++pos_;
            while (pos_ < text_.size() && paren_depth > 0) {
              if (text_[pos_] == '(')
                ++paren_depth;
              else if (text_[pos_] == ')')
                --paren_depth;
              ++pos_;
            }
          }
          skip_space();
          continue;
        }

        ++pos_;
        skip_space();
        auto start = pos_;
        while (pos_ < text_.size()) {
          auto const c = text_[pos_];
          if ((std::isalnum(static_cast<unsigned char>(c)) != 0) || c == '_') {
            ++pos_;
            continue;
          }
          break;
        }
        if (start == pos_) {
          throw render_error{"unexpected token after pipe operator"};
        }
        auto const func_name = text_.substr(start, pos_ - start);

        skip_space();
        auto args = std::vector<inja_value>{};
        args.reserve(4);
        if (pos_ < text_.size() && text_[pos_] == '(') {
          ++pos_;
          skip_space();

          if (pos_ < text_.size() && text_[pos_] != ')') {
            while (true) {
              args.push_back(parse_or(evaluate));
              skip_space();
              if (pos_ >= text_.size()) {
                throw render_error{"unclosed pipe function call"};
              }
              if (text_[pos_] == ')') {
                break;
              }
              if (text_[pos_] != ',') {
                throw render_error{"expected comma or closing parenthesis in pipe function call"};
              }
              ++pos_;
              skip_space();
            }
          }

          if (!consume(")")) {
            throw render_error{"missing closing parenthesis in pipe function call"};
          }
        }

        args.emplace(args.begin(), std::move(v));
        v = evaluate_function<EnvironmentBinding>(func_name, args, runtime_options_);
        skip_space();
      }
      return v;
    }

    /**
     * @brief 識別子（a.b.c 形式を含む）をスコープから解決する。
     * @param name 識別子
     * @param evaluate 評価フラグ
     * @return 解決した値
     */
    [[nodiscard]] auto resolve_variable(std::string_view name, bool evaluate) -> inja_value {
      if (!evaluate) {
        return inja_value{};
      }
      if (name.empty()) {
        throw render_error{"empty identifier"};
      }
      auto resolved = lookup_(name);
      if (resolved.has_value()) {
        return std::move(*resolved);
      }
      if (resolved.error() == lookup_status::not_convertible) {
        throw render_error{"value cannot be converted to inja_value"};
      }
      if (name.find('.') == std::string_view::npos) {
        using constant_list_t = typename environment_traits<EnvironmentBinding>::constant_list_type;
        if (auto constant_value = constant_list_t::lookup(name); constant_value.has_value()) {
          return std::move(*constant_value);
        }
      }
      throw render_error{"undefined variable: " + std::string{name}};
    }

    /**
     * @brief リテラル・括弧式・識別子を評価する。
     */
    [[nodiscard]] auto parse_primary(bool evaluate) -> inja_value {
      skip_space();
      if (consume("(")) {
        auto v = parse_or(evaluate);
        if (!consume(")")) {
          throw render_error{"missing closing parenthesis"};
        }
        return v;
      }
      if (consume("true")) {
        return inja_value{true};
      }
      if (consume("false")) {
        return inja_value{false};
      }
      if (consume("null")) {
        return inja_value{};
      }
      skip_space();
      if (pos_ < text_.size() && (text_[pos_] == '"' || text_[pos_] == '\'')) {
        auto const q   = text_[pos_++];
        auto       out = std::string{};
        while (pos_ < text_.size() && text_[pos_] != q) {
          if (text_[pos_] == '\\' && pos_ + 1 < text_.size()) {
            ++pos_;
            switch (text_[pos_]) {
            case 'n':
              out += '\n';
              break;
            case 't':
              out += '\t';
              break;
            case 'r':
              out += '\r';
              break;
            case '\\':
              out += '\\';
              break;
            case '"':
              out += '"';
              break;
            case '\'':
              out += '\'';
              break;
            default:
              out += '\\';
              out += text_[pos_];
              break;
            }
          } else {
            out += text_[pos_];
          }
          ++pos_;
        }
        if (pos_ >= text_.size()) {
          throw render_error{"unterminated string literal"};
        }
        ++pos_;
        return inja_value{std::move(out)};
      }
      if (pos_ < text_.size() && (std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0)) {
        auto start   = pos_;
        auto has_dot = false;
        while (pos_ < text_.size()) {
          auto const c = text_[pos_];
          if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
            ++pos_;
            continue;
          }
          if (c == '.' && !has_dot) {
            has_dot = true;
            ++pos_;
            continue;
          }
          break;
        }
        auto const* first = text_.data() + static_cast<std::ptrdiff_t>(start);
        auto const* last  = text_.data() + static_cast<std::ptrdiff_t>(pos_);
        if (has_dot) {
          auto       parsed = double{};
          auto const result = std::from_chars(first, last, parsed);
          if (result.ec != std::errc{} || result.ptr != last) {
            throw render_error{"invalid floating-point literal"};
          }
          return inja_value{parsed};
        }
        auto       parsed = std::int64_t{};
        auto const result = std::from_chars(first, last, parsed);
        if (result.ec != std::errc{} || result.ptr != last) {
          throw render_error{"invalid integer literal"};
        }
        return inja_value{parsed};
      }
      auto start = pos_;
      while (pos_ < text_.size()) {
        auto const c = text_[pos_];
        if ((std::isalnum(static_cast<unsigned char>(c)) != 0) || c == '_' || c == '.') {
          ++pos_;
          continue;
        }
        break;
      }
      if (start == pos_) {
        throw render_error{"unexpected token in expression"};
      }
      auto const name = text_.substr(start, pos_ - start);

      // '('に続く関数呼び出しをチェックする
      skip_space();
      if (pos_ < text_.size() && text_[pos_] == '(') {
        // 評価されないショートサーキットの場合は')'までスキップする
        if (!evaluate) {
          auto paren_depth = 1;
          ++pos_;  // skip opening paren
          while (pos_ < text_.size() && paren_depth > 0) {
            if (text_[pos_] == '(')
              ++paren_depth;
            else if (text_[pos_] == ')')
              --paren_depth;
            ++pos_;
          }
          return inja_value{};
        }
        ++pos_;  // '('をスキップする

        // 関数引数を解析する
        auto args = std::vector<inja_value>{};
        args.reserve(4);
        skip_space();

        // 空の引数リストをチェックする
        if (pos_ < text_.size() && text_[pos_] != ')') {
          while (true) {
            args.push_back(parse_or(evaluate));
            skip_space();
            if (pos_ >= text_.size()) {
              throw render_error{"unclosed function call"};
            }
            if (text_[pos_] == ')') {
              break;
            }
            if (text_[pos_] != ',') {
              throw render_error{"expected comma or closing parenthesis in function call"};
            }
            ++pos_;  // ','をスキップする
            skip_space();
          }
        }

        if (!consume(")")) {
          throw render_error{"missing closing parenthesis in function call"};
        }
        return evaluate_function<EnvironmentBinding>(name, args, runtime_options_);
      }
      return resolve_variable(name, evaluate);
    }
  };

  template <auto Src, typename OutputBuffer, typename RootLookup, typename RootObjectLookup, is_environment_binding EnvironmentBinding = default_environment,
            typename Delims = frozenchars::inja::default_delimiters>
  auto render_program_with_lookup(bytecode const& program, void const* root_ptr, RootLookup root_lookup, RootObjectLookup root_object_lookup, OutputBuffer& out,
                                  runtime_options_ref runtime_options = std::nullopt) -> void {
    auto constexpr src = Src.sv();
    auto frames        = std::vector<local_frame>{};
    frames.reserve(1 + program.max_for_depth);
    frames.push_back(local_frame{});

    auto lookup = [&](std::string_view name) -> lookup_result {
      if (auto local = resolve_local(frames, name); local.has_value()) {
        return *local;
      }
      return root_lookup(name);
    };

    auto const render_range = [&](this auto&& self, std::size_t begin, std::size_t end) -> void {
      auto const eval_expr_with_simple_path = [&](std::string_view expr, bool is_simple_path) -> inja_value {
        if (!is_simple_path) {
          return expr_parser<decltype(lookup), EnvironmentBinding>{expr, lookup, runtime_options}.parse();
        }
        auto const looked = lookup(expr);
        if (looked.has_value()) {
          return *looked;
        }
        if (looked.error() == lookup_status::not_convertible) {
          throw render_error{"value cannot be converted to inja_value"};
        }
        return expr_parser<decltype(lookup), EnvironmentBinding>{expr, lookup, runtime_options}.parse();
      };

      auto i = begin;
      while (i < end) {
        auto const& node = program.nodes[i];
        switch (node.kind) {
        case node_kind::text: {
          out.append(src.substr(node.begin, node.end - node.begin));
          ++i;
          break;
        }
        case node_kind::expr: {
          // ファストパス: コンパイル時にバインドされた appender があれば直接呼び出す。
          // split_variable_path も lookup_in_reflectable の線形探索も経由しない。
          // simple_appender は std::string& にバインドされているため、OutputBuffer が std::string の場合のみ利用可能。
          if constexpr (std::is_same_v<OutputBuffer, std::string>) {
            if (node.simple_appender != nullptr) {
              node.simple_appender(root_ptr, out);
              ++i;
              break;
            }
          }
          auto const expr  = src.substr(node.aux_begin, node.aux_end - node.aux_begin);
          auto const value = eval_expr_with_simple_path(expr, node.expr_is_simple_path);
          append_value(out, value);
          ++i;
          break;
        }
        case node_kind::if_stmt: {
          auto const cond_sv  = src.substr(node.aux_begin, node.aux_end - node.aux_begin);
          auto const cond     = eval_expr_with_simple_path(cond_sv, node.if_cond_is_simple_path);
          auto const then_end = node.else_index != std::numeric_limits<std::size_t>::max() ? node.else_index : node.end_index;
          if (truthy(cond)) {
            self(i + 1, then_end);
          } else {
            auto else_idx = node.else_index;
            while (else_idx != std::numeric_limits<std::size_t>::max()) {
              auto const& else_node  = program.nodes[else_idx];
              auto const  branch_end = else_node.else_index != std::numeric_limits<std::size_t>::max() ? else_node.else_index : node.end_index;
              if (else_node.aux_begin == std::numeric_limits<std::size_t>::max()) {
                self(else_idx + 1, branch_end);
                break;
              }
              auto const else_cond_sv = src.substr(else_node.aux_begin, else_node.aux_end - else_node.aux_begin);
              auto const else_cond    = eval_expr_with_simple_path(else_cond_sv, else_node.if_cond_is_simple_path);
              if (truthy(else_cond)) {
                self(else_idx + 1, branch_end);
                break;
              }
              else_idx = else_node.else_index;
            }
          }
          i = node.end_index + 1;
          break;
        }
        case node_kind::for_stmt: {
          auto const iter_expr  = src.substr(node.aux_begin, node.aux_end - node.aux_begin);
          auto const value_name = src.substr(node.aux2_begin, node.aux2_end - node.aux2_begin);
          auto const has_key    = node.for_has_key;
          auto const key_name   = has_key ? src.substr(node.aux3_begin, node.aux3_end - node.aux3_begin) : std::string_view{};
          auto       iterable   = inja_value{};
#if FROZENCHARS_HAS_GLAZE
          auto native_typed_object = std::optional<typed_object_view>{};
#endif
          if (node.for_iter_is_simple_path) {
            auto const looked = lookup(iter_expr);
            if (looked.has_value()) {
              iterable = std::move(*looked);
            } else if (looked.error() == lookup_status::not_convertible) {
#if FROZENCHARS_HAS_GLAZE
              native_typed_object = root_object_lookup(iter_expr);
              if (!native_typed_object.has_value()) {
                throw render_error{"value cannot be converted to inja_value"};
              }
#else
              throw render_error{"value cannot be converted to inja_value"};
#endif
            } else {
              iterable = expr_parser<decltype(lookup), EnvironmentBinding>{iter_expr, lookup, runtime_options}.parse();
            }
          } else {
            iterable = expr_parser<decltype(lookup), EnvironmentBinding>{iter_expr, lookup, runtime_options}.parse();
          }
          auto const body_begin = i + 1;
          auto const body_end   = node.end_index;

#if FROZENCHARS_HAS_GLAZE
          if (native_typed_object.has_value()) {
            auto const total = native_typed_object->size();
            auto       idx   = std::size_t{0};
            native_typed_object->for_each([&](std::string_view k, inja_value value) {
              auto frame          = local_frame{};
              frame.loop.index    = static_cast<std::int64_t>(idx);
              frame.loop.index1   = static_cast<std::int64_t>(idx + 1);
              frame.loop.is_first = idx == 0;
              frame.loop.is_last  = idx + 1 == total;
              frame.value         = local_binding{
                  .name  = std::string{value_name},
                  .value = std::move(value),
              };
              if (has_key) {
                frame.key = local_binding{
                    .name  = std::string{key_name},
                    .value = inja_value{std::string{k}},
                };
              }
              frames.push_back(std::move(frame));
              self(body_begin, body_end);
              frames.pop_back();
              ++idx;
            });
          } else
#endif
              if (std::holds_alternative<inja_array>(iterable.storage)) {
            auto const& arr_variant = std::get<inja_array>(iterable.storage);
            std::visit(
                [&](auto const& arr) {
                  using array_type = std::remove_cvref_t<decltype(arr)>;
                  for (auto idx = std::size_t{0}; idx < arr.size(); ++idx) {
                    auto frame          = local_frame{};
                    frame.loop.index    = static_cast<std::int64_t>(idx);
                    frame.loop.index1   = static_cast<std::int64_t>(idx + 1);
                    frame.loop.is_first = idx == 0;
                    frame.loop.is_last  = idx + 1 == arr.size();
                    if constexpr (std::same_as<array_type, std::vector<inja_value>>) {
                      frame.value = local_binding{
                          .name  = std::string{value_name},
                          .value = arr[idx],
                      };
                    } else {
                      frame.value = local_binding{
                          .name  = std::string{value_name},
                          .value = inja_value{arr[idx]},
                      };
                    }
                    if (has_key) {
                      frame.key = local_binding{
                          .name  = std::string{key_name},
                          .value = inja_value{static_cast<std::int64_t>(idx)},
                      };
                    }
                    frames.push_back(std::move(frame));
                    self(body_begin, body_end);
                    frames.pop_back();
                  }
                },
                arr_variant);
          } else if (std::holds_alternative<inja_object>(iterable.storage)) {
            auto const& obj = std::get<inja_object>(iterable.storage);
            auto        idx = std::size_t{0};
            for (auto const& [k, v] : obj) {
              auto frame          = local_frame{};
              frame.loop.index    = static_cast<std::int64_t>(idx);
              frame.loop.index1   = static_cast<std::int64_t>(idx + 1);
              frame.loop.is_first = idx == 0;
              frame.loop.is_last  = idx + 1 == obj.size();
              frame.value         = local_binding{
                  .name  = std::string{value_name},
                  .value = v,
              };
              if (has_key) {
                frame.key = local_binding{
                    .name  = std::string{key_name},
                    .value = inja_value{k},
                };
              }
              frames.push_back(std::move(frame));
              self(body_begin, body_end);
              frames.pop_back();
              ++idx;
            }
          } else {
            throw render_error{"for target must be array or object"};
          }
          i = node.end_index + 1;
          break;
        }
        case node_kind::set_stmt: {
          auto const set_expr   = src.substr(node.aux_begin, node.aux_end - node.aux_begin);
          auto const set_target = src.substr(node.aux2_begin, node.aux2_end - node.aux2_begin);
          auto       value      = expr_parser<decltype(lookup), EnvironmentBinding>{set_expr, lookup, runtime_options}.parse();
          assign_to_local(frames, set_target, std::move(value));
          ++i;
          break;
        }
        case node_kind::include_stmt: {
          auto const include_expr = src.substr(node.aux_begin, node.aux_end - node.aux_begin);
          auto       include_name = eval_expr_with_simple_path(include_expr, node.include_expr_is_simple_path);
          if (!std::holds_alternative<std::string>(include_name.storage)) {
            throw render_error{"include target must evaluate to string"};
          }
          auto const& key = std::get<std::string>(include_name.storage);
          if (runtime_options.has_value()) {
            auto const& options = runtime_options->get();
            if (auto const it = options.include_templates.find(key); it != options.include_templates.end()) {
              out.append(it->second);
              ++i;
              break;
            }
            if (options.include_call) {
              out.append(options.include_call(key));
              ++i;
              break;
            }
          }
          throw render_error{"unknown include: " + key};
        }
        case node_kind::else_stmt:
        case node_kind::endif_stmt:
        case node_kind::endfor_stmt:
          ++i;
          break;
        }
      }
    };

    render_range(0, program.count);
  }

  template <auto Src, typename OutputBuffer, is_environment_binding EnvironmentBinding = default_environment, typename Delims = frozenchars::inja::default_delimiters>
  auto render_program(inja_object const& root, OutputBuffer& out, runtime_options_ref runtime_options = std::nullopt) -> void {
    auto lookup                  = [&](std::string_view name) -> lookup_result { return lookup_in_object_root(root, name); };
    auto object_lookup           = [&](std::string_view) -> std::optional<typed_object_view> { return std::nullopt; };
    auto constexpr program_const = parse_program<Src, Delims>();
    auto program                 = program_const;
    render_program_with_lookup<Src, OutputBuffer, decltype(lookup), decltype(object_lookup), EnvironmentBinding, Delims>(program, &root, lookup, object_lookup, out, runtime_options);
  }

#if FROZENCHARS_HAS_GLAZE
  template <auto Src, typename OutputBuffer, typename Context, is_environment_binding EnvironmentBinding = default_environment, typename Delims = frozenchars::inja::default_delimiters>
    requires glaze_reflectable<Context>
  auto render_program(Context const& root, OutputBuffer& out, runtime_options_ref runtime_options = std::nullopt) -> void {
    auto lookup        = [&](std::string_view name) -> lookup_result { return lookup_in_typed_root(root, name); };
    auto object_lookup = [&](std::string_view name) -> std::optional<typed_object_view> { return lookup_typed_object_view_root(root, name); };
    // bytecode は consteval で構築し、appender table は初回呼び出し時にキャッシュする。
    static auto program = []() {
      auto p = parse_program<Src, Delims>();
      fill_appender_table<Context>(p);
      return p;
    }();
    render_program_with_lookup<Src, OutputBuffer, decltype(lookup), decltype(object_lookup), EnvironmentBinding, Delims>(program, &root, lookup, object_lookup, out, runtime_options);
  }
#endif

  template <auto Src, is_environment_binding EnvironmentBinding = default_environment, typename Delims = frozenchars::inja::default_delimiters>
  auto render_program(inja_object const& root, runtime_options_ref runtime_options = std::nullopt) -> std::string {
    auto out = std::string{};
    render_program<Src, std::string, EnvironmentBinding, Delims>(root, out, runtime_options);
    return out;
  }

#if FROZENCHARS_HAS_GLAZE
  template <auto Src, typename Context, is_environment_binding EnvironmentBinding = default_environment, typename Delims = frozenchars::inja::default_delimiters>
    requires glaze_reflectable<Context>
  auto render_program(Context const& root, runtime_options_ref runtime_options = std::nullopt) -> std::string {
    auto out = std::string{};
    render_program<Src, std::string, Context, EnvironmentBinding, Delims>(root, out, runtime_options);
    return out;
  }
#endif

}  // namespace detail

}  // namespace frozenchars::inja
