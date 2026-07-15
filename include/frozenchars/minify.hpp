/// @file minify.hpp
/// @brief HTML/XML/JSON/YAML/SQL/Cypher のコンパイル時・実行時ミニファイ関数

#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "detail/char_utils.hpp"
#include "string_ops.hpp"

namespace frozenchars {

// ─── HTML/XML minify オプション ──────────────────────────────────────────────

/**
 * @brief HTML/XML minify オプション（ビットフィールド）
 */
enum class minify_markup_opt : uint8_t {
  none            = 0,
  remove_quotes   = 1 << 0, ///< 属性値のクォートを除去する
  remove_end_tags = 1 << 1, ///< 省略可能な終了タグを除去する
};

inline constexpr auto operator|(minify_markup_opt a, minify_markup_opt b) noexcept {
  return static_cast<minify_markup_opt>(std::to_underlying(a) | std::to_underlying(b));
}

inline constexpr auto operator&(minify_markup_opt a, minify_markup_opt b) noexcept {
  return static_cast<minify_markup_opt>(std::to_underlying(a) & std::to_underlying(b));
}

inline constexpr auto has_flag(minify_markup_opt value, minify_markup_opt flag) noexcept {
  return (std::to_underlying(value) & std::to_underlying(flag)) != 0;
}

// ─── SQL minify オプション ──────────────────────────────────────────────────

/**
 * @brief SQL minify オプション（ビットフィールド）
 */
enum class minify_sql_opt : uint8_t {
  none           = 0,
  shorten_types  = 1 << 0, ///< 型キーワードを短縮する (INTEGER→INT 等)
  remove_as      = 1 << 1, ///< AS キーワードを除去する
  simplify_join  = 1 << 2, ///< INNER JOIN → JOIN に簡略化する
};

inline constexpr auto operator|(minify_sql_opt a, minify_sql_opt b) noexcept {
  return static_cast<minify_sql_opt>(std::to_underlying(a) | std::to_underlying(b));
}

inline constexpr auto operator&(minify_sql_opt a, minify_sql_opt b) noexcept {
  return static_cast<minify_sql_opt>(std::to_underlying(a) & std::to_underlying(b));
}

inline constexpr auto has_flag(minify_sql_opt value, minify_sql_opt flag) noexcept -> bool {
  return (std::to_underlying(value) & std::to_underlying(flag)) != 0;
}

// ─── 内部補助関数 ───────────────────────────────────────────────────────────

namespace detail {

  /**
   * @brief HTML/XML 向けの空白文字判定を行う
   */
  auto constexpr is_markup_space(char c) noexcept {
    return is_any_whitespace(c);
  }

  /**
   * @brief SQL で前後空白の削除対象にできる記号か判定する
   */
  auto constexpr is_sql_punct(char c) noexcept {
    return c == ',' || c == ';' || c == '(' || c == '=' || c == '+' || c == '-' || c == '/' || c == '<' || c == '>' || c == ':' || c == '|' || c == '@' || c == '#';
  }

  /**
   * @brief boolean属性名か判定する
   */
  auto constexpr is_boolean_attribute(char const* name, size_t len) noexcept {
    // oxfmt-ignore
    return (len == 8 && name[0] == 'd' && name[1] == 'i' && name[2] == 's' && name[3] == 'a' && name[4] == 'b' && name[5] == 'l' && name[6] == 'e' && name[7] == 'd')
        || (len == 7 && name[0] == 'c' && name[1] == 'h' && name[2] == 'e' && name[3] == 'c' && name[4] == 'k' && name[5] == 'e' && name[6] == 'd')
        || (len == 8 && name[0] == 'r' && name[1] == 'e' && name[2] == 'a' && name[3] == 'd' && name[4] == 'o' && name[5] == 'n' && name[6] == 'l' && name[7] == 'y')
        || (len == 8 && name[0] == 'r' && name[1] == 'e' && name[2] == 'q' && name[3] == 'u' && name[4] == 'i' && name[5] == 'r' && name[6] == 'e' && name[7] == 'd')
        || (len == 9 && name[0] == 'a' && name[1] == 'u' && name[2] == 't' && name[3] == 'o' && name[4] == 'f' && name[5] == 'o' && name[6] == 'c' && name[7] == 'u' && name[8] == 's')
        || (len == 8 && name[0] == 'a' && name[1] == 'u' && name[2] == 't' && name[3] == 'o' && name[4] == 'p' && name[5] == 'l' && name[6] == 'a' && name[7] == 'y')
        || (len == 8 && name[0] == 'c' && name[1] == 'o' && name[2] == 'n' && name[3] == 't' && name[4] == 'r' && name[5] == 'o' && name[6] == 'l' && name[7] == 's')
        || (len == 4 && name[0] == 'l' && name[1] == 'o' && name[2] == 'o' && name[3] == 'p')
        || (len == 5 && name[0] == 'm' && name[1] == 'u' && name[2] == 't' && name[3] == 'e' && name[4] == 'd')
        || (len == 6 && name[0] == 'h' && name[1] == 'i' && name[2] == 'd' && name[3] == 'd' && name[4] == 'e' && name[5] == 'n')
        || (len == 5 && name[0] == 'd' && name[1] == 'e' && name[2] == 'f' && name[3] == 'e' && name[4] == 'r')
        || (len == 5 && name[0] == 'a' && name[1] == 's' && name[2] == 'y' && name[3] == 'n' && name[4] == 'c')
        || (len == 9 && name[0] == 'n' && name[1] == 'o' && name[2] == 'v' && name[3] == 'a' && name[4] == 'l' && name[5] == 'i' && name[6] == 'd' && name[7] == 'a' && name[8] == 't')
        || (len == 13 && name[0] == 'f' && name[1] == 'o' && name[2] == 'r' && name[3] == 'm' && name[4] == 'n' && name[5] == 'o' && name[6] == 'v' && name[7] == 'a' && name[8] == 'l' && name[9] == 'i' && name[10] == 'd' && name[11] == 'a' && name[12] == 't')
        || (len == 8 && name[0] == 'r' && name[1] == 'e' && name[2] == 'v' && name[3] == 'e' && name[4] == 'r' && name[5] == 's' && name[6] == 'e' && name[7] == 'd')
        || (len == 6 && name[0] == 'o' && name[1] == 'p' && name[2] == 'e' && name[3] == 'n' && name[4] == 'e' && name[5] == 'd')
        || (len == 8 && name[0] == 's' && name[1] == 'e' && name[2] == 'l' && name[3] == 'e' && name[4] == 'c' && name[5] == 't' && name[6] == 'e' && name[7] == 'd')
        || (len == 6 && name[0] == 'a' && name[1] == 'u' && name[2] == 't' && name[3] == 'o' && name[4] == 'p' && name[5] == 'l')
        || (len == 6 && name[0] == 'n' && name[1] == 'o' && name[2] == 'w' && name[3] == 'r' && name[4] == 'a' && name[5] == 'p')
        || (len == 6 && name[0] == 's' && name[1] == 'c' && name[2] == 'o' && name[3] == 'p' && name[4] == 'e' && name[5] == 'd')
        || (len == 8 && name[0] == 's' && name[1] == 'e' && name[2] == 'a' && name[3] == 'm' && name[4] == 'l' && name[5] == 'e' && name[6] == 's' && name[7] == 's')
        || (len == 5 && name[0] == 'i' && name[1] == 's' && name[2] == 'm' && name[3] == 'a' && name[4] == 'p')
        || (len == 9 && name[0] == 'i' && name[1] == 't' && name[2] == 'e' && name[3] == 'm' && name[4] == 's' && name[5] == 'c' && name[6] == 'o' && name[7] == 'p' && name[8] == 'e');
  }

  /**
   * @brief 冗長な属性か判定する（デフォルト値と一致する場合）
   */
  auto constexpr is_redundant_attribute(char const* tag, size_t tag_len, char const* attr_name, size_t attr_len, char const* attr_val, size_t val_len) noexcept {
    // <script type="text/javascript"> → type 属性は冗長
    if (tag_len == 6 && tag[0] == 's' && tag[1] == 'c' && tag[2] == 'r' && tag[3] == 'i' && tag[4] == 'p' && tag[5] == 't' && attr_len == 4 && attr_name[0] == 't' && attr_name[1] == 'y' && attr_name[2] == 'p' && attr_name[3] == 'e') {
      if (val_len == 15 && attr_val[0] == 't' && attr_val[1] == 'e' && attr_val[2] == 'x' && attr_val[3] == 't' && attr_val[4] == '/' && attr_val[5] == 'j' && attr_val[6] == 'a' && attr_val[7] == 'v' && attr_val[8] == 'a' && attr_val[9] == 's' && attr_val[10] == 'c' && attr_val[11] == 'r' && attr_val[12] == 'i' && attr_val[13] == 'p' && attr_val[14] == 't') {
        return true;
      }
      if (val_len == 19 && attr_val[0] == 'a' && attr_val[1] == 'p' && attr_val[2] == 'p' && attr_val[3] == 'l' && attr_val[4] == 'i' && attr_val[5] == 'c' && attr_val[6] == 'a' && attr_val[7] == 't' && attr_val[8] == 'i' && attr_val[9] == 'o' && attr_val[10] == 'n' && attr_val[11] == '/' && attr_val[12] == 'j' && attr_val[13] == 'a' && attr_val[14] == 'v' && attr_val[15] == 'a' && attr_val[16] == 's' && attr_val[17] == 'c' && attr_val[18] == 'r') {
        return true;
      }
      if (val_len == 6 && attr_val[0] == 'm' && attr_val[1] == 'o' && attr_val[2] == 'd' && attr_val[3] == 'u' && attr_val[4] == 'l' && attr_val[5] == 'e') {
        return true;
      }
    }
    // <style type="text/css"> → type 属性は冗長
    if (tag_len == 5 && tag[0] == 's' && tag[1] == 't' && tag[2] == 'y' && tag[3] == 'l' && tag[4] == 'e' && attr_len == 4 && attr_name[0] == 't' && attr_name[1] == 'y' && attr_name[2] == 'p' && attr_name[3] == 'e') {
      if (val_len == 8 && attr_val[0] == 't' && attr_val[1] == 'e' && attr_val[2] == 'x' && attr_val[3] == 't' && attr_val[4] == '/' && attr_val[5] == 'c' && attr_val[6] == 's' && attr_val[7] == 's') {
        return true;
      }
    }
    // <input type="text"> → type 属性は冗長（デフォルトが text）
    if (tag_len == 5 && tag[0] == 'i' && tag[1] == 'n' && tag[2] == 'p' && tag[3] == 'u' && tag[4] == 't' && attr_len == 4 && attr_name[0] == 't' && attr_name[1] == 'y' && attr_name[2] == 'p' && attr_name[3] == 'e') {
      if (val_len == 4 && attr_val[0] == 't' && attr_val[1] == 'e' && attr_val[2] == 'x' && attr_val[3] == 't') {
        return true;
      }
    }
    // <form method="get"> → method 属性は冗長（デフォルトが get）
    if (tag_len == 4 && tag[0] == 'f' && tag[1] == 'o' && tag[2] == 'r' && tag[3] == 'm' && attr_len == 6 && attr_name[0] == 'm' && attr_name[1] == 'e' && attr_name[2] == 't' && attr_name[3] == 'h' && attr_name[4] == 'o' && attr_name[5] == 'd') {
      if (val_len == 3 && attr_val[0] == 'g' && attr_val[1] == 'e' && attr_val[2] == 't') {
        return true;
      }
    }
    return false;
  }

  /**
   * @brief 省略可能な終了タグか判定する
   */
  auto constexpr is_optional_end_tag(char const* tag, size_t tag_len) noexcept {
    // oxfmt-ignore
    return (tag_len == 2 && tag[0] == 'l' && tag[1] == 'i')
        || (tag_len == 2 && tag[0] == 'd' && tag[1] == 't')
        || (tag_len == 2 && tag[0] == 'd' && tag[1] == 'd')
        || (tag_len == 1 && tag[0] == 'p')
        || (tag_len == 2 && tag[0] == 't' && tag[1] == 'r')
        || (tag_len == 2 && tag[0] == 't' && tag[1] == 'd')
        || (tag_len == 2 && tag[0] == 't' && tag[1] == 'h')
        || (tag_len == 5 && tag[0] == 't' && tag[1] == 'h' && tag[2] == 'e' && tag[3] == 'a' && tag[4] == 'd')
        || (tag_len == 5 && tag[0] == 't' && tag[1] == 'b' && tag[2] == 'o' && tag[3] == 'd' && tag[4] == 'y')
        || (tag_len == 5 && tag[0] == 't' && tag[1] == 'f' && tag[2] == 'o' && tag[3] == 'o' && tag[4] == 't')
        || (tag_len == 6 && tag[0] == 'o' && tag[1] == 'p' && tag[2] == 't' && tag[3] == 'i' && tag[4] == 'o' && tag[5] == 'n')
        || (tag_len == 7 && tag[0] == 'o' && tag[1] == 'p' && tag[2] == 't' && tag[3] == 'g' && tag[4] == 'r' && tag[5] == 'o' && tag[6] == 'u')
        || (tag_len == 8 && tag[0] == 'c' && tag[1] == 'o' && tag[2] == 'l' && tag[3] == 'g' && tag[4] == 'r' && tag[5] == 'o' && tag[6] == 'u' && tag[7] == 'p')
        || (tag_len == 7 && tag[0] == 'c' && tag[1] == 'a' && tag[2] == 'p' && tag[3] == 't' && tag[4] == 'i' && tag[5] == 'o' && tag[6] == 'n');
  }

  /**
   * @brief 属性値がクォート不要か判定する
   */
  auto constexpr can_remove_attribute_quotes(char const* val, size_t len) noexcept {
    if (len == 0) {
      return false;
    }
    for (size_t j = 0; j < len; ++j) {
      auto const ch = val[j];
      if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\f' || ch == '\r' || ch == '"' || ch == '\'' || ch == '`' || ch == '=' || ch == '<' || ch == '>') {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief void 要素タグか判定する（閉じタグが不要な HTML5 void 要素）
   */
  auto constexpr is_void_element(char const* tag, size_t tag_len) noexcept {
    return (tag_len == 4 && tag[0] == 'a' && tag[1] == 'r' && tag[2] == 'e' && tag[3] == 'a')
        || (tag_len == 4 && tag[0] == 'b' && tag[1] == 'a' && tag[2] == 's' && tag[3] == 'e')
        || (tag_len == 2 && tag[0] == 'b' && tag[1] == 'r')
        || (tag_len == 3 && tag[0] == 'c' && tag[1] == 'o' && tag[2] == 'l')
        || (tag_len == 5 && tag[0] == 'e' && tag[1] == 'm' && tag[2] == 'b' && tag[3] == 'e' && tag[4] == 'd')
        || (tag_len == 2 && tag[0] == 'h' && tag[1] == 'r')
        || (tag_len == 3 && tag[0] == 'i' && tag[1] == 'm' && tag[2] == 'g')
        || (tag_len == 5 && tag[0] == 'i' && tag[1] == 'n' && tag[2] == 'p' && tag[3] == 'u' && tag[4] == 't')
        || (tag_len == 4 && tag[0] == 'l' && tag[1] == 'i' && tag[2] == 'n' && tag[3] == 'k')
        || (tag_len == 4 && tag[0] == 'm' && tag[1] == 'e' && tag[2] == 't' && tag[3] == 'a')
        || (tag_len == 5 && tag[0] == 'p' && tag[1] == 'a' && tag[2] == 'r' && tag[3] == 'a' && tag[4] == 'm')
        || (tag_len == 6 && tag[0] == 's' && tag[1] == 'o' && tag[2] == 'u' && tag[3] == 'r' && tag[4] == 'c' && tag[5] == 'e')
        || (tag_len == 5 && tag[0] == 't' && tag[1] == 'r' && tag[2] == 'a' && tag[3] == 'c' && tag[4] == 'k')
        || (tag_len == 3 && tag[0] == 'w' && tag[1] == 'b' && tag[2] == 'r');
  }

  /**
   * @brief タグ名が一致するか判定する（大文字小文字不区別）
   */
  auto constexpr tag_equal_ci(char const* tag, size_t tag_len, char const* ref, size_t ref_len) noexcept {
    if (tag_len != ref_len) {
      return false;
    }
    for (size_t j = 0; j < tag_len; ++j) {
      auto a = tag[j];
      auto b = ref[j];
      if (a >= 'A' && a <= 'Z') {
        a = static_cast<char>(a + 32);
      }
      if (b >= 'A' && b <= 'Z') {
        b = static_cast<char>(b + 32);
      }
      if (a != b) {
        return false;
      }
    }
    return true;
  }

  /**
   * @brief HTML/XML 本文を最小限の空白へ圧縮する内部実装
   *
   * 文字列リテラル内の文字は保持し、タグ周辺の不要空白と
   * コメント（`<!-- ... -->`）を除去します。
   * さらに、冗長な属性の削除、布爾属性の圧縮、省略可能な終了タグの除去、
   * 属性値のクォート除去を行います。
   *
   * @tparam N 文字列長（終端文字を含む）
   * @param str 入力文字列
   * @param options minify オプション（ビットフィールド）
   * @return auto 圧縮後の文字列
   */
  template <size_t N>
  [[nodiscard]] auto consteval minify_markup(FrozenString<N> const& str, minify_markup_opt options = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags) noexcept {
    // 出力バッファ位置・入力位置・クォート状態・空白遅延フラグの初期化
    auto res           = FrozenString<N>{};
    auto offset        = 0uz;
    auto i             = 0uz;
    auto in_quote      = '\0';
    auto pending_space = false;

    // 入力全体を1文字ずつ走査し、コメント除去・空白正規化・属性最適化を施しながら出力バッファに詰める
    while (i < str.length) {
      // HTML/XML コメント <!-- ... --> をブロックごとスキップする
      if (in_quote == '\0' && i + 3 < str.length && str.buffer[i] == '<' && str.buffer[i + 1] == '!' && str.buffer[i + 2] == '-' && str.buffer[i + 3] == '-') {
        i += 4;
        while (i + 2 < str.length && !(str.buffer[i] == '-' && str.buffer[i + 1] == '-' && str.buffer[i + 2] == '>')) {
          ++i;
        }
        if (i + 2 < str.length) {
          i += 3;
        }
        pending_space = true;
        continue;
      }

      auto const c = str.buffer[i];

      // クォート内は内容をそのまま保持する
      if (in_quote != '\0') {
        res.buffer[offset++] = c;
        if (c == in_quote) {
          in_quote = '\0';
        }
        ++i;
        continue;
      }

      if (c == '"' || c == '\'') {
        if (pending_space) {
          auto const prev = offset == 0 ? '\0' : res.buffer[offset - 1];
          if (prev != '\0' && prev != '<' && prev != '>' && prev != '=' && prev != '/') {
            res.buffer[offset++] = ' ';
          }
          pending_space = false;
        }
        in_quote             = c;
        res.buffer[offset++] = c;
        ++i;
        continue;
      }

      // タグ境界の前後空白は削除し、タグ内では単一空白に正規化する
      if (c == '<') {
        if (offset > 0 && res.buffer[offset - 1] == ' ') {
          --offset;
        }
        pending_space = false;

        // 閉じタグ処理: void 要素の閉じタグは常に除去し、省略可能なら minify オプションに応じて除去する
        if (i + 1 < str.length && str.buffer[i + 1] == '/') {
          // タグ名を読み取る
          auto tag_start = i + 2;
          auto tag_end   = tag_start;
          while (tag_end < str.length && str.buffer[tag_end] != '>' && !is_markup_space(str.buffer[tag_end])) {
            ++tag_end;
          }
          // '>' まで進めてタグ全体をスキップ
          auto scan = tag_end;
          while (scan < str.length && str.buffer[scan] != '>') {
            ++scan;
          }
          if (scan < str.length) {
            ++scan;
          }
          // void 要素（<br>, <img> 等）の閉じタグは HTML5 仕様上不正であるため、
          // minify_markup_opt に関わらず常にスキップする。
          if (is_void_element(str.buffer.data() + tag_start, tag_end - tag_start)) {
            i = scan;
            continue;
          }
          if (has_flag(options, minify_markup_opt::remove_end_tags) && is_optional_end_tag(str.buffer.data() + tag_start, tag_end - tag_start)) {
            i = scan;
            continue;
          }
          // 省略不可の閉じタグはそのまま出力
          for (auto k = i; k < scan; ++k) {
            res.buffer[offset++] = str.buffer[k];
          }
          i = scan;
          continue;
        }

          // 開いたタグ: 属性の最適化を行う
          if (i + 1 < str.length && str.buffer[i + 1] != '/' && str.buffer[i + 1] != '!' && str.buffer[i + 1] != '?') {
            // タグ名を読み取る（'<' の直後から空白または '>' までがタグ名）
            auto tag_start = i + 1;
            auto tag_end   = tag_start;
            while (tag_end < str.length && str.buffer[tag_end] != '>' && !is_markup_space(str.buffer[tag_end])) {
              ++tag_end;
            }
            auto const tag_len = tag_end - tag_start;

            // タグの閉じ '>' までスキャンし、タグ名・属性を順次処理する
            auto pos = tag_end;
          // '<' とタグ名を出力
          res.buffer[offset++] = '<';
          for (auto k = tag_start; k < tag_end; ++k) {
            res.buffer[offset++] = str.buffer[k];
          }

          // 属性を解析して出力する
          while (pos < str.length && str.buffer[pos] != '>') {
            // 空白をスキップして単一空白を出力
            if (is_markup_space(str.buffer[pos])) {
              // 次に有効な属性があるか先読み
              auto peek = pos;
              while (peek < str.length && is_markup_space(str.buffer[peek])) {
                ++peek;
              }
              // 次が属性名（アルファベット or '_' or ':'）の場合のみ空白を出力
              if (peek < str.length && str.buffer[peek] != '>' && str.buffer[peek] != '/') {
                // 連続空白を1つに集約（既に1つ目を出力済みならスキップ）
                if (res.buffer[offset - 1] != ' ') {
                  res.buffer[offset++] = ' ';
                }
              }
              pos = peek;
              continue;
            }

            // '/' (自己閉じタグ) なら空白を挟んでそのまま出力する
            if (str.buffer[pos] == '/') {
              if (offset > 0 && res.buffer[offset - 1] != ' ' && res.buffer[offset - 1] != '<') {
                res.buffer[offset++] = ' ';
              }
              res.buffer[offset++] = '/';
              ++pos;
              continue;
            }

            // 属性名の先頭（アルファベット・'_'・':'）を検出したら、名前・'='・値の3つ組を解析する
            if ((str.buffer[pos] >= 'a' && str.buffer[pos] <= 'z') || (str.buffer[pos] >= 'A' && str.buffer[pos] <= 'Z') || str.buffer[pos] == '_' || str.buffer[pos] == ':') {
              // '=' または空白または '>' に達するまでが属性名
              auto attr_start = pos;
              while (pos < str.length && str.buffer[pos] != '=' && str.buffer[pos] != '>' && !is_markup_space(str.buffer[pos])) {
                ++pos;
              }
              auto const attr_len = pos - attr_start;

              // '=' を探す（空白を飛ばして）
              auto eq_pos = pos;
              while (eq_pos < str.length && is_markup_space(str.buffer[eq_pos])) {
                ++eq_pos;
              }

              if (eq_pos < str.length && str.buffer[eq_pos] == '=') {
                // '=' 以降の空白を飛ばして属性値の開始位置を特定する
                auto val_start = eq_pos + 1;
                while (val_start < str.length && is_markup_space(str.buffer[val_start])) {
                  ++val_start;
                }
                // クォート付き値（"..." または '...'）か裸の値かを判定し、値範囲を切り出す
                auto val_quote = '\0';
                auto val_end   = val_start;
                if (val_start < str.length && (str.buffer[val_start] == '"' || str.buffer[val_start] == '\'')) {
                  val_quote = str.buffer[val_start];
                  ++val_end;
                  while (val_end < str.length && str.buffer[val_end] != val_quote) {
                    ++val_end;
                  }
                  if (val_end < str.length) {
                    ++val_end;
                  }
                } else {
                  while (val_end < str.length && !is_markup_space(str.buffer[val_end]) && str.buffer[val_end] != '>') {
                    ++val_end;
                  }
                }
                // 引用符を除いた値本体の範囲と長さを計算する
                auto const val_content_start = val_quote != '\0' ? val_start + 1 : val_start;
                auto const val_content_end   = val_quote != '\0' ? val_end - 1 : val_end;
                auto const val_content_len   = val_content_end - val_content_start;

                // 冗長属性チェック
                if (is_redundant_attribute(str.buffer.data() + tag_start, tag_len, str.buffer.data() + attr_start, attr_len, str.buffer.data() + val_content_start, val_content_len)) {
                  pos = val_end;
                  continue;
                }

                // 空の属性値を除去: <div class=""> → <div class>
                if (val_content_len == 0 && val_quote != '\0') {
                  for (auto k = attr_start; k < attr_start + attr_len; ++k) {
                    res.buffer[offset++] = str.buffer[k];
                  }
                  pos = val_end;
                  continue;
                }

                // 布爾属性チェック: value が属性名と同一なら value を省略
                if (val_content_len == attr_len) {
                  auto is_same = true;
                  for (size_t k = 0; k < attr_len; ++k) {
                    if (str.buffer[attr_start + k] != str.buffer[val_content_start + k]) {
                      is_same = false;
                      break;
                    }
                  }
                  if (is_same && is_boolean_attribute(str.buffer.data() + attr_start, attr_len)) {
                    // 属性名のみ出力
                    for (auto k = attr_start; k < attr_start + attr_len; ++k) {
                      res.buffer[offset++] = str.buffer[k];
                    }
                    pos = val_end;
                    continue;
                  }
                }

                // 通常の属性: 値のクォート除去を試みる
                // 属性値が安全ならクォートなしで出力
                auto can_unquote = has_flag(options, minify_markup_opt::remove_quotes) && val_quote != '\0' && can_remove_attribute_quotes(str.buffer.data() + val_content_start, val_content_len);
                if (can_unquote) {
                  // 属性名
                  for (auto k = attr_start; k < attr_start + attr_len; ++k) {
                    res.buffer[offset++] = str.buffer[k];
                  }
                  res.buffer[offset++] = '=';
                  // クォートなしの値
                  for (auto k = val_content_start; k < val_content_end; ++k) {
                    res.buffer[offset++] = str.buffer[k];
                  }
                } else {
                  // 属性名
                  for (auto k = attr_start; k < attr_start + attr_len; ++k) {
                    res.buffer[offset++] = str.buffer[k];
                  }
                  res.buffer[offset++] = '=';
                  // クォート付きの値
                  for (auto k = val_start; k < val_end; ++k) {
                    res.buffer[offset++] = str.buffer[k];
                  }
                }
                pos = val_end;
              } else {
                // 布爾属性（= なし）: そのまま出力
                for (auto k = attr_start; k < pos; ++k) {
                  res.buffer[offset++] = str.buffer[k];
                }
              }
              continue;
            }

            // その他の文字（タグ内）はそのまま出力
            res.buffer[offset++] = str.buffer[pos];
            ++pos;
          }

          // '>' を出力（不要な末尾空白を削除）
          while (offset > 0 && res.buffer[offset - 1] == ' ') {
            --offset;
          }
          if (pos < str.length && str.buffer[pos] == '>') {
            res.buffer[offset++] = '>';
            ++pos;
          }
          i = pos;
          continue;
        }

        res.buffer[offset++] = c;
        ++i;
        continue;
      }

      // テキスト中の '>' は直前に空白があれば削ってから出力する
      if (c == '>') {
        if (offset > 0 && res.buffer[offset - 1] == ' ') {
          --offset;
        }
        res.buffer[offset++] = c;
        ++i;
        continue;
      }

      // 空白文字は遅延フラグを立ててスキップ（実際の出力は次回の非空白文字で判断）
      if (is_markup_space(c)) {
        pending_space = true;
        ++i;
        continue;
      }

      // 遅延された空白を出力するか判定: 前後が記号やタグ境界でなければ1文字だけ出力する
      if (pending_space) {
        auto const prev              = offset == 0 ? '\0' : res.buffer[offset - 1];
        auto const should_emit_space = prev != '\0' && prev != '<' && prev != '>' && prev != '=' && prev != '/' && c != '>' && c != '=' && c != '/';
        if (should_emit_space) {
          res.buffer[offset++] = ' ';
        }
        pending_space = false;
      }
      res.buffer[offset++] = c;
      ++i;
    }

    // 末尾の余分な空白を除去して終端し、圧縮結果を返す
    if (offset > 0 && res.buffer[offset - 1] == ' ') {
      --offset;
    }
    res.buffer[offset] = '\0';
    res.length         = offset;
    return res;
  }

}  // namespace detail

// ─── HTML ─────────────────────────────────────────────────────────────────────

/**
 * @brief HTML 文字列を minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @param options minify オプション（ビットフィールド）
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_html(FrozenString<N> const& str, minify_markup_opt options = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags) noexcept {
  return detail::minify_markup(str, options);
}

/**
 * @brief HTML 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @param options minify オプション（ビットフィールド）
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_html(char const (&str)[N], minify_markup_opt options = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags) noexcept {
  return minify_html(FrozenString{str}, options);
}

// ─── XML ──────────────────────────────────────────────────────────────────────

/**
 * @brief XML 文字列を minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @param options minify オプション（ビットフィールド）
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_xml(FrozenString<N> const& str, minify_markup_opt options = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags) noexcept {
  return detail::minify_markup(str, options);
}

/**
 * @brief XML 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @param options minify オプション（ビットフィールド）
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_xml(char const (&str)[N], minify_markup_opt options = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags) noexcept {
  return minify_xml(FrozenString{str}, options);
}

// ─── JSON ─────────────────────────────────────────────────────────────────────

/**
 * @brief JSON 文字列を minify する
 *
 * 文字列リテラル内は保持し、リテラル外の空白とコメント
 * （行コメント `//` およびブロックコメント）を除去します。
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_json(FrozenString<N> const& str) noexcept {
  auto res       = FrozenString<N>{};
  auto offset    = 0uz;
  auto i         = 0uz;
  auto in_string = false;
  auto escaped   = false;

  while (i < str.length) {
    auto const c = str.buffer[i];
    // 文字列リテラル内部: エスケープ状態を管理しながらそのまま出力バッファにコピーする
    if (in_string) {
      res.buffer[offset++] = c;
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      ++i;
      continue;
    }

    // 文字列リテラルの開始: 引用符を出力して文字列モードに移行する
    if (c == '"') {
      in_string            = true;
      res.buffer[offset++] = c;
      ++i;
      continue;
    }

    // JSON トップレベルの行コメント // およびブロックコメント /* */ を除去する
    if (c == '/' && i + 1 < str.length && str.buffer[i + 1] == '/') {
      i += 2;
      while (i < str.length && str.buffer[i] != '\n') {
        ++i;
      }
      continue;
    }

    if (c == '/' && i + 1 < str.length && str.buffer[i + 1] == '*') {
      i += 2;
      while (i + 1 < str.length && !(str.buffer[i] == '*' && str.buffer[i + 1] == '/')) {
        ++i;
      }
      if (i + 1 < str.length) {
        i += 2;
      }
      continue;
    }

    // トップレベルの空白文字はすべてスキップする
    if (detail::is_any_whitespace(c)) {
      ++i;
      continue;
    }

    // コメント・空白以外のトークン（構造文字・数値・キーワード）はそのまま出力する
    res.buffer[offset++] = c;
    ++i;
  }

  // 末尾に null 終端を設定し圧縮後の長さを確定する
  res.buffer[offset] = '\0';
  res.length         = offset;
  return res;
}

/**
 * @brief JSON 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_json(char const (&str)[N]) noexcept {
  return minify_json(FrozenString{str});
}

// ─── YAML ─────────────────────────────────────────────────────────────────────

/**
 * @brief YAML 文字列を minify する
 *
 * インデント構造を壊さない範囲で、行末空白とコメントを削除します。
 * （引用符内の `#` は保持）
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_yaml(FrozenString<N> const& str) noexcept {
  // 出力バッファ・行先頭位置・最終非空白位置・クォート状態の初期化
  auto res            = FrozenString<N>{};
  auto offset         = 0uz;
  auto line_start     = 0uz;
  auto last_non_space = std::string_view::npos;
  auto in_single      = false;
  auto in_double      = false;
  auto escaped        = false;

  // 1行ずつ処理: コメント削除・行末空白削除・空行圧縮を行いながら出力バッファに詰める
  for (auto i = 0uz; i < str.length; ++i) {
    auto const c = str.buffer[i];
    // 非クォート領域の '#' 以降を YAML コメントとして除去する
    if (!in_single && !in_double && c == '#') {
      while (i < str.length && str.buffer[i] != '\n') {
        ++i;
      }
      if (i >= str.length) {
        break;
      }
    }

    // 行末空白を落としてから改行を出力する（空行は圧縮）
    if (i < str.length && str.buffer[i] == '\n') {
      if (last_non_space == std::string_view::npos) {
        offset = line_start;
      } else {
        offset               = last_non_space + 1;
        res.buffer[offset++] = '\n';
      }
      line_start     = offset;
      last_non_space = std::string_view::npos;
      in_single      = false;
      in_double      = false;
      escaped        = false;
      continue;
    }

    if (i >= str.length) {
      break;
    }

    auto const current   = str.buffer[i];
    res.buffer[offset++] = current;

    // クォート状態を更新し、引用符内の # はコメント扱いしない
    if (in_double) {
      if (escaped) {
        escaped = false;
      } else if (current == '\\') {
        escaped = true;
      } else if (current == '"') {
        in_double = false;
      }
    } else if (in_single) {
      if (current == '\'') {
        in_single = false;
      }
    } else {
      if (current == '"') {
        in_double = true;
      } else if (current == '\'') {
        in_single = true;
      }
    }

    if (current != ' ' && current != '\t' && current != '\r') {
      last_non_space = offset - 1;
    }
  }

  // 最終行の行末空白を除去してから終端する
  if (last_non_space == std::string_view::npos) {
    offset = line_start;
  } else {
    offset = last_non_space + 1;
  }

  res.buffer[offset] = '\0';
  res.length         = offset;
  return res;
}

/**
 * @brief YAML 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_yaml(char const (&str)[N]) noexcept {
  return minify_yaml(FrozenString{str});
}

// ─── SQL ──────────────────────────────────────────────────────────────────────

/**
 * @brief SQL 文字列を minify する
 *
 * 文字列リテラル・識別子引用を保持しつつ、コメントと不要空白を削除します。
 * shorten_types フラグを指定した場合、型キーワードも短縮します（INTEGER→INT 等）。
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @param options minify オプション（ビットフィールド）
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_sql(FrozenString<N> const& str, minify_sql_opt options = minify_sql_opt::shorten_types) noexcept {
  // 出力バッファ・入力位置・各種引用符モード・空白遅延フラグの初期化
  auto res           = FrozenString<N>{};
  auto offset        = 0uz;
  auto i             = 0uz;
  auto in_single     = false;
  auto in_double     = false;
  auto in_backtick   = false;
  auto in_bracket    = false;
  auto pending_space = false;

  /// @brief 型キーワードの短縮を試みる（shorten_types フラグが有効な場合のみ）
  auto try_shorten = [&](size_t word_start, size_t word_len) -> bool {
    if (!has_flag(options, minify_sql_opt::shorten_types)) {
      return false;
    }
    auto upper_buf = std::array<char, 256>{};
    if (word_len > upper_buf.size()) {
      return false;
    }
    for (auto j = 0uz; j < word_len; ++j) {
      auto const ch = str.buffer[word_start + j];
      upper_buf[j]  = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - ('a' - 'A')) : ch;
    }
    auto const* mapping = detail::sql_find_type_shortening(upper_buf.data(), word_len);
    if (mapping == nullptr) {
      return false;
    }
    // CHARACTER VARYING → VARCHAR の特殊処理
    if (word_len == 9 && upper_buf[0] == 'C') {
      auto peek = i;
      while (peek < str.length && detail::is_any_whitespace(str.buffer[peek])) {
        ++peek;
      }
      if (peek + 7 <= str.length) {
        auto varying_match = true;
        for (auto j = 0uz; j < 7; ++j) {
          auto const ch       = str.buffer[peek + j];
          auto const upper_ch = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - ('a' - 'A')) : ch;
          if (upper_ch != "VARYING"[j]) {
            varying_match = false;
            break;
          }
        }
        if (varying_match && peek + 7 < str.length && !detail::is_sql_id_char(str.buffer[peek + 7])) {
          constexpr char VARCHAR[] = "VARCHAR";
          for (auto j = 0uz; j < 7; ++j) {
            res.buffer[offset++] = VARCHAR[j];
          }
          i             = peek + 7;
          pending_space = true;
          return true;
        }
      }
    }
    for (auto j = 0uz; j < mapping->short_len; ++j) {
      res.buffer[offset++] = mapping->short_form[j];
    }
    return true;
  };

  // 入力を1文字ずつ走査し、引用符内部の保持・コメント除去・空白正規化・キーワード短縮を施す
  while (i < str.length) {
    auto const c = str.buffer[i];

    // シングルクォート文字列: 内部はそのまま出力し、'' エスケープも処理する
    if (in_single) {
      res.buffer[offset++] = c;
      if (c == '\'') {
        if (i + 1 < str.length && str.buffer[i + 1] == '\'') {
          res.buffer[offset++] = '\'';
          i += 2;
          continue;
        }
        in_single = false;
      }
      ++i;
      continue;
    }

    if (in_double) {
      res.buffer[offset++] = c;
      if (c == '"') {
        if (i + 1 < str.length && str.buffer[i + 1] == '"') {
          res.buffer[offset++] = '"';
          i += 2;
          continue;
        }
        in_double = false;
      }
      ++i;
      continue;
    }

    if (in_backtick) {
      res.buffer[offset++] = c;
      if (c == '`') {
        in_backtick = false;
      }
      ++i;
      continue;
    }

    if (in_bracket) {
      res.buffer[offset++] = c;
      if (c == ']') {
        in_bracket = false;
      }
      ++i;
      continue;
    }

    // ラインコメント/ブロックコメントを削除する
    if (c == '-' && i + 1 < str.length && str.buffer[i + 1] == '-') {
      i += 2;
      while (i < str.length && str.buffer[i] != '\n') {
        ++i;
      }
      pending_space = true;
      continue;
    }

    if (c == '/' && i + 1 < str.length && str.buffer[i + 1] == '*') {
      i += 2;
      while (i + 1 < str.length && !(str.buffer[i] == '*' && str.buffer[i + 1] == '/')) {
        ++i;
      }
      if (i + 1 < str.length) {
        i += 2;
      }
      pending_space = true;
      continue;
    }

    // 空白は遅延出力し、前後が記号でない場合のみ1文字出力する
    if (detail::is_any_whitespace(c)) {
      pending_space = true;
      ++i;
      continue;
    }

    if (pending_space) {
      auto const prev          = offset == 0 ? '\0' : res.buffer[offset - 1];
      auto const prev_is_punct = detail::is_sql_punct(prev);
      auto const next_is_punct = detail::is_sql_punct(c);
      auto const next_is_close = c == ')';
      if (prev != '\0' && !prev_is_punct && !next_is_punct && !next_is_close) {
        res.buffer[offset++] = ' ';
      }
      pending_space = false;
    }

    // AS キーワードの除去 / INNER JOIN の簡略化
    if ((has_flag(options, minify_sql_opt::remove_as) || has_flag(options, minify_sql_opt::simplify_join)) && detail::is_sql_id_start(c)) {
      auto const word_start = i;
      while (i < str.length && detail::is_sql_id_char(str.buffer[i])) {
        ++i;
      }
      auto const word_len = i - word_start;

      // AS キーワードを検出（remove_as フラグが設定されている場合のみ除去）
      if (has_flag(options, minify_sql_opt::remove_as) && word_len == 2) {
        auto upper0 = (str.buffer[word_start] >= 'a' && str.buffer[word_start] <= 'z') ? static_cast<char>(str.buffer[word_start] - ('a' - 'A')) : str.buffer[word_start];
        auto upper1 = (str.buffer[word_start + 1] >= 'a' && str.buffer[word_start + 1] <= 'z') ? static_cast<char>(str.buffer[word_start + 1] - ('a' - 'A')) : str.buffer[word_start + 1];
        if (upper0 == 'A' && upper1 == 'S') {
          // 次が識別子なら AS は省略可能
          auto peek = i;
          while (peek < str.length && detail::is_any_whitespace(str.buffer[peek])) {
            ++peek;
          }
          if (peek < str.length && detail::is_sql_id_start(str.buffer[peek])) {
            i = peek;
            continue;
          }
        }
      }

      // INNER JOIN の検出
      if (has_flag(options, minify_sql_opt::simplify_join) && word_len == 5) {
        auto upper_buf = std::array<char, 5>{};
        for (auto j = 0uz; j < 5; ++j) {
          auto const ch = str.buffer[word_start + j];
          upper_buf[j]  = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - ('a' - 'A')) : ch;
        }
        if (upper_buf[0] == 'I' && upper_buf[1] == 'N' && upper_buf[2] == 'N' && upper_buf[3] == 'E' && upper_buf[4] == 'R') {
          // 次が JOIN なら INNER をスキップ
          auto peek = i;
          while (peek < str.length && detail::is_any_whitespace(str.buffer[peek])) {
            ++peek;
          }
          if (peek + 4 <= str.length) {
            auto join_match = true;
            for (auto j = 0uz; j < 4; ++j) {
              auto const ch = str.buffer[peek + j];
              auto upper_ch = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - ('a' - 'A')) : ch;
              if (upper_ch != "JOIN"[j]) {
                join_match = false;
                break;
              }
            }
            if (join_match && peek + 4 < str.length && !detail::is_sql_id_char(str.buffer[peek + 4])) {
              // INNER をスキップして次の文字から処理
              i = peek;
              pending_space = false;
              continue;
            }
          }
        }
      }

      // 通常の識別子: shorten_types が有効なら短縮を試み、それ以外はそのまま出力
      if (!try_shorten(word_start, word_len)) {
        for (auto j = 0uz; j < word_len; ++j) {
          res.buffer[offset++] = str.buffer[word_start + j];
        }
      }
      continue;
    }

    // shorten_types 専用パス: 空白スキップ直後の識別子を型キーワード短縮の対象とする
    if (has_flag(options, minify_sql_opt::shorten_types) && detail::is_sql_id_start(c)) {
      auto const word_start = i;
      while (i < str.length && detail::is_sql_id_char(str.buffer[i])) {
        ++i;
      }
      auto const word_len = i - word_start;

      if (!try_shorten(word_start, word_len)) {
        for (auto j = 0uz; j < word_len; ++j) {
          res.buffer[offset++] = str.buffer[word_start + j];
        }
      }
      continue;
    }

    if (c == '\'') {
      in_single = true;
    } else if (c == '"') {
      in_double = true;
    } else if (c == '`') {
      in_backtick = true;
    } else if (c == '[') {
      in_bracket = true;
    }

    res.buffer[offset++] = c;
    ++i;
  }

  // 末尾の余分な空白を除去して終端し、圧縮結果を返す
  if (offset > 0 && res.buffer[offset - 1] == ' ') {
    --offset;
  }
  res.buffer[offset] = '\0';
  res.length         = offset;
  return res;
}

/**
 * @brief SQL 文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @param options minify オプション（ビットフィールド）
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_sql(char const (&str)[N], minify_sql_opt options = minify_sql_opt::shorten_types) noexcept {
  return minify_sql(FrozenString{str}, options);
}

// ─── Cypher ───────────────────────────────────────────────────────────────────

namespace detail {

/// @brief 文字が識別子文字 [a-zA-Z0-9_] かどうかを判定する
constexpr bool isIdentChar(char c) noexcept
{
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

/// @brief 文字列リテラル・リスト・関数呼び出しなどの終端文字かどうかを判定する
constexpr bool isCloseChar(char c) noexcept
{
  return c == '\'' || c == '"' || c == '`' || c == ')' || c == ']' || c == '}';
}

/// @brief 識別子トークンが指定キーワードと一致するかを大文字小文字を無視して判定する
constexpr bool cypher_token_eq(char const* tok, std::size_t len, char const* ref) noexcept
{
  std::size_t rlen = 0;
  while (ref[rlen] != '\0') {
    ++rlen;
  }
  if (len != rlen) {
    return false;
  }
  for (std::size_t k = 0; k < len; ++k) {
    char a = tok[k];
    if (a >= 'a' && a <= 'z') {
      a = static_cast<char>(a - ('a' - 'A'));
    }
    char b = ref[k];
    if (b >= 'a' && b <= 'z') {
      b = static_cast<char>(b - ('a' - 'A'));
    }
    if (a != b) {
      return false;
    }
  }
  return true;
}

/// @brief 直後の '(' の前に空白が必須な Cypher キーワードか判定する
/// （文法 iC_CopyTO: COPY SP '(' が SP を必須とする）
constexpr bool needs_space_before_paren(char const* tok, std::size_t len) noexcept
{
  return cypher_token_eq(tok, len, "COPY");
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
constexpr std::size_t minify_cypher(const char *input, char *output,
                                   std::size_t output_capacity) noexcept
{
  if (output_capacity == 0) {
    return 0;
  }

  using detail::isCloseChar;
  using detail::isIdentChar;
  using detail::minify_state;
  using detail::needs_space_before_paren;

  // 字句解析状態・出力長・空白遅延フラグ・最終出力文字の初期化
  auto state = minify_state::normal;
  std::size_t out_len = 0;
  bool pending_space = false;
  char last_out = '\0';

  // 直前の識別子トークン（'(' の直前に空白が必須なキーワード判定用）
  char last_id[16]{};
  std::size_t last_id_len = 0;

  // 出力ヘルパ: バッファ容量の範囲内で1文字書き込み、最終出力文字を記録する
  auto write_char = [&](char c) noexcept {
    if (out_len < output_capacity - 1) {
      output[out_len++] = c;
      last_out = c;
    }
  };

  // 入力を1文字ずつ走査: 現在の状態に応じて引用符内の保持・コメント除去・空白正規化を行う
  for (std::size_t i = 0; input[i] != '\0';) {
    char const c = input[i];
    char const next = input[i + 1];

    switch (state) {
    case minify_state::normal: {
      // 行コメント // またはブロックコメント /* */ の開始を検出する
      if (c == '/' && next == '/') {
        state = minify_state::line_comment;
        i += 2;
      } else if (c == '/' && next == '*') {
        state = minify_state::block_comment;
        i += 2;
      } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
                 c == '\f') {
        // 空白は遅延フラグを立ててスキップする
        pending_space = true;
        ++i;
      } else {
        // 遅延された空白を出力するか判定: 識別子-識別子間など必要な場合のみ空白を挿入する
        if (pending_space) {
          bool const id_to_id    = isIdentChar(last_out) && isIdentChar(c);
          bool const id_to_q     = isIdentChar(last_out) && (c == '\'' || c == '"' || c == '`');
          bool const close_to_id = isCloseChar(last_out) && isIdentChar(c);
          bool const wild_to_id  = last_out == '*' && isIdentChar(c);
          bool const id_to_wild  = isIdentChar(last_out) && c == '*';
          bool const id_to_paren = isIdentChar(last_out) && c == '(';
          bool const close_to_paren = isCloseChar(last_out) && c == '(';
          if (id_to_id || id_to_q || close_to_id || wild_to_id || id_to_wild || close_to_paren) {
            write_char(' ');
          } else if (id_to_paren && needs_space_before_paren(last_id, last_id_len)) {
            // COPY 等、直後の '(' の前に空白が必須なキーワードのみ空白を残す
            write_char(' ');
          }
        }
        pending_space = false;

        // 直前の識別子トークンを記録する（'(' の直前判定用）
        if (isIdentChar(c)) {
          if (isIdentChar(last_out)) {
            if (last_id_len < sizeof(last_id)) {
              last_id[last_id_len++] = c;
            }
          } else {
            last_id[0] = c;
            last_id_len = 1;
          }
        } else {
          last_id_len = 0;
        }

        // 引用符開始を検出したら対応する状態へ遷移し、通常文字はそのまま出力する
        if (c == '\'') {
          write_char(c);
          state = minify_state::single_quote;
        } else if (c == '"') {
          write_char(c);
          state = minify_state::double_quote;
        } else if (c == '`') {
          write_char(c);
          state = minify_state::backtick;
        } else {
          write_char(c);
        }
        ++i;
      }
      break;
    }

    case minify_state::single_quote: {
      // シングルクォート文字列: エスケープシーケンスを処理しながら内容をそのまま出力する
      write_char(c);
      if (c == '\\') {
        ++i;
        if (input[i] != '\0') {
          write_char(input[i]);
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
      // ダブルクォート文字列: エスケープシーケンスを処理しながら内容をそのまま出力する
      write_char(c);
      if (c == '\\') {
        ++i;
        if (input[i] != '\0') {
          write_char(input[i]);
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
      // バッククォート識別子: `` エスケープを処理しながら内容をそのまま出力する
      write_char(c);
      if (c == '`') {
        if (next == '`') {
          write_char(next);
          i += 2;
        } else {
          state = minify_state::normal;
          ++i;
        }
      } else {
        ++i;
      }
      break;
    }

    case minify_state::line_comment: {
      // 行コメント: 行末まで読み飛ばし、改行で通常状態に戻る
      if (c == '\n' || c == '\r') {
        state = minify_state::normal;
        pending_space = true;
      }
      ++i;
      break;
    }

    case minify_state::block_comment: {
      // ブロックコメント: */ まで読み飛ばし、終了後に通常状態に戻る
      if (c == '*' && next == '/') {
        state = minify_state::normal;
        pending_space = true;
        i += 2;
      } else {
        ++i;
      }
      break;
    }
    }
  }

  // 末尾の余分なセミコロンを除去する（Cypher クエリの末尾セミコロンは省略可能）
  if (out_len > 0 && output[out_len - 1] == ';') {
    --out_len;
  }

  // 終端文字を設定して圧縮後の長さを返す
  output[out_len] = '\0';
  return out_len;
}

/// @brief Cypher ミニファイ結果を格納する実行時コンテナ（配列 + 長さ）
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

/// @brief 実行時用 Cypher minify: コンパイル時固定長配列に結果を格納する
template <std::size_t N>
[[nodiscard]] constexpr auto minify(char const (&input)[N]) noexcept
    -> minified_query<N>
{
  minified_query<N> result{};
  result.length = minify_cypher(input, result.data.data(), N);
  return result;
}

/**
 * @brief Cypher クエリ文字列を minify する
 *
 * 内部でバッファベースの minify_cypher を呼び出し、結果を FrozenString に詰めて返す。
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_cypher(FrozenString<N> const& str) noexcept {
  auto res = FrozenString<N>{};
  auto len = minify_cypher(str.buffer.data(), res.buffer.data(), N);
  res.length = len;
  return res;
}

/**
 * @brief Cypher クエリ文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_cypher(char const (&str)[N]) noexcept {
  return minify_cypher(FrozenString{str});
}

} // namespace frozenchars
