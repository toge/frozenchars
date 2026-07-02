#include "frozenchars.hpp"

int main() {
  constexpr auto x = frozenchars::frozen_format<"hello {">(42);
  (void)x;
  return 0;
}
