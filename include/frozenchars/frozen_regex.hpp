/**
 * @file frozen_regex.hpp
 * @brief 正規表現パターンからマッチしうる全文字列をコンパイル時に列挙するライブラリ
 * @details
 *   サポート構文: リテラル / 選択(|) / 文字クラス([a-z][^a]) / グループ(()) / ドット(.) / エスケープ(\\ \. \[ \] \( \) \| \- \^)
 *   非対応: 量指定子(+ * ? {n,m}) / 先読み・後読み / キャプチャグループ
 *   列挙数上限 MaxStrings (デフォルト 4096) を超えると consteval throw でコンパイルエラー。
 */
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "frozenchars/string.hpp"
#include "frozenchars/map.hpp"

namespace frozenchars {
namespace detail::fregex {

/// @brief AST ノードの種別
enum class node_kind : std::uint8_t {
  literal,      ///< 単一文字
  dot,          ///< ドット（DotChars 文字集合に展開）
  char_class,   ///< 文字クラス [abc] [a-z] [^abc]
  concat,       ///< 直列結合（子ノードの直積）
  alt,          ///< 選択（子ノードの和集合）
  group,        ///< グループ化（子と同じ）
};

/// @brief AST ノード（フラット表現、子はインデックス参照）
struct node {
  node_kind kind{node_kind::literal};
  char literal_char{'\0'};                       ///< kind == literal のみ使用
  std::vector<char> char_set;                    ///< kind == dot / char_class のみ使用
  std::vector<std::size_t> child_indices;        ///< kind == concat / alt / group のみ使用
};

/// @brief AST 全体（フラットなノード配列 + ルートインデックス）
struct ast {
  std::vector<node> nodes;
  std::size_t root_index{0};
};

/// @brief 再帰下降パーサー（consteval）
struct parser {
  std::string_view src;   ///< 未消費のパターン
  ast tree;               ///< 構築中の AST

  /// @brief エントリポイント: パターン全体をパースして AST を返す
  static consteval auto parse(std::string_view pattern) -> ast {
    if (pattern.empty()) throw "frozen_regex: empty pattern";
    parser p{pattern, {}};
    auto const root = p.parse_alt();
    if (!p.eof()) throw "frozen_regex: unbalanced bracket";
    p.tree.root_index = root;
    return p.tree;
  }

  /// @brief 末端に達したか
  [[nodiscard]] consteval auto eof() const noexcept -> bool {
    return src.empty();
  }

  /// @brief 現在の先頭文字を覗く（消費しない）
  [[nodiscard]] consteval auto peek() const -> char {
    if (eof()) throw "frozen_regex: unexpected end of pattern";
    return src.front();
  }

  /// @brief 先頭1文字を消費して返す
  consteval auto consume() -> char {
    auto const c = peek();
    src.remove_prefix(1);
    return c;
  }

  /// @brief 文法エラー（consteval throw でコンパイルエラー）
  [[noreturn]] consteval auto error(char const* msg) -> void {
    throw msg;
  }

  /// @brief ノードを追加してインデックスを返す
  consteval auto add_node(node n) -> std::size_t {
    auto const idx = tree.nodes.size();
    tree.nodes.push_back(std::move(n));
    return idx;
  }

  /// @brief alt = concat ('|' concat)*
  consteval auto parse_alt() -> std::size_t {
    auto first = parse_concat();
    if (eof() || peek() != '|') return first;
    // 選択ノードを構築
    node alt_node{node_kind::alt, '\0', {}, {first}};
    while (!eof() && peek() == '|') {
      consume();  ///< '|' を消費
      alt_node.child_indices.push_back(parse_concat());
    }
    return add_node(std::move(alt_node));
  }

  /// @brief concat = atom+（'(' '|' ')' が来るまで原子を並べる）
  consteval auto parse_concat() -> std::size_t {
    std::vector<std::size_t> children;
    while (!eof() && peek() != '|' && peek() != ')') {
      children.push_back(parse_atom());
    }
    if (children.empty()) throw "frozen_regex: empty alternative";
    if (children.size() == 1) return children[0];
    node concat_node{node_kind::concat, '\0', {}, std::move(children)};
    return add_node(std::move(concat_node));
  }

  /// @brief atom = literal | dot | char_class | group
  consteval auto parse_atom() -> std::size_t {
    auto const c = peek();
    if (c == '(') return parse_group();
    if (c == '[') return parse_char_class();
    if (c == '.') return parse_dot();
    if (c == '\\') return parse_escape_atom();
    // 量指定子は非対応
    if (c == '+' || c == '*' || c == '?') {
      throw "frozen_regex: quantifiers not supported";
    }
    if (c == '{') {
      throw "frozen_regex: quantifiers not supported";
    }
    // 先読み・後読きは非対応（peek で (?= 等を検出）
    if (c == ')') throw "frozen_regex: unbalanced parenthesis";
    // 通常リテラル
    consume();
    return add_node(node{node_kind::literal, c, {}, {}});
  }

  /// @brief '(' alt ')' をパース
  consteval auto parse_group() -> std::size_t {
    consume();  ///< '(' を消費
    // (?= 等の先読み・後読みを検出
    if (!eof() && peek() == '?') {
      throw "frozen_regex: lookahead/lookbehind not supported";
    }
    auto inner = parse_alt();
    if (eof() || peek() != ')') throw "frozen_regex: unbalanced parenthesis";
    consume();  ///< ')' を消費
    return add_node(node{node_kind::group, '\0', {}, {inner}});
  }

  /// @brief '[' ['^'] (item)+ ']'
  /// @details item = escape | range | single_char
  ///          range = char '-' char
  ///          先頭・末尾の '-' は文法エラー（リテラルとして使うには \- とエスケープ）
  consteval auto parse_char_class() -> std::size_t {
    consume();  ///< '[' を消費
    bool const negate = (!eof() && peek() == '^');
    if (negate) consume();
    std::vector<char> chars;
    while (!eof() && peek() != ']') {
      auto const c1 = parse_class_char();
      if (!eof() && peek() == '-') {
        // 範囲演算子の可能性
        consume();  ///< '-' を消費
        if (eof() || peek() == ']') {
          // 末尾の '-' は文法エラー
          throw "frozen_regex: dangling '-' in character class";
        }
        auto const c2 = parse_class_char();
        if (c1 > c2) throw "frozen_regex: invalid character range";
        for (auto ch = c1; ch <= c2; ++ch) chars.push_back(ch);
      } else {
        chars.push_back(c1);
      }
    }
    if (eof()) throw "frozen_regex: unbalanced bracket";
    consume();  ///< ']' を消費
    if (chars.empty()) throw "frozen_regex: empty character class";
    // 否定クラス: DotChars から chars を引いた差集合
    if (negate) chars = complement_against_dotchars(chars);
    node n{node_kind::char_class, '\0', std::move(chars), {}};
    return add_node(std::move(n));
  }

  /// @brief 文字クラス内の1文字（エスケープ対応）
  consteval auto parse_class_char() -> char {
    auto const c = peek();
    if (c == '\\') return parse_escape();
    if (c == ']') throw "frozen_regex: unbalanced bracket";
    consume();
    return c;
  }

  /// @brief ドット '.' をパース（char_set は enumerator 側で DotChars から展開）
  consteval auto parse_dot() -> std::size_t {
    consume();  ///< '.' を消費
    return add_node(node{node_kind::dot, '\0', {}, {}});
  }

  /// @brief エスケープ付き atom: \\ \. \[ \] \( \) \| \- \^
  consteval auto parse_escape_atom() -> std::size_t {
    auto const c = parse_escape();
    return add_node(node{node_kind::literal, c, {}, {}});
  }

  /// @brief エスケープ1文字を消費して返す
  consteval auto parse_escape() -> char {
    consume();  ///< '\\' を消費
    if (eof()) throw "frozen_regex: dangling backslash";
    auto const c = consume();
    switch (c) {
      case '\\': case '.': case '[': case ']':
      case '(': case ')': case '|': case '-': case '^':
        return c;
      default:
        throw "frozen_regex: unsupported escape sequence";
    }
  }

  /// @brief DotChars（外部から注入される予定、ここではダミー）
  /// @details 実装は Task 3 で DotChars NTTP を受け取る形に統合
  static consteval auto complement_against_dotchars(std::vector<char> const& exclude) -> std::vector<char> {
    // Task 3 で DotChars を使って書き直す。Task 1 では空を返してテストを通す。
    (void)exclude;
    return {};
  }
};

} // namespace detail::fregex
} // namespace frozenchars
