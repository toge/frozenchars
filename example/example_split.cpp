#include "frozenchars.hpp"
#include <iostream>

int main() {
  using namespace frozenchars;

  // 例1: FrozenStringを区切り文字で分割
  constexpr auto str1 = FrozenString{"apple,banana,cherry,date"};
  constexpr auto count1 = split_count(str1, ',');
  constexpr auto parts1 = split<count1>(str1, ',');

  std::cout << "Split by comma:\n";
  for (auto const& part : parts1) {
    std::cout << "  - " << part.sv() << '\n';
  }

  // 例2: 文字列リテラルを区切り文字で分割
  constexpr auto count2 = split_count("one:two:three:four", ':');
  constexpr auto parts2 = split<count2>("one:two:three:four", ':');

  std::cout << "\nSplit by colon:\n";
  for (auto const& part : parts2) {
    std::cout << "  - " << part.sv() << '\n';
  }

  // 例3: セミコロン区切り
  constexpr auto str3 = FrozenString{"x;y;z"};
  constexpr auto count3 = split_count(str3, ';');
  constexpr auto parts3 = split<count3>(str3, ';');

  std::cout << "\nSplit by semicolon:\n";
  for (auto const& part : parts3) {
    std::cout << "  - " << part.sv() << '\n';
  }

  // 例4: トークン数を事前に取得
  constexpr auto count4 = split_count(FrozenString{"a-b-c-d"}, '-');
  std::cout << "\nToken count for 'a-b-c-d' split by '-': " << count4 << '\n';

  return 0;
}


