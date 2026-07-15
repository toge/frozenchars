#pragma once

#include <array>
#include <chrono>
#include <cstddef>

namespace frozenchars::detail {

/**
 * @brief 4桁の数字文字を整数に変換する
 *
 * @param a 千の位の数字文字
 * @param b 百の位の数字文字
 * @param c 十の位の数字文字
 * @param d 一の位の数字文字
 * @return int 変換した0..9999の整数値
 */
[[nodiscard]] constexpr auto decode_four_digits(char a, char b, char c, char d) noexcept -> int {
  return (a - '0') * 1000 + (b - '0') * 100 + (c - '0') * 10 + (d - '0');
}

/**
 * @brief 2桁の数字文字を符号なし整数に変換する
 *
 * @param a 十の位の数字文字
 * @param b 一の位の数字文字
 * @return unsigned 変換した0..99の値
 */
[[nodiscard]] constexpr auto decode_two_digits(char a, char b) noexcept -> unsigned {
  return (a - '0') * 10 + (b - '0');
}

/**
 * @brief 英語月名の先頭3文字を月番号(1..12)に変換する
 *
 * @param s 月名文字列の先頭ポインタ（先頭3文字のみ参照、大文字小文字は不問）
 * @param len 文字列長。3未満なら変換不可
 * @return unsigned 該当する月番号(1..12)、不一致なら0
 */
[[nodiscard]] constexpr auto month_name_to_number(char const* s, std::size_t len) noexcept -> unsigned {
  if (len < 3) {
    return 0;
  }
  // 大文字小文字を無視するため先頭3文字を 0x20 で小文字化して比較する

  auto const c0 = static_cast<unsigned>(s[0]) | 0x20u;
  auto const c1 = static_cast<unsigned>(s[1]) | 0x20u;
  auto const c2 = static_cast<unsigned>(s[2]) | 0x20u;
  if (c0 == 'j' && c1 == 'a' && c2 == 'n') return 1;
  if (c0 == 'f' && c1 == 'e' && c2 == 'b') return 2;
  if (c0 == 'm' && c1 == 'a' && c2 == 'r') return 3;
  if (c0 == 'a' && c1 == 'p' && c2 == 'r') return 4;
  if (c0 == 'm' && c1 == 'a' && c2 == 'y') return 5;
  if (c0 == 'j' && c1 == 'u' && c2 == 'n') return 6;
  if (c0 == 'j' && c1 == 'u' && c2 == 'l') return 7;
  if (c0 == 'a' && c1 == 'u' && c2 == 'g') return 8;
  if (c0 == 's' && c1 == 'e' && c2 == 'p') return 9;
  if (c0 == 'o' && c1 == 'c' && c2 == 't') return 10;
  if (c0 == 'n' && c1 == 'o' && c2 == 'v') return 11;
  if (c0 == 'd' && c1 == 'e' && c2 == 'c') return 12;
  return 0;
}

/**
 * @brief ISO 8601 形式 "YYYY-MM-DD" の日付文字列を year_month_day に変換する
 *
 * @param s 日付文字列の先頭ポインタ
 * @param len 文字列長。10未満なら空の year_month_day を返す
 * @return std::chrono::year_month_day 変換した年月日
 * @note 桁位置は固定（0-3:年, 5-6:月, 8-9:日）。区切り文字の妥当性は検証しない
 */
[[nodiscard]] constexpr auto parse_ymd_iso(char const* s, std::size_t len) noexcept -> std::chrono::year_month_day {
  using namespace std::chrono;
  if (len < 10) {
    return {};
  }
  auto const y = year{decode_four_digits(s[0], s[1], s[2], s[3])};
  auto const m = month{decode_two_digits(s[5], s[6])};
  auto const d = day{decode_two_digits(s[8], s[9])};
  return y / m / d;
}

/**
 * @brief __DATE__ マクロ形式 "Mmm DD YYYY" の日付文字列を year_month_day に変換する
 *
 * @param s 日付文字列の先頭ポインタ
 * @param len 文字列長。11未満なら空の year_month_day を返す
 * @return std::chrono::year_month_day 変換した年月日
 * @note 日はスペース詰め（例 "Jan  1 2024"）に対応し、先頭が空白なら1桁として解釈する
 */
[[nodiscard]] constexpr auto parse_ymd_macro(char const* s, std::size_t len) noexcept -> std::chrono::year_month_day {
  using namespace std::chrono;
  if (len < 11) {
    return {};
  }
  auto const m = month{month_name_to_number(s, 3)};
  auto const d = (s[4] == ' ')
                   ? day{static_cast<unsigned>(s[5] - '0')}
                   : day{decode_two_digits(s[4], s[5])};
  auto const y = year{decode_four_digits(s[7], s[8], s[9], s[10])};
  return y / m / d;
}

/**
 * @brief "HH:MM:SS" 形式の時刻文字列を秒に変換する
 *
 * @param s 時刻文字列の先頭ポインタ
 * @param len 文字列長。8未満なら0秒を返す
 * @return std::chrono::seconds 時・分・秒を合算した経過秒数
 */
[[nodiscard]] constexpr auto parse_hms(char const* s, std::size_t len) noexcept -> std::chrono::seconds
{
  using namespace std::chrono;
  if (len < 8) {
    return {};
  }
  auto const h = hours{decode_two_digits(s[0], s[1])};
  auto const m = minutes{decode_two_digits(s[3], s[4])};
  auto const sec = seconds{decode_two_digits(s[6], s[7])};
  return h + m + sec;
}

/**
 * @brief タイムゾーンオフセット文字列を分単位のオフセットに変換する
 *
 * @param s オフセット文字列の先頭ポインタ（"Z" または "+HH:MM" / "-HH:MM"）
 * @param len 文字列長。空文字列または先頭が 'Z'/'z' なら 0 分、6未満なら空値を返す
 * @return std::chrono::minutes UTC からの符号付きオフセット（分）
 */
[[nodiscard]] constexpr auto parse_tz_offset(char const* s, std::size_t len) noexcept -> std::chrono::minutes {
  using namespace std::chrono;
  if (len == 0 || s[0] == 'Z' || s[0] == 'z') {
    return minutes{0};
  }
  if (len < 6) {
    return {};
  }
  auto const sign = (s[0] == '-') ? -1 : 1;
  auto const h = decode_two_digits(s[1], s[2]);
  auto const m = decode_two_digits(s[4], s[5]);
  return minutes{sign * static_cast<int>(h * 60 + m)};
}

/**
 * @brief 符号なし整数を2桁の数字文字配列に変換する
 *
 * @param v 変換する値（0..99を想定）
 * @return std::array<char, 2> 上位桁・下位桁の順に格納した数字文字配列
 */
[[nodiscard]] constexpr auto to_two_digits(unsigned v) noexcept -> std::array<char, 2> {
  return {static_cast<char>('0' + v / 10), static_cast<char>('0' + v % 10)};
}

/**
 * @brief 符号なし整数を4桁の数字文字配列に変換する
 *
 * @param v 変換する値（0..9999を想定）
 * @return std::array<char, 4> 千の位から一の位の順に格納した数字文字配列
 */
[[nodiscard]] constexpr auto to_four_digits(unsigned v) noexcept -> std::array<char, 4> {
  return {static_cast<char>('0' + v / 1000),
          static_cast<char>('0' + (v / 100) % 10),
          static_cast<char>('0' + (v / 10) % 10),
          static_cast<char>('0' + v % 10)};
}

} // namespace frozenchars::detail
