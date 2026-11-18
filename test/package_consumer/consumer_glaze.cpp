#include "frozenchars/glaze_frozen_map.hpp"

#include <array>

auto main() -> int {
  constexpr auto value = frozenchars::frozen_map<int, "key">{std::array<int, 1>{1}};
  return value.at("key") == 1 ? 0 : 1;
}
