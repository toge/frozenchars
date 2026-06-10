#pragma once

#include "string.hpp"
#include <cstddef>
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
   *
   * @tparam PAT パターン文字列（FrozenString NTTP）
   * @param pos '(' の位置
   * @return size_t ')' の位置（見つからない場合は PAT.size() - 1、この場合は depth > 0）
   */
  template <FrozenString PAT>
  [[nodiscard]] constexpr size_t find_matching_close_paren(size_t pos) noexcept {
    auto i = pos + 1uz;
    auto depth = 1;
    while (i < PAT.size() && depth > 0) {
      if (PAT.data()[i] == '\\') {
        i += 2;
        continue;
      }
      if (PAT.data()[i] == '(') ++depth;
      else if (PAT.data()[i] == ')') --depth;
      ++i;
    }
    return i - 1;
  }

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
   * @return match_result
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
        do { ++pi; } while (pi < pi_end && PAT.data()[pi] == '*');

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

      // Set: [abc] or [!abc]
      if (c == '[') {
        auto close = find_matching_close_bracket<PAT>(pi);
        if (close < PAT.size() && close < pi_end) {
          auto negated = false;
          auto si = pi + 1uz;
          if (si < close && PAT.data()[si] == '!') {
            negated = true;
            ++si;
          }

          if (ti >= text.size()) return {false, ti};
          auto tc = text[ti];

          auto in_set = false;
          for (auto i = si; i < close; ++i) {
            auto sc = PAT.data()[i];
            if (sc == '\\') {
              ++i;
              if (i < close && PAT.data()[i] == tc) { in_set = true; break; }
            } else if (sc == tc) {
              in_set = true;
              break;
            }
          }

          if (negated) in_set = !in_set;
          if (!in_set) return {false, ti};

          ++ti;
          pi = close + 1;
          continue;
        }
        // Not a valid set; fall through to literal match for '['
      }

      // Alternative: (branch1|branch2|...)
      if (c == '(') {
        auto close = find_matching_close_paren<PAT>(pi);
        if (close < PAT.size() && close < pi_end) {
          auto after_alt = close + 1uz;

          auto branch_start = pi + 1uz;
          auto depth = 0;

          for (auto i = branch_start; i < close; ++i) {
            auto bc = PAT.data()[i];
            if (bc == '\\') { ++i; continue; }
            if (bc == '(') { ++depth; continue; }
            if (bc == ')') { --depth; continue; }
            if (bc == '|' && depth == 0) {
              // partial=true: branch doesn't need to consume all text
              auto br = wildcard_match_impl<PAT>(text, ti, branch_start, i, true);
              if (br.matched) {
                auto rr = wildcard_match_impl<PAT>(text, br.pos, after_alt, pi_end, partial);
                if (rr.matched) return rr;
              }
              branch_start = i + 1uz;
            }
          }
          // Last branch
          {
            auto br = wildcard_match_impl<PAT>(text, ti, branch_start, close, true);
            if (br.matched) {
              return wildcard_match_impl<PAT>(text, br.pos, after_alt, pi_end, partial);
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


} // namespace detail

/**
 * @brief ワイルドカードパターンでテキストがマッチするか判定する
 *
 * パターン構文:
 * - `*` : 任意の文字列（空文字列を含む）
 * - `?` : 任意の1文字
 * - `\\` : エスケープ（次の特殊文字をリテラルとして扱う）
 * - `[abc]` : セット内の任意の1文字にマッチ
 * - `[!abc]` : セット外の任意の1文字にマッチ
 * - `(ab|cd)` : alternatives のいずれかにマッチ
 * - その他の文字: リテラル一致
 *
 * パターンは NTTP (Non-Type Template Parameter) として渡される。
 * テキストがコンパイル時定数の場合は consteval で全評価、
 * 実行時の場合はコンパイル時最適化された高速マッチングを実行。
 *
 * @tparam PAT ワイルドカードパターン（FrozenString NTTP）
 * @param text マッチング対象のテキスト
 * @return auto マッチした場合 true
 *
 * @code
 * // 実行時テキスト
 * bool result = frozenchars::wildcard_match<"a*b">(text);
 *
 * // コンパイル時テキスト
 * constexpr auto r = frozenchars::wildcard_match<"a*b">("axxb"_fs);
 * static_assert(r);
 *
 * // セット
 * constexpr auto s = frozenchars::wildcard_match<"[abc]">("b"_fs);
 * static_assert(s);
 *
 * // alternatives
 * constexpr auto t = frozenchars::wildcard_match<"(ab|cd)">("cd"_fs);
 * static_assert(t);
 * @endcode
 */
template <FrozenString PAT>
[[nodiscard]] bool wildcard_match(std::string_view text) noexcept {
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
[[nodiscard]] bool wildcard_match(char const (&text)[M]) noexcept {
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
[[nodiscard]] bool wildcard_match(FrozenString<M> const& text) noexcept {
  return wildcard_match<PAT>(text.sv());
}

namespace ops {

  /**
   * @brief ワイルドカードマッチングのパイプ演算子アダプタ
   *
   * @tparam PAT ワイルドカードパターン（FrozenString NTTP）
   */
  template <FrozenString PAT>
  struct wildcard_adaptor : pipe_adaptor_base {
    /**
     * @brief テキストに対してワイルドカードマッチングを実行する
     *
     * @tparam N テキストの長さ（終端文字を含む）
     * @param text マッチング対象のテキスト
     * @return auto マッチした場合 true
     */
    template <size_t N>
    [[nodiscard]] bool operator()(FrozenString<N> const& text) const noexcept {
      return frozenchars::wildcard_match<PAT>(text.sv());
    }
  };

} // namespace ops

} // namespace frozenchars
