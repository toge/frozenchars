#include <cstdio>
#include "frozenchars.hpp"

int main() {
  using namespace frozenchars;
  using namespace frozenchars::literals;

  // Basic replacement
  constexpr auto msg1 = frozen_format<"The answer is {}."_fs>(42);
  std::printf("%s\n", msg1.sv().data());

  // Hex with width and zero-pad
  constexpr auto msg2 = frozen_format<"Hex: {:08X}"_fs>(0xABCD);
  std::printf("%s\n", msg2.sv().data());

  // Alignment
  constexpr auto msg3 = frozen_format<"{:<10}|{:>10}"_fs>("left", "right");
  std::printf("%s\n", msg3.sv().data());

  // Float with precision
  constexpr auto msg4 = frozen_format<"Pi = {:.5f}"_fs>(3.1415926535);
  std::printf("%s\n", msg4.sv().data());

  // Sign control
  constexpr auto msg5 = frozen_format<"{:+} {:+}"_fs>(1, -1);
  std::printf("%s\n", msg5.sv().data());

  return 0;
}
