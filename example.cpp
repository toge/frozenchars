#include <iostream>
#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L
#include <print>
#endif

#include "frozenchars.hpp"
#include "frozenchars/frozen_map.hpp"

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

  auto map = frozenchars::make_frozen_map<int, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", 5},
    std::pair{"timeout", 30}
  );

#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L
  std::print("{}\n{}\n", msg.sv(), removed.sv());
  if (auto const it = map.find("timeout"); it != map.end()) {
    auto const [key, value] = *it;
    std::print("{}={}\n", key, value);
  }
  for (auto it = map.begin(); it != map.end(); it++) {
    auto const [key, value] = *it;
    std::print("{}={}\n", key, value);
  }
#else
  std::cout << msg.sv() << '\n';
  std::cout << removed.sv() << '\n';
  if (auto const it = map.find("timeout"); it != map.end()) {
    auto const [key, value] = *it;
    std::cout << key << '=' << value << '\n';
  }
  for (auto it = map.begin(); it != map.end(); it++) {
    auto const [key, value] = *it;
    std::cout << key << '=' << value << '\n';
  }
#endif
  return 0;
}
