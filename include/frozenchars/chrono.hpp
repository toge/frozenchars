#pragma once

#include <chrono>
#include <expected>
#include <system_error>

#include "frozenchars/string.hpp"
#include "detail/chrono_impl.hpp"

namespace frozenchars {

/**
 * @brief ISO 8601 日付文字列 "YYYY-MM-DD" をコンパイル時に std::chrono::year_month_day に変換する。
 *
 * @tparam S 変換対象の ISO 8601 日付文字列（FrozenString  NTTP）
 * @return std::chrono::year_month_day 変換後の日付
 */
template <FrozenString S>
[[nodiscard]] consteval auto parse_iso_date() noexcept -> std::chrono::year_month_day {
  static_assert(S.length == 10 && S.buffer[4] == '-' && S.buffer[7] == '-',
    "chrono: invalid ISO 8601 date format, expected YYYY-MM-DD");

  auto const m = detail::decode_two_digits(S.buffer[5], S.buffer[6]);
  static_assert(m >= 1 && m <= 12, "chrono: invalid month value");

  auto const d = detail::decode_two_digits(S.buffer[8], S.buffer[9]);
  static_assert(d >= 1 && d <= 31, "chrono: invalid day value");

  return detail::parse_ymd_iso(S.data(), 10);
}

/**
 * @brief ISO 8601 日付文字列 "YYYY-MM-DD" を実行時に std::chrono::year_month_day に変換する。
 *
 * @tparam N 入力文字列のバッファ長
 * @param s 変換対象の ISO 8601 日付文字列
 * @return std::expected<std::chrono::year_month_day, std::errc> 変換後の日付。フォーマット不正の場合はエラーを返す
 */
template <size_t N>
[[nodiscard]] constexpr auto parse_iso_date(FrozenString<N> const& s) noexcept
  -> std::expected<std::chrono::year_month_day, std::errc> {
  if (s.length != 10) return std::unexpected(std::errc::invalid_argument);
  if (s.buffer[4] != '-') return std::unexpected(std::errc::invalid_argument);
  if (s.buffer[7] != '-') return std::unexpected(std::errc::invalid_argument);
  auto const m = detail::decode_two_digits(s.buffer[5], s.buffer[6]);
  if (m < 1 || m > 12) return std::unexpected(std::errc::invalid_argument);
  auto const d = detail::decode_two_digits(s.buffer[8], s.buffer[9]);
  if (d < 1 || d > 31) return std::unexpected(std::errc::invalid_argument);
  return detail::parse_ymd_iso(s.data(), 10);
}

/**
 * @brief ISO 8601 日時文字列 "YYYY-MM-DDTHH:MM:SS[±HH:MM]" をコンパイル時に std::chrono::sys_seconds に変換する。
 *
 * @tparam S 変換対象の ISO 8601 日時文字列（FrozenString NTTP）。長さは 19（オフセットなし）、20（Z）、25（±HH:MM）
 * @return std::chrono::sys_seconds 変換後のUTC日時
 */
template <FrozenString S>
[[nodiscard]] consteval auto parse_iso_datetime() noexcept -> std::chrono::sys_seconds {
  using namespace std::chrono;
  static_assert(S.length == 19 || S.length == 20 || S.length == 25,
    "chrono: invalid ISO 8601 datetime format, expected YYYY-MM-DDTHH:MM:SS");
  static_assert(S.buffer[4] == '-' && S.buffer[7] == '-',
    "chrono: invalid ISO 8601 datetime format, expected YYYY-MM-DDTHH:MM:SS");
  static_assert(S.buffer[10] == 'T' && S.buffer[13] == ':' && S.buffer[16] == ':',
    "chrono: invalid ISO 8601 datetime format, expected YYYY-MM-DDTHH:MM:SS");

  auto const h = detail::decode_two_digits(S.buffer[11], S.buffer[12]);
  auto const mi = detail::decode_two_digits(S.buffer[14], S.buffer[15]);
  auto const sec = detail::decode_two_digits(S.buffer[17], S.buffer[18]);

  static_assert(h <= 23 && mi <= 59 && sec <= 59, "chrono: invalid time component");

  if constexpr (S.length >= 20) {
    static_assert(S.buffer[19] == 'Z' || S.buffer[19] == 'z'
      || S.buffer[19] == '+' || S.buffer[19] == '-',
      "chrono: invalid timezone offset format");
  }
  if constexpr (S.length == 25) {
    static_assert(S.buffer[22] == ':',
      "chrono: invalid timezone offset format");
    auto const oh = detail::decode_two_digits(S.buffer[20], S.buffer[21]);
    auto const om = detail::decode_two_digits(S.buffer[23], S.buffer[24]);
    static_assert(oh <= 23 && om <= 59, "chrono: invalid timezone offset format");
  }
  auto const date = detail::parse_ymd_iso(S.data(), 10);
  auto const time = detail::parse_hms(S.data() + 11, 8);
  auto offset = minutes{0};
  if constexpr (S.length == 25) {
    offset = detail::parse_tz_offset(S.data() + 19, 6);
  }
  return sys_days{date} + time - offset;
}

/**
 * @brief ISO 8601 日時文字列 "YYYY-MM-DDTHH:MM:SS[±HH:MM]" を実行時に std::chrono::sys_seconds に変換する。
 *
 * @tparam N 入力文字列のバッファ長
 * @param s 変換対象の ISO 8601 日時文字列
 * @return std::expected<std::chrono::sys_seconds, std::errc> 変換後のUTC日時。フォーマット不正の場合はエラーを返す
 */
template <size_t N>
[[nodiscard]] constexpr auto parse_iso_datetime(FrozenString<N> const& s) noexcept
  -> std::expected<std::chrono::sys_seconds, std::errc> {
  using namespace std::chrono;
  if (s.length < 19) return std::unexpected(std::errc::invalid_argument);
  if (s.length != 19 && s.length != 20 && s.length != 25) return std::unexpected(std::errc::invalid_argument);
  if (s.buffer[4] != '-') return std::unexpected(std::errc::invalid_argument);
  if (s.buffer[7] != '-') return std::unexpected(std::errc::invalid_argument);
  if (s.buffer[10] != 'T') return std::unexpected(std::errc::invalid_argument);
  if (s.buffer[13] != ':') return std::unexpected(std::errc::invalid_argument);
  if (s.buffer[16] != ':') return std::unexpected(std::errc::invalid_argument);
  auto const h = detail::decode_two_digits(s.buffer[11], s.buffer[12]);
  auto const mi = detail::decode_two_digits(s.buffer[14], s.buffer[15]);
  auto const sec = detail::decode_two_digits(s.buffer[17], s.buffer[18]);
  if (h > 23) return std::unexpected(std::errc::invalid_argument);
  if (mi > 59) return std::unexpected(std::errc::invalid_argument);
  if (sec > 59) return std::unexpected(std::errc::invalid_argument);
  if (s.length >= 20 && s.buffer[19] != 'Z' && s.buffer[19] != 'z' && s.buffer[19] != '+' && s.buffer[19] != '-')
    return std::unexpected(std::errc::invalid_argument);
  if (s.length == 25 && s.buffer[22] != ':')
    return std::unexpected(std::errc::invalid_argument);
  auto const date = detail::parse_ymd_iso(s.data(), 10);
  auto const time = detail::parse_hms(s.data() + 11, 8);
  auto offset = minutes{0};
  if (s.length == 25) {
    offset = detail::parse_tz_offset(s.data() + 19, 6);
  }
  return sys_days{date} + time - offset;
}

/**
 * @brief C プリプロセッサマクロ __DATE__ の書式 "Mmm dd yyyy" をコンパイル時に std::chrono::year_month_day に変換する。
 *
 * @tparam S 変換対象の日付文字列（FrozenString NTTP、__DATE__ 書式）
 * @return std::chrono::year_month_day 変換後の日付
 */
template <FrozenString S>
[[nodiscard]] consteval auto parse_date_macro() noexcept -> std::chrono::year_month_day {
  static_assert(S.length == 11 && S.buffer[6] == ' ', "chrono: invalid __DATE__ format");
  auto const mon = detail::month_name_to_number(S.data(), 3);
  static_assert(mon >= 1 && mon <= 12, "chrono: invalid __DATE__ format");
  auto const day_val = (S.buffer[4] == ' ')
    ? static_cast<unsigned>(S.buffer[5] - '0')
    : detail::decode_two_digits(S.buffer[4], S.buffer[5]);
  static_assert(day_val >= 1 && day_val <= 31, "chrono: invalid __DATE__ format");
  auto const year_val = detail::decode_four_digits(S.buffer[7], S.buffer[8], S.buffer[9], S.buffer[10]);
  static_assert(year_val >= 1970, "chrono: invalid __DATE__ format");
  return detail::parse_ymd_macro(S.data(), 11);
}

/**
 * @brief C プリプロセッサマクロ __DATE__ の書式 "Mmm dd yyyy" を実行時に std::chrono::year_month_day に変換する。
 *
 * @tparam N 入力文字列のバッファ長
 * @param s 変換対象の日付文字列（__DATE__ 書式）
 * @return std::expected<std::chrono::year_month_day, std::errc> 変換後の日付。フォーマット不正の場合はエラーを返す
 */
template <size_t N>
[[nodiscard]] constexpr auto parse_date_macro(FrozenString<N> const& s) noexcept
  -> std::expected<std::chrono::year_month_day, std::errc> {
  if (s.length != 11) return std::unexpected(std::errc::invalid_argument);
  if (s.buffer[6] != ' ') return std::unexpected(std::errc::invalid_argument);
  auto const mon = detail::month_name_to_number(s.data(), 3);
  if (mon < 1 || mon > 12) return std::unexpected(std::errc::invalid_argument);
  auto const day_val = (s.buffer[4] == ' ')
    ? static_cast<unsigned>(s.buffer[5] - '0')
    : detail::decode_two_digits(s.buffer[4], s.buffer[5]);
  if (day_val < 1 || day_val > 31) return std::unexpected(std::errc::invalid_argument);
  auto const year_val = detail::decode_four_digits(s.buffer[7], s.buffer[8], s.buffer[9], s.buffer[10]);
  if (year_val < 1970) return std::unexpected(std::errc::invalid_argument);
  return detail::parse_ymd_macro(s.data(), 11);
}

/**
 * @brief コンパイル時の __DATE__ と __TIME__ を組み合わせて、ビルド日時を std::chrono::sys_seconds として取得する。
 *
 * @return std::chrono::sys_seconds コンパイル日時（UTC）
 */
[[nodiscard]] consteval auto compilation_timestamp() noexcept -> std::chrono::sys_seconds {
  auto const date = detail::parse_ymd_macro(__DATE__, 11);
  auto const time = detail::parse_hms(__TIME__, 8);
  return std::chrono::sys_days{date} + time;
}

/**
 * @brief コンパイル時の __DATE__ と __TIME__ から、指定されたタイムゾーンオフセットを適用した日時を取得する。
 *
 * @param offset UTC からのオフセット（分単位）
 * @return std::chrono::sys_seconds コンパイル日時から offset を差し引いたUTC日時
 */
[[nodiscard]] consteval auto compilation_timestamp(std::chrono::minutes offset) noexcept -> std::chrono::sys_seconds {
  return compilation_timestamp() - offset;
}

/**
 * @brief std::chrono::year_month_day を ISO 8601 日付文字列 "YYYY-MM-DD" に変換する。
 *
 * @param ymd 変換対象の日付
 * @return FrozenString<11> 変換後の ISO 8601 日付文字列（末尾に '\0' を含む）
 */
[[nodiscard]] constexpr auto format_iso_date(std::chrono::year_month_day ymd) noexcept -> FrozenString<11> {
  auto const y = static_cast<int>(ymd.year());
  auto const m = static_cast<unsigned>(ymd.month());
  auto const d = static_cast<unsigned>(ymd.day());
  auto const yb = detail::to_four_digits(static_cast<unsigned>(y));
  auto const mb = detail::to_two_digits(m);
  auto const db = detail::to_two_digits(d);
  auto result = FrozenString<11>{};
  result.buffer[0] = yb[0];
  result.buffer[1] = yb[1];
  result.buffer[2] = yb[2];
  result.buffer[3] = yb[3];
  result.buffer[4] = '-';
  result.buffer[5] = mb[0];
  result.buffer[6] = mb[1];
  result.buffer[7] = '-';
  result.buffer[8] = db[0];
  result.buffer[9] = db[1];
  result.length = 10;
  return result;
}

/**
 * @brief std::chrono::sys_seconds を ISO 8601 日時文字列 "YYYY-MM-DDTHH:MM:SSZ" に変換する。
 *
 * @param tp 変換対象のUTC日時
 * @return FrozenString<21> 変換後の ISO 8601 日時文字列（末尾に '\0' を含む、UTC）
 */
[[nodiscard]] constexpr auto format_iso_datetime(std::chrono::sys_seconds tp) noexcept -> FrozenString<21> {
  using namespace std::chrono;
  auto const dd = floor<days>(tp);
  auto const ymd = year_month_day{dd};
  auto const tod = hh_mm_ss{tp - dd};
  auto const y = static_cast<int>(ymd.year());
  auto const m = static_cast<unsigned>(ymd.month());
  auto const d = static_cast<unsigned>(ymd.day());
  auto const h = static_cast<unsigned>(tod.hours().count());
  auto const mi = static_cast<unsigned>(tod.minutes().count());
  auto const s = static_cast<unsigned>(tod.seconds().count());
  auto const yb = detail::to_four_digits(static_cast<unsigned>(y));
  auto const mb = detail::to_two_digits(m);
  auto const db = detail::to_two_digits(d);
  auto const hb = detail::to_two_digits(h);
  auto const mib = detail::to_two_digits(mi);
  auto const sb = detail::to_two_digits(s);
  auto result = FrozenString<21>{};
  result.buffer[0] = yb[0];
  result.buffer[1] = yb[1];
  result.buffer[2] = yb[2];
  result.buffer[3] = yb[3];
  result.buffer[4] = '-';
  result.buffer[5] = mb[0];
  result.buffer[6] = mb[1];
  result.buffer[7] = '-';
  result.buffer[8] = db[0];
  result.buffer[9] = db[1];
  result.buffer[10] = 'T';
  result.buffer[11] = hb[0];
  result.buffer[12] = hb[1];
  result.buffer[13] = ':';
  result.buffer[14] = mib[0];
  result.buffer[15] = mib[1];
  result.buffer[16] = ':';
  result.buffer[17] = sb[0];
  result.buffer[18] = sb[1];
  result.buffer[19] = 'Z';
  result.length = 20;
  return result;
}

} // namespace frozenchars
