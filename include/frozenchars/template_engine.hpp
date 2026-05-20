#pragma once

#include "frozen_string.hpp"
#include "template_value.hpp"

#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

namespace frozenchars::detail {

constexpr auto k_template_max_nodes = std::size_t{1024};

enum class template_node_kind : std::uint8_t { text, expr, if_stmt, else_stmt, endif_stmt, for_stmt, endfor_stmt };

struct template_node {
  template_node_kind kind{};
  std::size_t begin{};
  std::size_t end{};
  std::size_t else_index{std::numeric_limits<std::size_t>::max()};
  std::size_t end_index{std::numeric_limits<std::size_t>::max()};
};

struct template_program {
  std::array<template_node, k_template_max_nodes> nodes{};
  std::size_t count{};
};

[[nodiscard]] constexpr auto is_space(char c) -> bool {
  return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

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

template <auto Src>
consteval auto parse_program() -> template_program {
  auto program = template_program{};
  auto const src = Src.sv();
  auto stack = std::array<std::size_t, k_template_max_nodes>{};
  auto stack_kind = std::array<template_node_kind, k_template_max_nodes>{};
  auto stack_size = std::size_t{0};

  auto push_node = [&](template_node_kind kind, std::size_t begin, std::size_t end) consteval -> std::size_t {
    if (program.count >= k_template_max_nodes) {
      throw "template parse error: too many nodes";
    }
    auto const index = program.count++;
    program.nodes[index] = template_node{kind, begin, end};
    return index;
  };

  auto pos = std::size_t{0};
  while (pos < src.size()) {
    auto tag = src.find('{', pos);
    if (tag == std::string_view::npos) {
      if (pos < src.size()) {
        std::ignore = push_node(template_node_kind::text, pos, src.size());
      }
      break;
    }
    if (tag > pos) {
      std::ignore = push_node(template_node_kind::text, pos, tag);
    }
    if (tag + 1 >= src.size()) {
      std::ignore = push_node(template_node_kind::text, tag, src.size());
      break;
    }

    auto const open2 = src.substr(tag, 2);
    if (open2 == "{{") {
      auto close = src.find("}}", tag + 2);
      if (close == std::string_view::npos) {
        throw "template parse error: unclosed expression tag";
      }
      std::ignore = push_node(template_node_kind::expr, tag + 2, close);
      pos = close + 2;
      continue;
    }
    if (open2 == "{#") {
      auto close = src.find("#}", tag + 2);
      if (close == std::string_view::npos) {
        throw "template parse error: unclosed comment tag";
      }
      pos = close + 2;
      continue;
    }
    if (open2 == "{%") {
      auto close = src.find("%}", tag + 2);
      if (close == std::string_view::npos) {
        throw "template parse error: unclosed statement tag";
      }
      auto stmt = trim_view(src.substr(tag + 2, close - (tag + 2)));
      if (stmt.starts_with("if ")) {
        auto const idx = push_node(template_node_kind::if_stmt, tag + 2, close);
        stack[stack_size] = idx;
        stack_kind[stack_size] = template_node_kind::if_stmt;
        ++stack_size;
      } else if (stmt == "else") {
        if (stack_size == 0 || stack_kind[stack_size - 1] != template_node_kind::if_stmt) {
          throw "template parse error: unexpected else";
        }
        auto const if_idx = stack[stack_size - 1];
        auto const else_idx = push_node(template_node_kind::else_stmt, tag + 2, close);
        program.nodes[if_idx].else_index = else_idx;
        stack[stack_size - 1] = else_idx;
        stack_kind[stack_size - 1] = template_node_kind::else_stmt;
      } else if (stmt == "endif") {
        if (stack_size == 0) {
          throw "template parse error: unexpected endif";
        }
        auto const top_kind = stack_kind[stack_size - 1];
        if (top_kind != template_node_kind::if_stmt && top_kind != template_node_kind::else_stmt) {
          throw "template parse error: unexpected endif";
        }
        auto const end_idx = push_node(template_node_kind::endif_stmt, tag + 2, close);
        auto const top_idx = stack[stack_size - 1];
        if (top_kind == template_node_kind::else_stmt) {
          program.nodes[top_idx].end_index = end_idx;
          for (auto i = end_idx; i > 0; --i) {
            auto const candidate = i - 1;
            if (program.nodes[candidate].kind == template_node_kind::if_stmt && program.nodes[candidate].else_index == top_idx) {
              program.nodes[candidate].end_index = end_idx;
              break;
            }
          }
        } else {
          program.nodes[top_idx].end_index = end_idx;
        }
        --stack_size;
      } else if (stmt.starts_with("for ")) {
        if (stmt.find(" in ") == std::string_view::npos) {
          throw "template parse error: invalid for statement";
        }
        auto const idx = push_node(template_node_kind::for_stmt, tag + 2, close);
        stack[stack_size] = idx;
        stack_kind[stack_size] = template_node_kind::for_stmt;
        ++stack_size;
      } else if (stmt == "endfor") {
        if (stack_size == 0 || stack_kind[stack_size - 1] != template_node_kind::for_stmt) {
          throw "template parse error: unexpected endfor";
        }
        auto const for_idx = stack[stack_size - 1];
        auto const end_idx = push_node(template_node_kind::endfor_stmt, tag + 2, close);
        program.nodes[for_idx].end_index = end_idx;
        --stack_size;
      } else {
        throw "template parse error: unsupported statement";
      }
      pos = close + 2;
      continue;
    }

    std::ignore = push_node(template_node_kind::text, tag, tag + 1);
    pos = tag + 1;
  }

  if (stack_size != 0) {
    throw "template parse error: unclosed block";
  }
  return program;
}

[[nodiscard]] inline auto try_as_double(template_value const& v) -> std::optional<double> {
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return static_cast<double>(*p);
  }
  if (auto const* p = std::get_if<double>(&v.storage)) {
    return *p;
  }
  return std::nullopt;
}

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

class expr_parser {
public:
  expr_parser(std::string_view text, std::vector<template_object> const& scopes)
    : text_(text), scopes_(scopes) {}

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

  auto skip_space() -> void {
    while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_])) != 0) {
      ++pos_;
    }
  }

  [[nodiscard]] auto consume(std::string_view token) -> bool {
    skip_space();
    if (text_.substr(pos_, token.size()) == token) {
      auto const next = pos_ + token.size();
      if (!token.empty() && std::isalpha(static_cast<unsigned char>(token.back())) != 0) {
        if (next < text_.size()) {
          auto const c = text_[next];
          if ((std::isalnum(static_cast<unsigned char>(c)) != 0) || c == '_') {
            return false;
          }
        }
      }
      pos_ = next;
      return true;
    }
    return false;
  }

  [[nodiscard]] auto parse_or(bool evaluate) -> template_value {
    auto lhs = parse_and(evaluate);
    while (consume("or")) {
      auto const lhs_truth = evaluate && template_truthy(lhs);
      auto rhs = parse_and(evaluate && !lhs_truth);
      if (evaluate) {
        lhs = template_value{lhs_truth || template_truthy(rhs)};
      }
    }
    return lhs;
  }

  [[nodiscard]] auto parse_and(bool evaluate) -> template_value {
    auto lhs = parse_in(evaluate);
    while (consume("and")) {
      auto const lhs_truth = evaluate && template_truthy(lhs);
      auto rhs = parse_in(evaluate && lhs_truth);
      if (evaluate) {
        lhs = template_value{lhs_truth && template_truthy(rhs)};
      }
    }
    return lhs;
  }

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

  [[nodiscard]] auto parse_mul(bool evaluate) -> template_value {
    auto lhs = parse_unary(evaluate);
    while (true) {
      if (consume("*")) {
        auto rhs = parse_unary(evaluate);
        if (evaluate) {
          auto const lnum = try_as_double(lhs);
          auto const rnum = try_as_double(rhs);
          if (!lnum.has_value() || !rnum.has_value()) {
            throw template_render_error{"invalid operands for *"};
          }
          lhs = template_value{*lnum * *rnum};
        }
      } else if (consume("/")) {
        auto rhs = parse_unary(evaluate);
        if (evaluate) {
          auto const lnum = try_as_double(lhs);
          auto const rnum = try_as_double(rhs);
          if (!lnum.has_value() || !rnum.has_value()) {
            throw template_render_error{"invalid operands for /"};
          }
          lhs = template_value{*lnum / *rnum};
        }
      } else if (consume("%")) {
        auto rhs = parse_unary(evaluate);
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
      auto const num = try_as_double(v);
      if (!num.has_value()) {
        throw template_render_error{"invalid operand for unary -"};
      }
      return template_value{-*num};
    }
    return parse_primary(evaluate);
  }

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
    return resolve_name(text_.substr(start, pos_ - start), evaluate);
  }
};

struct for_header {
  std::string key_name{};
  std::string value_name{};
  std::string expr{};
  bool has_key{};
};

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

template <auto Src>
auto render_program(template_object const& root) -> std::string {
  auto constexpr program = parse_program<Src>();
  auto const src = Src.sv();
  auto out = std::string{};
  auto scopes = std::vector<template_object>{root};

  auto render_range = [&](this auto&& self, std::size_t begin, std::size_t end) -> void {
    auto i = begin;
    while (i < end) {
      auto const& node = program.nodes[i];
      switch (node.kind) {
        case template_node_kind::text: {
          out.append(src.substr(node.begin, node.end - node.begin));
          ++i;
          break;
        }
        case template_node_kind::expr: {
          auto const expr = trim_view(src.substr(node.begin, node.end - node.begin));
          auto value = expr_parser{expr, scopes}.parse();
          out.append(template_to_string(value));
          ++i;
          break;
        }
        case template_node_kind::if_stmt: {
          auto const stmt = trim_view(src.substr(node.begin, node.end - node.begin));
          auto const cond_expr = trim_view(stmt.substr(2));
          auto const cond = expr_parser{cond_expr, scopes}.parse();
          auto const then_end = (node.else_index != std::numeric_limits<std::size_t>::max()) ? node.else_index : node.end_index;
          if (template_truthy(cond)) {
            self(i + 1, then_end);
          } else if (node.else_index != std::numeric_limits<std::size_t>::max()) {
            self(node.else_index + 1, node.end_index);
          }
          i = node.end_index + 1;
          break;
        }
        case template_node_kind::for_stmt: {
          auto const stmt = trim_view(src.substr(node.begin, node.end - node.begin));
          auto const header = parse_for_header(stmt);
          auto iterable = expr_parser{header.expr, scopes}.parse();
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

template <auto Src>
auto render_template(template_value const& root) -> std::string {
  if (!std::holds_alternative<template_object>(root.storage)) {
    throw template_render_error{"root context must be object"};
  }
  return detail::render_program<Src>(std::get<template_object>(root.storage));
}

template <auto Src>
struct template_vm {
  static auto render(template_value const& root) -> std::string {
    return render_template<Src>(root);
  }
};

} // namespace frozenchars
