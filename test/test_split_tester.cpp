#include <iostream>
#include "frozenchars/frozen_string.hpp"
#include "frozenchars/detail/split_impl.hpp"
#include "frozenchars/detail/char_utils.hpp"
#include <array>

using namespace frozenchars;

/** @brief split のテンプレートテスト補助構造体。手動デバッグ用スタンドアロンファイル。
    @details コンパイル時文字列分割と型安全なトークン格納を検証する。*/

/** @brief コンパイル時分割テスト用テンプレート構造体。
    @tparam Str 分割対象の FrozenString
    @tparam IsDelim 区切り判定関数オブジェクト型
    @details トークン数を事前計算し固定サイズ配列に分割結果を格納する。*/
template <auto Str, auto IsDelim>
struct split_tester {
  static constexpr auto token_count = detail::split_count_impl<IsDelim>(Str);
  static constexpr auto max_len     = detail::max_token_len_impl<IsDelim>(Str);

  static constexpr auto get_value() {
    std::array<FrozenString<max_len + 1>, token_count> res{};
    std::size_t src = 0;
    std::size_t dst = 0;
    while (src < Str.length && dst < token_count) {
      while (src < Str.length && IsDelim(Str.buffer[src])) ++src;
      if (src >= Str.length) break;
      std::size_t token_len = 0;
      while (src < Str.length && !IsDelim(Str.buffer[src])) {
        res[dst].buffer[token_len++] = Str.buffer[src++];
      }
      res[dst].buffer[token_len] = '\0';
      res[dst].length = token_len;
      ++dst;
    }
    return res;
  }
  static constexpr auto value = get_value();
};

/** @brief カンマ区切り判定用関数オブジェクト。*/
struct comma_delim {
  constexpr bool operator()(char c) const noexcept { return c == ','; }
};

int main() {
  static constexpr auto fs = FrozenString<20>("apple,banana,cherry");
  using Tester = split_tester<fs, comma_delim{}>;

  std::cout << "Tester Token Count: " << Tester::token_count << "\n";
  std::cout << "Tester Max Len: " << Tester::max_len << "\n";
  std::cout << "Result Size: " << Tester::value.size() << "\n";

  for (auto const& s : Tester::value) {
    std::cout << "Token: [" << s.sv() << "]\n";
  }

  if (Tester::token_count == 3 && Tester::value[0].sv() == "apple") {
    std::cout << "SUCCESS\n";
    return 0;
  }
  return 1;
}
