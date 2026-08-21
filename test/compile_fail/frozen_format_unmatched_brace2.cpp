#include "frozenchars/frozen_format.hpp"

int main() {
  constexpr auto x = frozenchars::frozen_format<"}">();
  (void)x;
  return 0;
}
