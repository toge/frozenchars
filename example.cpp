#include <iostream>
#include <tuple>
#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L
#include <print>
#endif

#include "frozenchars.hpp"
#include "frozenchars/static_perfect_map.hpp"

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

  frozenchars::StaticPerfectMap<int, "timeout"_fs, "retry"_fs> map{};
  map["timeout"] = 30;
  map["retry"] = 5;

  auto const pair = std::tuple{map["timeout"], map["retry"]};
#if defined(__cpp_placeholder_variables) && __cpp_placeholder_variables >= 202306L
  auto const [timeout, _] = pair;
#else
  auto const timeout = std::get<0>(pair);
#endif

#if defined(__cpp_lib_print) && __cpp_lib_print >= 202207L
  std::print("{}\n{}\ntimeout={}, retry={}\n", msg.sv(), removed.sv(), timeout, map["retry"]);
#else
  std::cout << msg.sv() << '\n';
  std::cout << removed.sv() << '\n';
  std::cout << "timeout=" << timeout << ", retry=" << map["retry"] << '\n';
#endif
  return 0;
}
