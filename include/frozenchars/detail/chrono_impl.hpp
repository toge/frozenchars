#pragma once

#include <array>
#include <chrono>
#include <cstddef>

namespace frozenchars::detail {

[[nodiscard]] constexpr auto decode_four_digits(char a, char b, char c, char d) noexcept -> int
{
  return (a - '0') * 1000 + (b - '0') * 100 + (c - '0') * 10 + (d - '0');
}

[[nodiscard]] constexpr auto decode_two_digits(char a, char b) noexcept -> unsigned
{
  return (a - '0') * 10 + (b - '0');
}

[[nodiscard]] constexpr auto month_name_to_number(char const* s, std::size_t len) noexcept -> unsigned
{
  if (len < 3)
    return 0;
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

[[nodiscard]] constexpr auto parse_ymd_iso(char const* s, std::size_t len) noexcept -> std::chrono::year_month_day
{
  using namespace std::chrono;
  if (len < 10)
    return {};
  auto const y = year{decode_four_digits(s[0], s[1], s[2], s[3])};
  auto const m = month{decode_two_digits(s[5], s[6])};
  auto const d = day{decode_two_digits(s[8], s[9])};
  return y / m / d;
}

[[nodiscard]] constexpr auto parse_ymd_macro(char const* s, std::size_t len) noexcept -> std::chrono::year_month_day
{
  using namespace std::chrono;
  if (len < 11)
    return {};
  auto const m = month{month_name_to_number(s, 3)};
  auto const d = (s[4] == ' ')
                   ? day{static_cast<unsigned>(s[5] - '0')}
                   : day{decode_two_digits(s[4], s[5])};
  auto const y = year{decode_four_digits(s[7], s[8], s[9], s[10])};
  return y / m / d;
}

[[nodiscard]] constexpr auto parse_hms(char const* s, std::size_t len) noexcept -> std::chrono::seconds
{
  using namespace std::chrono;
  if (len < 8)
    return {};
  auto const h = hours{decode_two_digits(s[0], s[1])};
  auto const m = minutes{decode_two_digits(s[3], s[4])};
  auto const sec = seconds{decode_two_digits(s[6], s[7])};
  return h + m + sec;
}

[[nodiscard]] constexpr auto parse_tz_offset(char const* s, std::size_t len) noexcept -> std::chrono::minutes
{
  using namespace std::chrono;
  if (len == 0 || s[0] == 'Z' || s[0] == 'z')
    return minutes{0};
  if (len < 6)
    return {};
  auto const sign = (s[0] == '-') ? -1 : 1;
  auto const h = decode_two_digits(s[1], s[2]);
  auto const m = decode_two_digits(s[4], s[5]);
  return minutes{sign * static_cast<int>(h * 60 + m)};
}

[[nodiscard]] constexpr auto to_two_digits(unsigned v) noexcept -> std::array<char, 2>
{
  return {static_cast<char>('0' + v / 10), static_cast<char>('0' + v % 10)};
}

[[nodiscard]] constexpr auto to_four_digits(unsigned v) noexcept -> std::array<char, 4>
{
  return {static_cast<char>('0' + v / 1000),
          static_cast<char>('0' + (v / 100) % 10),
          static_cast<char>('0' + (v / 10) % 10),
          static_cast<char>('0' + v % 10)};
}

} // namespace frozenchars::detail
