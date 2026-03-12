#include <iostream>

#include "frozenchars.hpp"

int main() {
  auto constexpr msg = frozenchars::concat("answer=", 42, ", hex=0x", frozenchars::Hex(255));
  static_assert(msg.sv() == "answer=42, hex=0xff");

  std::cout << msg.sv() << '\n';
  return 0;
}
