#pragma once

#include "string.hpp"
#include "frozen_regex.hpp"

#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace frozenchars {

namespace detail {

/**
 * @brief 再帰的マッチングの中間結果
 */
struct match_result {
  bool matched;
  size_t pos;
};

/**
 * @brief '[' に対応する ']' の位置を探す
 *
 * @tparam PAT パターン文字列（FrozenString NTTP）
 * @param pos '[' の位置
 * @return size_t ']' の位置（見つからない場合は PAT.size()）
 */
template <FrozenString PAT>
[[nodiscard]] constexpr size_t find_matching_close_bracket(size_t pos) noexcept {
  auto i = pos + 1uz;
  while (i < PAT.size() && PAT.data()[i] != ']') {
    if (PAT.data()[i] == '\\') ++i;
    ++i;
  }
  return i;
}

/**
 * @brief '(' に対応する ')' の位置を探す（ネスト対応）
 */
template <FrozenString PAT>
[[nodiscard]] constexpr size_t find_matching_close_paren(size_t pos) noexcept {
  auto i = pos + 1uz;
  auto depth = 1;
  while (i < PAT.size() && depth > 0) {
    if (PAT.data()[i] == '\\') { i += 2; continue; }
    if (PAT.data()[i] == '(') ++depth;
    else if (PAT.data()[i] == ')') --depth;
    ++i;
  }
  return (depth == 0) ? i - 1 : PAT.size();
}

/**
 * @brief 文字集合用の 256 エントリ事前計算テーブル
 */
template <FrozenString PAT>
struct wildcard_char_class_table {
  std::array<bool, 256> entries{};

  [[nodiscard]] constexpr bool operator[](unsigned char ch) const noexcept {
    return entries[ch];
  }
};

/**
 * @brief wildcard 一般 matcher 用の事前計算メタデータ
 */
template <FrozenString PAT>
struct wildcard_plan {
private:
  struct metadata {
    std::array<size_t, PAT.size()> close_brackets{};
    std::array<size_t, PAT.size()> close_parens{};
    std::array<wildcard_char_class_table<PAT>, PAT.size()> char_class_tables{};
    std::array<size_t, PAT.size()> first_branch{};
    std::array<size_t, PAT.size()> branch_end{};
    std::array<size_t, PAT.size()> next_branch{};
    size_t fixed_prefix_length = 0;
  };

  [[nodiscard]] static consteval auto read_char_class_token(size_t i, size_t close) noexcept -> std::pair<unsigned char, size_t> {
    if (PAT.data()[i] == '\\' && i + 1 < close) {
      return {static_cast<unsigned char>(PAT.data()[i + 1]), i + 2};
    }
    return {static_cast<unsigned char>(PAT.data()[i]), i + 1};
  }

  [[nodiscard]] static consteval auto make_metadata() noexcept -> metadata {
    auto data = metadata{};

    for (auto& pos : data.close_brackets) {
      pos = PAT.size();
    }
    for (auto& pos : data.close_parens) {
      pos = PAT.size();
    }
    for (auto& pos : data.first_branch) {
      pos = PAT.size();
    }
    for (auto& pos : data.branch_end) {
      pos = PAT.size();
    }
    for (auto& pos : data.next_branch) {
      pos = PAT.size();
    }

    for (auto i = 0uz; i < PAT.size(); ++i) {
      if (PAT.data()[i] == '[') {
        data.close_brackets[i] = find_matching_close_bracket<PAT>(i);
      }
      if (PAT.data()[i] == '(') {
        data.close_parens[i] = find_matching_close_paren<PAT>(i);
      }
    }

    for (auto i = 0uz; i < PAT.size(); ++i) {
      if (PAT.data()[i] != '[') {
        continue;
      }

      auto const close = data.close_brackets[i];
      if (close >= PAT.size()) {
        continue;
      }

      auto negated = false;
      auto si = i + 1uz;
      if (si < close && PAT.data()[si] == '!') {
        negated = true;
        ++si;
      }

      auto& table = data.char_class_tables[i].entries;
      if (negated) {
        for (auto& entry : table) {
          entry = true;
        }
      }

      for (auto j = si; j < close; ) {
        auto const [lc, next_j] = read_char_class_token(j, close);
        auto const is_range_token = PAT.data()[j] != '\\' && lc == static_cast<unsigned char>('-') && j != si && next_j < close;

        if (next_j < close && PAT.data()[next_j] == '-' && !is_range_token) {
          auto const dash_i = next_j;
          if (dash_i + 1 < close) {
            auto const [rc, after_range] = read_char_class_token(dash_i + 1, close);
            if (lc <= rc) {
              for (auto ch = static_cast<size_t>(lc); ch <= static_cast<size_t>(rc); ++ch) {
                table[ch] = !negated;
                if (ch == static_cast<size_t>(rc)) {
                  break;
                }
              }
            }
            j = after_range;
            continue;
          }
        }

        table[lc] = !negated;
        j = next_j;
      }
    }

    for (auto i = 0uz; i < PAT.size(); ++i) {
      if (PAT.data()[i] != '(') {
        continue;
      }

      auto const close = data.close_parens[i];
      if (close >= PAT.size()) {
        continue;
      }

      data.first_branch[i] = i + 1;

      auto branch_start = i + 1uz;
      auto depth = 0;

      for (auto j = branch_start; j < close; ++j) {
        auto const bc = PAT.data()[j];
        if (bc == '\\') {
          ++j;
          continue;
        }
        if (bc == '(') {
          ++depth;
          continue;
        }
        if (bc == ')') {
          --depth;
          continue;
        }
        if (bc == '|' && depth == 0) {
          data.branch_end[branch_start] = j;
          data.next_branch[branch_start] = j + 1;
          branch_start = j + 1;
        }
      }

      data.branch_end[branch_start] = close;
      data.next_branch[branch_start] = PAT.size();
    }

    while (data.fixed_prefix_length < PAT.size()) {
      auto const c = PAT.data()[data.fixed_prefix_length];
      if (c == '*' || c == '?' || c == '[' || c == '(' || c == '\\') {
        break;
      }
      ++data.fixed_prefix_length;
    }

    return data;
  }

  static constexpr auto data = make_metadata();

public:
  static constexpr auto NO_BRANCH = PAT.size();
  static constexpr auto close_brackets = data.close_brackets;
  static constexpr auto close_parens = data.close_parens;
  static constexpr auto char_class_tables = data.char_class_tables;
  static constexpr auto first_branch = data.first_branch;
  static constexpr auto branch_end = data.branch_end;
  static constexpr auto branch_next = data.next_branch;
  static constexpr auto next_branch = branch_next;
  static constexpr auto fixed_prefix_length = data.fixed_prefix_length;
};

/**
 * @brief wildcard パターンを frozen_regex に委譲するためのヘルパ
 *
 * パターンに * や ? が含まれず、かつ frozen_regex で扱える構文のみで構成されている場合に
 * can_delegate == true となる。その場合 regex_pattern は frozen_regex にそのまま渡せる
 * 文字列（[! → [^ に変換済み）を保持する。
 */
template <FrozenString PAT>
struct wildcard_to_regex_helper {
  static constexpr bool can_delegate = []() consteval {
    if (PAT.empty()) return false;
    int paren_depth = 0;
    for (auto i = 0uz; i < PAT.size(); ++i) {
      auto const c = PAT.data()[i];
      if (c == '*' || c == '?') return false;
      if (c == '\\') {
        if (i + 1 >= PAT.size()) return false;
        auto const n = PAT.data()[i + 1];
        switch (n) {
          case '\\': case '.': case '[': case ']':
          case '(': case ')': case '|': case '-': case '^':
            ++i; break;
          default: return false;
        }
        continue;
      }
      if (c == '[') {
        auto close = find_matching_close_bracket<PAT>(i);
        if (close >= PAT.size()) return false;
        i = close;
        continue;
      }
      if (c == '.') return false;
      if (c == '(') ++paren_depth;
      if (c == ')') {
        if (paren_depth == 0) return false;
        --paren_depth;
      }
      if (c == '|' && paren_depth == 0) return false;
    }
    return paren_depth == 0;
  }();

  static constexpr FrozenString<PAT.size() + 1> regex_pattern = []() consteval {
    FrozenString<PAT.size() + 1> fs;
    fs.length = PAT.size();
    for (auto i = 0uz; i < PAT.size(); ++i) {
      auto c = PAT.data()[i];
      if (c == '\\') {
        fs.buffer[i] = c;
        if (i + 1 < PAT.size()) {
          fs.buffer[i + 1] = PAT.data()[i + 1];
          ++i;
        }
        continue;
      }
      // [! → [^ に変換（[ が \\ エスケープされていない場合のみ）
      if (c == '!' && i > 0 && PAT.data()[i - 1] == '[') {
        bool bracket_escaped = false;
        if (i >= 2 && PAT.data()[i - 2] == '\\') {
          auto count = 0uz;
          auto j = i - 2;
          while (PAT.data()[j] == '\\') {
            ++count;
            if (j == 0) break;
            --j;
          }
          bracket_escaped = (count % 2 == 1);
        }
        fs.buffer[i] = bracket_escaped ? '!' : '^';
        continue;
      }
      fs.buffer[i] = c;
    }
    fs.buffer[PAT.size()] = '\0';
    return fs;
  }();
};

/**
 * @brief パターン範囲に対して再帰的ワイルドカードマッチングを実行する
 *
 * 対応構文:
 * - `*` : 任意の文字列（空文字列を含む）
 * - `?` : 任意の1文字
 * - `\\` : エスケープ（次の文字をリテラルとして扱う）
 * - `[abc]` : セット内の任意の1文字
 * - `[!abc]` : セット外の任意の1文字
 * - `(ab|cd)` : alternatives（このうちいずれかにマッチ）
 *
 * @tparam PAT パターン文字列（FrozenString NTTP）
 * @param text 対象テキスト
 * @param ti テキスト内の現在位置
 * @param pi パターン内の開始位置
 * @param pi_end パターン内の終了位置（この範囲をマッチングに使う）
 * @return match_result1
 */
template <FrozenString PAT>
[[nodiscard]] constexpr match_result
wildcard_match_impl(std::string_view text, size_t ti, size_t pi, size_t pi_end, bool partial = false) noexcept {
  while (pi < pi_end) {
    auto c = PAT.data()[pi];

    // Escape: match next char literally
    if (c == '\\') {
      if (pi + 1 >= pi_end) return {false, ti};
      ++pi;
      if (ti >= text.size() || text[ti] != PAT.data()[pi]) return {false, ti};
      ++ti;
      ++pi;
      continue;
    }

    // Star: match any sequence (including empty)
    if (c == '*') {
      while (pi < pi_end && PAT.data()[pi] == '*') ++pi;

      // Fast path: no remaining pattern after * — always matches.
      if (pi == pi_end) {
        if (partial) return {true, ti};
        return {true, text.size()};
      }

      // Fast path: if the remaining pattern starts with a literal chunk,
      // use std::string_view::find() to skip non-matching text positions.
      // libstdc++'s find() is SIMD-optimized, making this much faster than
      // backtracking character by character through long strings.
      auto lit_start = pi;
      auto lit_end = pi;
      while (lit_end < pi_end && PAT.data()[lit_end] != '*' && PAT.data()[lit_end] != '?' &&
              PAT.data()[lit_end] != '[' && PAT.data()[lit_end] != '(' &&
              PAT.data()[lit_end] != '\\') {
        ++lit_end;
      }

      if (lit_start < lit_end) {
        auto lit_sv = std::string_view{PAT.data() + lit_start, lit_end - lit_start};
        for (auto found = text.find(lit_sv, ti);
              found != std::string_view::npos;
              found = text.find(lit_sv, found + 1)) {
          auto r = wildcard_match_impl<PAT>(text, found + lit_sv.size(), lit_end, pi_end, partial);
          if (r.matched) return r;
        }
        return {false, ti};
      }

      // Fallback: full backtracking for complex cases (? [ ( \ after *)
      for (auto i = ti; i <= text.size(); ++i) {
        auto r = wildcard_match_impl<PAT>(text, i, pi, pi_end, partial);
        if (r.matched) return r;
      }
      return {false, ti};
    }

    // Question: match any single character
    if (c == '?') {
      if (ti >= text.size()) return {false, ti};
      ++ti;
      ++pi;
      continue;
    }

    // Set: [abc], [!abc], [a-z], [-abc], [abc-], [a\\-z]
    if (c == '[') {
      auto const close = wildcard_plan<PAT>::close_brackets[pi];
      if (close < pi_end) {
        if (ti >= text.size()) return {false, ti};
        auto const tc = static_cast<unsigned char>(text[ti]);
        auto const in_set = wildcard_plan<PAT>::char_class_tables[pi][tc];
        if (!in_set) return {false, ti};

        ++ti;
        pi = close + 1;
        continue;
      }
      // Not a valid set; fall through to literal match for '['
    }

    // Alternative: (branch1|branch2|...)
    if (c == '(') {
      auto const close = wildcard_plan<PAT>::close_parens[pi];
      if (close < pi_end) {
        auto after_alt = close + 1uz;

        for (auto branch = wildcard_plan<PAT>::first_branch[pi];
             branch != wildcard_plan<PAT>::NO_BRANCH;
             branch = wildcard_plan<PAT>::next_branch[branch]) {
          auto const br = wildcard_match_impl<PAT>(text, ti, branch, wildcard_plan<PAT>::branch_end[branch], true);
          if (br.matched) {
            if (after_alt >= pi_end) {
              if (partial || br.pos == text.size()) return br;
            } else {
              auto const rr = wildcard_match_impl<PAT>(text, br.pos, after_alt, pi_end, partial);
              if (rr.matched) return rr;
            }
          }
        }
        return {false, ti};
      }
      // Not a valid alternative; fall through to literal match for '('
    }

    // Literal character
    if (ti >= text.size() || text[ti] != c) return {false, ti};
    ++ti;
    ++pi;
  }

  if (partial) return {true, ti};
  return {ti == text.size(), ti};
}

/**
 * @brief 単純 glob (`*`, `?`, リテラルのみ) を貪欲にマッチングする
 *
 * @tparam PAT パターン文字列（FrozenString NTTP）
 * @param text 対象テキスト
 * @return bool マッチした場合 true
 */
template <FrozenString PAT>
[[nodiscard]] constexpr bool wildcard_match_simple_glob(std::string_view text) noexcept {
  auto ti = 0uz;
  auto pi = 0uz;
  auto star_pi = std::string_view::npos;
  auto retry_ti = 0uz;

  while (ti < text.size()) {
    if (pi < PAT.size()) {
      auto const c = PAT.data()[pi];
      if (c == '*') {
        while (pi < PAT.size() && PAT.data()[pi] == '*') {
          ++pi;
        }
        star_pi = pi;
        retry_ti = ti;
        if (pi == PAT.size()) {
          return true;
        }
        continue;
      }
      if (c == '?' || c == text[ti]) {
        ++ti;
        ++pi;
        continue;
      }
    }

    if (star_pi != std::string_view::npos) {
      pi = star_pi;
      ti = ++retry_ti;
      continue;
    }

    return false;
  }

  while (pi < PAT.size() && PAT.data()[pi] == '*') {
    ++pi;
  }

  return pi == PAT.size();
}

struct find_result {
  size_t start;
  size_t end;
};

template <FrozenString PAT>
[[nodiscard]] constexpr auto wildcard_find_shortest_match_from(std::string_view text, size_t start) noexcept
  -> std::optional<find_result> {
  auto const r = wildcard_match_impl<PAT>(text, start, 0, PAT.size(), true);
  if (r.matched && r.pos > start) {
    return find_result{start, r.pos};
  }
  return std::nullopt;
}

template <FrozenString PAT>
[[nodiscard]] consteval auto wildcard_simple_glob_leading_literal_anchor() noexcept -> find_result {
  auto start = 0uz;
  while (start < PAT.size() && PAT.data()[start] == '*') {
    ++start;
  }

  auto end = start;
  while (end < PAT.size() && PAT.data()[end] != '*' && PAT.data()[end] != '?') {
    ++end;
  }

  return find_result{start, end};
}

template <FrozenString PAT>
[[nodiscard]] constexpr auto wildcard_find_simple_glob_from(std::string_view text, size_t search_from) noexcept
  -> std::optional<find_result> {
  if constexpr (PAT.size() == 0) {
    return std::nullopt;
  }

  constexpr auto prefix_length = wildcard_plan<PAT>::fixed_prefix_length;
  constexpr auto starts_with_star = PAT.size() > 0 && PAT.data()[0] == '*';

  if constexpr (prefix_length > 0) {
    constexpr auto prefix = std::string_view{PAT.data(), prefix_length};

    for (auto candidate = text.find(prefix, search_from);
         candidate != std::string_view::npos;
         candidate = text.find(prefix, candidate + 1)) {
      if (auto const result = wildcard_find_shortest_match_from<PAT>(text, candidate)) {
        return result;
      }
    }
    return std::nullopt;
  }

  if constexpr (starts_with_star) {
    constexpr auto anchor = wildcard_simple_glob_leading_literal_anchor<PAT>();
    if constexpr (anchor.end > anchor.start) {
      auto const literal = std::string_view{PAT.data() + anchor.start, anchor.end - anchor.start};
      if (text.find(literal, search_from) == std::string_view::npos) {
        return std::nullopt;
      }
    }
    return wildcard_find_shortest_match_from<PAT>(text, search_from);
  }

  for (auto candidate = search_from; candidate <= text.size(); ++candidate) {
    if (auto const result = wildcard_find_shortest_match_from<PAT>(text, candidate)) {
      return result;
    }
  }

  return std::nullopt;
}

template <FrozenString PAT>
[[nodiscard]] constexpr auto wildcard_find_simple_glob(std::string_view text) noexcept
  -> std::optional<std::string_view> {
  if (auto const result = wildcard_find_simple_glob_from<PAT>(text, 0)) {
    return text.substr(result->start, result->end - result->start);
  }
  return std::nullopt;
}

template <FrozenString PAT>
[[nodiscard]] constexpr auto wildcard_find_from(std::string_view text, size_t search_from) noexcept
  -> std::optional<find_result> {
  if constexpr (PAT.size() == 0) {
    return std::nullopt;
  }

  constexpr auto is_simple_glob = []() consteval {
    for (auto i = 0uz; i < PAT.size(); ++i) {
      auto const c = PAT.data()[i];
      if (c == '[' || c == '(' || c == '\\') {
        return false;
      }
    }
    return true;
  }();

  if constexpr (is_simple_glob) {
    return wildcard_find_simple_glob_from<PAT>(text, search_from);
  }

  constexpr auto prefix_length = wildcard_plan<PAT>::fixed_prefix_length;
  constexpr auto starts_with_star = PAT.size() > 0 && PAT.data()[0] == '*';

  if constexpr (starts_with_star) {
    if (auto const result = wildcard_find_shortest_match_from<PAT>(text, search_from)) {
      return result;
    }

    if (search_from < text.size()) {
      for (auto candidate = search_from + 1; candidate <= text.size(); ++candidate) {
        if (auto const result = wildcard_find_shortest_match_from<PAT>(text, candidate)) {
          return result;
        }
      }
    }

    return std::nullopt;
  }

  if constexpr (prefix_length > 0) {
    constexpr auto prefix = std::string_view{PAT.data(), prefix_length};

    for (auto candidate = text.find(prefix, search_from);
         candidate != std::string_view::npos;
         candidate = text.find(prefix, candidate + 1)) {
      if (auto const result = wildcard_find_shortest_match_from<PAT>(text, candidate)) {
        return result;
      }
    }
    return std::nullopt;
  }

  for (auto candidate = search_from; candidate <= text.size(); ++candidate) {
    if (auto const result = wildcard_find_shortest_match_from<PAT>(text, candidate)) {
      return result;
    }
  }

  return std::nullopt;
}

} // namespace detail

/**
 * @brief ワイルドカードパターンでテキストがマッチするか判定する
 */
template <FrozenString PAT>
[[nodiscard]] constexpr bool wildcard_match(std::string_view text) noexcept {
  // frozen_regex 委譲: * と ? を含まないパターンは frozen_regex で二分探索
  if constexpr (detail::wildcard_to_regex_helper<PAT>::can_delegate) {
    constexpr auto regex_pat = detail::wildcard_to_regex_helper<PAT>::regex_pattern;
    return frozen_regex<regex_pat>::contains(text);
  }

  constexpr auto is_simple_glob = []() consteval {
    for (auto i = 0uz; i < PAT.size(); ++i) {
      auto const c = PAT.data()[i];
      if (c == '[' || c == '(' || c == '\\') {
        return false;
      }
    }
    return true;
  }();

  if constexpr (is_simple_glob) {
    return detail::wildcard_match_simple_glob<PAT>(text);
  }

  // エスケープ文字の有無
  constexpr auto has_escape = []() consteval {
    for (auto i = 0uz; i < PAT.size(); ++i) {
      if (PAT.data()[i] == '\\') return true;
    }
    return false;
  }();

  // 早期リジェクト: エスケープ文字を含む場合はスキップ（正しい位置計算が複雑なため）
  if constexpr (!has_escape) {
    // プレフィックス早期リジェクト
    constexpr auto pref_len = []() consteval {
      for (auto i = 0uz; i < PAT.size(); ++i) {
        auto const c = PAT.data()[i];
        if (c == '*' || c == '?' || c == '[' || c == '(') return i;
      }
      return PAT.size();
    }();

    // サフィックス早期リジェクト
    constexpr auto suff_start = []() consteval {
      auto last = PAT.size();
      for (auto i = PAT.size(); i > 0; --i) {
        if (PAT.data()[i - 1] == '*') { last = i - 1; break; }
      }
      if (last == PAT.size()) return PAT.size();
      for (auto i = last + 1; i < PAT.size(); ++i) {
        auto const c = PAT.data()[i];
        if (c == '*' || c == '?' || c == '[' || c == '(') return PAT.size();
      }
      return last + 1;
    }();

    if (pref_len > 0 && text.size() >= pref_len) {
      for (auto i = 0uz; i < pref_len; ++i) {
        if (text[i] != PAT.data()[i]) return false;
      }
    } else if (pref_len > 0) {
      return false;
    }

    if (suff_start < PAT.size()) {
      auto const suff_len = PAT.size() - suff_start;
      if (text.size() < suff_len) return false;
      for (auto i = 0uz; i < suff_len; ++i) {
        if (text[text.size() - suff_len + i] != PAT.data()[suff_start + i]) return false;
      }
    }

    return detail::wildcard_match_impl<PAT>(text, pref_len, pref_len, suff_start).matched;
  }

  return detail::wildcard_match_impl<PAT>(text, 0, 0, PAT.size()).matched;
}

/**
 * @brief ワイルドカードパターンでテキストがマッチするか判定する（文字列リテラル版）
 *
 * @param pattern ワイルドカードパターン文字列リテラル
 * @param text マッチング対象のテキスト
 * @return auto マッチした場合 true
 */
template <size_t N>
[[nodiscard]] bool wildcard_match(char const (&pattern)[N], std::string_view text) noexcept {
  return wildcard_match<FrozenString<N>{pattern}>(text);
}

/**
 * @brief ワイルドカードパターンでテキストがマッチするか判定する（FrozenString パターン版）
 *
 * @tparam M パターンの長さ（終端文字を含む）
 * @param pattern ワイルドカードパターン（FrozenString）
 * @param text マッチング対象のテキスト
 * @return auto マッチした場合 true
 */
template <size_t M>
[[nodiscard]] bool wildcard_match(FrozenString<M> const& pattern, std::string_view text) noexcept {
  return wildcard_match<FrozenString<M>{pattern.data()}>(text);
}

/**
 * @brief ワイルドカードパターンでテキストがマッチするか判定する（文字列リテラルテキスト版）
 *
 * @tparam PAT ワイルドカードパターン（FrozenString NTTP）
 * @tparam M テキストの長さ（終端文字を含む）
 * @param text マッチング対象のテキスト（文字列リテラル）
 * @return auto マッチした場合 true
 */
template <FrozenString PAT, size_t M>
[[nodiscard]] constexpr bool wildcard_match(char const (&text)[M]) noexcept {
  return wildcard_match<PAT>(std::string_view{text, M - 1});
}

/**
 * @brief ワイルドカードパターンでテキストがマッチするか判定する（FrozenString テキスト版）
 *
 * @tparam PAT ワイルドカードパターン（FrozenString NTTP）
 * @tparam M テキストの長さ（終端文字を含む）
 * @param text マッチング対象のテキスト（FrozenString）
 * @return auto マッチした場合 true
 */
template <FrozenString PAT, size_t M>
[[nodiscard]] constexpr bool wildcard_match(FrozenString<M> const& text) noexcept {
  return wildcard_match<PAT>(text.sv());
}

/**
 * @brief ワイルドカードパターンでテキスト内の最初のマッチ部分を検索する
 *
 * @tparam PAT ワイルドカードパターン（FrozenString NTTP）
 * @param text 検索対象のテキスト
 * @return std::optional<std::string_view> マッチした部分文字列。非マッチなら std::nullopt
 */
template <FrozenString PAT>
[[nodiscard]] constexpr std::optional<std::string_view> wildcard_find(std::string_view text) noexcept {
  if (auto const result = detail::wildcard_find_from<PAT>(text, 0)) {
    return text.substr(result->start, result->end - result->start);
  }
  return std::nullopt;
}

/**
 * @brief ワイルドカードパターンでテキスト内のマッチ部分を全て検索する
 *
 * @tparam PAT ワイルドカードパターン（FrozenString NTTP）
 * @param text 検索対象のテキスト
 * @return マッチ部分を順次 std::string_view として返す range
 */
template <FrozenString PAT>
[[nodiscard]] constexpr auto wildcard_find_all(std::string_view text) noexcept {
  struct sentinel_type {};

  struct iterator {
    std::string_view text_;
    size_t start_ = 0;
    size_t end_ = 0;
    bool done_ = true;

    std::string_view operator*() const noexcept {
      return text_.substr(start_, end_ - start_);
    }

    iterator& operator++() noexcept {
      auto search_from = end_;
      if (auto const result = detail::wildcard_find_from<PAT>(text_, search_from)) {
        start_ = result->start;
        end_ = result->end;
        done_ = false;
        return *this;
      }
      done_ = true;
      return *this;
    }

    bool operator!=(sentinel_type) const noexcept { return !done_; }
  };

  struct range {
    std::string_view text_;

    iterator begin() const noexcept {
      auto it = iterator{};
      it.text_ = text_;
      ++it;
      return it;
    }

    sentinel_type end() const noexcept { return {}; }
  };

  return range{text};
}

namespace ops {

/**
 * @brief ワイルドカードマッチングのパイプ演算子アダプタ
 *
 * @tparam PAT ワイルドカードパターン（FrozenString NTTP）
 *
 * @details 以下の型をパイプ演算子で受け付ける:
 *   - `FrozenString<N>`   : コンパイル時文字列
 *   - `std::string_view`  : 実行時文字列（ビュー）
 *   - `std::string const&`: 実行時文字列
 *   - `char const (&)[N]` : 文字列リテラル（string_view に暗黙変換）
 */
template <FrozenString PAT>
struct wildcard_adaptor : pipe_adaptor_base {
  /**
   * @brief FrozenString テキストに対してワイルドカードマッチングを実行する
   *
   * @tparam N テキストの長さ（終端文字を含む）
   * @param text マッチング対象のテキスト
   * @return bool マッチした場合 true
   */
  template <size_t N>
  [[nodiscard]] consteval bool operator()(FrozenString<N> const& text) const noexcept {
    return frozenchars::wildcard_match<PAT>(text.sv());
  }

  /**
   * @brief std::string_view テキストに対してワイルドカードマッチングを実行する
   *
   * @param text マッチング対象のテキスト
   * @return bool マッチした場合 true
   */
  [[nodiscard]] constexpr bool operator()(std::string_view text) const noexcept {
    return frozenchars::wildcard_match<PAT>(text);
  }

  /**
   * @brief std::string テキストに対してワイルドカードマッチングを実行する
   *
   * @param text マッチング対象のテキスト
   * @return bool マッチした場合 true
   */
  [[nodiscard]] constexpr bool operator()(std::string const& text) const noexcept {
    return frozenchars::wildcard_match<PAT>(std::string_view{text});
  }
};

} // namespace ops

} // namespace frozenchars
