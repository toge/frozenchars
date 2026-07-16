#pragma once

#include <array>
#include <cstddef>
#include <string_view>

#include "case_conv.hpp"
#include "encoding.hpp"
#include "multiline.hpp"
#include "regex_comment.hpp"
#include "string.hpp"
#include "string_ops.hpp"
#include "minify.hpp"

namespace frozenchars::ops {

/**
 * @brief 述語 Pred を満たす文字を両端から除去するパイプアダプタ
 * @tparam Pred 文字判定述語（デフォルト: is_space_char）
 */
template <auto Pred = detail::is_space_char>
struct trim_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::trim_if<Pred>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::trim_if<Pred>(FrozenString{str});
  }
};

/**
 * @brief 述語 Pred を満たす文字を左端から除去するパイプアダプタ
 * @tparam Pred 文字判定述語（デフォルト: is_space_char）
 */
template <auto Pred = detail::is_space_char>
struct ltrim_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::ltrim_if<Pred>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::ltrim_if<Pred>(FrozenString{str});
  }
};

/**
 * @brief 述語 Pred を満たす文字を右端から除去するパイプアダプタ
 * @tparam Pred 文字判定述語（デフォルト: is_space_char）
 */
template <auto Pred = detail::is_space_char>
struct rtrim_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::rtrim_if<Pred>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::rtrim_if<Pred>(FrozenString{str});
  }
};

/**
 * @brief ASCII 英字を大文字に変換するパイプアダプタ
 */
struct toupper_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::toupper(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::toupper(FrozenString{str});
  }
};

/**
 * @brief ASCII 英字を小文字に変換するパイプアダプタ
 */
struct tolower_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::tolower(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::tolower(FrozenString{str});
  }
};

/**
 * @brief 述語 Pred を満たす連続文字を1文字に畳み込むパイプアダプタ
 * @tparam Pred 文字判定述語（デフォルト: is_space_char）
 */
template <auto Pred = detail::is_space_char>
struct collapse_spaces_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::collapse_spaces_if<Pred>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::collapse_spaces_if<Pred>(FrozenString{str});
  }
};

/**
 * @brief 部分文字列を切り出すパイプアダプタ
 */
struct substr_adaptor : pipe_adaptor_base {
  std::size_t    pos;  ///< 切り出し開始位置
  std::ptrdiff_t len;  ///< 切り出し長（負数で左方向への切り出し）

  constexpr substr_adaptor(std::size_t p, std::ptrdiff_t l) noexcept : pos(p), len(l) {}

  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::substr(str, pos, len);
  }
};

/**
 * @brief 先頭文字を大文字、残りを小文字に変換するパイプアダプタ
 */
struct capitalize_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::capitalize(str);
  }
};

/**
 * @brief snake_case を camelCase に変換するパイプアダプタ
 */
struct to_snake_case_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_snake_case(str);
  }
};

/**
 * @brief snake_case を camelCase に変換するパイプアダプタ
 */
struct to_camel_case_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_camel_case(str);
  }
};

/**
 * @brief snake_case を PascalCase に変換するパイプアダプタ
 */
struct to_pascal_case_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::to_pascal_case(str);
  }
};

/**
 * @brief 各行の先頭から述語 Pred を満たす文字を n 個除去するパイプアダプタ
 * @tparam Pred 文字判定述語（デフォルト: is_space_char）
 */
template <auto Pred = detail::is_space_char>
struct remove_leading_spaces_adaptor : pipe_adaptor_base {
  size_t n;  ///< 除去する文字数（0 で全除去）
  constexpr remove_leading_spaces_adaptor(size_t count = 0) noexcept : n(count) {}
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_leading_spaces_if<Pred>(str, n);
  }
  [[nodiscard]] consteval auto operator()(size_t count) const noexcept { return remove_leading_spaces_adaptor<Pred>{count}; }
};

/**
 * @brief 指定文字列で始まる行を丸ごと削除するパイプアダプタ
 */
struct remove_comment_lines_adaptor : pipe_adaptor_base {
  std::string_view comment_seq;  ///< コメント行の開始文字列（デフォルト "#"）
  constexpr remove_comment_lines_adaptor(std::string_view seq = "#") noexcept : comment_seq(seq) {}
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_comment_lines(str, comment_seq);
  }
};

/**
 * @brief 各行で指定文字列以降（行末まで）を削除するパイプアダプタ
 */
struct remove_comments_adaptor : pipe_adaptor_base {
  std::string_view comment_seq;  ///< コメント開始文字列（デフォルト "#"）
  constexpr remove_comments_adaptor(std::string_view seq = "#") noexcept : comment_seq(seq) {}
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_comments(str, comment_seq);
  }
};

/**
 * @brief 各行の末尾から述語 Pred を満たす文字を n 個除去するパイプアダプタ
 * @tparam Pred 文字判定述語（デフォルト: is_space_char）
 */
template <auto Pred = detail::is_space_char>
struct remove_trailing_spaces_adaptor : pipe_adaptor_base {
  size_t n;  ///< 除去する文字数（0 で全除去）
  constexpr remove_trailing_spaces_adaptor(size_t count = 0) noexcept : n(count) {}
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_trailing_spaces_if<Pred>(str, n);
  }
  [[nodiscard]] consteval auto operator()(size_t count) const noexcept { return remove_trailing_spaces_adaptor<Pred>{count}; }
};

/**
 * @brief 開始文字列から終了文字列までの範囲（ブロックコメント等）を除去するパイプアダプタ
 */
struct remove_range_comments_adaptor : pipe_adaptor_base {
  std::string_view start_seq;  ///< 範囲開始文字列
  std::string_view end_seq;    ///< 範囲終了文字列
  constexpr remove_range_comments_adaptor(std::string_view start, std::string_view end) noexcept : start_seq(start), end_seq(end) {}

  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_range_comments(str, start_seq, end_seq);
  }
};

/**
 * @brief 拡張正規表現（(?x) mode）の # コメントと余白を除去するパイプアダプタ
 */
struct remove_regex_comment_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_regex_comment(str);
  }

  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::remove_regex_comment(FrozenString{str});
  }
};

/**
 * @brief 全行をセパレータで連結するパイプアダプタ
 */
struct join_lines_adaptor : pipe_adaptor_base {
  std::string_view sep;  ///< 行間のセパレータ文字列（デフォルト空文字）
  constexpr join_lines_adaptor(std::string_view s = "") noexcept : sep(s) {}

  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const {
    return frozenchars::join_lines(str, sep);
  }

  [[nodiscard]] consteval auto operator()(std::string_view s) const noexcept { return join_lines_adaptor{s}; }
};

/**
 * @brief NTTP でセパレータを指定する行連結パイプアダプタ
 * @tparam Sep 区切り文字列（NTTP）
 */
template <FrozenString Sep>
struct join_lines_nttp_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
#if defined(_MSC_VER)
    return frozenchars::join_lines(str, Sep);
#else
    return frozenchars::join_lines<Sep>(str);
#endif
  }
};

/**
 * @brief 全行の末尾空白文字を除去するパイプアダプタ
 */
struct trim_trailing_spaces_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::trim_trailing_spaces(str);
  }
};

/**
 * @brief 空行をすべて削除するパイプアダプタ
 */
struct remove_empty_lines_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_empty_lines(str);
  }
};

/**
 * @brief 先頭の連続する空行を n 行だけ残して削除するパイプアダプタ
 */
struct remove_leading_empty_lines_adaptor : pipe_adaptor_base {
  size_t n;  ///< 残す空行数（0 で全削除）
  constexpr remove_leading_empty_lines_adaptor(size_t count = 0) noexcept : n(count) {}

  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_leading_empty_lines(str, n);
  }

  [[nodiscard]] consteval auto operator()(size_t count) const noexcept { return remove_leading_empty_lines_adaptor{count}; }
};

/**
 * @brief 末尾の連続する空行を n 行だけ残して削除するパイプアダプタ
 */
struct remove_trailing_empty_lines_adaptor : pipe_adaptor_base {
  size_t n;  ///< 残す空行数（0 で全削除）
  constexpr remove_trailing_empty_lines_adaptor(size_t count = 0) noexcept : n(count) {}

  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::remove_trailing_empty_lines(str, n);
  }

  [[nodiscard]] consteval auto operator()(size_t count) const noexcept { return remove_trailing_empty_lines_adaptor{count}; }
};

/**
 * @brief 連続する空行を1行にまとめるパイプアダプタ
 */
struct collapse_empty_lines_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::collapse_empty_lines(str);
  }
};

/**
 * @brief 各行の先頭に指定文字列を付加するパイプアダプタ
 * @tparam M 接頭辞のバッファ長
 */
template <size_t M>
struct prefix_lines_adaptor : pipe_adaptor_base {
  FrozenString<M> prefix;  ///< 各行に付加する接頭辞
  constexpr prefix_lines_adaptor(FrozenString<M> p) noexcept : prefix(p) {}
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::prefix_lines(str, prefix);
  }
};

/**
 * @brief 各行の末尾に指定文字列を付加するパイプアダプタ
 * @tparam M 接尾辞のバッファ長
 */
template <size_t M>
struct postfix_lines_adaptor : pipe_adaptor_base {
  FrozenString<M> postfix;  ///< 各行に付加する接尾辞
  constexpr postfix_lines_adaptor(FrozenString<M> p) noexcept : postfix(p) {}
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::postfix_lines(str, postfix);
  }
};

/**
 * @brief 各行の両端に指定文字列を付加するパイプアダプタ
 * @tparam M1 接頭辞のバッファ長
 * @tparam M2 接尾辞のバッファ長
 */
template <size_t M1, size_t M2>
struct surround_lines_adaptor : pipe_adaptor_base {
  FrozenString<M1> prefix;   ///< 各行先頭に付加する接頭辞
  FrozenString<M2> postfix;  ///< 各行末尾に付加する接尾辞
  constexpr surround_lines_adaptor(FrozenString<M1> pr, FrozenString<M2> po) noexcept : prefix(pr), postfix(po) {}

  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::surround_lines(str, prefix, postfix);
  }
};

/**
 * @brief URL エンコード（%XX 形式）するパイプアダプタ
 */
struct url_encode_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::url_encode(str);
  }
};

/**
 * @brief URL デコード（%XX → 元文字）するパイプアダプタ
 */
struct url_decode_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::url_decode(str);
  }
};

/**
 * @brief Base64 エンコードするパイプアダプタ
 */
struct base64_encode_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::base64_encode(str);
  }
};

/**
 * @brief Base64 デコードするパイプアダプタ
 */
struct base64_decode_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::base64_decode(str);
  }
};

/**
 * @brief 16進数エンコード（各バイト → 2桁 hex）するパイプアダプタ
 */
struct hex_encode_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::hex_encode(str);
  }
};

/**
 * @brief 16進数デコード（2桁 hex → 元バイト）するパイプアダプタ
 */
struct hex_decode_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::hex_decode(str);
  }
};

/**
 * @brief HTML エンティティエンコードするパイプアダプタ
 */
struct html_encode_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::html_encode(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::html_encode(FrozenString{str});
  }
};

/**
 * @brief HTML エンティティデコードするパイプアダプタ
 */
struct html_decode_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::html_decode(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::html_decode(FrozenString{str});
  }
};

/**
 * @brief 指定幅で単語折り返しするパイプアダプタ
 */
struct word_wrap_adaptor : pipe_adaptor_base {
  size_t width;  ///< 最大行幅（文字数）
  constexpr word_wrap_adaptor(size_t w) noexcept : width(w) {}

  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::word_wrap(str, width);
  }
};

/**
 * @brief HTML minify をパイプ演算子で適用するアダプタ
 */
struct minify_html_adaptor : pipe_adaptor_base {
  minify_markup_opt options;  ///< ミニファイオプション（デフォルト: remove_quotes | remove_end_tags）
  constexpr minify_html_adaptor(
    minify_markup_opt opts = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags
  ) noexcept : options(opts) {}
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::minify_html(str, options);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::minify_html(FrozenString{str}, options);
  }
  [[nodiscard]] consteval auto operator()(minify_markup_opt opts) const noexcept {
    return minify_html_adaptor{opts};
  }
};

/**
 * @brief XML minify をパイプ演算子で適用するアダプタ
 */
struct minify_xml_adaptor : pipe_adaptor_base {
  minify_markup_opt options;  ///< ミニファイオプション（デフォルト: remove_quotes | remove_end_tags）
  constexpr minify_xml_adaptor(
    minify_markup_opt opts = minify_markup_opt::remove_quotes | minify_markup_opt::remove_end_tags
  ) noexcept : options(opts) {}
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::minify_xml(str, options);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::minify_xml(FrozenString{str}, options);
  }
  [[nodiscard]] consteval auto operator()(minify_markup_opt opts) const noexcept {
    return minify_xml_adaptor{opts};
  }
};

/**
 * @brief JSON minify をパイプ演算子で適用するアダプタ
 */
struct minify_json_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::minify_json(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::minify_json(FrozenString{str});
  }
};

/**
 * @brief YAML minify をパイプ演算子で適用するアダプタ
 */
struct minify_yaml_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::minify_yaml(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::minify_yaml(FrozenString{str});
  }
};

/**
 * @brief SQL minify をパイプ演算子で適用するアダプタ
 */
struct minify_sql_adaptor : pipe_adaptor_base {
  minify_sql_opt options;  ///< ミニファイオプション（デフォルト: shorten_types）
  constexpr minify_sql_adaptor(
    minify_sql_opt opts = minify_sql_opt::shorten_types
  ) noexcept : options(opts) {}
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::minify_sql(str, options);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::minify_sql(FrozenString{str}, options);
  }
  [[nodiscard]] consteval auto operator()(minify_sql_opt opts) const noexcept {
    return minify_sql_adaptor{opts};
  }
};

/**
 * @brief Cypher minify をパイプ演算子で適用するアダプタ
 */
struct minify_cypher_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::minify_cypher(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::minify_cypher(FrozenString{str});
  }
};

/**
 * @brief Lua/Luau minify をパイプ演算子で適用するアダプタ
 */
struct minify_lua_adaptor : pipe_adaptor_base {
  minify_lua_opt options;  ///< ミニファイオプション（デフォルト: none）
  constexpr minify_lua_adaptor(
    minify_lua_opt opts = minify_lua_opt::none
  ) noexcept : options(opts) {}
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::minify_lua(str, options);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::minify_lua(FrozenString{str}, options);
  }
  [[nodiscard]] consteval auto operator()(minify_lua_opt opts) const noexcept {
    return minify_lua_adaptor{opts};
  }
};

/**
 * @brief SQL 予約語を大文字化するパイプ演算子アダプタ
 */
struct sql_uppercase_keywords_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::sql_uppercase_keywords(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::sql_uppercase_keywords(FrozenString{str});
  }
};

// パイプ演算子で使用できるインスタンス
inline constexpr trim_adaptor<>            trim{};             ///< 述語デフォルト（空白文字）で両端トリム
inline constexpr ltrim_adaptor<>           ltrim{};            ///< 述語デフォルト（空白文字）で左端トリム
inline constexpr rtrim_adaptor<>           rtrim{};            ///< 述語デフォルト（空白文字）で右端トリム
inline constexpr toupper_adaptor           toupper{};          ///< ASCII 大文字変換
inline constexpr tolower_adaptor           tolower{};          ///< ASCII 小文字変換
inline constexpr collapse_spaces_adaptor<> collapse_spaces{};  ///< 連続空白を1空白に圧縮
inline constexpr capitalize_adaptor        capitalize{};       ///< 先頭大文字化、残り小文字化
inline constexpr to_snake_case_adaptor     to_snake_case{};    ///< 大文字小文字区切り → snake_case
inline constexpr to_camel_case_adaptor     to_camel_case{};    ///< snake_case → camelCase
inline constexpr to_pascal_case_adaptor    to_pascal_case{};   ///< snake_case → PascalCase
inline constexpr join_lines_adaptor        join_lines{};       ///< 全行を連結（セパレータなし）
template <FrozenString Sep>
inline constexpr join_lines_nttp_adaptor<Sep>        join_lines_nttp{};         ///< NTTP セパレータで行連結
inline constexpr trim_trailing_spaces_adaptor        trim_trailing_spaces{};    ///< 各行末尾の空白を除去
inline constexpr remove_empty_lines_adaptor          remove_empty_lines{};      ///< 空行を全削除
inline constexpr remove_leading_empty_lines_adaptor  remove_leading_empty_lines{};    ///< 先頭空行を削除
inline constexpr remove_trailing_empty_lines_adaptor remove_trailing_empty_lines{};   ///< 末尾空行を削除
inline constexpr collapse_empty_lines_adaptor        collapse_empty_lines{};    ///< 連続空行を1行に
inline constexpr url_encode_adaptor                  url_encode{};              ///< URL エンコード
inline constexpr url_decode_adaptor                  url_decode{};              ///< URL デコード
inline constexpr base64_encode_adaptor               base64_encode{};           ///< Base64 エンコード
inline constexpr base64_decode_adaptor               base64_decode{};           ///< Base64 デコード
inline constexpr hex_encode_adaptor                  hex_encode{};              ///< 16進数エンコード
inline constexpr hex_decode_adaptor                  hex_decode{};              ///< 16進数デコード
inline constexpr hex_encode_adaptor                  to_ascii{};                ///< hex_encode の別名
inline constexpr hex_decode_adaptor                  from_ascii{};              ///< hex_decode の別名
inline constexpr html_encode_adaptor                 html_encode{};             ///< HTML エンティティエンコード
inline constexpr html_decode_adaptor                 html_decode{};             ///< HTML エンティティデコード
inline constexpr minify_html_adaptor                 minify_html{};             ///< HTML ミニファイ
inline constexpr minify_xml_adaptor                  minify_xml{};              ///< XML ミニファイ
inline constexpr minify_json_adaptor                 minify_json{};             ///< JSON ミニファイ
inline constexpr minify_yaml_adaptor                 minify_yaml{};             ///< YAML ミニファイ
inline constexpr minify_sql_adaptor                  minify_sql{};              ///< SQL ミニファイ
inline constexpr minify_cypher_adaptor               minify_cypher{};           ///< Cypher ミニファイ
inline constexpr minify_lua_adaptor                   minify_lua{};             ///< Lua/Luau ミニファイ
inline constexpr sql_uppercase_keywords_adaptor      sql_uppercase_keywords{};  ///< SQL 予約語を大文字化
inline constexpr remove_leading_spaces_adaptor<>     remove_leading_spaces{};   ///< 各行先頭の空白を全除去
inline constexpr remove_trailing_spaces_adaptor<>    remove_trailing_spaces{};  ///< 各行末尾の空白を全除去
inline constexpr remove_regex_comment_adaptor        remove_regex_comment{};    ///< 拡張正規表現コメント除去

// 述語指定版インスタンス
template <auto Pred>
inline constexpr trim_adaptor<Pred> trim_if{};                            ///< 自作述語で両端トリム
template <auto Pred>
inline constexpr ltrim_adaptor<Pred> ltrim_if{};                          ///< 自作述語で左端トリム
template <auto Pred>
inline constexpr rtrim_adaptor<Pred> rtrim_if{};                          ///< 自作述語で右端トリム
template <auto Pred>
inline constexpr collapse_spaces_adaptor<Pred> collapse_spaces_if{};      ///< 自作述語で連続文字圧縮
template <auto Pred>
inline constexpr remove_leading_spaces_adaptor<Pred> remove_leading_spaces_if{};   ///< 自作述語で各行先頭文字除去
template <auto Pred>
inline constexpr remove_trailing_spaces_adaptor<Pred> remove_trailing_spaces_if{};  ///< 自作述語で各行末尾文字除去

/**
 * @brief 各行に接頭辞を付与するパイプアダプタを生成
 * @tparam M 接頭辞のバッファ長
 * @param prefix 付与する接頭辞
 */
template <size_t M>
[[nodiscard]] consteval auto prefix_lines(FrozenString<M> const& prefix) noexcept {
  return prefix_lines_adaptor<M>{prefix};
}

/**
 * @brief 各行に接尾辞を付与するパイプアダプタを生成
 * @tparam M 接尾辞のバッファ長
 * @param postfix 付与する接尾辞
 */
template <size_t M>
[[nodiscard]] consteval auto postfix_lines(FrozenString<M> const& postfix) noexcept {
  return postfix_lines_adaptor<M>{postfix};
}

/**
 * @brief 各行の両端に文字列を付与するパイプアダプタを生成
 * @tparam M1 接頭辞のバッファ長
 * @tparam M2 接尾辞のバッファ長
 * @param prefix 付与する接頭辞
 * @param postfix 付与する接尾辞
 */
template <size_t M1, size_t M2>
[[nodiscard]] consteval auto surround_lines(FrozenString<M1> const& prefix, FrozenString<M2> const& postfix) noexcept {
  return surround_lines_adaptor<M1, M2>{prefix, postfix};
}

/**
 * @brief 各行の両端に同一文字列を付与するパイプアダプタを生成
 * @tparam M バッファ長
 * @param both 接頭辞・接尾辞両方に使う文字列
 */
template <size_t M>
[[nodiscard]] consteval auto surround_lines(FrozenString<M> const& both) noexcept {
  return surround_lines_adaptor<M, M>{both, both};
}

/**
 * @brief 部分文字列を切り出すパイプアダプタを生成
 * @param pos 開始位置
 * @param len 切り出し長（負数で左方向）
 */
[[nodiscard]] consteval auto substr(std::size_t pos, std::ptrdiff_t len) noexcept {
  return substr_adaptor{pos, len};
}

/**
 * @brief コメント行を削除するパイプアダプタを生成
 * @param comment_seq コメント開始文字列（デフォルト "#"）
 */
[[nodiscard]] consteval auto remove_comment_lines(std::string_view comment_seq = "#") noexcept {
  return remove_comment_lines_adaptor{comment_seq};
}

/**
 * @brief 行内コメント（コメント開始以降）を削除するパイプアダプタを生成
 * @param comment_seq コメント開始文字列（デフォルト "#"）
 */
[[nodiscard]] consteval auto remove_comments(std::string_view comment_seq = "#") noexcept {
  return remove_comments_adaptor{comment_seq};
}

/**
 * @brief 範囲コメントを除去するパイプアダプタを生成
 * @param start_seq 範囲開始文字列
 * @param end_seq 範囲終了文字列
 */
[[nodiscard]] consteval auto remove_range_comments(std::string_view start_seq, std::string_view end_seq) noexcept {
  return remove_range_comments_adaptor{start_seq, end_seq};
}

/**
 * @brief 単語折り返しパイプアダプタを生成
 * @param width 最大行幅
 */
[[nodiscard]] consteval auto word_wrap(size_t width) noexcept {
  return word_wrap_adaptor{width};
}

/**
 * @brief 文字列配列を NTTP 区切り文字で結合するパイプアダプタ
 * @tparam Delim 結合に使う区切り文字列（NTTP）
 */
template <FrozenString Delim>
struct join_adaptor : pipe_adaptor_base {
  template <size_t ElemN, size_t Count>
  [[nodiscard]] consteval auto operator()(std::array<FrozenString<ElemN>, Count> const& arr) const noexcept {
    return frozenchars::join<Delim>(arr);
  }
};

template <FrozenString Delim>
inline constexpr join_adaptor<Delim> join{};  ///< NTTP 区切り文字で配列結合

/**
 * @brief FrozenString 配列に join アダプタをパイプ適用する演算子
 */
template <size_t ElemN, size_t Count, FrozenString Delim>
[[nodiscard]] consteval auto operator|(std::array<FrozenString<ElemN>, Count> const& lhs, join_adaptor<Delim> const& rhs) noexcept(noexcept(rhs(lhs))) {
  return rhs(lhs);
}

/**
 * @brief 指定幅と埋め文字で左詰めするパイプアダプタ
 * @tparam Width 目標幅
 * @tparam Fill 埋め文字（デフォルト空白）
 */
template <size_t Width, char Fill = ' '>
struct pad_left_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::pad_left<Width, Fill>(str);
  }
  template <Integral T>
  [[nodiscard]] consteval auto operator()(T const& v) const noexcept {
    return frozenchars::pad_left<Width, Fill>(v);
  }
};

template <size_t Width, char Fill = ' '>
inline constexpr pad_left_adaptor<Width, Fill> pad_left{};  ///< 左詰め（数値も可）

/**
 * @brief 指定幅と埋め文字で右詰めするパイプアダプタ
 * @tparam Width 目標幅
 * @tparam Fill 埋め文字（デフォルト空白）
 */
template <size_t Width, char Fill = ' '>
struct pad_right_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::pad_right<Width, Fill>(str);
  }
  template <Integral T>
  [[nodiscard]] consteval auto operator()(T const& v) const noexcept {
    return frozenchars::pad_right<Width, Fill>(v);
  }
};

template <size_t Width, char Fill = ' '>
inline constexpr pad_right_adaptor<Width, Fill> pad_right{};  ///< 右詰め（数値も可）

/**
 * @brief 最初にマッチした From を To に置換するパイプアダプタ
 * @tparam From 検索文字列（NTTP）
 * @tparam To 置換文字列（NTTP）
 */
template <FrozenString From, FrozenString To>
struct replace_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::replace<From, To>(str);
  }
};

template <FrozenString From, FrozenString To>
inline constexpr replace_adaptor<From, To> replace{};  ///< 最初の1件のみ置換

/**
 * @brief すべての From を To に置換するパイプアダプタ
 * @tparam From 検索文字列（NTTP）
 * @tparam To 置換文字列（NTTP）
 */
template <FrozenString From, FrozenString To>
struct replace_all_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::replace_all<From, To>(str);
  }
};

template <FrozenString From, FrozenString To>
inline constexpr replace_all_adaptor<From, To> replace_all{};  ///< 全件置換

/**
 * @brief 部分文字列を含むか判定するパイプアダプタ
 * @tparam Substr 検索文字列
 */
template <FrozenString Substr>
struct contains_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept -> bool {
    return frozenchars::contains<Substr>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept -> bool {
    return frozenchars::contains<Substr>(FrozenString{str});
  }
};

template <FrozenString Substr>
inline constexpr contains_adaptor<Substr> contains{};  ///< 部分文字列含有判定

/**
 * @brief 指定接頭辞で始まるか判定するパイプアダプタ
 * @tparam Prefix 接頭辞
 */
template <FrozenString Prefix>
struct starts_with_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept -> bool {
    return frozenchars::starts_with<Prefix>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept -> bool {
    return frozenchars::starts_with<Prefix>(FrozenString{str});
  }
};

template <FrozenString Prefix>
inline constexpr starts_with_adaptor<Prefix> starts_with{};  ///< 接頭辞判定

/**
 * @brief 指定接尾辞で終わるか判定するパイプアダプタ
 * @tparam Suffix 接尾辞
 */
template <FrozenString Suffix>
struct ends_with_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept -> bool {
    return frozenchars::ends_with<Suffix>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept -> bool {
    return frozenchars::ends_with<Suffix>(FrozenString{str});
  }
};

template <FrozenString Suffix>
inline constexpr ends_with_adaptor<Suffix> ends_with{};  ///< 接尾辞判定

/**
 * @brief 区切り文字で文字列を3分割するパイプアダプタ
 * @tparam Delim 区切り文字列
 */
template <FrozenString Delim>
struct partition_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::partition<Delim>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::partition<Delim>(FrozenString{str});
  }
};

template <FrozenString Delim>
inline constexpr partition_adaptor<Delim> partition{};  ///< Delim で3分割

/**
 * @brief 行区切り表現を相互変換するアダプタ
 * @tparam From 変換元の行区切り形式
 * @tparam To 変換先の行区切り形式
 */
template <frozenchars::LineBreak From, frozenchars::LineBreak To>
struct linebreak_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const noexcept {
    return frozenchars::convert_linebreak<From, To>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::convert_linebreak<From, To>(FrozenString{str});
  }
};

template <frozenchars::LineBreak From, frozenchars::LineBreak To>
inline constexpr linebreak_adaptor<From, To> linebreak{};  ///< 行区切り変換アダプタ

// 行区切り変換の便利エイリアス
inline constexpr auto br_to_nl     = linebreak<frozenchars::LineBreak::Br, frozenchars::LineBreak::Nl>;     ///< <br> → 実改行
inline constexpr auto nl_to_br     = linebreak<frozenchars::LineBreak::Nl, frozenchars::LineBreak::Br>;     ///< 実改行 → <br>
inline constexpr auto br_to_esc_n  = linebreak<frozenchars::LineBreak::Br, frozenchars::LineBreak::EscN>;  ///< <br> → \n リテラル
inline constexpr auto esc_n_to_br  = linebreak<frozenchars::LineBreak::EscN, frozenchars::LineBreak::Br>;  ///< \n リテラル → <br>
inline constexpr auto nl_to_esc_n  = linebreak<frozenchars::LineBreak::Nl, frozenchars::LineBreak::EscN>;  ///< 実改行 → \n リテラル
inline constexpr auto esc_n_to_nl  = linebreak<frozenchars::LineBreak::EscN, frozenchars::LineBreak::Nl>;  ///< \n リテラル → 実改行

/*===============================================================================*\
 * 検索系アダプタ: find / rfind / find_first_of / find_last_of / count_substring
\*===============================================================================*/

/**
 * @brief 部分文字列の最初の出現位置を検索するパイプアダプタ
 * @tparam Sub 検索文字列
 */
template <frozenchars::FrozenString Sub>
struct find_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(frozenchars::FrozenString<N> const& str) const noexcept -> std::size_t {
    return frozenchars::find<Sub>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept -> std::size_t {
    return frozenchars::find<Sub>(frozenchars::FrozenString{str});
  }
};

template <frozenchars::FrozenString Sub>
inline constexpr find_adaptor<Sub> find{};  ///< 前方検索

/**
 * @brief 部分文字列の最後の出現位置を検索するパイプアダプタ
 * @tparam Sub 検索文字列
 */
template <frozenchars::FrozenString Sub>
struct rfind_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(frozenchars::FrozenString<N> const& str) const noexcept -> std::size_t {
    return frozenchars::rfind<Sub>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept -> std::size_t {
    return frozenchars::rfind<Sub>(frozenchars::FrozenString{str});
  }
};

template <frozenchars::FrozenString Sub>
inline constexpr rfind_adaptor<Sub> rfind{};  ///< 後方検索

/**
 * @brief 文字集合のいずれかが最初に出現する位置を検索するパイプアダプタ
 * @tparam Chars 検索文字集合
 */
template <frozenchars::FrozenString Chars>
struct find_first_of_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(frozenchars::FrozenString<N> const& str) const noexcept -> std::size_t {
    return frozenchars::find_first_of<Chars>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept -> std::size_t {
    return frozenchars::find_first_of<Chars>(frozenchars::FrozenString{str});
  }
};

template <frozenchars::FrozenString Chars>
inline constexpr find_first_of_adaptor<Chars> find_first_of{};  ///< 文字集合の前方検索

/**
 * @brief 文字集合のいずれかが最後に出現する位置を検索するパイプアダプタ
 * @tparam Chars 検索文字集合
 */
template <frozenchars::FrozenString Chars>
struct find_last_of_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(frozenchars::FrozenString<N> const& str) const noexcept -> std::size_t {
    return frozenchars::find_last_of<Chars>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept -> std::size_t {
    return frozenchars::find_last_of<Chars>(frozenchars::FrozenString{str});
  }
};

template <frozenchars::FrozenString Chars>
inline constexpr find_last_of_adaptor<Chars> find_last_of{};  ///< 文字集合の後方検索

/**
 * @brief 部分文字列の出現回数を計数するパイプアダプタ
 * @tparam Sub 検索文字列
 */
template <frozenchars::FrozenString Sub>
struct count_substring_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(frozenchars::FrozenString<N> const& str) const noexcept -> std::size_t {
    return frozenchars::count_substring<Sub>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept -> std::size_t {
    return frozenchars::count_substring<Sub>(frozenchars::FrozenString{str});
  }
};

template <frozenchars::FrozenString Sub>
inline constexpr count_substring_adaptor<Sub> count_substring{};  ///< 出現回数計数（重複なし）

/**
 * @brief 文字列を反転するパイプアダプタ
 */
struct reverse_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(frozenchars::FrozenString<N> const& str) const noexcept {
    return frozenchars::reverse(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::reverse(frozenchars::FrozenString{str});
  }
};

inline constexpr reverse_adaptor reverse{};  ///< 文字列反転

/**
 * @brief 各行の先頭に指定文字をインデントとして付加するパイプアダプタ
 * @tparam IndentWidth インデントの深さ（文字数）
 * @tparam IndentChar インデントに使う文字（デフォルトタブ）
 */
template <size_t IndentWidth, char IndentChar = '\t'>
struct indent_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(frozenchars::FrozenString<N> const& str) const noexcept {
    return frozenchars::indent<IndentWidth, IndentChar>(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::indent<IndentWidth, IndentChar>(frozenchars::FrozenString{str});
  }
};

template <size_t IndentWidth, char IndentChar = '\t'>
inline constexpr indent_adaptor<IndentWidth, IndentChar> indent{};  ///< インデント付与（空行除く）

/**
 * @brief 全行の共通インデントを除去するパイプアダプタ
 */
struct dedent_adaptor : pipe_adaptor_base {
  template <size_t N>
  [[nodiscard]] consteval auto operator()(frozenchars::FrozenString<N> const& str) const noexcept {
    return frozenchars::dedent(str);
  }
  template <size_t N>
  [[nodiscard]] consteval auto operator()(char const (&str)[N]) const noexcept {
    return frozenchars::dedent(frozenchars::FrozenString{str});
  }
};

inline constexpr dedent_adaptor dedent{};  ///< 共通インデント除去

/**
 * @brief 数値型に対してアダプタを適用するパイプ演算子
 */
template <Integral T, PipeAdaptor Adaptor>
[[nodiscard]] auto consteval operator|(T const& lhs, Adaptor const& rhs) noexcept(noexcept(rhs(lhs))) {
  return rhs(lhs);
}

}  // namespace frozenchars::ops
