#include <cstdio>
#include <tuple>

#include "color.hpp"

int main() {
  using namespace frozenchars;

  // #RRGGBB
  constexpr auto c1 = parse_hex_rgb("#1a2b3c");
  static_assert(std::get<0>(c1) == 0x1a);
  static_assert(std::get<1>(c1) == 0x2b);
  static_assert(std::get<2>(c1) == 0x3c);
  std::printf("parse_hex_rgb(\"#1a2b3c\") = %02x %02x %02x\n", std::get<0>(c1), std::get<1>(c1), std::get<2>(c1));

  // #RGB 短縮形
  constexpr auto c2 = parse_hex_rgb("#abc");
  static_assert(std::get<0>(c2) == 0xaa);
  std::printf("parse_hex_rgb(\"#abc\")     = %02x %02x %02x\n", std::get<0>(c2), std::get<1>(c2), std::get<2>(c2));

  // #RRGGBBAA
  constexpr auto c3 = parse_hex_rgba("#1a2b3c4d");
  std::printf("parse_hex_rgba(\"#1a2b3c4d\") = %02x %02x %02x %02x\n", std::get<0>(c3), std::get<1>(c3), std::get<2>(c3), std::get<3>(c3));

  // BGR/BGRA/ABGR 変換
  constexpr auto bgr = to_bgr(c1);
  std::printf("to_bgr = %02x %02x %02x\n", std::get<0>(bgr), std::get<1>(bgr), std::get<2>(bgr));

  constexpr auto rgba = parse_hex_rgba("#1a2b3c4d");
  constexpr auto bgra = to_bgra(rgba);
  constexpr auto abgr = to_abgr(rgba);
  std::printf("to_bgra = %02x %02x %02x %02x\n", std::get<0>(bgra), std::get<1>(bgra), std::get<2>(bgra), std::get<3>(bgra));
  std::printf("to_abgr = %02x %02x %02x %02x\n", std::get<0>(abgr), std::get<1>(abgr), std::get<2>(abgr), std::get<3>(abgr));

  return 0;
}
