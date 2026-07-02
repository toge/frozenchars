#include "frozenchars.hpp"

int main() {
  constexpr auto x = frozenchars::frozen_format<"{} {}">(42);
  (void)x;
  return 0;
}
