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
#include <cstdint>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "frozenchars/string.hpp"
#include "frozenchars/map.hpp"
#include "frozenchars/set.hpp"

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
  std::vector<char> char_set;                    ///< char_class: 正クラスの場合のみ設定（否定は enumerator で解決）
  bool negate_class{false};                      ///< char_class の否定フラグ
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
    auto const first = parse_concat();
    if (eof() || peek() != '|') return first;
    // 選択ノードを構築
    node alt_node{node_kind::alt, '\0', {}, false, {first}};
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
    node concat_node{node_kind::concat, '\0', {}, false, std::move(children)};
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
    return add_node(node{node_kind::literal, c, {}, false, {}});
  }

  /// @brief '(' alt ')' をパース
  consteval auto parse_group() -> std::size_t {
    consume();  ///< '(' を消費
    // (?= 等の先読み・後読みを検出
    if (!eof() && peek() == '?') {
      throw "frozen_regex: lookahead/lookbehind not supported";
    }
    auto const inner = parse_alt();
    if (eof() || peek() != ')') throw "frozen_regex: unbalanced parenthesis";
    consume();  ///< ')' を消費
    return add_node(node{node_kind::group, '\0', {}, false, {inner}});
  }

  /// @brief '[' ['^'] (item)+ ']'
  /// @details item = escape | range | single_char
  ///          range = char '-' char
  ///          末尾の '-' は文法エラー（リテラルとして使うには \- とエスケープ）。先頭の '-' は通常文字として扱う（POSIX/PCRE 互換）
  consteval auto parse_char_class() -> std::size_t {
    consume();  ///< '[' を消費
    bool const negate = (!eof() && peek() == '^');
    if (negate) consume();
    std::vector<char> chars;
    while (!eof() && peek() != ']') {
      auto const c1 = parse_class_char();
      if (!eof() && peek() == '-') {
        consume();
        if (eof() || peek() == ']') throw "frozen_regex: dangling '-' in character class";
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
    node n{node_kind::char_class, '\0', std::move(chars), negate, {}};
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
    return add_node(node{node_kind::dot, '\0', {}, false, {}});
  }

  /// @brief エスケープ付き atom: \\ \. \[ \] \( \) \| \- \^
  consteval auto parse_escape_atom() -> std::size_t {
    auto const c = parse_escape();
    return add_node(node{node_kind::literal, c, {}, false, {}});
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

  };

/// @brief 列挙結果（中間表現、動的サイズ）
struct enumerate_result {
  std::vector<std::string> strings;
  std::size_t max_length{0};
};

/// @brief AST を辿って全文字列を列挙
/// @tparam MaxStrings 列挙上限（超過で consteval throw）
template <std::size_t MaxStrings>
struct enumerator {
  ast const& tree;
  std::string_view dot_chars;  ///< ドットがマッチする文字集合

  /// @brief エントリポイント
  static consteval auto run(ast const& t, std::string_view dot) -> enumerate_result {
    enumerator e{t, dot};
    auto strs = e.enumerate_node(t.root_index);
    return finalize(std::move(strs));
  }

  /// @brief 指定ノードの列挙結果を返す
  consteval auto enumerate_node(std::size_t idx) -> std::vector<std::string> {
    auto const& n = tree.nodes[idx];
    switch (n.kind) {
      case node_kind::literal:
        return {std::string{static_cast<char>(n.literal_char)}};
      case node_kind::dot: {
        std::vector<std::string> res;
        res.reserve(dot_chars.size());
        for (auto c : dot_chars) res.push_back(std::string{c});
        return res;
      }
      case node_kind::char_class: {
        std::vector<char> chars = n.char_set;
        if (n.negate_class) chars = complement(chars);
        std::vector<std::string> res;
        res.reserve(chars.size());
        for (auto c : chars) res.push_back(std::string{c});
        return res;
      }
      case node_kind::group: {
        return enumerate_node(n.child_indices[0]);
      }
      case node_kind::concat: {
        std::vector<std::vector<std::string>> child_results;
        child_results.reserve(n.child_indices.size());
        for (auto ci : n.child_indices) child_results.push_back(enumerate_node(ci));
        return cartesian_concat(std::move(child_results));
      }
      case node_kind::alt: {
        std::vector<std::vector<std::string>> child_results;
        child_results.reserve(n.child_indices.size());
        for (auto ci : n.child_indices) child_results.push_back(enumerate_node(ci));
        return merge_alt(std::move(child_results));
      }
    }
    throw "frozen_regex: unreachable";
  }

  /// @brief CONCAT の直積: 単位元 "" から各子の文字列を順に結合
  consteval auto cartesian_concat(std::vector<std::vector<std::string>> children)
    -> std::vector<std::string> {
    std::vector<std::string> acc{""};
    for (auto& child : children) {
      std::vector<std::string> next;
      next.reserve(acc.size() * child.size());
      for (auto const& prefix : acc) {
        for (auto const& suffix : child) {
          next.push_back(prefix + suffix);
          check_overflow(next.size());
        }
      }
      acc = std::move(next);
    }
    return acc;
  }

  /// @brief ALT の和集合（重複除去は finalize で実施）
  consteval auto merge_alt(std::vector<std::vector<std::string>> children)
    -> std::vector<std::string> {
    std::vector<std::string> res;
    for (auto& child : children) {
      for (auto& s : child) {
        res.push_back(std::move(s));
        check_overflow(res.size());
      }
    }
    return res;
  }

  /// @brief 列挙数が MaxStrings を超えたら throw
  consteval auto check_overflow(std::size_t current) const -> void {
    if (current > MaxStrings) throw "frozen_regex: enumeration exceeds MaxStrings";
  }

  /// @brief 否定クラスの差集合: dot_chars から exclude を引く
  [[nodiscard]] consteval auto complement(std::vector<char> const& exclude) const
    -> std::vector<char> {
    std::vector<char> res;
    for (auto c : dot_chars) {
      if (std::find(exclude.begin(), exclude.end(), c) == exclude.end()) {
        res.push_back(c);
      }
    }
    return res;
  }

  /// @brief sort + dedup + max_length 計算
  static consteval auto finalize(std::vector<std::string> strs) -> enumerate_result {
    std::ranges::sort(strs);
    strs.erase(std::unique(strs.begin(), strs.end()), strs.end());
    if (strs.size() > MaxStrings) throw "frozen_regex: enumeration exceeds MaxStrings";
    std::size_t maxlen = 0;
    for (auto const& s : strs) maxlen = std::max(maxlen, s.size());
    return enumerate_result{std::move(strs), maxlen};
  }
};

/// @brief パース＋列挙＋finalize してメタデータだけ返す（static constexpr に格納可能）
struct regex_metadata {
  std::size_t max_length;
  std::size_t count;
};

/// @brief 正規表現をパースして列挙し、メタデータのみを返す
template <std::size_t MaxStrings>
[[nodiscard]] consteval auto compute_metadata(std::string_view pattern, std::string_view dot_chars)
  -> regex_metadata {
  auto const tree = parser::parse(pattern);
  auto result = enumerator<MaxStrings>::run(tree, dot_chars);
  return {result.max_length, result.strings.size()};
}

/// @brief 正規表現をパースして列挙し、FrozenString 配列を返す（vector フリー）
template <std::size_t MaxStrings, std::size_t N, std::size_t Count>
[[nodiscard]] consteval auto compute_keys(std::string_view pattern, std::string_view dot_chars)
  -> std::array<FrozenString<N>, Count> {
  auto const tree = parser::parse(pattern);
  auto result = enumerator<MaxStrings>::run(tree, dot_chars);
  std::array<FrozenString<N>, Count> arr{};
  for (auto i = 0uz; i < Count; ++i) {
    auto const& s = result.strings[i];
    for (auto j = 0uz; j < s.size(); ++j) arr[i].buffer[j] = s[j];
    arr[i].buffer[s.size()] = '\0';
    arr[i].length = s.size();
  }
  return arr;
}

} // namespace detail::fregex

/// @brief デフォルトのドット文字集合（`.` がマッチする文字）
static constexpr char default_dot_chars[] =
  "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";

/// @brief 正規表現パターンからマッチしうる全文字列をコンパイル時に列挙する
/// @tparam Pattern 正規表現パターン（FrozenString）
/// @tparam MaxStrings 列挙上限（デフォルト 4096）
/// @tparam DotChars ドットがマッチする文字集合
template <FrozenString Pattern, std::size_t MaxStrings = 4096,
          FrozenString DotChars = FrozenString<sizeof(default_dot_chars)>{default_dot_chars}>
struct frozen_regex {
  static constexpr auto meta_ =
    detail::fregex::compute_metadata<MaxStrings>(Pattern.sv(), DotChars.sv());
  static constexpr std::size_t max_length_v = meta_.max_length;
  static constexpr std::size_t count_v = meta_.count;

  static constexpr auto enumerated_keys_ =
    detail::fregex::compute_keys<MaxStrings, max_length_v + 1, count_v>(
      Pattern.sv(), DotChars.sv());

  /// @brief ソート済み string_view 配列（contains / keys 用）
  static constexpr auto key_views_ = [] {
    std::array<std::string_view, count_v> arr{};
    for (auto i = 0uz; i < count_v; ++i) arr[i] = enumerated_keys_[i].sv();
    return arr;
  }();

  /// @brief 指定文字列がパターンにマッチするかを二分探索で判定
  [[nodiscard]] static constexpr auto contains(std::string_view s) noexcept -> bool {
    return std::ranges::binary_search(key_views_, s);
  }

  /// @brief 列挙された全文字列を FrozenString 配列として取得
  [[nodiscard]] static constexpr auto enumerate() noexcept
    -> std::span<FrozenString<max_length_v + 1> const, count_v> {
    return enumerated_keys_;
  }

  /// @brief ソート済み文字列ビューの配列を取得
  [[nodiscard]] static constexpr auto keys() noexcept
    -> std::span<std::string_view const, count_v> {
    return key_views_;
  }

  /// @brief 列挙キーをキーとする frozen_map を生成
  template <typename V, V... Values>
  static consteval auto to_frozen_map() {
    static_assert(sizeof...(Values) == count_v,
                  "to_frozen_map requires exactly count_v values matching sorted key order");
    return to_frozen_map_impl<V, Values...>(std::make_index_sequence<count_v>{});
  }

  /// @brief 列挙キーをキーとする frozen_set を生成
  [[nodiscard]] static consteval auto to_frozen_set() {
    return to_frozen_set_impl(std::make_index_sequence<count_v>{});
  }

  /// @brief 列挙キーをキー、ラムダの戻り値を値とする frozen_map を生成
  /// @tparam MapFn 各キー string_view から値を計算する constexpr ラムダ（キャプチャ不可）
  template <auto MapFn>
  [[nodiscard]] static consteval auto regex_map() {
    return regex_map_impl<MapFn>(std::make_index_sequence<count_v>{});
  }

private:
  template <typename V, V... Values, std::size_t... Is>
  static consteval auto to_frozen_map_impl(std::index_sequence<Is...>)
    -> frozen_map<V, enumerated_keys_[Is]...> {
    return frozen_map<V, enumerated_keys_[Is]...>{
      std::array<V, count_v>{ Values... }
    };
  }

  template <std::size_t... Is>
  static consteval auto to_frozen_set_impl(std::index_sequence<Is...>)
    -> frozen_set<enumerated_keys_[Is]...> {
    return {};
  }

  template <auto MapFn, std::size_t... Is>
  static consteval auto regex_map_impl(std::index_sequence<Is...>)
    -> frozen_map<decltype(MapFn(enumerated_keys_[0].sv())), enumerated_keys_[Is]...> {
    using V = decltype(MapFn(enumerated_keys_[0].sv()));
    return frozen_map<V, enumerated_keys_[Is]...>{
      std::array<V, count_v>{ MapFn(enumerated_keys_[Is].sv())... }
    };
  }
};

} // namespace frozenchars
