#include <iostream>

#include "frozenchars.hpp"

int main() {
  auto constexpr msg = frozenchars::concat("answer=", 42, ", hex=0x", frozenchars::Hex(255));
  static_assert(msg.sv() == "answer=42, hex=0xff");

  using namespace frozenchars::literals;
  namespace fops = frozenchars::ops;
  auto constexpr src = "value /* remove\nblock */ end"_fs;
  auto constexpr removed = frozenchars::remove_range_comments(src, "/*", "*/");
  auto constexpr removed_pipe = src | fops::remove_range_comments("/*", "*/");
  static_assert(removed.sv() == "value  end");
  static_assert(removed_pipe.sv() == "value  end");

  std::cout << msg.sv() << '\n';
  std::cout << removed.sv() << '\n';
  return 0;
}
