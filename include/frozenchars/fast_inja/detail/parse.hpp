#pragma once

#include "chunk.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace frozenchars::fast_inja::detail {

/// @brief 前後空白をトリムした string_view を返す
[[nodiscard]] inline std::string_view trim_sv(std::string_view s) noexcept {
  while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) {
    s.remove_prefix(1);
  }
  while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) {
    s.remove_suffix(1);
  }
  return s;
}

/// @brief @変数名から kind を判定する
[[nodiscard]] inline chunk_at_var::kind parse_at_kind(std::string_view key) noexcept {
  if (key == "@last") {
    return chunk_at_var::kind::last;
  }
  if (key == "@first") {
    return chunk_at_var::kind::first;
  }
  if (key == "@index") {
    return chunk_at_var::kind::index;
  }
  return chunk_at_var::kind::root;
}

/// @brief if ボディ内のトップレベル {{else}} の位置を検索する
/// @details ネストされたセクション/if 内の else は無視する
/// @return {{else}} の開始位置、なければ npos
[[nodiscard]] inline std::size_t find_toplevel_else(std::string_view body) noexcept {
  std::size_t pos = 0;
  int depth = 0;
  while (pos < body.size()) {
    auto tag_pos = body.find("{{", pos);
    if (tag_pos == std::string_view::npos) {
      break;
    }
    auto end = body.find("}}", tag_pos + 2);
    if (end == std::string_view::npos) {
      break;
    }
    auto tag_inner = trim_sv(body.substr(tag_pos + 2, end - tag_pos - 2));
    if (tag_inner.starts_with("#")) {
      ++depth;
    } else if (tag_inner.starts_with("/")) {
      if (depth > 0) {
        --depth;
      }
    } else if (tag_inner == "else" && depth == 0) {
      return tag_pos;
    }
    pos = end + 2;
  }
  return std::string_view::npos;
}

/// @brief テンプレート文字列をチャンク列にパースする
/// @param tmpl テンプレート文字列
/// @return チャンクのベクター
[[nodiscard]] inline auto parse(std::string_view tmpl) -> std::vector<chunk> {
  std::vector<chunk> result;
  std::size_t pos = 0;

  while (pos < tmpl.size()) {
    auto tag_start = tmpl.find("{{", pos);
    if (tag_start == std::string_view::npos) {
      result.push_back(chunk_literal{std::string{tmpl.substr(pos)}});
      break;
    }

    if (tag_start > pos) {
      result.push_back(chunk_literal{std::string{tmpl.substr(pos, tag_start - pos)}});
    }

    // {{{ rawプレースホルダー
    if (tag_start + 2 < tmpl.size() && tmpl[tag_start + 2] == '{') {
      auto end = tmpl.find("}}}", tag_start + 3);
      if (end == std::string_view::npos) {
        result.push_back(chunk_literal{std::string{tmpl.substr(tag_start, 1)}});
        pos = tag_start + 1;
        continue;
      }
      auto key = trim_sv(tmpl.substr(tag_start + 3, end - tag_start - 3));
      result.push_back(chunk_placeholder{std::string{key}, true});
      pos = end + 3;
      continue;
    }

    auto tag_end = tmpl.find("}}", tag_start + 2);
    if (tag_end == std::string_view::npos) {
      result.push_back(chunk_literal{std::string{tmpl.substr(tag_start, 1)}});
      pos = tag_start + 1;
      continue;
    }

    auto inner = trim_sv(tmpl.substr(tag_start + 2, tag_end - tag_start - 2));
    pos = tag_end + 2;

    if (inner.empty()) {
      continue;
    }

    if (inner.starts_with("/")) {
      continue;
    }

    // # で始まる（セクション or if）
    if (inner.starts_with("#")) {
      auto key = trim_sv(inner.substr(1));

      // {{#if X}} — if ブロック
      if (key.starts_with("if") && (key.size() == 2 || key[2] == ' ')) {
        auto expr = key.size() > 2 ? trim_sv(key.substr(3)) : std::string_view{};

        int depth = 1;
        std::size_t search_pos = pos;
        std::size_t close_pos = std::string_view::npos;

        while (search_pos < tmpl.size()) {
          auto next_open = tmpl.find("{{#if", search_pos);
          auto next_close = tmpl.find("{{/if}}", search_pos);
          if (next_close == std::string_view::npos) {
            break;
          }
          if (next_open != std::string_view::npos && next_open < next_close) {
            ++depth;
            search_pos = next_open + 5;
          } else {
            --depth;
            if (depth == 0) {
              close_pos = next_close;
              break;
            }
            search_pos = next_close + 7;
          }
        }

        std::string_view body;
        if (close_pos != std::string_view::npos) {
          body = tmpl.substr(pos, close_pos - pos);
          pos = close_pos + 7;
        } else {
          body = tmpl.substr(pos);
          pos = tmpl.size();
        }

        auto else_pos = find_toplevel_else(body);
        std::string_view then_body, else_body;
        if (else_pos != std::string_view::npos) {
          then_body = body.substr(0, else_pos);
          auto else_tag_end = body.find("}}", else_pos + 2);
          else_body = (else_tag_end != std::string_view::npos) ? body.substr(else_tag_end + 2) : std::string_view{};
        } else {
          then_body = body;
          else_body = {};
        }

        chunk_if ci;
        ci.expr = std::string{expr};
        ci.then_branch = wrap_body_chunks(parse(then_body));
        ci.else_branch = wrap_body_chunks(parse(else_body));
        result.push_back(std::move(ci));
        continue;
      }

      // {{#@var}} — @var セクション
      if (key.starts_with("@")) {
        auto var_kind = parse_at_kind(key);
        auto close_tag_str = std::string{"{{/"} + std::string{key} + "}}";
        auto body_start = pos;
        auto close_pos = tmpl.find(close_tag_str, pos);
        if (close_pos != std::string_view::npos) {
          auto body = tmpl.substr(body_start, close_pos - body_start);
          pos = close_pos + close_tag_str.size();
          chunk_at_section cas;
          cas.var = var_kind;
          cas.body = wrap_body_chunks(parse(body));
          cas.inverted = false;
          result.push_back(std::move(cas));
        }
        continue;
      }

      // {{#key}} — 通常のセクション
      {
        auto close_tag_str = std::string{"{{/"} + std::string{key} + "}}";
        auto open_tag_str = std::string{"{{#"} + std::string{key} + "}}";
        auto body_start = pos;

        int depth2 = 1;
        std::size_t search = pos;
        std::size_t close_pos = std::string_view::npos;

        while (search < tmpl.size()) {
          auto next_open = tmpl.find(open_tag_str, search);
          auto next_close = tmpl.find(close_tag_str, search);
          if (next_close == std::string_view::npos) {
            break;
          }
          if (next_open != std::string_view::npos && next_open < next_close) {
            ++depth2;
            search = next_open + open_tag_str.size();
          } else {
            --depth2;
            if (depth2 == 0) {
              close_pos = next_close;
              break;
            }
            search = next_close + close_tag_str.size();
          }
        }

        if (close_pos != std::string_view::npos) {
          auto body = tmpl.substr(body_start, close_pos - body_start);
          pos = close_pos + close_tag_str.size();
          chunk_section cs;
          cs.key = std::string{key};
          cs.body = wrap_body_chunks(parse(body));
          result.push_back(std::move(cs));
        } else {
          auto body = tmpl.substr(body_start);
          pos = tmpl.size();
          chunk_section cs;
          cs.key = std::string{key};
          cs.body = wrap_body_chunks(parse(body));
          result.push_back(std::move(cs));
        }
      }
      continue;
    }

    // ^ で始まる逆セクション
    if (inner.starts_with("^")) {
      auto key = trim_sv(inner.substr(1));

      // {{^@var}} — @var 逆セクション
      if (key.starts_with("@")) {
        auto var_kind = parse_at_kind(key);
        auto close_tag_str = std::string{"{{/"} + std::string{key} + "}}";
        auto body_start = pos;
        auto close_pos = tmpl.find(close_tag_str, pos);
        if (close_pos != std::string_view::npos) {
          auto body = tmpl.substr(body_start, close_pos - body_start);
          pos = close_pos + close_tag_str.size();
          chunk_at_section cas;
          cas.var = var_kind;
          cas.body = wrap_body_chunks(parse(body));
          cas.inverted = true;
          result.push_back(std::move(cas));
        }
        continue;
      }

      // {{^key}} — 逆セクション
      {
        auto close_tag_str = std::string{"{{/"} + std::string{key} + "}}";
        auto open_tag_str = std::string{"{{^"} + std::string{key} + "}}";
        auto body_start = pos;

        int depth2 = 1;
        std::size_t search = pos;
        std::size_t close_pos = std::string_view::npos;

        while (search < tmpl.size()) {
          auto next_open = tmpl.find(open_tag_str, search);
          auto next_close = tmpl.find(close_tag_str, search);
          if (next_close == std::string_view::npos) {
            break;
          }
          if (next_open != std::string_view::npos && next_open < next_close) {
            ++depth2;
            search = next_open + open_tag_str.size();
          } else {
            --depth2;
            if (depth2 == 0) {
              close_pos = next_close;
              break;
            }
            search = next_close + close_tag_str.size();
          }
        }

        std::string_view body;
        if (close_pos != std::string_view::npos) {
          body = tmpl.substr(body_start, close_pos - body_start);
          pos = close_pos + close_tag_str.size();
        } else {
          body = tmpl.substr(body_start);
          pos = tmpl.size();
        }
        chunk_inverted ci;
        ci.key = std::string{key};
        ci.body = wrap_body_chunks(parse(body));
        result.push_back(std::move(ci));
      }
      continue;
    }

    // @ で始まる @vars
    if (inner.starts_with("@")) {
      result.push_back(chunk_at_var{parse_at_kind(inner)});
      continue;
    }

    // 通常のプレースホルダー
    result.push_back(chunk_placeholder{std::string{inner}, false});
  }

  return result;
}

} // namespace frozenchars::fast_inja::detail
