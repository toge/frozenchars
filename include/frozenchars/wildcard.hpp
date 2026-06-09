#pragma once

#include "string.hpp"
#include <array>
#include <cstddef>
#include <string_view>

namespace frozenchars {

namespace detail {

  /**
   * @brief ワイルドカードパターンのセグメント種別
   */
  enum class wildcard_seg_type : char {
    LITERAL,        ///< リテラル文字列
    STAR,           ///< * (任意の文字列、空含む)
    QUESTION,       ///< ? (任意の1文字)
  };

  /**
   * @brief ワイルドカードパターンのセグメント
   */
  struct wildcard_segment {
    wildcard_seg_type type;  ///< 種別
    size_t offset;           ///< リテラルセグメントのバッファ内オフセット
    size_t length;           ///< リテラルの長さ
  };

  /**
   * @brief セグメント配列から有効なセグメント数を計算する
   *
   * @tparam Segments セグメント配列の型
   * @return auto 有効なセグメント数
   */
  template <typename Segments>
  [[nodiscard]] consteval auto count_valid_segments(Segments const& segments) noexcept -> size_t {
    auto count = 0uz;
    for (auto i = 0uz; i < segments.size(); ++i) {
      if (segments[i].type != wildcard_seg_type::LITERAL || segments[i].length > 0) {
        ++count;
      }
    }
    return count;
  }

  /**
   * @brief セグメント配列から最小一致長を計算する
   *
   * @tparam Segments セグメント配列の型
   * @tparam NumSegments セグメント数
   * @return auto 最小一致長
   */
  template <typename Segments, size_t NumSegments>
  [[nodiscard]] consteval auto wildcard_min_length(Segments const& segments) noexcept -> size_t {
    auto min_len = 0uz;
    for (auto i = 0uz; i < NumSegments; ++i) {
      auto const& seg = segments[i];
      if (seg.type == wildcard_seg_type::LITERAL) {
        min_len += seg.length;
      } else if (seg.type == wildcard_seg_type::QUESTION) {
        min_len += 1;
      }
    }
    return min_len;
  }

  /**
   * @brief リテラルセグメントをテキストの指定位置から前方比較する
   *
   * @tparam PAT パターン文字列（FrozenString NTTP）
   * @tparam SegIdx セグメントのインデックス
   * @tparam Segments セグメント配列の型
   * @param segments セグメント配列
   * @param text 対象テキスト
   * @param pos テキスト内の開始位置
   * @return auto 一致した場合 true
   */
  template <FrozenString PAT, size_t SegIdx, typename Segments>
  [[nodiscard]] constexpr bool match_literal_at(Segments const& segments, std::string_view text, size_t pos) noexcept {
    auto const& seg = segments[SegIdx];
    auto const lit_len = seg.length;

    if (pos + lit_len > text.size()) {
      return false;
    }
    for (auto i = 0uz; i < lit_len; ++i) {
      if (text[pos + i] != PAT.buffer[seg.offset + i]) {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief セグメント配列を使ったワイルドカードマッチングの内部実装
   *
   * @tparam PAT パターン文字列（FrozenString NTTP）
   * @tparam SegIdx 現在のセグメントインデックス
   * @tparam NumSegments セグメント数
   * @tparam Segments セグメント配列の型
   * @param segments 解析済みセグメント配列
   * @param text 対象テキスト
   * @param pos テキスト内の現在位置
   * @return auto マッチした場合 true
   */
  template <FrozenString PAT, size_t SegIdx, size_t NumSegments, typename Segments>
  [[nodiscard]] constexpr bool wildcard_match_segments(Segments const& segments, std::string_view text, size_t pos) noexcept {
    if constexpr (SegIdx == NumSegments) {
      return pos == text.size();
    } else {
      auto const& seg = segments[SegIdx];

      if (seg.type == wildcard_seg_type::LITERAL) {
        if (!match_literal_at<PAT, SegIdx>(segments, text, pos)) {
          return false;
        }
        return wildcard_match_segments<PAT, SegIdx + 1, NumSegments>(segments, text, pos + seg.length);
      } else if (seg.type == wildcard_seg_type::QUESTION) {
        if (pos >= text.size()) {
          return false;
        }
        return wildcard_match_segments<PAT, SegIdx + 1, NumSegments>(segments, text, pos + 1);
      } else {
        // STAR: 残りのテキストに対して再帰的に試行（pos から text.size() まで）
        for (auto remaining = text.size(); remaining + 1 > pos; --remaining) {
          if (wildcard_match_segments<PAT, SegIdx + 1, NumSegments>(segments, text, remaining)) {
            return true;
          }
          if (remaining == 0) break;
        }
        return false;
      }
    }
  }

} // namespace detail

/**
 * @brief ワイルドカードパターンでテキストがマッチするか判定する
 *
 * パターン構文:
 * - `*` : 任意の文字列（空文字列を含む）
 * - `?` : 任意の1文字
 * - その他の文字: リテラル一致
 *
 * パターンは NTTP (Non-Type Template Parameter) として渡され、
 * コンパイル時にセグメント分割される。テキストがコンパイル時定数の場合は
 * consteval で全評価、実行時の場合はコンパイル時前処理済みの高速マッチングを実行。
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
 * @endcode
 */
template <FrozenString PAT>
[[nodiscard]] bool wildcard_match(std::string_view text) noexcept {
  constexpr auto PAT_LEN = PAT.size();
  constexpr auto MAX_SEGS = PAT_LEN > 0 ? PAT_LEN : 1;

  // パターンをセグメントに分割（constexpr lambda でコンパイル時評価）
  constexpr auto segments = []() constexpr {
    auto segs = std::array<detail::wildcard_segment, MAX_SEGS>{};
    auto count = 0uz;
    auto lit_start = 0uz;
    auto has_literal = false;

    for (size_t i = 0; i < PAT_LEN; ++i) {
      auto ch = PAT.buffer[i];
      if (ch == '*') {
        if (has_literal) {
          segs[count++] = {detail::wildcard_seg_type::LITERAL, lit_start, i - lit_start};
          has_literal = false;
        }
        segs[count++] = {detail::wildcard_seg_type::STAR, 0, 0};
      } else if (ch == '?') {
        if (has_literal) {
          segs[count++] = {detail::wildcard_seg_type::LITERAL, lit_start, i - lit_start};
          has_literal = false;
        }
        segs[count++] = {detail::wildcard_seg_type::QUESTION, 0, 1};
      } else {
        if (!has_literal) {
          lit_start = i;
          has_literal = true;
        }
      }
    }
    if (has_literal) {
      segs[count++] = {detail::wildcard_seg_type::LITERAL, lit_start, PAT_LEN - lit_start};
    }

    auto result = std::array<detail::wildcard_segment, MAX_SEGS>{};
    for (auto j = 0uz; j < count; ++j) {
      result[j] = segs[j];
    }
    return result;
  }();

  constexpr auto NUM_SEGS = detail::count_valid_segments(segments);

  if constexpr (NUM_SEGS == 0) {
    return text.empty();
  } else {
    constexpr auto MIN_LEN = detail::wildcard_min_length<decltype(segments), NUM_SEGS>(segments);
    if (text.size() < MIN_LEN) {
      return false;
    }
    return detail::wildcard_match_segments<PAT, 0, NUM_SEGS>(segments, text, 0);
  }
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
