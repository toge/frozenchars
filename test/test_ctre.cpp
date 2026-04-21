#include "catch2/catch_all.hpp"
#include "frozenchars/ctre.hpp"

#if __has_include(<ctll/fixed_string.hpp>)

#include <ctre.hpp>
#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

// ---- コンパイル時検証: to_ctre パイプ ----
// "hello"_fs は FrozenString<6>: length==5, N-1==5 → to_ctre パイプ適用可
static constexpr auto ctre_pipe_hello = "hello"_fs | ops::to_ctre;
static_assert(ctre_pipe_hello.real_size == 5);

// ---- コンパイル時検証: to_ctre<S>() テンプレート ----
static constexpr auto s_world = "world"_fs;
static constexpr auto ctre_tmpl_world = frozenchars::to_ctre<s_world>();
static_assert(ctre_tmpl_world.real_size == 5);

// ---- コンパイル時検証: shrink_to_fit + to_ctre<S>() ----
// FrozenString<10> に FrozenString<3> からコピーして length==2, N-1==9 の状態を作る
// to_ctre<S>() は S.length ベースで変換するので正しく動作する
static constexpr FrozenString<3> s_hi_small{"hi"};
static constexpr auto s_expanded   = FrozenString<10>{s_hi_small};
static constexpr auto s_shrunk     = shrink_to_fit<s_expanded>();
static constexpr auto ctre_tmpl_hi = frozenchars::to_ctre<s_shrunk>();
static_assert(ctre_tmpl_hi.real_size == 2);

TEST_CASE("to_ctre pipe: FrozenString を ctll::fixed_string に変換") {
  static constexpr auto pattern = "hello"_fs | ops::to_ctre;
  static_assert(pattern.real_size == 5);
  REQUIRE(pattern.real_size == 5);

  // ctre::match で正規表現マッチングに使用できることを確認
  REQUIRE(ctre::match<ctre_pipe_hello>("hello"));
  REQUIRE_FALSE(ctre::match<ctre_pipe_hello>("world"));
  REQUIRE_FALSE(ctre::match<ctre_pipe_hello>("hello!"));
}

TEST_CASE("to_ctre<S>() template: FrozenString を ctll::fixed_string に変換") {
  static constexpr auto s = "world"_fs;
  static constexpr auto pattern = frozenchars::to_ctre<s>();
  static_assert(pattern.real_size == 5);
  REQUIRE(pattern.real_size == 5);

  REQUIRE(ctre::match<ctre_tmpl_world>("world"));
  REQUIRE_FALSE(ctre::match<ctre_tmpl_world>("hello"));
}

TEST_CASE("to_ctre<S>() template: 正規表現パターンとして使用") {
  // "[0-9]+" パターン
  static constexpr auto digit_fs      = "[0-9]+"_fs;
  static constexpr auto digit_pattern = frozenchars::to_ctre<digit_fs>();
  REQUIRE(ctre::match<digit_pattern>("123"));
  REQUIRE_FALSE(ctre::match<digit_pattern>("abc"));

  // "(foo|bar)" パターン
  static constexpr auto alt_fs      = "(foo|bar)"_fs;
  static constexpr auto alt_pattern = frozenchars::to_ctre<alt_fs>();
  REQUIRE(ctre::match<alt_pattern>("foo"));
  REQUIRE(ctre::match<alt_pattern>("bar"));
  REQUIRE_FALSE(ctre::match<alt_pattern>("baz"));
}

TEST_CASE("to_ctre<S>() template: shrink_to_fit と組み合わせ") {
  // バッファサイズ > length+1 の FrozenString は to_ctre パイプ不可
  // to_ctre<S>() は length ベースで変換するため正しく動作する
  static constexpr FrozenString<3> hi_small{"hi"};
  static constexpr auto expanded = FrozenString<10>{hi_small};
  static constexpr auto shrunk   = shrink_to_fit<expanded>();
  static constexpr auto pattern  = frozenchars::to_ctre<shrunk>();
  static_assert(pattern.real_size == 2);
  REQUIRE(pattern.real_size == 2);

  REQUIRE(ctre::match<ctre_tmpl_hi>("hi"));
  REQUIRE_FALSE(ctre::match<ctre_tmpl_hi>("hello"));
}

#endif // __has_include(<ctll/fixed_string.hpp>)
