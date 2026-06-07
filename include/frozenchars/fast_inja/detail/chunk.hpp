#pragma once

#include <string>
#include <variant>
#include <vector>

namespace frozenchars::fast_inja {

// parsed_template の前方宣言（chunk_section::body で使用）
struct parsed_template;

namespace detail {

/// @brief リテラルテキストチャンク
struct chunk_literal {
  std::string text;
};

/// @brief プレースホルダーチャンク（{{ key }} または {{{ key }}}）
struct chunk_placeholder {
  std::string key;
  bool raw = false; ///< true の場合 HTML エスケープなし
};

/// @brief セクションチャンク（{{#key}}...{{/key}}）
struct chunk_section {
  std::string key;
  std::vector<parsed_template> body;
};

/// @brief 逆セクションチャンク（{{^key}}...{{/key}}）
struct chunk_inverted {
  std::string key;
  std::vector<parsed_template> body;
};

/// @brief @変数チャンク（{{@last}} など）
struct chunk_at_var {
  enum class kind { index, first, last, root };
  kind var;
};

/// @brief @変数セクションチャンク（{{#@last}}...{{/@last}}）
struct chunk_at_section {
  chunk_at_var::kind var = chunk_at_var::kind::root;
  std::vector<parsed_template> body;
  bool inverted = false;
};

/// @brief if/else チャンク（{{#if X}}...{{else}}...{{/if}}）
struct chunk_if {
  std::string expr;
  std::vector<parsed_template> then_branch;
  std::vector<parsed_template> else_branch;
};

/// @brief すべてのチャンク型のバリアント
using chunk = std::variant<
    chunk_literal,
    chunk_placeholder,
    chunk_section,
    chunk_inverted,
    chunk_at_var,
    chunk_at_section,
    chunk_if>;

/// @brief チャンクリストを各1チャンクの parsed_template にラップする
[[nodiscard]] auto wrap_body_chunks(std::vector<chunk> chunks) -> std::vector<parsed_template>;

} // namespace detail

/// @brief パース済みテンプレート
struct parsed_template {
  std::vector<detail::chunk> chunks;
};

namespace detail {

inline auto wrap_body_chunks(std::vector<chunk> chunks) -> std::vector<parsed_template> {
  std::vector<parsed_template> result;
  result.reserve(chunks.size());
  for (auto& c : chunks) {
    parsed_template pt;
    pt.chunks.push_back(std::move(c));
    result.push_back(std::move(pt));
  }
  return result;
}

} // namespace detail

} // namespace frozenchars::fast_inja
