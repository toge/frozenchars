#include "frozenchars.hpp"

int main() {
  constexpr auto x = frozenchars::frozen_format<"}">();
  (void)x;
  return 0;
}
