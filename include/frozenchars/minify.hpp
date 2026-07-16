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

// ─── Lua/Luau minify オプション ─────────────────────────────────────────────

/**
 * @brief Lua/Luau minify オプション（ビットフィールド）
 */
enum class minify_lua_opt : uint8_t {
  none = 0,
  keep_directives = 1 << 0, ///< Luau 型ディレクティブ (--!strict 等) を保持する
};

inline constexpr auto operator|(minify_lua_opt a, minify_lua_opt b) noexcept {
  return static_cast<minify_lua_opt>(std::to_underlying(a) | std::to_underlying(b));
}

inline constexpr auto operator&(minify_lua_opt a, minify_lua_opt b) noexcept {
  return static_cast<minify_lua_opt>(std::to_underlying(a) & std::to_underlying(b));
}

inline constexpr auto has_flag(minify_lua_opt value, minify_lua_opt flag) noexcept -> bool {
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
/// @brief スキップすべき HTML/XML コメント <!-- ... --> を検出し、開始位置を進める
/// @return true ならコメントをスキップ済み（呼び出し側は continue すること）
constexpr auto skip_markup_comment(char const* buf, size_t len, size_t& i) noexcept -> bool {
  if (i + 3 < len && buf[i] == '<' && buf[i + 1] == '!' && buf[i + 2] == '-' && buf[i + 3] == '-') {
    i += 4;
    while (i + 2 < len && !(buf[i] == '-' && buf[i + 1] == '-' && buf[i + 2] == '>')) {
      ++i;
    }
    if (i + 2 < len) {
      i += 3;
    }
    return true;
  }
  return false;
}

/// @brief 閉じタグ </tag> を処理する
/// @return true なら出力バッファへ書き込み済み（呼び出し側は continue すること）
constexpr auto emit_close_tag(char const* buf, size_t len, size_t& i, char* out, size_t& offset, minify_markup_opt options) noexcept -> bool {
  if (i + 1 >= len || buf[i + 1] != '/') {
    return false;
  }
  auto tag_start = i + 2;
  auto tag_end   = tag_start;
  while (tag_end < len && buf[tag_end] != '>' && !is_markup_space(buf[tag_end])) {
    ++tag_end;
  }
  // タグ全体の終端 '>'（を含む）までスキャン
  auto scan = tag_end;
  while (scan < len && buf[scan] != '>') {
    ++scan;
  }
  if (scan < len) {
    ++scan;
  }
  auto const tag_len = tag_end - tag_start;
  // void 要素の閉じタグは HTML5 仕様上不正なので常に除去
  if (is_void_element(buf + tag_start, tag_len)) {
    i = scan;
    return true;
  }
  // 省略可能な終了タグはオプション指定時に除去
  if (has_flag(options, minify_markup_opt::remove_end_tags) && is_optional_end_tag(buf + tag_start, tag_len)) {
    i = scan;
    return true;
  }
  // 省略不可の閉じタグはそのまま出力
  for (auto k = i; k < scan; ++k) {
    out[offset++] = buf[k];
  }
  i = scan;
  return true;
}

/// @brief 属性値を出力バッファへ書き込む（冗長/空/布爾/クォート除去を適用）
/// @return 次に処理すべき入力位置（値の終端）
constexpr auto emit_attribute(char const* buf, size_t len, size_t attr_start, size_t attr_len, size_t eq_pos, size_t val_start, char val_quote, size_t val_end, char* out, size_t& offset, char const* tag_name, size_t tag_len, minify_markup_opt options) noexcept -> size_t {
  // '=' が無ければ布爾属性（= なし）としてそのまま出力
  if (!(eq_pos < len && buf[eq_pos] == '=')) {
    for (auto k = attr_start; k < val_start; ++k) {
      out[offset++] = buf[k];
    }
    return val_start;
  }
  // 引用符を除いた値本体の範囲を計算
  auto const val_content_start = val_quote != '\0' ? val_start + 1 : val_start;
  auto const val_content_end   = val_quote != '\0' ? val_end - 1 : val_end;
  auto const val_content_len   = val_content_end - val_content_start;

  // 冗長属性（デフォルト値と一致）は丸ごと除去
  if (is_redundant_attribute(tag_name, tag_len, buf + attr_start, attr_len, buf + val_content_start, val_content_len)) {
    return val_end;
  }
  // 空値: <div class=""> → <div class>
  if (val_content_len == 0 && val_quote != '\0') {
    for (auto k = attr_start; k < attr_start + attr_len; ++k) {
      out[offset++] = buf[k];
    }
    return val_end;
  }
  // 布爾属性: value が属性名と同一なら value を省略
  if (val_content_len == attr_len) {
    auto is_same = true;
    for (size_t k = 0; k < attr_len; ++k) {
      if (buf[attr_start + k] != buf[val_content_start + k]) {
        is_same = false;
        break;
      }
    }
    if (is_same && is_boolean_attribute(buf + attr_start, attr_len)) {
      for (auto k = attr_start; k < attr_start + attr_len; ++k) {
        out[offset++] = buf[k];
      }
      return val_end;
    }
  }
  // 通常属性: 安全ならクォートなしで出力
  auto const can_unquote = has_flag(options, minify_markup_opt::remove_quotes) && val_quote != '\0' && can_remove_attribute_quotes(buf + val_content_start, val_content_len);
  auto const value_from  = can_unquote ? val_content_start : val_start;
  auto const value_to    = can_unquote ? val_content_end : val_end;
  for (auto k = attr_start; k < attr_start + attr_len; ++k) {
    out[offset++] = buf[k];
  }
  out[offset++] = '=';
  for (auto k = value_from; k < value_to; ++k) {
    out[offset++] = buf[k];
  }
  return val_end;
}

/// @brief 開きタグ <tag ...> を処理し、属性の最適化を施して出力する
/// @return 次に処理すべき入力位置（タグの終端 '>' の直後）
constexpr auto emit_open_tag(char const* buf, size_t len, size_t i, char* out, size_t& offset, minify_markup_opt options) noexcept -> size_t {
  auto tag_start = i + 1;
  auto tag_end   = tag_start;
  while (tag_end < len && buf[tag_end] != '>' && !is_markup_space(buf[tag_end])) {
    ++tag_end;
  }
  auto const tag_len = tag_end - tag_start;

  // '<' とタグ名を出力
  out[offset++] = '<';
  for (auto k = tag_start; k < tag_end; ++k) {
    out[offset++] = buf[k];
  }

  // 属性を順次解析・出力する
  auto pos = tag_end;
  while (pos < len && buf[pos] != '>') {
    // 空白は次に有効な属性名（アルファベット/_/:）がある場合のみ単一空白に集約
    if (is_markup_space(buf[pos])) {
      auto peek = pos;
      while (peek < len && is_markup_space(buf[peek])) {
        ++peek;
      }
      if (peek < len && buf[peek] != '>' && buf[peek] != '/') {
        if (out[offset - 1] != ' ') {
          out[offset++] = ' ';
        }
      }
      pos = peek;
      continue;
    }
    // 自己閉じ '/' は空白を挟んで出力
    if (buf[pos] == '/') {
      if (offset > 0 && out[offset - 1] != ' ' && out[offset - 1] != '<') {
        out[offset++] = ' ';
      }
      out[offset++] = '/';
      ++pos;
      continue;
    }
    // 属性名の開始: 名前・'='・値を解析して emit_attribute へ委譲
    auto const is_attr_start = (buf[pos] >= 'a' && buf[pos] <= 'z') || (buf[pos] >= 'A' && buf[pos] <= 'Z') || buf[pos] == '_' || buf[pos] == ':';
    if (!is_attr_start) {
      out[offset++] = buf[pos];
      ++pos;
      continue;
    }
    auto attr_start = pos;
    while (pos < len && buf[pos] != '=' && buf[pos] != '>' && !is_markup_space(buf[pos])) {
      ++pos;
    }
    auto eq_pos = pos;
    while (eq_pos < len && is_markup_space(buf[eq_pos])) {
      ++eq_pos;
    }
    auto const has_eq = eq_pos < len && buf[eq_pos] == '=';
    auto val_start    = has_eq ? eq_pos + 1 : pos;
    while (val_start < len && is_markup_space(buf[val_start])) {
      ++val_start;
    }
    auto val_quote = '\0';
    auto val_end   = val_start;
    if (val_start < len && (buf[val_start] == '"' || buf[val_start] == '\'')) {
      val_quote = buf[val_start];
      ++val_end;
      while (val_end < len && buf[val_end] != val_quote) {
        ++val_end;
      }
      if (val_end < len) {
        ++val_end;
      }
    } else {
      while (val_end < len && !is_markup_space(buf[val_end]) && buf[val_end] != '>') {
        ++val_end;
      }
    }
    auto const attr_len = pos - attr_start;
    pos = emit_attribute(buf, len, attr_start, attr_len, eq_pos, val_start, val_quote, val_end, out, offset, buf + tag_start, tag_len, options);
  }

  // 不要な末尾空白を削って '>' を出力
  while (offset > 0 && out[offset - 1] == ' ') {
    --offset;
  }
  if (pos < len && buf[pos] == '>') {
    out[offset++] = '>';
    ++pos;
  }
  return pos;
}

/// @brief 直前に空白があれば削ってから 1 文字を出力する
constexpr void emit_trimmed(char c, char* out, size_t& offset) noexcept {
  if (offset > 0 && out[offset - 1] == ' ') {
    --offset;
  }
  out[offset++] = c;
}

/// @brief 遅延空白を出力するか判定し、必要なら 1 文字出力する
constexpr void flush_pending_space(char prev, char c, char* out, size_t& offset, bool& pending_space) noexcept {
  auto const emit = prev != '\0' && prev != '<' && prev != '>' && prev != '=' && prev != '/' && c != '>' && c != '=' && c != '/';
  if (emit) {
    out[offset++] = ' ';
  }
  pending_space = false;
}

/// @brief HTML/XML 本文を最小限の空白へ圧縮する内部実装（バッファベース）
///
/// 文字列リテラル内の文字は保持し、タグ周辺の不要空白とコメント（<!-- ... -->）を除去します。
/// さらに、冗長な属性の削除、布爾属性の圧縮、省略可能な終了タグの除去、
/// 属性値のクォート除去を行います。
///
/// @param input 入力文字列
/// @param output 出力バッファ
/// @param output_capacity 出力バッファの容量
/// @param options minify オプション（ビットフィールド）
/// @return constexpr std::size_t 圧縮後の長さ
constexpr auto minify_markup(char const* input, char* output, std::size_t output_capacity, minify_markup_opt options = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags) noexcept -> std::size_t {
  // 出力位置・入力位置・クォート状態・空白遅延フラグの初期化
  auto offset        = 0uz;
  auto i             = 0uz;
  auto in_quote      = '\0';
  auto pending_space = false;
  auto const len     = std::char_traits<char>::length(input);

  // 入力全体を1文字ずつ走査し、コメント除去・空白正規化・属性最適化を施しながら出力バッファに詰める
  while (i < len) {
    // HTML/XML コメントをブロックごとスキップ
    if (in_quote == '\0' && skip_markup_comment(input, len, i)) {
      pending_space = true;
      continue;
    }

    auto const c = input[i];

    // クォート内は内容をそのまま保持する
    if (in_quote != '\0') {
      if (offset < output_capacity) {
        output[offset++] = c;
      }
      if (c == in_quote) {
        in_quote = '\0';
      }
      ++i;
      continue;
    }

    if (c == '"' || c == '\'') {
      if (pending_space && offset > 0) {
        auto const prev = output[offset - 1];
        if (prev != '\0' && prev != '<' && prev != '>' && prev != '=' && prev != '/') {
          output[offset++] = ' ';
        }
        pending_space = false;
      }
      in_quote = c;
      if (offset < output_capacity) {
        output[offset++] = c;
      }
      ++i;
      continue;
    }

    // タグ境界の前後空白は削除し、タグ内では単一空白に正規化する
    if (c == '<') {
      if (offset > 0 && output[offset - 1] == ' ') {
        --offset;
      }
      pending_space = false;

      // 閉じタグ処理（void / 省略可能ならスキップ、それ以外は出力）
      if (emit_close_tag(input, len, i, output, offset, options)) {
        continue;
      }
      // 開いたタグ: <! や <? 等はそのまま出力、通常タグは属性最適化
      if (i + 1 < len && input[i + 1] != '/' && input[i + 1] != '!' && input[i + 1] != '?') {
        i = emit_open_tag(input, len, i, output, offset, options);
        continue;
      }

      if (offset < output_capacity) {
        output[offset++] = c;
      }
      ++i;
      continue;
    }

    // テキスト中の '>' は直前に空白があれば削ってから出力する
    if (c == '>') {
      emit_trimmed(c, output, offset);
      ++i;
      continue;
    }

    // 空白文字は遅延フラグを立ててスキップ
    if (is_markup_space(c)) {
      pending_space = true;
      ++i;
      continue;
    }

    // 遅延された空白を出力するか判定してから文字を出力
    if (pending_space && offset > 0) {
      flush_pending_space(output[offset - 1], c, output, offset, pending_space);
    }
    if (offset < output_capacity) {
      output[offset++] = c;
    }
    ++i;
  }

  // 末尾の余分な空白を除去して終端し、圧縮結果を返す
  if (offset > 0 && output[offset - 1] == ' ') {
    --offset;
  }
  if (offset < output_capacity) {
    output[offset] = '\0';
  }
  return offset;
}

/// @brief HTML/XML 本文を最小限の空白へ圧縮する内部実装（FrozenString ラッパ）
///
/// @tparam N 文字列長（終端文字を含む）
/// @param str 入力文字列
/// @param options minify オプション（ビットフィールド）
/// @return auto 圧縮後の文字列
template <size_t N>
[[nodiscard]] auto consteval minify_markup(FrozenString<N> const& str, minify_markup_opt options = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags) noexcept {
  auto res = FrozenString<N>{};
  res.length = detail::minify_markup(str.buffer.data(), res.buffer.data(), N, options);
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
namespace detail {

/// @brief 識別子トークンを大文字に正規化して buf に書き込む（長さ word_len）
constexpr void to_upper_into(char const* src, size_t word_len, char* buf) noexcept {
  for (auto j = 0uz; j < word_len; ++j) {
    auto const ch = src[j];
    buf[j]        = (ch >= 'a' && ch <= 'z') ? static_cast<char>(ch - ('a' - 'A')) : ch;
  }
}

/// @brief 型キーワードの短縮を試みる（shorten_types が無効なら常に失敗）
///
/// 短縮できた場合は出力バッファへ短縮形を書き込み、入力を消費済みの
/// 位置へ @p i を進めて true を返す。できなかった場合は false を返す。
constexpr auto sql_try_shorten(char const* input, size_t len, size_t& i, size_t word_start, size_t word_len, minify_sql_opt options, char* out, size_t& offset, bool& pending_space) noexcept -> bool {
  if (!has_flag(options, minify_sql_opt::shorten_types)) {
    return false;
  }
  auto upper_buf = std::array<char, 256>{};
  if (word_len > upper_buf.size()) {
    return false;
  }
  to_upper_into(input + word_start, word_len, upper_buf.data());

  auto const* mapping = detail::sql_find_type_shortening(upper_buf.data(), word_len);
  if (mapping == nullptr) {
    return false;
  }
  // CHARACTER VARYING → VARCHAR の特殊処理
  if (word_len == 9 && upper_buf[0] == 'C') {
    auto peek = i;
    while (peek < len && detail::is_any_whitespace(input[peek])) {
      ++peek;
    }
    if (peek + 7 < len && !detail::is_sql_id_char(input[peek + 7])) {
      auto varying_match = true;
      for (auto j = 0uz; j < 7; ++j) {
        auto const upper_ch = (input[peek + j] >= 'a' && input[peek + j] <= 'z') ? static_cast<char>(input[peek + j] - ('a' - 'A')) : input[peek + j];
        if (upper_ch != "VARYING"[j]) {
          varying_match = false;
          break;
        }
      }
      if (varying_match) {
        constexpr char VARCHAR[] = "VARCHAR";
        for (auto j = 0uz; j < 7; ++j) {
          out[offset++] = VARCHAR[j];
        }
        i             = peek + 7;
        pending_space = true;
        return true;
      }
    }
  }
  for (auto j = 0uz; j < mapping->short_len; ++j) {
    out[offset++] = mapping->short_form[j];
  }
  return true;
}

/// @brief 識別子トークンから AS 除去 / INNER JOIN 簡略化を試みる
/// @return 最適化を適用して入力を消費したら true（呼び出し側は continue）
constexpr auto sql_optimize_keyword(char const* input, size_t len, size_t& i, size_t word_start, size_t word_len, minify_sql_opt options) noexcept -> bool {
  // AS キーワードの除去（次が識別子なら AS をスキップ）
  if (has_flag(options, minify_sql_opt::remove_as) && word_len == 2) {
    auto const u0 = (input[word_start] >= 'a' && input[word_start] <= 'z') ? static_cast<char>(input[word_start] - ('a' - 'A')) : input[word_start];
    auto const u1 = (input[word_start + 1] >= 'a' && input[word_start + 1] <= 'z') ? static_cast<char>(input[word_start + 1] - ('a' - 'A')) : input[word_start + 1];
    if (u0 == 'A' && u1 == 'S') {
      auto peek = i;
      while (peek < len && detail::is_any_whitespace(input[peek])) {
        ++peek;
      }
      if (peek < len && detail::is_sql_id_start(input[peek])) {
        i = peek;
        return true;
      }
    }
  }
  // INNER JOIN の簡略化（INNER をスキップして次の JOIN から処理）
  if (has_flag(options, minify_sql_opt::simplify_join) && word_len == 5) {
    auto upper = std::array<char, 5>{};
    to_upper_into(input + word_start, 5, upper.data());
    if (upper[0] == 'I' && upper[1] == 'N' && upper[2] == 'N' && upper[3] == 'E' && upper[4] == 'R') {
      auto peek = i;
      while (peek < len && detail::is_any_whitespace(input[peek])) {
        ++peek;
      }
      if (peek + 4 < len && !detail::is_sql_id_char(input[peek + 4])) {
        auto join_match = true;
        for (auto j = 0uz; j < 4; ++j) {
          auto const upper_ch = (input[peek + j] >= 'a' && input[peek + j] <= 'z') ? static_cast<char>(input[peek + j] - ('a' - 'A')) : input[peek + j];
          if (upper_ch != "JOIN"[j]) {
            join_match = false;
            break;
          }
        }
        if (join_match) {
          i = peek;
          return true;
        }
      }
    }
  }
  return false;
}

/// @brief 識別子トークンを出力する（型短縮を試み、失敗ならそのまま出力）
constexpr void sql_emit_identifier(char const* input, size_t len, size_t& i, size_t word_start, size_t word_len, minify_sql_opt options, char* out, size_t& offset, bool& pending_space) noexcept {
  if (sql_try_shorten(input, len, i, word_start, word_len, options, out, offset, pending_space)) {
    return;
  }
  for (auto j = 0uz; j < word_len; ++j) {
    out[offset++] = input[word_start + j];
  }
}

/// @brief SQL 文字列を minify する（バッファベース）
///
/// 文字列リテラル・識別子引用を保持しつつ、コメントと不要空白を削除します。
/// shorten_types フラグを指定した場合、型キーワードも短縮します（INTEGER→INT 等）。
///
/// @param input 入力文字列
/// @param output 出力バッファ
/// @param output_capacity 出力バッファの容量
/// @param options minify オプション（ビットフィールド）
/// @return constexpr std::size_t 圧縮後の長さ
constexpr auto minify_sql(char const* input, char* output, std::size_t output_capacity, minify_sql_opt options = minify_sql_opt::shorten_types) noexcept -> std::size_t {
  // 出力位置・入力位置・各種引用符モード・空白遅延フラグの初期化
  auto offset        = 0uz;
  auto i             = 0uz;
  auto in_single     = false;
  auto in_double     = false;
  auto in_backtick   = false;
  auto in_bracket    = false;
  auto pending_space = false;
  auto const len     = std::char_traits<char>::length(input);

  // 入力を1文字ずつ走査し、引用符内部の保持・コメント除去・空白正規化・キーワード短縮を施す
  while (i < len) {
    auto const c = input[i];

    // 引用符内部は内容をそのまま保持し、エスケープを処理する
    if (in_single) {
      if (offset < output_capacity) {
        output[offset++] = c;
      }
      if (c == '\'') {
        if (i + 1 < len && input[i + 1] == '\'') {
          if (offset < output_capacity) {
            output[offset++] = '\'';
          }
          i += 2;
          continue;
        }
        in_single = false;
      }
      ++i;
      continue;
    }
    if (in_double) {
      if (offset < output_capacity) {
        output[offset++] = c;
      }
      if (c == '"') {
        if (i + 1 < len && input[i + 1] == '"') {
          if (offset < output_capacity) {
            output[offset++] = '"';
          }
          i += 2;
          continue;
        }
        in_double = false;
      }
      ++i;
      continue;
    }
    if (in_backtick) {
      if (offset < output_capacity) {
        output[offset++] = c;
      }
      if (c == '`') {
        in_backtick = false;
      }
      ++i;
      continue;
    }
    if (in_bracket) {
      if (offset < output_capacity) {
        output[offset++] = c;
      }
      if (c == ']') {
        in_bracket = false;
      }
      ++i;
      continue;
    }

    // ラインコメント --  / ブロックコメント /* */ を削除する
    if (c == '-' && i + 1 < len && input[i + 1] == '-') {
      i += 2;
      while (i < len && input[i] != '\n') {
        ++i;
      }
      pending_space = true;
      continue;
    }
    if (c == '/' && i + 1 < len && input[i + 1] == '*') {
      i += 2;
      while (i + 1 < len && !(input[i] == '*' && input[i + 1] == '/')) {
        ++i;
      }
      if (i + 1 < len) {
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
      auto const prev          = offset == 0 ? '\0' : output[offset - 1];
      auto const prev_is_punct = detail::is_sql_punct(prev);
      auto const next_is_punct = detail::is_sql_punct(c);
      auto const next_is_close = c == ')';
      if (prev != '\0' && !prev_is_punct && !next_is_punct && !next_is_close) {
        if (offset < output_capacity) {
          output[offset++] = ' ';
        }
      }
      pending_space = false;
    }

    // 識別子トークンの開始: AS 除去 / INNER JOIN 簡略化 / 型短縮を順に試みる
    if (detail::is_sql_id_start(c)) {
      auto const word_start = i;
      while (i < len && detail::is_sql_id_char(input[i])) {
        ++i;
      }
      auto const word_len = i - word_start;
      if ((has_flag(options, minify_sql_opt::remove_as) || has_flag(options, minify_sql_opt::simplify_join)) && sql_optimize_keyword(input, len, i, word_start, word_len, options)) {
        continue;
      }
      sql_emit_identifier(input, len, i, word_start, word_len, options, output, offset, pending_space);
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
    if (offset < output_capacity) {
      output[offset++] = c;
    }
    ++i;
  }

  // 末尾の余分な空白を除去して終端し、圧縮結果を返す
  if (offset > 0 && output[offset - 1] == ' ') {
    --offset;
  }
  if (offset < output_capacity) {
    output[offset] = '\0';
  }
  return offset;
}

}  // namespace detail

/// @brief SQL 文字列を minify する（FrozenString ラッパ）
///
/// @tparam N 文字列長（終端文字を含む）
/// @param str 対象文字列
/// @param options minify オプション（ビットフィールド）
/// @return auto minify 後の文字列
template <size_t N>
[[nodiscard]] auto consteval minify_sql(FrozenString<N> const& str, minify_sql_opt options = minify_sql_opt::shorten_types) noexcept {
  auto res = FrozenString<N>{};
  res.length = detail::minify_sql(str.buffer.data(), res.buffer.data(), N, options);
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
constexpr bool isIdentChar(char c) noexcept {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '_';
}

/// @brief 文字列リテラル・リスト・関数呼び出しなどの終端文字かどうかを判定する
constexpr bool isCloseChar(char c) noexcept {
  return c == '\'' || c == '"' || c == '`' || c == ')' || c == ']' || c == '}';
}

/// @brief 識別子トークンが指定キーワードと一致するかを大文字小文字を無視して判定する
constexpr bool cypher_token_eq(char const* tok, std::size_t len, char const* ref) noexcept {
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
constexpr bool needs_space_before_paren(char const* tok, std::size_t len) noexcept {
  return cypher_token_eq(tok, len, "COPY");
}

/// @brief 直前の空白を維持すべきか判定する（識別子-識別子間など必要な場合のみ空白を挿入）
constexpr bool cypher_needs_space(char last_out, char c, char const* last_id, std::size_t last_id_len) noexcept {
  auto const id_to_id      = isIdentChar(last_out) && isIdentChar(c);
  auto const id_to_q       = isIdentChar(last_out) && (c == '\'' || c == '"' || c == '`');
  auto const close_to_id   = isCloseChar(last_out) && isIdentChar(c);
  auto const wild_to_id    = last_out == '*' && isIdentChar(c);
  auto const id_to_wild    = isIdentChar(last_out) && c == '*';
  auto const id_to_paren   = isIdentChar(last_out) && c == '(';
  auto const close_to_paren = isCloseChar(last_out) && c == '(';
  if (id_to_id || id_to_q || close_to_id || wild_to_id || id_to_wild || close_to_paren) {
    return true;
  }
  // COPY 等、直後の '(' の前に空白が必須なキーワードのみ空白を残す
  return id_to_paren && needs_space_before_paren(last_id, last_id_len);
}

/// @brief Lua 用: 直前の空白を維持すべきか判定する（識別子-識別子間など）
constexpr bool lua_needs_space(char last_out, char c) noexcept {
  auto const id_to_id    = isIdentChar(last_out) && isIdentChar(c);
  auto const id_to_q     = isIdentChar(last_out) && (c == '\'' || c == '"');
  auto const close_to_id = isCloseChar(last_out) && isIdentChar(c);
  return id_to_id || id_to_q || close_to_id;
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

/// @brief 位置 i の '[' から始まる長括弧 `[=*[` を検出する
/// @param input 入力文字列
/// @param i 調査開始位置（'[' を指すこと）
/// @return 有効なら '=' の数（レベル）、無効なら -1
constexpr int long_bracket_level(char const *input, std::size_t i) noexcept {
  // input[i] は '[' を想定。i+1 から '=' が連続し、直後に '[' があれば長括弧開始。
  std::size_t j = i + 1;
  while (input[j] == '=') {
    ++j;
  }
  if (input[j] != '[') {
    return -1;
  }
  return static_cast<int>(j - (i + 1)); // レベル = '=' の数
}

/// @brief レベル level の長括弧閉じ `]=*]` の位置を検索する
/// @param input 入力文字列
/// @param from 検索開始位置
/// @param level 期待レベル
/// @return 閉じ位置（']' の添字）、見つからなければ npos
constexpr std::size_t find_long_bracket_close(char const *input, std::size_t from, int level) noexcept {
  for (std::size_t i = from; input[i] != '\0'; ++i) {
    if (input[i] != ']') {
      continue;
    }
    std::size_t j = i + 1;
    int k = 0;
    while (k < level && input[j] == '=') {
      ++j;
      ++k;
    }
    if (k == level && input[j] == ']') {
      return i;
    }
  }
  return static_cast<std::size_t>(-1);
}

} // namespace detail

/// @brief Lua/Luau ソースをミニファイする（コンパイル時・実行時共用）
constexpr std::size_t minify_lua(const char *input, char *output,
                                 std::size_t output_capacity,
                                 minify_lua_opt options = minify_lua_opt::none) noexcept;

/// @brief Cypher クエリをミニファイする（コンパイル時・実行時共用）
constexpr std::size_t minify_cypher(const char *input, char *output,
                                   std::size_t output_capacity) noexcept {
  if (output_capacity == 0) {
    return 0;
  }

  using detail::cypher_needs_space;
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
      } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f') {
        // 空白は遅延フラグを立ててスキップする
        pending_space = true;
        ++i;
      } else {
        // 遅延された空白を出力するか判定: 識別子-識別子間など必要な場合のみ空白を挿入する
        if (pending_space && cypher_needs_space(last_out, c, last_id, last_id_len)) {
          write_char(' ');
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

// ─── Lua ──────────────────────────────────────────────────────────────────────

namespace detail {

  /// @brief Lua/Luau ミニファイ用の字句解析状態
enum class minify_lua_state : unsigned char {
  normal,        ///< 通常コード
  single_quote,  ///< シングルクォート文字列 '...'
  double_quote,  ///< ダブルクォート文字列 "..."
  line_comment,  ///< 行コメント -- ～ 行末
  long_comment,  ///< 長括弧コメント --[=*[ ... ]=*]
  long_string,   ///< 長括弧文字列 [=*[ ... ]=*]（内容をそのまま保持）
  directive_line, ///< Luau 型ディレクティブ --!...（keep_directives 時のみ）
};

} // namespace detail

/**
 * @brief Lua/Luau ソースをミニファイする
 *
 * @param input 入力文字列（終端文字を含む）
 * @param output 出力バッファ
 * @param output_capacity 出力バッファの容量
 * @param options ミニファイオプション
 * @return constexpr std::size_t 圧縮後の長さ
 */
constexpr std::size_t minify_lua(const char *input, char *output,
                                 std::size_t output_capacity,
                                 minify_lua_opt options) noexcept {
  if (output_capacity == 0) {
    return 0;
  }

  using detail::find_long_bracket_close;
  using detail::isCloseChar;
  using detail::isIdentChar;
  using detail::long_bracket_level;
  using detail::lua_needs_space;
  using detail::minify_lua_state;

  auto state = minify_lua_state::normal;
  std::size_t out_len = 0;
  bool pending_space = false;
  char last_out = '\0';

  // ponytail: Lua 5.2+ の「--[[ で対応する ]] が無い場合は行コメント扱い」という
  // Edge case は捨て、Luau セマンティクス（常に長括弧扱い、EOF まで close が無ければ
  // EOF まで長コメント/文字列）に統一。
  auto write_char = [&](char c) noexcept {
    if (out_len < output_capacity - 1) {
      output[out_len++] = c;
      last_out = c;
    }
  };

  for (std::size_t i = 0; input[i] != '\0';) {
    char const c = input[i];
    char const next = input[i + 1];

    switch (state) {
    case minify_lua_state::normal: {
      if (c == '-' && next == '-') {
        // 長括弧コメント --[=*[ ... ]=*] の開始を検出する
        if (input[i + 2] == '[') {
          int const level = long_bracket_level(input, i + 2);
          if (level >= 0) {
            std::size_t const close = find_long_bracket_close(input, i + 2, level);
            if (close != static_cast<std::size_t>(-1)) {
              i = close + static_cast<std::size_t>(level) + 2; // close の直後へ
              state = minify_lua_state::normal;
              pending_space = true;
              break;
            }
          }
        }
        // Luau 型ディレクティブ --!strict 等を保持する（オプション時のみ）
        if (has_flag(options, minify_lua_opt::keep_directives) && input[i + 2] == '!') {
          write_char('-');
          write_char('-');
          state = minify_lua_state::directive_line;
          i += 2;
          break;
        }
        // それ以外は行コメント -- ～ 行末
        state = minify_lua_state::line_comment;
        i += 2;
      } else if (c == '[') {
        // 長括弧文字列 [=*[ ... ]=*] の開始を検出する（内容はそのまま保持）
        int const level = long_bracket_level(input, i);
        if (level >= 0) {
          std::size_t const close = find_long_bracket_close(input, i, level);
          if (close != static_cast<std::size_t>(-1)) {
            for (std::size_t k = i; k <= close + static_cast<std::size_t>(level) + 1; ++k) {
              write_char(input[k]); // 開き～閉じまで verbatim 出力
            }
            i = close + static_cast<std::size_t>(level) + 2;
            state = minify_lua_state::normal;
            break;
          }
        }
        // 普通の '['（テーブル索引 t[1] 等）として通常文字を出力する
        if (pending_space && isIdentChar(last_out)) {
          write_char(' ');
        }
        pending_space = false;
        write_char(c);
        ++i;
      } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f') {
        pending_space = true;
        ++i;
      } else {
        // 遅延された空白を出力するか判定: 識別子-識別子間など必要な場合のみ空白を挿入する
        if (pending_space && lua_needs_space(last_out, c)) {
          write_char(' ');
        }
        pending_space = false;

        // 引用符開始を検出したら対応する状態へ遷移し、通常文字はそのまま出力する
        if (c == '\'') {
          write_char(c);
          state = minify_lua_state::single_quote;
        } else if (c == '"') {
          write_char(c);
          state = minify_lua_state::double_quote;
        } else {
          write_char(c);
        }
        ++i;
      }
      break;
    }

    case minify_lua_state::single_quote: {
      write_char(c);
      if (c == '\\') {
        ++i;
        if (input[i] != '\0') {
          write_char(input[i]);
          ++i;
        }
      } else if (c == '\'') {
        state = minify_lua_state::normal;
        ++i;
      } else {
        ++i;
      }
      break;
    }

    case minify_lua_state::double_quote: {
      write_char(c);
      if (c == '\\') {
        ++i;
        if (input[i] != '\0') {
          write_char(input[i]);
          ++i;
        }
      } else if (c == '"') {
        state = minify_lua_state::normal;
        ++i;
      } else {
        ++i;
      }
      break;
    }

    case minify_lua_state::line_comment: {
      // 行コメント: 行末まで読み飛ばし、改行で通常状態に戻る
      if (c == '\n' || c == '\r') {
        state = minify_lua_state::normal;
        pending_space = true;
      }
      ++i;
      break;
    }

    case minify_lua_state::directive_line: {
      // Luau 型ディレクティブ: --! から行末までをそのまま出力する
      write_char(c);
      if (c == '\n' || c == '\r') {
        state = minify_lua_state::normal;
        pending_space = true;
      }
      ++i;
      break;
    }

    case minify_lua_state::long_comment:
    case minify_lua_state::long_string: {
      // 到達しない: 長括弧は normal 状態で一括スキップ/出力する
      ++i;
      break;
    }
    }
  }

  // 終端文字を設定して圧縮後の長さを返す（Lua では末尾 ';' は意味を持つため除去しない）
  output[out_len] = '\0';
  return out_len;
}

/**
 * @brief Lua/Luau ソース文字列を minify する
 *
 * 内部でバッファベースの minify_lua を呼び出し、結果を FrozenString に詰めて返す。
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列
 * @param options minify オプション（デフォルト: none）
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_lua(FrozenString<N> const& str,
                                        minify_lua_opt options = minify_lua_opt::none) noexcept {
  auto res = FrozenString<N>{};
  auto len = minify_lua(str.buffer.data(), res.buffer.data(), N, options);
  res.length = len;
  return res;
}

/**
 * @brief Lua/Luau ソース文字列リテラルを minify する
 *
 * @tparam N 文字列長（終端文字を含む）
 * @param str 対象文字列リテラル
 * @param options minify オプション（デフォルト: none）
 * @return auto minify 後の文字列
 */
template <size_t N>
[[nodiscard]] auto consteval minify_lua(char const (&str)[N],
                                        minify_lua_opt options = minify_lua_opt::none) noexcept {
  return minify_lua(FrozenString{str}, options);
}

} // namespace frozenchars
