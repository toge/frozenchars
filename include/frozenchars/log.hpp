#pragma once

#include "frozenchars/chrono.hpp"
#include "frozenchars/frozen_format.hpp"
#include "frozenchars/string.hpp"

#include <array>
#include <cstddef>
#include <string_view>

namespace frozenchars {

/**
 * @brief コンパイル時ロギングのログレベル。
 *
 * @details 数値が大きいほど重要度が高い。constexpr_log の MinLevel 未満の
 * レベルを持つメッセージは蓄積されない。
 */
enum class log_level : int {
  trace = 0,
  debug,
  info,
  warn,
  error,
  fatal,
};

/**
 * @brief ビルド時刻を ISO 8601 日時文字列 "YYYY-MM-DDTHH:MM:SSZ" に変換する。
 *
 * @details __DATE__ / __TIME__ は通常の文字列リテラル（翻訳単位に焼き込まれる）であり、
 * 下層の parse_ymd_macro / parse_hms / format_iso_datetime は全て constexpr のため、
 * consteval 制約なしの完全 constexpr として実装できる（ランタイムでも同じビルド時刻が取得できる）。
 *
 * @return FrozenString<21> ビルド時刻の ISO 8601 日時文字列（UTC、末尾に '\0' を含む）
 */
[[nodiscard]] constexpr auto build_timestamp() noexcept -> FrozenString<21> {
  using namespace std::chrono;
  auto const date = detail::parse_ymd_macro(__DATE__, 11);
  auto const time = detail::parse_hms(__TIME__, 8);
  return format_iso_datetime(sys_days{date} + time);
}

/**
 * @brief コンパイル時ログ蓄積バッファ。
 *
 * @details consteval 関数内で log() / log_format() によりメッセージを蓄積し、
 * ネームスペーススコープの constexpr 変数として静的記憶域に置くことで、
 * 実行時に std::string_view（sv()）または行ごとの std::array<std::string_view>
 * （messages()）として取得できる。
 *
 * MinLevel 未満のレベルのメッセージは if constexpr で破棄されるため、
 * 蓄積も書式化も行われず constexpr ステップを消費しない。バッファ / メッセージ数は
 * 上限で飽和し、constexpr UB を起こさない。
 *
 * @tparam Capacity ログ本文バッファのサイズ（文字数）
 * @tparam MaxMessages 保持できるメッセージ数
 * @tparam WithTimestamp 各行の先頭にビルド時刻プレフィクス "[YYYY-MM-DDTHH:MM:SSZ] " を付けるか
 * @tparam MinLevel このレベル未満のメッセージを破棄する閾値
 */
template <size_t Capacity, size_t MaxMessages = 32, bool WithTimestamp = false, log_level MinLevel = log_level::trace>
struct constexpr_log {
  /// @brief ログ本文の連続領域（'\n' 区切り）
  std::array<char, Capacity> buffer{};
  /// @brief 各メッセージの先頭オフセット
  std::array<size_t, MaxMessages> starts{};
  /// @brief 現在の書込位置
  size_t length = 0;
  /// @brief 蓄積済みメッセージ数
  size_t count = 0;

  /**
   * @brief メッセージを1行追加する。
   *
   * @tparam L メッセージのログレベル（既定 info）。MinLevel 未満なら蓄積しない
   * @param msg メッセージ本文
   * @return bool 処理済みなら true。バッファ / メッセージ数が上限で false
   */
  template <log_level L = log_level::info>
  constexpr bool log(std::string_view msg) {
    if constexpr (static_cast<int>(L) >= static_cast<int>(MinLevel)) {
      return append_line(msg);
    }
    return true;
  }

  /**
   * @brief frozen_format でフォーマットしたメッセージを1行追加する。
   *
   * @note frozen_format が consteval のため、この関数も consteval（コンパイル時評価のみ）。
   * 実行時に呼び出すことはできない。フォーマット対象は常にコンパイル時のみで十分なため制約はない。
   *
   * @tparam Fmt フォーマット文字列（FrozenString NTTP）
   * @tparam L メッセージのログレベル（既定 info）。MinLevel 未満ならフォーマットも行わない
   * @tparam Cap フォーマット結果の最大サイズ（既定 256）
   * @tparam Args フォーマット引数の型パラメータパック
   * @param args フォーマット引数
   * @return bool 処理済みなら true。バッファ / メッセージ数が上限で false
   */
  template <FrozenString Fmt, log_level L = log_level::info, size_t Cap = 256, typename... Args>
  consteval bool log_format(Args const&... args) {
    if constexpr (static_cast<int>(L) >= static_cast<int>(MinLevel)) {
      auto const s = frozen_format<Fmt, Cap>(args...);
      return append_line(s.sv());
    }
    return true;
  }

  /**
   * @brief ログ全文を取得する。
   * @return std::string_view ログ全文（'\n' 区切り）
   */
  [[nodiscard]] constexpr auto sv() const noexcept -> std::string_view { return {buffer.data(), length}; }

  /**
   * @brief 蓄積済みメッセージ数を取得する。
   * @return size_t メッセージ数
   */
  [[nodiscard]] constexpr auto message_count() const noexcept -> size_t { return count; }

  /**
   * @brief 指定メッセージを取得する。
   * @param i メッセージインデックス
   * @return std::string_view メッセージ本文（末尾の '\n' は除く）。範囲外なら空
   */
  [[nodiscard]] constexpr auto message(size_t i) const noexcept -> std::string_view {
    if (i >= count) {
      return {};
    }
    size_t const start = starts[i];
    size_t const end   = (i + 1 < count) ? starts[i + 1] : length;
    size_t       len   = end - start;
    if (len > 0 && buffer[start + len - 1] == '\n') {
      --len;
    }
    return {buffer.data() + start, len};
  }

  /**
   * @brief 全メッセージを行ごとの view 配列として取得する。
   * @return std::array<std::string_view, MaxMessages> メッセージ配列（count 以降は空）
   */
  [[nodiscard]] constexpr auto messages() const noexcept -> std::array<std::string_view, MaxMessages> {
    std::array<std::string_view, MaxMessages> out{};
    for (size_t i = 0; i < count; ++i) {
      out[i] = message(i);
    }
    return out;
  }

  private:
  /**
   * @brief メッセージ1行をバッファに追記する（タイムスタンプ付き対応、飽和あり）。
   * @param msg メッセージ本文
   * @return bool 追記できたら true。メッセージ数上限なら false
   */
  constexpr bool append_line(std::string_view msg) {
    if (count == MaxMessages) {
      return false;
    }
    starts[count] = length;
    if constexpr (WithTimestamp) {
      auto const ts = build_timestamp();
      append_char('[');
      for (size_t i = 0; i < ts.length; ++i) {
        append_char(ts.buffer[i]);
      }
      append_char(']');
      append_char(' ');
    }
    for (char c : msg) {
      append_char(c);
    }
    append_char('\n');
    ++count;
    return true;
  }

  /**
   * @brief 1文字をバッファに追記する。バッファが満杯なら何もしない。
   * @param c 追記する文字
   */
  constexpr void append_char(char c) {
    if (length < Capacity) {
      buffer[length++] = c;
    }
  }
};

namespace detail {

  /// @brief 型が constexpr_log か判定する
  template <typename T>
  inline constexpr bool is_constexpr_log_v = false;

  template <size_t C, size_t M, bool TS, log_level L>
  inline constexpr bool is_constexpr_log_v<constexpr_log<C, M, TS, L>> = true;

}  // namespace detail

/**
 * @brief コンパイル時にログ全体をコンパイラ診断へ吐き出すための宣言のみの型。
 *
 * @details 意図的に未定義のままにし、dump() の戻り値型として参照することで
 * "invalid use of incomplete type" エラーの診断にログ内容（テンプレート引数）を出力させる。
 */
template <auto L>
  requires detail::is_constexpr_log_v<decltype(L)>
struct constexpr_log_dump;

/**
 * @brief ログの内容をコンパイラ診断に出力する（意図的なコンパイルエラー）。
 *
 * @details Constexpr-Doom の inspect テクニックと同様に、ログ全体を NTTP として
 * 未定義の不完全型 constexpr_log_dump に渡すことで、コンパイラが型名として
 * ログ内容を診断に出力する。デバッグ用途であり、呼び出すとコンパイルは失敗する。
 *
 * @tparam L ダンプする constexpr_log のインスタンス
 */
template <auto L>
  requires detail::is_constexpr_log_v<decltype(L)>
consteval auto dump() -> constexpr_log_dump<L> {
  return {};
}

/**
 * @brief コンパイル時に任意の FrozenString をコンパイラ診断に出力する（意図的なコンパイルエラー）。
 *
 * @tparam Str ダンプする文字列（FrozenString NTTP）
 */
template <FrozenString Str>
struct string_dump;

/**
 * @brief 任意の FrozenString の内容をコンパイラ診断に出力する（意図的なコンパイルエラー）。
 *
 * @details dump() の FrozenString 版。呼び出すとコンパイルは失敗する。
 *
 * @tparam Str ダンプする文字列（FrozenString NTTP）
 */
template <FrozenString Str>
consteval auto dump_string() -> string_dump<Str> {
  return {};
}

}  // namespace frozenchars