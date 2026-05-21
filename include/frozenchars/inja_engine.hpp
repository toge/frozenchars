#pragma once

#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "frozen_string.hpp"
#include "inja_value.hpp"

namespace frozenchars {

/**
 * @brief テンプレート区切り文字を保持する型。
 *
 * inja/Jinja と同様に、式・文・コメント・行文プレフィックスを
 * テンプレート引数で切り替えるために使用する。
 */
template <
  auto ExprOpen,      // 式開始
  auto ExprClose,     // 式終了
  auto StmtOpen,      // 文開始
  auto StmtClose,     // 文終了
  auto CommentOpen,   // コメント開始
  auto CommentClose,  // コメント終了
  auto LineStmtPrefix // 行文プレフィックス
>
struct template_delimiters {
  static constexpr auto expression_open = ExprOpen;
  static constexpr auto expression_close = ExprClose;
  static constexpr auto statement_open = StmtOpen;
  static constexpr auto statement_close = StmtClose;
  static constexpr auto comment_open = CommentOpen;
  static constexpr auto comment_close = CommentClose;
  static constexpr auto line_statement_prefix = LineStmtPrefix;
};

inline constexpr auto default_expr_open = FrozenString<3>{"{{"};
inline constexpr auto default_expr_close = FrozenString<3>{"}}"};
inline constexpr auto default_stmt_open = FrozenString<3>{"{%"};
inline constexpr auto default_stmt_close = FrozenString<3>{"%}"};
inline constexpr auto default_comment_open = FrozenString<3>{"{#"};
inline constexpr auto default_comment_close = FrozenString<3>{"#}"};
inline constexpr auto default_line_stmt_prefix = FrozenString<3>{"##"};

using default_template_delimiters = template_delimiters<
  default_expr_open,
  default_expr_close,
  default_stmt_open,
  default_stmt_close,
  default_comment_open,
  default_comment_close,
  default_line_stmt_prefix
>;

using template_function_callback = std::function<template_value(std::vector<template_value> const&)>;
using template_include_callback = std::function<std::string(std::string_view, template_object const&)>;

/**
 * @brief テンプレート実行時の拡張オプション。
 *
 * - function_callbacks: ユーザー定義関数の登録先
 * - include_templates: `{% include %}` 用の名前付きテンプレート本体
 * - include_callback: include 解決時のフォールバック
 */
struct template_runtime_options {
  std::unordered_map<std::string, template_function_callback> function_callbacks{};
  std::unordered_map<std::string, std::string> include_templates{};
  template_include_callback include_callback{};

  auto add_function(std::string name, template_function_callback callback) -> void {
    function_callbacks.insert_or_assign(std::move(name), std::move(callback));
  }

  auto add_include_template(std::string name, std::string content) -> void {
    include_templates.insert_or_assign(std::move(name), std::move(content));
  }
};

} // namespace frozenchars

namespace frozenchars::detail {

/**
 * @brief テンプレートで保持する最大ノード数。
 */
constexpr auto k_template_max_nodes = std::size_t{2048};

/**
 * @brief テンプレート構文ノードの種類。
 */
enum class template_node_kind : std::uint8_t { text, expr, if_stmt, else_stmt, endif_stmt, for_stmt, endfor_stmt, set_stmt, include_stmt };

/**
 * @brief パース済みテンプレートの単一ノード。
 */
struct template_node {
  /// ノード種別
  template_node_kind kind{};
  /// 元テンプレート中の開始位置（半開区間）
  std::size_t begin{};
  /// 元テンプレート中の終了位置（半開区間）
  std::size_t end{};
  /// if ノードに紐づく else ノード位置
  std::size_t else_index{std::numeric_limits<std::size_t>::max()};
  /// if / for ノードに紐づく閉じノード位置
  std::size_t end_index{std::numeric_limits<std::size_t>::max()};
};

/**
 * @brief consteval パースで生成されるバイトコード相当データ。
 *
 * 現状は命令配列ではなく、実行に必要なノード列を保持する。
 */
struct template_bytecode {
  /// ノード列
  std::array<template_node, k_template_max_nodes> nodes{};
  /// 使用ノード数
  std::size_t count{};
};

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
  auto begin = std::size_t{0};
  auto end = s.size();
  while (begin < end && is_space(s[begin])) {
    ++begin;
  }
  while (end > begin && is_space(s[end - 1])) {
    --end;
  }
  return s.substr(begin, end - begin);
}

enum class template_open_kind : std::uint8_t {
  expression,      // 式
  statement,       // 文
  comment,         // コメント
  line_statement,  // 行文
  none,            // 該当なし
};

struct template_open_match {
  std::size_t pos{std::string_view::npos};
  template_open_kind kind{template_open_kind::none};
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
[[nodiscard]] constexpr auto find_next_open(std::string_view src, std::size_t from) -> template_open_match {
  auto best = template_open_match{};

  auto consider = [&](std::size_t pos, template_open_kind kind) constexpr {
    if (pos == std::string_view::npos) {
      return;
    }
    if (best.pos == std::string_view::npos || pos < best.pos) {
      best = {pos, kind};
      return;
    }
    if (pos == best.pos) {
      auto const priority = [&](template_open_kind k) constexpr -> int {
        switch (k) {
        case template_open_kind::expression:
          return 0;
        case template_open_kind::statement:
          return 1;
        case template_open_kind::comment:
          return 2;
        case template_open_kind::line_statement:
          return 3;
        case template_open_kind::none:
          return 4;
        }
        return 4;
      };
      if (priority(kind) < priority(best.kind)) {
        best = {pos, kind};
      }
    }
  };

  consider(src.find(Delims::expression_open.sv(), from), template_open_kind::expression);
  consider(src.find(Delims::statement_open.sv(), from), template_open_kind::statement);
  consider(src.find(Delims::comment_open.sv(), from), template_open_kind::comment);
  consider(find_line_statement<Delims>(src, from), template_open_kind::line_statement);
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
template <auto Src, typename Delims = frozenchars::default_template_delimiters>
consteval auto parse_program() -> template_bytecode {
  auto constexpr src = Src.sv();
  auto program = template_bytecode{};
  auto constexpr expr_open = Delims::expression_open.sv();
  auto constexpr expr_close = Delims::expression_close.sv();
  auto constexpr stmt_open = Delims::statement_open.sv();
  auto constexpr stmt_close = Delims::statement_close.sv();
  auto constexpr comment_open = Delims::comment_open.sv();
  auto constexpr comment_close = Delims::comment_close.sv();
  auto constexpr line_stmt_prefix = Delims::line_statement_prefix.sv();

  if (expr_open.empty() || expr_close.empty() || stmt_open.empty() || stmt_close.empty() || comment_open.empty() || comment_close.empty()) {
    throw "template parse error: delimiters must not be empty";
  }

  auto stack = std::array<std::size_t, k_template_max_nodes>{};
  auto stack_size = std::size_t{0};

  auto push_node = [&](template_node_kind kind, std::size_t begin, std::size_t end) consteval -> std::size_t {
    if (program.count >= k_template_max_nodes) {
      throw "template parse error: too many nodes";
    }
    auto const index = program.count++;
    program.nodes[index] = template_node{kind, begin, end};
    return index;
  };

  // 1. text ブロックを先に切り出す
  // 2. {{ }}, {# #}, {% %} を識別してノード化
  // 3. if/for の対応関係は stack で後方解決
  auto process_statement = [&](std::string_view stmt, std::size_t begin, std::size_t end) consteval {
    if (stmt.starts_with("if ")) {
      auto const idx = push_node(template_node_kind::if_stmt, begin, end);
      stack[stack_size] = idx;
      ++stack_size;
      return;
    }
    // else / else if を同じノード種別で連結し、else_index で鎖状に接続する。
    if (stmt == "else" || stmt.starts_with("else if ")) {
      if (stack_size == 0 || program.nodes[stack[stack_size - 1]].kind != template_node_kind::if_stmt) {
        throw "template parse error: unexpected else";
      }
      auto const if_idx = stack[stack_size - 1];
      auto const else_idx = push_node(template_node_kind::else_stmt, begin, end);

      if (program.nodes[if_idx].else_index == std::numeric_limits<std::size_t>::max()) {
        program.nodes[if_idx].else_index = else_idx;
      } else {
        auto tail_else = program.nodes[if_idx].else_index;
        while (program.nodes[tail_else].else_index != std::numeric_limits<std::size_t>::max()) {
          tail_else = program.nodes[tail_else].else_index;
        }
        auto const tail_stmt = trim_view(src.substr(
          program.nodes[tail_else].begin,
          program.nodes[tail_else].end - program.nodes[tail_else].begin
        ));
        if (tail_stmt == "else") {
          throw "template parse error: unexpected else";
        }
        program.nodes[tail_else].else_index = else_idx;
      }
      return;
    }
    if (stmt == "endif") {
      if (stack_size == 0 || program.nodes[stack[stack_size - 1]].kind != template_node_kind::if_stmt) {
        throw "template parse error: unexpected endif";
      }
      auto const end_idx = push_node(template_node_kind::endif_stmt, begin, end);
      auto const if_idx = stack[stack_size - 1];
      program.nodes[if_idx].end_index = end_idx;
      --stack_size;
      return;
    }
    if (stmt.starts_with("for ")) {
      if (stmt.find(" in ") == std::string_view::npos) {
        throw "template parse error: invalid for statement";
      }
      auto const idx = push_node(template_node_kind::for_stmt, begin, end);
      stack[stack_size] = idx;
      ++stack_size;
      return;
    }
    if (stmt == "endfor") {
      if (stack_size == 0 || program.nodes[stack[stack_size - 1]].kind != template_node_kind::for_stmt) {
        throw "template parse error: unexpected endfor";
      }
      auto const for_idx = stack[stack_size - 1];
      auto const end_idx = push_node(template_node_kind::endfor_stmt, begin, end);
      program.nodes[for_idx].end_index = end_idx;
      --stack_size;
      return;
    }
    // set/include は単独ステートメントノードとして保持し、実行時に評価する。
    if (stmt.starts_with("set ")) {
      std::ignore = push_node(template_node_kind::set_stmt, begin, end);
      return;
    }
    if (stmt.starts_with("include ")) {
      std::ignore = push_node(template_node_kind::include_stmt, begin, end);
      return;
    }
    throw "template parse error: unsupported statement";
  };

  auto pos = std::size_t{0};
  while (pos < src.size()) {
    auto const next = find_next_open<Delims>(src, pos);
    auto const tag = next.pos;
    if (tag == std::string_view::npos) {
      if (pos < src.size()) {
        std::ignore = push_node(template_node_kind::text, pos, src.size());
      }
      break;
    }
    if (tag > pos) {
      std::ignore = push_node(template_node_kind::text, pos, tag);
    }
    switch (next.kind) {
    // 式
    case template_open_kind::expression: {
      auto const content_begin = tag + expr_open.size();
      auto const close = src.find(expr_close, content_begin);
      if (close == std::string_view::npos) {
        throw "template parse error: unclosed expression tag";
      }
      std::ignore = push_node(template_node_kind::expr, content_begin, close);
      pos = close + expr_close.size();
      break;
    }
    // コメント
    case template_open_kind::comment: {
      auto const content_begin = tag + comment_open.size();
      auto const close = src.find(comment_close, content_begin);
      if (close == std::string_view::npos) {
        throw "template parse error: unclosed comment tag";
      }
      pos = close + comment_close.size();
      break;
    }
    // 文
    case template_open_kind::statement: {
      auto const content_begin = tag + stmt_open.size();
      auto const close = src.find(stmt_close, content_begin);
      if (close == std::string_view::npos) {
        throw "template parse error: unclosed statement tag";
      }
      auto const stmt = trim_view(src.substr(content_begin, close - content_begin));
      process_statement(stmt, content_begin, close);
      pos = close + stmt_close.size();
      break;
    }
    // 行文
    case template_open_kind::line_statement: {
      auto const content_begin = tag + line_stmt_prefix.size();
      auto const line_end = src.find('\n', content_begin);
      auto const content_end = (line_end == std::string_view::npos) ? src.size() : line_end;
      auto const stmt = trim_view(src.substr(content_begin, content_end - content_begin));
      process_statement(stmt, content_begin, content_end);
      pos = (line_end == std::string_view::npos) ? src.size() : line_end + 1;
      break;
    }
    // 該当なし
    case template_open_kind::none:
      throw "template parse error: internal opener resolution failure";
    }
  }

  if (stack_size != 0) {
    throw "template parse error: unclosed block";
  }
  return program;
}

/**
 * @brief template_value を数値（double）として扱えるか試す。
 * @param v 対象値
 * @return 数値化できる場合は値、できない場合は std::nullopt
 */
[[nodiscard]] inline auto try_as_double(template_value const& v) -> std::optional<double> {
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
[[nodiscard]] inline auto equals_value(template_value const& lhs, template_value const& rhs) -> bool {
  if (lhs.storage.index() == rhs.storage.index()) {
    if (std::holds_alternative<template_null>(lhs.storage)) {
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
 * @throws template_render_error 比較不能な型の組み合わせ
 */
[[nodiscard]] inline auto less_value(template_value const& lhs, template_value const& rhs) -> bool {
  if (auto const lnum = try_as_double(lhs); lnum.has_value()) {
    auto const rnum = try_as_double(rhs);
    if (!rnum.has_value()) {
      throw template_render_error{"comparison requires same type"};
    }
    return *lnum < *rnum;
  }
  if (std::holds_alternative<std::string>(lhs.storage) && std::holds_alternative<std::string>(rhs.storage)) {
    return std::get<std::string>(lhs.storage) < std::get<std::string>(rhs.storage);
  }
  throw template_render_error{"comparison requires same type"};
}

/// @brief Forward declaration of evaluate_function
[[nodiscard]] inline auto evaluate_function(std::string_view func_name,
                                            std::vector<template_value> const& args,
                                            template_runtime_options const* runtime_options) -> template_value;

/**
 * @brief 実行時式評価器（簡易再帰下降パーサ）。
 */
class expr_parser {
public:
  /**
   * @brief 評価器を初期化する。
   * @param text 式文字列
   * @param scopes 変数探索スコープ（内側 -> 外側）
   */
  expr_parser(std::string_view text,
              std::vector<template_object> const& scopes,
              template_runtime_options const* runtime_options = nullptr)
    : text_(text), scopes_(scopes), runtime_options_(runtime_options) {}

  /**
   * @brief 式を評価する。
   * @param evaluate false のときは短絡用に副作用なしで走査だけ行う
   * @return 評価結果
   */
  [[nodiscard]] auto parse(bool evaluate = true) -> template_value {
    auto v = parse_or(evaluate);
    skip_space();
    if (pos_ != text_.size()) {
      throw template_render_error{"unexpected token in expression"};
    }
    return v;
  }

private:
  std::string_view text_;
  std::size_t pos_{};
  std::vector<template_object> const& scopes_;
  template_runtime_options const* runtime_options_{};

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
  [[nodiscard]] auto parse_or(bool evaluate) -> template_value {
    auto lhs = parse_and(evaluate);
    while (consume("or")) {
      auto const lhs_truth = evaluate && frozenchars::template_truthy(lhs);
      auto rhs = parse_and(evaluate && !lhs_truth);
      if (evaluate) {
        lhs = template_value{lhs_truth || template_truthy(rhs)};
      }
    }
    return lhs;
  }

  /**
   * @brief AND 優先順位レベルを評価する。
   */
  [[nodiscard]] auto parse_and(bool evaluate) -> template_value {
    auto lhs = parse_in(evaluate);
    while (consume("and")) {
      auto const lhs_truth = evaluate && frozenchars::template_truthy(lhs);
      auto rhs = parse_in(evaluate && lhs_truth);
      if (evaluate) {
        lhs = template_value{lhs_truth && frozenchars::template_truthy(rhs)};
      }
    }
    return lhs;
  }

  /**
   * @brief IN 演算レベルを評価する。
   */
  [[nodiscard]] auto parse_in(bool evaluate) -> template_value {
    auto lhs = parse_eq(evaluate);
    while (consume("in")) {
      auto rhs = parse_eq(evaluate);
      if (!evaluate) {
        lhs = template_value{};
        continue;
      }
      if (std::holds_alternative<std::string>(lhs.storage) && std::holds_alternative<std::string>(rhs.storage)) {
        lhs = template_value{std::get<std::string>(rhs.storage).find(std::get<std::string>(lhs.storage)) != std::string::npos};
      } else if (std::holds_alternative<template_array>(rhs.storage)) {
        auto found = false;
        for (auto const& v : std::get<template_array>(rhs.storage)) {
          if (equals_value(lhs, v)) {
            found = true;
            break;
          }
        }
        lhs = template_value{found};
      } else if (std::holds_alternative<template_object>(rhs.storage) && std::holds_alternative<std::string>(lhs.storage)) {
        auto const& obj = std::get<template_object>(rhs.storage);
        lhs = template_value{obj.contains(std::get<std::string>(lhs.storage))};
      } else {
        throw template_render_error{"invalid operands for in"};
      }
    }
    return lhs;
  }

  /**
   * @brief 等価比較レベルを評価する。
   */
  [[nodiscard]] auto parse_eq(bool evaluate) -> template_value {
    auto lhs = parse_rel(evaluate);
    while (true) {
      if (consume("==")) {
        auto rhs = parse_rel(evaluate);
        if (evaluate) {
          lhs = template_value{equals_value(lhs, rhs)};
        }
      } else if (consume("!=")) {
        auto rhs = parse_rel(evaluate);
        if (evaluate) {
          lhs = template_value{!equals_value(lhs, rhs)};
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
  [[nodiscard]] auto parse_rel(bool evaluate) -> template_value {
    auto lhs = parse_add(evaluate);
    while (true) {
      if (consume("<=")) {
        auto rhs = parse_add(evaluate);
        if (evaluate) {
          lhs = template_value{!less_value(rhs, lhs)};
        }
      } else if (consume(">=")) {
        auto rhs = parse_add(evaluate);
        if (evaluate) {
          lhs = template_value{!less_value(lhs, rhs)};
        }
      } else if (consume("<")) {
        auto rhs = parse_add(evaluate);
        if (evaluate) {
          lhs = template_value{less_value(lhs, rhs)};
        }
      } else if (consume(">")) {
        auto rhs = parse_add(evaluate);
        if (evaluate) {
          lhs = template_value{less_value(rhs, lhs)};
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
  [[nodiscard]] auto parse_add(bool evaluate) -> template_value {
    auto lhs = parse_mul(evaluate);
    while (true) {
      if (consume("+")) {
        auto rhs = parse_mul(evaluate);
        if (!evaluate) {
          lhs = template_value{};
          continue;
        }
        if (std::holds_alternative<std::string>(lhs.storage) && std::holds_alternative<std::string>(rhs.storage)) {
          lhs = template_value{std::get<std::string>(lhs.storage) + std::get<std::string>(rhs.storage)};
        } else {
          auto const lnum = try_as_double(lhs);
          auto const rnum = try_as_double(rhs);
          if (!lnum.has_value() || !rnum.has_value()) {
            throw template_render_error{"invalid operands for +"};
          }
          lhs = template_value{*lnum + *rnum};
        }
      } else if (consume("-")) {
        auto rhs = parse_mul(evaluate);
        if (evaluate) {
          auto const lnum = try_as_double(lhs);
          auto const rnum = try_as_double(rhs);
          if (!lnum.has_value() || !rnum.has_value()) {
            throw template_render_error{"invalid operands for -"};
          }
          lhs = template_value{*lnum - *rnum};
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
  [[nodiscard]] auto parse_mul(bool evaluate) -> template_value {
    auto lhs = parse_postfix(evaluate);
    while (true) {
      if (consume("*")) {
        auto rhs = parse_postfix(evaluate);
        if (evaluate) {
          auto const lnum = try_as_double(lhs);
          auto const rnum = try_as_double(rhs);
          if (!lnum.has_value() || !rnum.has_value()) {
            throw template_render_error{"invalid operands for *"};
          }
          lhs = template_value{*lnum * *rnum};
        }
      } else if (consume("/")) {
        auto rhs = parse_postfix(evaluate);
        if (evaluate) {
          auto const lnum = try_as_double(lhs);
          auto const rnum = try_as_double(rhs);
          if (!lnum.has_value() || !rnum.has_value()) {
            throw template_render_error{"invalid operands for /"};
          }
          lhs = template_value{*lnum / *rnum};
        }
      } else if (consume("%")) {
        auto rhs = parse_postfix(evaluate);
        if (evaluate) {
          if (!std::holds_alternative<std::int64_t>(lhs.storage) || !std::holds_alternative<std::int64_t>(rhs.storage)) {
            throw template_render_error{"invalid operands for %"};
          }
          lhs = template_value{std::get<std::int64_t>(lhs.storage) % std::get<std::int64_t>(rhs.storage)};
        }
      } else {
        break;
      }
    }
    return lhs;
  }

  [[nodiscard]] auto parse_unary(bool evaluate) -> template_value {
    if (consume("not")) {
      auto v = parse_unary(evaluate);
      return evaluate ? template_value{!template_truthy(v)} : template_value{};
    }
    if (consume("-")) {
      auto v = parse_unary(evaluate);
      if (!evaluate) {
        return template_value{};
      }
      if (std::holds_alternative<std::int64_t>(v.storage)) {
        return template_value{-std::get<std::int64_t>(v.storage)};
      }
      auto const num = try_as_double(v);
      if (!num.has_value()) {
        throw template_render_error{"invalid operand for unary -"};
      }
      return template_value{-*num};
    }
    return parse_primary(evaluate);
  }

  [[nodiscard]] auto parse_postfix(bool evaluate) -> template_value {
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
            if (text_[pos_] == '(') ++paren_depth;
            else if (text_[pos_] == ')') --paren_depth;
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
        throw template_render_error{"unexpected token after pipe operator"};
      }
      auto const func_name = text_.substr(start, pos_ - start);

      skip_space();
      auto args = std::vector<template_value>{};
      if (pos_ < text_.size() && text_[pos_] == '(') {
        ++pos_;
        skip_space();

        if (pos_ < text_.size() && text_[pos_] != ')') {
          while (true) {
            args.push_back(parse_or(evaluate));
            skip_space();
            if (pos_ >= text_.size()) {
              throw template_render_error{"unclosed pipe function call"};
            }
            if (text_[pos_] == ')') {
              break;
            }
            if (text_[pos_] != ',') {
              throw template_render_error{"expected comma or closing parenthesis in pipe function call"};
            }
            ++pos_;
            skip_space();
          }
        }

        if (!consume(")")) {
          throw template_render_error{"missing closing parenthesis in pipe function call"};
        }
      }

      args.insert(args.begin(), v);
      v = evaluate_function(func_name, args, runtime_options_);
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
  [[nodiscard]] auto resolve_name(std::string_view name, bool evaluate) -> template_value {
    if (!evaluate) {
      return template_value{};
    }
    auto path = std::vector<std::string_view>{};
    auto b = std::size_t{0};
    for (auto i = std::size_t{0}; i <= name.size(); ++i) {
      if (i == name.size() || name[i] == '.') {
        path.push_back(name.substr(b, i - b));
        b = i + 1;
      }
    }
    if (path.empty()) {
      throw template_render_error{"empty identifier"};
    }
    auto const* current = static_cast<template_value const*>(nullptr);
    for (auto s = scopes_.rbegin(); s != scopes_.rend(); ++s) {
      auto const it = s->find(std::string{path[0]});
      if (it != s->end()) {
        current = &it->second;
        break;
      }
    }
    if (current == nullptr) {
      throw template_render_error{"undefined variable: " + std::string{name}};
    }
    for (auto i = std::size_t{1}; i < path.size(); ++i) {
      if (!std::holds_alternative<template_object>(current->storage)) {
        throw template_render_error{"cannot resolve path: " + std::string{name}};
      }
      auto const& obj = std::get<template_object>(current->storage);
      auto const it = obj.find(std::string{path[i]});
      if (it == obj.end()) {
        throw template_render_error{"undefined variable: " + std::string{name}};
      }
      current = &it->second;
    }
    return *current;
  }

  /**
   * @brief リテラル・括弧式・識別子を評価する。
   */
  [[nodiscard]] auto parse_primary(bool evaluate) -> template_value {
    skip_space();
    if (consume("(")) {
      auto v = parse_or(evaluate);
      if (!consume(")")) {
        throw template_render_error{"missing closing parenthesis"};
      }
      return v;
    }
    if (consume("true")) {
      return template_value{true};
    }
    if (consume("false")) {
      return template_value{false};
    }
    if (consume("null")) {
      return template_value{};
    }
    skip_space();
    if (pos_ < text_.size() && (text_[pos_] == '"' || text_[pos_] == '\'')) {
      auto const q = text_[pos_++];
      auto start = pos_;
      while (pos_ < text_.size() && text_[pos_] != q) {
        ++pos_;
      }
      if (pos_ >= text_.size()) {
        throw template_render_error{"unterminated string literal"};
      }
      auto out = std::string{text_.substr(start, pos_ - start)};
      ++pos_;
      return template_value{std::move(out)};
    }
    if (pos_ < text_.size() && (std::isdigit(static_cast<unsigned char>(text_[pos_])) != 0)) {
      auto start = pos_;
      auto has_dot = false;
      while (pos_ < text_.size()) {
        auto const c = text_[pos_];
        if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
          ++pos_;
          continue;
        }
        if (c == '.') {
          has_dot = true;
          ++pos_;
          continue;
        }
        break;
      }
      auto const n = std::string{text_.substr(start, pos_ - start)};
      if (has_dot) {
        return template_value{std::stod(n)};
      }
      return template_value{std::stoll(n)};
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
      throw template_render_error{"unexpected token in expression"};
    }
    auto const name = text_.substr(start, pos_ - start);

    // Check if this is a function call (identifier followed by '(')
    skip_space();
    if (pos_ < text_.size() && text_[pos_] == '(') {
      // If we're in a non-evaluation context (short-circuit), skip to closing paren
      if (!evaluate) {
        auto paren_depth = 1;
        ++pos_;  // skip opening paren
        while (pos_ < text_.size() && paren_depth > 0) {
          if (text_[pos_] == '(') ++paren_depth;
          else if (text_[pos_] == ')') --paren_depth;
          ++pos_;
        }
        return template_value{};
      }

      ++pos_;  // consume '('

      // Parse function arguments
      auto args = std::vector<template_value>{};
      skip_space();

      // Check for empty argument list
      if (pos_ < text_.size() && text_[pos_] != ')') {
        while (true) {
          args.push_back(parse_or(evaluate));
          skip_space();
          if (pos_ >= text_.size()) {
            throw template_render_error{"unclosed function call"};
          }
          if (text_[pos_] == ')') {
            break;
          }
          if (text_[pos_] != ',') {
            throw template_render_error{"expected comma or closing parenthesis in function call"};
          }
          ++pos_;  // consume ','
          skip_space();
        }
      }

      if (!consume(")")) {
        throw template_render_error{"missing closing parenthesis in function call"};
      }

      return evaluate_function(name, args, runtime_options_);
    }

    return resolve_name(name, evaluate);

  }
};

/**
 * @brief for 文ヘッダを分解した結果。
 */
struct for_header {
  /// key 変数名（key,value 形式時のみ使用）
  std::string key_name{};
  /// value 変数名
  std::string value_name{};
  /// 反復対象式
  std::string expr{};
  /// key,value 形式かどうか
  bool has_key{};
};

/**
 * @brief `{% for ... in ... %}` 文ヘッダを解析する。
 * @param stmt ステートメント本体
 * @return 分解済みヘッダ
 * @throws template_render_error 構文が不正な場合
 */
[[nodiscard]] inline auto parse_for_header(std::string_view stmt) -> for_header {
  auto body = trim_view(stmt);
  if (!body.starts_with("for ")) {
    throw template_render_error{"invalid for statement"};
  }
  body.remove_prefix(4);
  auto const in_pos = body.find(" in ");
  if (in_pos == std::string_view::npos) {
    throw template_render_error{"invalid for statement"};
  }
  auto lhs = trim_view(body.substr(0, in_pos));
  auto rhs = trim_view(body.substr(in_pos + 4));
  if (rhs.empty()) {
    throw template_render_error{"invalid for statement"};
  }
  auto header = for_header{};
  auto comma = lhs.find(',');
  if (comma == std::string_view::npos) {
    header.value_name = std::string{trim_view(lhs)};
    header.has_key = false;
  } else {
    header.key_name = std::string{trim_view(lhs.substr(0, comma))};
    header.value_name = std::string{trim_view(lhs.substr(comma + 1))};
    header.has_key = true;
  }
  if (header.value_name.empty() || (header.has_key && header.key_name.empty())) {
    throw template_render_error{"invalid for statement"};
  }
  header.expr = std::string{rhs};
  return header;
}

struct set_statement {
  std::string target{};
  std::string expr{};
};

/**
 * @brief `{% set a.b = expr %}` を `target` と `expr` に分解する。
 */
[[nodiscard]] inline auto parse_set_statement(std::string_view stmt) -> set_statement {
  auto body = trim_view(stmt);
  if (!body.starts_with("set ")) {
    throw template_render_error{"invalid set statement"};
  }
  body.remove_prefix(4);
  auto const eq_pos = body.find('=');
  if (eq_pos == std::string_view::npos) {
    throw template_render_error{"invalid set statement"};
  }
  auto const target = trim_view(body.substr(0, eq_pos));
  auto const expr = trim_view(body.substr(eq_pos + 1));
  if (target.empty() || expr.empty()) {
    throw template_render_error{"invalid set statement"};
  }
  return set_statement{std::string{target}, std::string{expr}};
}

/**
 * @brief ドット区切りパスへ値を代入する（必要なら中間 object を生成）。
 */
inline auto assign_path(template_object& scope, std::string_view path, template_value value) -> void {
  auto pos = 0uz;
  auto key_begin = 0uz;
  auto* current_scope = &scope;

  while (true) {
    pos = path.find('.', key_begin);
    if (pos == std::string_view::npos) {
      auto const key = std::string{trim_view(path.substr(key_begin))};
      if (key.empty()) {
        throw template_render_error{"invalid set target"};
      }
      current_scope->insert_or_assign(key, std::move(value));
      return;
    }

    auto const key = std::string{trim_view(path.substr(key_begin, pos - key_begin))};
    if (key.empty()) {
      throw template_render_error{"invalid set target"};
    }

    auto [it, inserted] = current_scope->insert_or_assign(key, template_value{template_object{}});
    if (!inserted && !std::holds_alternative<template_object>(it->second.storage)) {
      throw template_render_error{"set target path is not object"};
    }
    current_scope = &std::get<template_object>(it->second.storage);
    key_begin = pos + 1;
  }
}

/**
 * @brief スコープ連鎖（内側→外側）から代入先を決定して set を適用する。
 */
inline auto assign_to_scopes(std::vector<template_object>& scopes, std::string_view path, template_value value) -> void {
  auto const dot_pos = path.find('.');
  auto const root_name = std::string{
    trim_view(dot_pos == std::string_view::npos ? path : path.substr(0, dot_pos))
  };
  if (root_name.empty()) {
    throw template_render_error{"invalid set target"};
  }

  for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
    if (it->contains(root_name)) {
      assign_path(*it, path, std::move(value));
      return;
    }
  }

  assign_path(scopes.back(), path, std::move(value));
}

/**
 * @brief if / else if 文から条件式部分のみを取り出す。
 */
[[nodiscard]] inline auto if_condition_expr(std::string_view stmt) -> std::string_view {
  auto body = trim_view(stmt);
  if (body.starts_with("if ")) {
    return trim_view(body.substr(2));
  }
  if (body.starts_with("else if ")) {
    return trim_view(body.substr(7));
  }
  throw template_render_error{"invalid if statement"};
}

/// @brief Evaluate function call by name with arguments
/// @param func_name Function name (e.g., "upper", "lower")
/// @param args Vector of evaluated arguments
/// @return Result of function call
[[nodiscard]] inline auto evaluate_function(std::string_view func_name,
                                            std::vector<template_value> const& args,
                                            template_runtime_options const* runtime_options) -> template_value {
  if (func_name == "upper") {
    if (args.size() != 1) {
      throw template_render_error{"upper() expects 1 argument"};
    }
    if (!std::holds_alternative<std::string>(args[0].storage)) {
      throw template_render_error{"upper() expects string argument"};
    }
    return template_value{fn_upper(std::get<std::string>(args[0].storage))};
  }

  if (func_name == "lower") {
    if (args.size() != 1) {
      throw template_render_error{"lower() expects 1 argument"};
    }
    if (!std::holds_alternative<std::string>(args[0].storage)) {
      throw template_render_error{"lower() expects string argument"};
    }
    return template_value{fn_lower(std::get<std::string>(args[0].storage))};
  }

  if (func_name == "capitalize") {
    if (args.size() != 1) {
      throw template_render_error{"capitalize() expects 1 argument"};
    }
    if (!std::holds_alternative<std::string>(args[0].storage)) {
      throw template_render_error{"capitalize() expects string argument"};
    }
    return template_value{fn_capitalize(std::get<std::string>(args[0].storage))};
  }

  if (func_name == "replace") {
    if (args.size() != 3) {
      throw template_render_error{"replace() expects 3 arguments"};
    }
    if (!std::holds_alternative<std::string>(args[0].storage) ||
        !std::holds_alternative<std::string>(args[1].storage) ||
        !std::holds_alternative<std::string>(args[2].storage)) {
      throw template_render_error{"replace() expects 3 string arguments"};
    }
    return template_value{fn_replace(
      std::get<std::string>(args[0].storage),
      std::get<std::string>(args[1].storage),
      std::get<std::string>(args[2].storage)
    )};
  }

  if (func_name == "length") {
    if (args.size() != 1) {
      throw template_render_error{"length() expects 1 argument"};
    }
    if (!std::holds_alternative<template_array>(args[0].storage)) {
      throw template_render_error{"length() expects array argument"};
    }
    return template_value{fn_length(std::get<template_array>(args[0].storage))};
  }

  if (func_name == "first") {
    if (args.size() != 1) {
      throw template_render_error{"first() expects 1 argument"};
    }
    if (!std::holds_alternative<template_array>(args[0].storage)) {
      throw template_render_error{"first() expects array argument"};
    }
    return fn_first(std::get<template_array>(args[0].storage));
  }

  if (func_name == "last") {
    if (args.size() != 1) {
      throw template_render_error{"last() expects 1 argument"};
    }
    if (!std::holds_alternative<template_array>(args[0].storage)) {
      throw template_render_error{"last() expects array argument"};
    }
    return fn_last(std::get<template_array>(args[0].storage));
  }

  if (func_name == "join") {
    if (args.size() != 2) {
      throw template_render_error{"join() expects 2 arguments"};
    }
    if (!std::holds_alternative<template_array>(args[0].storage)) {
      throw template_render_error{"join() expects first argument to be array"};
    }
    if (!std::holds_alternative<std::string>(args[1].storage)) {
      throw template_render_error{"join() expects second argument to be string"};
    }
    return template_value{fn_join(
      std::get<template_array>(args[0].storage),
      std::get<std::string>(args[1].storage)
    )};
  }

  if (func_name == "sort") {
    if (args.size() != 1) {
      throw template_render_error{"sort() expects 1 argument"};
    }
    if (!std::holds_alternative<template_array>(args[0].storage)) {
      throw template_render_error{"sort() expects array argument"};
    }
    return template_value{fn_sort(std::get<template_array>(args[0].storage))};
  }

  if (func_name == "range") {
    if (args.size() == 1) {
      if (!std::holds_alternative<std::int64_t>(args[0].storage)) {
        throw template_render_error{"range() expects integer argument"};
      }
      return template_value{fn_range(std::get<std::int64_t>(args[0].storage))};
    } else if (args.size() == 2) {
      if (!std::holds_alternative<std::int64_t>(args[0].storage) ||
          !std::holds_alternative<std::int64_t>(args[1].storage)) {
        throw template_render_error{"range() expects integer arguments"};
      }
      return template_value{fn_range(
        std::get<std::int64_t>(args[0].storage),
        std::get<std::int64_t>(args[1].storage)
      )};
    } else if (args.size() == 3) {
      if (!std::holds_alternative<std::int64_t>(args[0].storage) ||
          !std::holds_alternative<std::int64_t>(args[1].storage) ||
          !std::holds_alternative<std::int64_t>(args[2].storage)) {
        throw template_render_error{"range() expects integer arguments"};
      }
      return template_value{fn_range(
        std::get<std::int64_t>(args[0].storage),
        std::get<std::int64_t>(args[1].storage),
        std::get<std::int64_t>(args[2].storage)
      )};
    } else {
      throw template_render_error{"range() expects 1, 2, or 3 arguments"};
    }
  }

  if (func_name == "abs") {
    if (args.size() != 1) {
      throw template_render_error{"abs() expects 1 argument"};
    }
    return fn_abs(args[0]);
  }

  if (func_name == "round") {
    if (args.size() == 1) {
      return fn_round(args[0]);
    } else if (args.size() == 2) {
      return fn_round(args[0], args[1]);
    } else {
      throw template_render_error{"round() expects 1 or 2 arguments"};
    }
  }

  if (func_name == "max") {
    if (args.size() != 1) {
      throw template_render_error{"max() expects 1 argument"};
    }
    if (!std::holds_alternative<template_array>(args[0].storage)) {
      throw template_render_error{"max() expects array argument"};
    }
    return fn_max(std::get<template_array>(args[0].storage));
  }

  if (func_name == "min") {
    if (args.size() != 1) {
      throw template_render_error{"min() expects 1 argument"};
    }
    if (!std::holds_alternative<template_array>(args[0].storage)) {
      throw template_render_error{"min() expects array argument"};
    }
    return fn_min(std::get<template_array>(args[0].storage));
  }

  if (func_name == "even") {
    if (args.size() != 1) {
      throw template_render_error{"even() expects 1 argument"};
    }
    return fn_even(args[0]);
  }

  if (func_name == "odd") {
    if (args.size() != 1) {
      throw template_render_error{"odd() expects 1 argument"};
    }
    return fn_odd(args[0]);
  }

  if (func_name == "divisibleBy") {
    if (args.size() != 2) {
      throw template_render_error{"divisibleBy() expects 2 arguments"};
    }
    return fn_divisibleBy(args[0], args[1]);
  }

  if (func_name == "int") {
    if (args.size() != 1) {
      throw template_render_error{"int() expects 1 argument"};
    }
    return fn_int(args[0]);
  }

  if (func_name == "float") {
    if (args.size() != 1) {
      throw template_render_error{"float() expects 1 argument"};
    }
    return fn_float(args[0]);
  }

  if (func_name == "isString") {
    if (args.size() != 1) {
      throw template_render_error{"isString() expects 1 argument"};
    }
    return fn_isString(args[0]);
  }

  if (func_name == "isArray") {
    if (args.size() != 1) {
      throw template_render_error{"isArray() expects 1 argument"};
    }
    return fn_isArray(args[0]);
  }

  if (func_name == "isNumber") {
    if (args.size() != 1) {
      throw template_render_error{"isNumber() expects 1 argument"};
    }
    return fn_isNumber(args[0]);
  }

  if (func_name == "isObject") {
    if (args.size() != 1) {
      throw template_render_error{"isObject() expects 1 argument"};
    }
    return fn_isObject(args[0]);
  }

  if (func_name == "isBoolean") {
    if (args.size() != 1) {
      throw template_render_error{"isBoolean() expects 1 argument"};
    }
    return fn_isBoolean(args[0]);
  }

  if (func_name == "isFloat") {
    if (args.size() != 1) {
      throw template_render_error{"isFloat() expects 1 argument"};
    }
    return fn_isFloat(args[0]);
  }

  if (func_name == "isInteger") {
    if (args.size() != 1) {
      throw template_render_error{"isInteger() expects 1 argument"};
    }
    return fn_isInteger(args[0]);
  }

  if (func_name == "isNone") {
    if (args.size() != 1) {
      throw template_render_error{"isNone() expects 1 argument"};
    }
    return fn_isNone(args[0]);
  }

  if (func_name == "isEmpty") {
    if (args.size() != 1) {
      throw template_render_error{"isEmpty() expects 1 argument"};
    }
    return fn_isEmpty(args[0]);
  }

  if (func_name == "default") {
    if (args.size() != 2) {
      throw template_render_error{"default() expects 2 arguments"};
    }
    return fn_default(args[0], args[1]);
  }

  if (func_name == "at") {
    if (args.size() != 2) {
      throw template_render_error{"at() expects 2 arguments"};
    }
    return fn_at(args[0], args[1]);
  }

  if (func_name == "exists") {
    if (args.size() != 1) {
      throw template_render_error{"exists() expects 1 argument"};
    }
    return fn_exists(args[0]);
  }

  if (func_name == "existsIn") {
    if (args.size() != 2) {
      throw template_render_error{"existsIn() expects 2 arguments"};
    }
    return fn_existsIn(args[0], args[1]);
  }

  // 組み込み関数で見つからない場合のみ、実行時登録コールバックを参照する。
  if (runtime_options != nullptr) {
    auto const it = runtime_options->function_callbacks.find(std::string{func_name});
    if (it != runtime_options->function_callbacks.end()) {
      return it->second(args);
    }
  }

  throw template_render_error{"unknown function: " + std::string{func_name}};
}

/**
 * @brief パース済みバイトコードを実行して文字列を生成する。
 * @tparam Src 元テンプレート文字列
 * @param program consteval で生成済みの内部プログラム
 * @param root ルートスコープ
 * @return レンダリング結果
 *
 * @note 長いメソッドのため、以下の3フェーズに分けて読めるようにしている。
 * - ノード走査
 * - ノード種別ごとの評価
 * - if/for 制御の分岐・反復
 */
template <auto Src, typename OutputBuffer, typename Delims = frozenchars::default_template_delimiters>
auto render_program(template_bytecode const& program,
                    template_object const& root,
                    OutputBuffer& out,
                    template_runtime_options const* runtime_options = nullptr) -> void {
  auto constexpr src = Src.sv();
  auto scopes = std::vector<template_object>{root};

  // begin/end は program.nodes の半開区間。
  auto const render_range = [&](this auto&& self, std::size_t begin, std::size_t end) -> void {
    auto i = begin;
    while (i < end) {
      auto const& node = program.nodes[i];
      // ノード種別ごとに分岐し、必要に応じて i をジャンプ更新する
      switch (node.kind) {
      // text ノードはそのまま出力に追加する
      case template_node_kind::text: {
        out.append(src.substr(node.begin, node.end - node.begin));
        ++i;
        break;
      }

      // expr ノードは式を評価して出力に追加する
      case template_node_kind::expr: {
        auto const expr = trim_view(src.substr(node.begin, node.end - node.begin));
        auto value = expr_parser{expr, scopes, runtime_options}.parse();
        out.append(frozenchars::template_to_string(value));
        ++i;
        break;
      }

      // cond を評価し、then または else 節の範囲だけを再帰実行する
      case template_node_kind::if_stmt: {
        auto const stmt = trim_view(src.substr(node.begin, node.end - node.begin));
        auto const cond = expr_parser{if_condition_expr(stmt), scopes, runtime_options}.parse();
        auto const then_end = node.else_index != std::numeric_limits<std::size_t>::max()
                                ? node.else_index
                                : node.end_index;
        if (frozenchars::template_truthy(cond)) {
          self(i + 1, then_end);
        } else {
          // else-if 連鎖を順に評価し、最初に真になった分岐だけを実行する。
          auto else_idx = node.else_index;
          while (else_idx != std::numeric_limits<std::size_t>::max()) {
            auto const& else_node = program.nodes[else_idx];
            auto const else_stmt = trim_view(src.substr(else_node.begin, else_node.end - else_node.begin));
            auto const branch_end = else_node.else_index != std::numeric_limits<std::size_t>::max()
                                      ? else_node.else_index
                                      : node.end_index;
            if (else_stmt == "else") {
              self(else_idx + 1, branch_end);
              break;
            }
            if (else_stmt.starts_with("else if ")) {
              auto const else_cond = expr_parser{if_condition_expr(else_stmt), scopes, runtime_options}.parse();
              if (frozenchars::template_truthy(else_cond)) {
                self(else_idx + 1, branch_end);
                break;
              }
              else_idx = else_node.else_index;
              continue;
            }
            throw template_render_error{"invalid else statement"};
          }
        }
        i = node.end_index + 1;
        break;
      }

      // 配列反復とオブジェクト反復で束縛方法を切り替える
      case template_node_kind::for_stmt: {
        auto const stmt = trim_view(src.substr(node.begin, node.end - node.begin));
        auto const header = parse_for_header(stmt);
        auto iterable = expr_parser{header.expr, scopes, runtime_options}.parse();
        auto const body_begin = i + 1;
        auto const body_end = node.end_index;
        if (std::holds_alternative<template_array>(iterable.storage)) {
          auto const& arr = std::get<template_array>(iterable.storage);
          for (auto idx = std::size_t{0}; idx < arr.size(); ++idx) {
            auto frame = template_object{};
            frame.insert_or_assign(header.value_name, arr[idx]);
            auto loop_obj = template_object{};
            loop_obj.insert_or_assign("index", template_value{static_cast<std::int64_t>(idx)});
            loop_obj.insert_or_assign("index1", template_value{static_cast<std::int64_t>(idx + 1)});
            loop_obj.insert_or_assign("is_first", template_value{idx == 0});
            loop_obj.insert_or_assign("is_last", template_value{idx + 1 == arr.size()});
            frame.insert_or_assign("loop", template_value{std::move(loop_obj)});
            scopes.push_back(std::move(frame));
            self(body_begin, body_end);
            scopes.pop_back();
          }
        } else if (std::holds_alternative<template_object>(iterable.storage)) {
          auto const& obj = std::get<template_object>(iterable.storage);
          auto idx = std::size_t{0};
          for (auto const& [k, v] : obj) {
            auto frame = template_object{};
            if (header.has_key) {
              frame.insert_or_assign(header.key_name, template_value{k});
            }
            frame.insert_or_assign(header.value_name, v);
            auto loop_obj = template_object{};
            loop_obj.insert_or_assign("index", template_value{static_cast<std::int64_t>(idx)});
            loop_obj.insert_or_assign("index1", template_value{static_cast<std::int64_t>(idx + 1)});
            loop_obj.insert_or_assign("is_first", template_value{idx == 0});
            loop_obj.insert_or_assign("is_last", template_value{idx + 1 == obj.size()});
            frame.insert_or_assign("loop", template_value{std::move(loop_obj)});
            scopes.push_back(std::move(frame));
            self(body_begin, body_end);
            scopes.pop_back();
            ++idx;
          }
        } else {
          throw template_render_error{"for target must be array or object"};
        }
        i = node.end_index + 1;
        break;
      }

      case template_node_kind::set_stmt: {
        // set は式を評価して、現在のスコープ連鎖へ代入する。
        auto const stmt = trim_view(src.substr(node.begin, node.end - node.begin));
        auto const assignment = parse_set_statement(stmt);
        auto value = expr_parser{assignment.expr, scopes, runtime_options}.parse();
        assign_to_scopes(scopes, assignment.target, std::move(value));
        ++i;
        break;
      }

      case template_node_kind::include_stmt: {
        // include は登録テンプレート優先、未登録時は callback をフォールバックに使う。
        auto const stmt = trim_view(src.substr(node.begin, node.end - node.begin));
        auto const include_expr = trim_view(stmt.substr(8));
        auto include_name = expr_parser{include_expr, scopes, runtime_options}.parse();
        if (!std::holds_alternative<std::string>(include_name.storage)) {
          throw template_render_error{"include target must evaluate to string"};
        }
        auto const& key = std::get<std::string>(include_name.storage);
        if (runtime_options != nullptr) {
          if (auto const it = runtime_options->include_templates.find(key);
              it != runtime_options->include_templates.end()) {
            out.append(it->second);
            ++i;
            break;
          }
          if (runtime_options->include_callback) {
            out.append(runtime_options->include_callback(key, scopes.back()));
            ++i;
            break;
          }
        }
        throw template_render_error{"unknown include: " + key};
      }

      // ブロックの終了は無視する
      case template_node_kind::else_stmt:
      case template_node_kind::endif_stmt:
      case template_node_kind::endfor_stmt:
        ++i;
        break;
      }
    }
  };

  render_range(0, program.count);
}

template <auto Src, typename Delims = frozenchars::default_template_delimiters>
auto render_program(template_bytecode const& program,
                    template_object const& root,
                    template_runtime_options const* runtime_options = nullptr) -> std::string {
  auto constexpr src = Src.sv();
  auto out = std::string{};
  auto scopes = std::vector<template_object>{root};

  // begin/end は program.nodes の半開区間。
  auto const render_range = [&](this auto&& self, std::size_t begin, std::size_t end) -> void {
    auto i = begin;
    while (i < end) {
      auto const& node = program.nodes[i];
      // ノード種別ごとに分岐し、必要に応じて i をジャンプ更新する
      switch (node.kind) {
      // text ノードはそのまま出力に追加する
      case template_node_kind::text: {
        out.append(src.substr(node.begin, node.end - node.begin));
        ++i;
        break;
      }

      // expr ノードは式を評価して出力に追加する
      case template_node_kind::expr: {
        auto const expr = trim_view(src.substr(node.begin, node.end - node.begin));
        auto value = expr_parser{expr, scopes, runtime_options}.parse();
        out.append(frozenchars::template_to_string(value));
        ++i;
        break;
      }

      // cond を評価し、then または else 節の範囲だけを再帰実行する
      case template_node_kind::if_stmt: {
        auto const stmt = trim_view(src.substr(node.begin, node.end - node.begin));
        auto const cond = expr_parser{if_condition_expr(stmt), scopes, runtime_options}.parse();
        auto const then_end = node.else_index != std::numeric_limits<std::size_t>::max()
                                ? node.else_index
                                : node.end_index;
        if (frozenchars::template_truthy(cond)) {
          self(i + 1, then_end);
        } else {
          // else-if 連鎖を順に評価し、最初に真になった分岐だけを実行する。
          auto else_idx = node.else_index;
          while (else_idx != std::numeric_limits<std::size_t>::max()) {
            auto const& else_node = program.nodes[else_idx];
            auto const else_stmt = trim_view(src.substr(else_node.begin, else_node.end - else_node.begin));
            auto const branch_end = else_node.else_index != std::numeric_limits<std::size_t>::max()
                                      ? else_node.else_index
                                      : node.end_index;
            if (else_stmt == "else") {
              self(else_idx + 1, branch_end);
              break;
            }
            if (else_stmt.starts_with("else if ")) {
              auto const else_cond = expr_parser{if_condition_expr(else_stmt), scopes, runtime_options}.parse();
              if (frozenchars::template_truthy(else_cond)) {
                self(else_idx + 1, branch_end);
                break;
              }
              else_idx = else_node.else_index;
              continue;
            }
            throw template_render_error{"invalid else statement"};
          }
        }
        i = node.end_index + 1;
        break;
      }

      // 配列反復とオブジェクト反復で束縛方法を切り替える
      case template_node_kind::for_stmt: {
        auto const stmt = trim_view(src.substr(node.begin, node.end - node.begin));
        auto const header = parse_for_header(stmt);
        auto iterable = expr_parser{header.expr, scopes, runtime_options}.parse();
        auto const body_begin = i + 1;
        auto const body_end = node.end_index;
        if (std::holds_alternative<template_array>(iterable.storage)) {
          auto const& arr = std::get<template_array>(iterable.storage);
          for (auto idx = std::size_t{0}; idx < arr.size(); ++idx) {
            auto frame = template_object{};
            frame.insert_or_assign(header.value_name, arr[idx]);
            auto loop_obj = template_object{};
            loop_obj.insert_or_assign("index", template_value{static_cast<std::int64_t>(idx)});
            loop_obj.insert_or_assign("index1", template_value{static_cast<std::int64_t>(idx + 1)});
            loop_obj.insert_or_assign("is_first", template_value{idx == 0});
            loop_obj.insert_or_assign("is_last", template_value{idx + 1 == arr.size()});
            frame.insert_or_assign("loop", template_value{std::move(loop_obj)});
            scopes.push_back(std::move(frame));
            self(body_begin, body_end);
            scopes.pop_back();
          }
        } else if (std::holds_alternative<template_object>(iterable.storage)) {
          auto const& obj = std::get<template_object>(iterable.storage);
          auto idx = std::size_t{0};
          for (auto const& [k, v] : obj) {
            auto frame = template_object{};
            if (header.has_key) {
              frame.insert_or_assign(header.key_name, template_value{k});
            }
            frame.insert_or_assign(header.value_name, v);
            auto loop_obj = template_object{};
            loop_obj.insert_or_assign("index", template_value{static_cast<std::int64_t>(idx)});
            loop_obj.insert_or_assign("index1", template_value{static_cast<std::int64_t>(idx + 1)});
            loop_obj.insert_or_assign("is_first", template_value{idx == 0});
            loop_obj.insert_or_assign("is_last", template_value{idx + 1 == obj.size()});
            frame.insert_or_assign("loop", template_value{std::move(loop_obj)});
            scopes.push_back(std::move(frame));
            self(body_begin, body_end);
            scopes.pop_back();
            ++idx;
          }
        } else {
          throw template_render_error{"for target must be array or object"};
        }
        i = node.end_index + 1;
        break;
      }

      case template_node_kind::set_stmt: {
        // set は式を評価して、現在のスコープ連鎖へ代入する。
        auto const stmt = trim_view(src.substr(node.begin, node.end - node.begin));
        auto const assignment = parse_set_statement(stmt);
        auto value = expr_parser{assignment.expr, scopes, runtime_options}.parse();
        assign_to_scopes(scopes, assignment.target, std::move(value));
        ++i;
        break;
      }

      case template_node_kind::include_stmt: {
        // include は登録テンプレート優先、未登録時は callback をフォールバックに使う。
        auto const stmt = trim_view(src.substr(node.begin, node.end - node.begin));
        auto const include_expr = trim_view(stmt.substr(8));
        auto include_name = expr_parser{include_expr, scopes, runtime_options}.parse();
        if (!std::holds_alternative<std::string>(include_name.storage)) {
          throw template_render_error{"include target must evaluate to string"};
        }
        auto const& key = std::get<std::string>(include_name.storage);
        if (runtime_options != nullptr) {
          if (auto const it = runtime_options->include_templates.find(key);
              it != runtime_options->include_templates.end()) {
            out.append(it->second);
            ++i;
            break;
          }
          if (runtime_options->include_callback) {
            out.append(runtime_options->include_callback(key, scopes.back()));
            ++i;
            break;
          }
        }
        throw template_render_error{"unknown include: " + key};
      }

      // ブロックの終了は無視する
      case template_node_kind::else_stmt:
      case template_node_kind::endif_stmt:
      case template_node_kind::endfor_stmt:
        ++i;
        break;
      }
    }
  };

  render_range(0, program.count);
  return out;
}

} // namespace frozenchars::detail

namespace frozenchars {

/**
 * @brief テンプレートをレンダリングする高水準API。
 * @tparam Src テンプレート文字列
 * @param root ルートコンテキスト（object 必須）
 * @return 出力文字列
 */
template <auto Src, typename Delims = frozenchars::default_template_delimiters>
auto render_template(template_value const& root) -> std::string {
  if (!std::holds_alternative<template_object>(root.storage)) {
    throw template_render_error{"root context must be object"};
  }
  auto constexpr program = detail::parse_program<Src, Delims>();
  return detail::render_program<Src, Delims>(program, std::get<template_object>(root.storage));
}

template <auto Src, typename Delims = frozenchars::default_template_delimiters>
auto render_template(template_value const& root, template_runtime_options const& runtime_options) -> std::string {
  if (!std::holds_alternative<template_object>(root.storage)) {
    throw template_render_error{"root context must be object"};
  }
  auto constexpr program = detail::parse_program<Src, Delims>();
  return detail::render_program<Src, Delims>(program, std::get<template_object>(root.storage), &runtime_options);
}

/**
 * @brief テンプレートをレンダリングし、カスタム出力バッファへ結果を追加する。
 *
 * @tparam Src テンプレート文字列
 * @tparam OutputBuffer append() と result() メソッドを持つクラス
 * @tparam Delims テンプレート区切り文字
 * @param root ルートコンテキスト（object 必須）
 * @param output 出力バッファ（append() メソッドを呼び出される）
 * @return 成功時は void、エラー時は std::string のエラーメッセージ
 */
template <auto Src, typename OutputBuffer, typename Delims = frozenchars::default_template_delimiters>
  requires requires(OutputBuffer& output, std::string_view chunk) {
    output.append(chunk);
  }
auto render_template(template_value const& root, OutputBuffer& output) -> std::expected<void, std::string> {
  try {
    if (!std::holds_alternative<template_object>(root.storage)) {
      return std::unexpected(std::string{"root context must be object"});
    }
    auto constexpr program = detail::parse_program<Src, Delims>();
    detail::render_program<Src, OutputBuffer, Delims>(program, std::get<template_object>(root.storage), output);
    return {};
  } catch (std::exception const& e) {
    return std::unexpected(std::string{e.what()});
  } catch (...) {
    return std::unexpected(std::string{"unknown error during template rendering"});
  }
}

template <auto Src, typename OutputBuffer, typename Delims = frozenchars::default_template_delimiters>
  requires requires(OutputBuffer& output, std::string_view chunk) {
    output.append(chunk);
  }
auto render_template(template_value const& root,
                     OutputBuffer& output,
                     template_runtime_options const& runtime_options) -> std::expected<void, std::string> {
  try {
    if (!std::holds_alternative<template_object>(root.storage)) {
      return std::unexpected(std::string{"root context must be object"});
    }
    auto constexpr program = detail::parse_program<Src, Delims>();
    detail::render_program<Src, OutputBuffer, Delims>(
      program,
      std::get<template_object>(root.storage),
      output,
      &runtime_options
    );
    return {};
  } catch (std::exception const& e) {
    return std::unexpected(std::string{e.what()});
  } catch (...) {
    return std::unexpected(std::string{"unknown error during template rendering"});
  }
}

/**
 * @brief `Src` 固定のテンプレートVMラッパ。
 * @tparam Src テンプレート文字列
 */
template <auto Src, typename Delims = frozenchars::default_template_delimiters>
struct template_vm {
  /**
   * @brief レンダリングを実行する。
   * @param root ルートコンテキスト
   * @return 出力文字列
   */
  static auto render(template_value const& root) -> std::string {
    return render_template<Src, Delims>(root);
  }
};

} // namespace frozenchars

namespace frozenchars::inja {

/**
 * @brief テンプレートをレンダリングする高水準API。
 * @tparam Src テンプレート文字列
 * @param root ルートコンテキスト（object 必須）
 * @return 出力文字列
 */
template <auto Src, typename Delims = frozenchars::default_template_delimiters>
auto render(template_value const& root) -> std::string {
  return frozenchars::render_template<Src, Delims>(root);
}

template <auto Src, typename Delims = frozenchars::default_template_delimiters>
auto render(template_value const& root, template_runtime_options const& runtime_options) -> std::string {
  return frozenchars::render_template<Src, Delims>(root, runtime_options);
}

/**
 * @brief テンプレートをレンダリングし、カスタム出力バッファへ結果を追加する。
 *
 * @tparam Src テンプレート文字列
 * @tparam OutputBuffer append() と result() メソッドを持つクラス
 * @tparam Delims テンプレート区切り文字
 * @param root ルートコンテキスト（object 必須）
 * @param output 出力バッファ（append() メソッドを呼び出される）
 * @return 成功時は void、エラー時は std::string のエラーメッセージ
 */
template <auto Src, typename OutputBuffer, typename Delims = frozenchars::default_template_delimiters>
  requires requires(OutputBuffer& output, std::string_view chunk) {
    output.append(chunk);
  }
auto render(template_value const& root, OutputBuffer& output) -> std::expected<void, std::string> {
  return frozenchars::render_template<Src, OutputBuffer, Delims>(root, output);
}

template <auto Src, typename OutputBuffer, typename Delims = frozenchars::default_template_delimiters>
  requires requires(OutputBuffer& output, std::string_view chunk) {
    output.append(chunk);
  }
auto render(template_value const& root,
            OutputBuffer& output,
            template_runtime_options const& runtime_options) -> std::expected<void, std::string> {
  return frozenchars::render_template<Src, OutputBuffer, Delims>(root, output, runtime_options);
}

/**
 * @brief `Src` 固定のテンプレートVMラッパ。
 * @tparam Src テンプレート文字列
 */
template <auto Src, typename Delims = frozenchars::default_template_delimiters>
struct template_vm {
  /**
   * @brief レンダリングを実行する。
   * @param root ルートコンテキスト
   * @return 出力文字列
   */
  static auto render(template_value const& root) -> std::string {
    return frozenchars::inja::render<Src, Delims>(root);
  }
};

} // namespace frozenchars::inja
