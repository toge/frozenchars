/// @file cypher_minifier.hpp
/// @brief CypherQL（Neo4j/openCypher）クエリをコンパイル時・実行時の両方で
///        ミニファイする constexpr 関数群
///
/// ## ミニファイの仕様
///
/// 1. **コメント除去**
///    - `//` から行末まで削除
///    - `/* ... */` 削除（複数行に跨るもの可、入れ子なし）
///    - 文字列リテラル / バッククォート識別子内のコメント記号は無視
///
/// 2. **空白削除**
///    - スペース・タブ・改行・CR・FF はすべて削除
///    - ただし隣接する両トークンが識別子文字（[a-zA-Z0-9_]）の場合のみ
///      スペース 1 つを挿入してトークン結合を防ぐ
///
/// 3. **末尾セミコロン除去**
///    - 出力末尾が `;` の場合のみ 1 つ除去する
///    - 複数文を区切る中間の `;` は保持する
///
/// 4. **文字列リテラル完全保存**
///    - シングルクォート `'...'`、ダブルクォート `"..."` の内容は一切変更しない
///    - エスケープシーケンス（`\'`, `\"`, `\\`, `\n`, `\uXXXX` 等）もそのまま出力
///
/// 5. **バッククォート識別子完全保存**
///    - `` `...` `` の内容は空白も改行もそのまま出力
///    - `` `` `` のエスケープ表記もそのまま維持

#pragma once

#include <array>
#include <cstddef>

namespace frozenchars {

namespace detail {

/// @brief 文字が識別子文字 [a-zA-Z0-9_] かどうかを判定する
constexpr bool isIdentChar(char c) noexcept
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

/// @brief Cypher ミニファイ用の字句解析状態
enum class minify_state : unsigned char {
  normal,        ///< 通常コード
  single_quote,  ///< シングルクォート文字列 '...'
  double_quote,  ///< ダブルクォート文字列 "..."
  backtick,      ///< バッククォート識別子 `...`
  line_comment,  ///< 行コメント // ～ 行末
  block_comment, ///< ブロックコメント /* ～ */
};

} // namespace detail

/// @brief Cypher クエリをミニファイする（コンパイル時・実行時共用）
///
/// 入力より出力が長くなることはないため、output_capacity に入力長+1 を渡せば安全。
///
/// @param input           null 終端の Cypher クエリ
/// @param output          出力バッファ（少なくとも output_capacity バイト）
/// @param output_capacity 出力バッファ容量（0 の場合は 0 を返す）
/// @return 出力文字数（null 終端を除く）
///
/// @note セミコロン除去ポリシー:
///       出力末尾の `;` を 1 つ除去する。複数文の中間セミコロンは保持する。
constexpr std::size_t minify_cypher(const char *input, char *output,
                                   std::size_t output_capacity) noexcept
{
  if (output_capacity == 0) {
    return 0;
  }

  using detail::isIdentChar;
  using detail::minify_state;

  auto state = minify_state::normal;
  std::size_t out_len = 0;
  bool pending_space = false; // 次の識別子文字出力前にスペースを挿入するフラグ
  char last_out = '\0';       // 直前に出力した文字（スペース要否の判定用）

  // 出力バッファへ 1 文字書き込む（null 終端分を残す）
  auto writeChar = [&](char c) noexcept {
    if (out_len < output_capacity - 1) {
      output[out_len++] = c;
      last_out = c;
    }
  };

  for (std::size_t i = 0; input[i] != '\0';) {
    char const c = input[i];
    char const next = input[i + 1]; // 終端なら '\0'

    switch (state) {
    case minify_state::normal: {
      if (c == '/' && next == '/') {
        // 行コメント開始
        state = minify_state::line_comment;
        i += 2;
      } else if (c == '/' && next == '*') {
        // ブロックコメント開始
        state = minify_state::block_comment;
        i += 2;
      } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
                 c == '\f') {
        // 空白文字: 遅延フラグに記録し、後続トークンで必要なら挿入
        pending_space = true;
        ++i;
      } else {
        // 出力対象文字: 識別子同士の隣接ならスペースを挿入
        if (pending_space && isIdentChar(last_out) && isIdentChar(c)) {
          writeChar(' ');
        }
        pending_space = false;

        if (c == '\'') {
          writeChar(c);
          state = minify_state::single_quote;
        } else if (c == '"') {
          writeChar(c);
          state = minify_state::double_quote;
        } else if (c == '`') {
          writeChar(c);
          state = minify_state::backtick;
        } else {
          writeChar(c);
        }
        ++i;
      }
      break;
    }

    case minify_state::single_quote: {
      // 文字列内は空白・コメント問わずすべてそのまま出力
      writeChar(c);
      if (c == '\\') {
        // エスケープシーケンス: 直後の 1 文字もそのまま出力
        ++i;
        if (input[i] != '\0') {
          writeChar(input[i]);
          ++i;
        }
      } else if (c == '\'') {
        state = minify_state::normal;
        ++i;
      } else {
        ++i;
      }
      break;
    }

    case minify_state::double_quote: {
      writeChar(c);
      if (c == '\\') {
        ++i;
        if (input[i] != '\0') {
          writeChar(input[i]);
          ++i;
        }
      } else if (c == '"') {
        state = minify_state::normal;
        ++i;
      } else {
        ++i;
      }
      break;
    }

    case minify_state::backtick: {
      // バッククォート識別子: 空白も含めすべて出力
      writeChar(c);
      if (c == '`') {
        if (next == '`') {
          // `` はエスケープされたバッククォート: 2 文字まとめて出力し継続
          writeChar(next);
          i += 2;
        } else {
          // 識別子終了
          state = minify_state::normal;
          ++i;
        }
      } else {
        ++i;
      }
      break;
    }

    case minify_state::line_comment: {
      // 行末まで読み飛ばす（改行自体は通常の空白と同扱い）
      if (c == '\n' || c == '\r') {
        state = minify_state::normal;
        pending_space = true;
      }
      ++i;
      break;
    }

    case minify_state::block_comment: {
      // */ が来るまで読み飛ばす
      if (c == '*' && next == '/') {
        state = minify_state::normal;
        pending_space = true; // コメント除去箇所は空白扱い
        i += 2;
      } else {
        ++i;
      }
      break;
    }
    }
  }

  // 末尾セミコロンを 1 つ除去する
  if (out_len > 0 && output[out_len - 1] == ';') {
    --out_len;
  }

  output[out_len] = '\0';
  return out_len;
}

// ─── テンプレートヘルパー ───────────────────────────────────────────────────

/// @brief minify_cypher の結果を保持するコンテナ（static_assert 比較対応）
///
/// @tparam N 最大バッファ長（入力文字列長と同一で十分）
template <std::size_t N>
struct minified_query {
  std::array<char, N> data{};
  std::size_t length{0};

  /// @brief 文字列リテラルとの等値比較
  template <std::size_t M>
  constexpr bool operator==(char const (&str)[M]) const noexcept
  {
    if (length != M - 1) {
      return false;
    }
    for (std::size_t i = 0; i < length; ++i) {
      if (data[i] != str[i]) {
        return false;
      }
    }
    return true;
  }

  /// @brief 別の minified_query との等値比較
  template <std::size_t M>
  constexpr bool operator==(minified_query<M> const &other) const noexcept
  {
    if (length != other.length) {
      return false;
    }
    for (std::size_t i = 0; i < length; ++i) {
      if (data[i] != other.data[i]) {
        return false;
      }
    }
    return true;
  }
};

/// @brief Cypher クエリをコンパイル時にミニファイする
///
/// static_assert での検証に使用できる。
///
/// @code
/// constexpr auto result = minify("MATCH (n) RETURN n");
/// static_assert(result == "MATCH(n)RETURN n");
/// @endcode
///
/// @tparam N  入力文字列の長さ（null 終端込み、コンパイラが推論）
/// @param  input  null 終端の Cypher クエリ
/// @return minified_query<N>（入力以下のサイズに収まる）
template <std::size_t N>
[[nodiscard]] constexpr auto minify(char const (&input)[N]) noexcept
    -> minified_query<N>
{
  minified_query<N> result{};
  result.length = minify_cypher(input, result.data.data(), N);
  return result;
}

} // namespace frozenchars
