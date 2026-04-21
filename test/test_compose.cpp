#include "catch2/catch_all.hpp"
#include "frozenchars.hpp"

using namespace frozenchars;

namespace {

// ユーザー定義のアダプタ
struct my_custom_adaptor : pipe_adaptor_base {
  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const noexcept {
    FrozenString<N + 2> res{};
    res.buffer[0] = '[';
    for (size_t i = 0; i < str.length; ++i) {
      res.buffer[i + 1] = str.buffer[i];
    }
    res.buffer[str.length + 1] = ']';
    res.buffer[str.length + 2] = '\0';
    res.length = str.length + 2;
    return res;
  }
};

} // namespace

TEST_CASE("compose adaptor", "[pipe]") {
  SECTION("normalize multiline") {
    constexpr auto normalize = compose(
      ops::remove_comments("##"),
      ops::remove_leading_spaces,
      ops::remove_trailing_spaces,
      ops::join_lines(" ")
    );

    auto constexpr input = FrozenString(
      "  hello  ## comment\n"
      "  world  ## comment\n"
    );

    auto constexpr stripped = input | normalize;

    // expected: "hello world"
    STATIC_CHECK(stripped.sv() == "hello world");
  }

  SECTION("custom adaptor") {
    auto constexpr input = FrozenString("test");
    auto constexpr result = input | my_custom_adaptor{};
    STATIC_CHECK(result.sv() == "[test]");
  }

  SECTION("complex compose") {
    constexpr auto complex = compose(
      ops::trim,
      ops::toupper,
      my_custom_adaptor{}
    );

    auto constexpr input = FrozenString("  hello  ");
    auto constexpr result = input | complex;
    STATIC_CHECK(result.sv() == "[HELLO]");
  }

  SECTION("collapse spaces") {
    auto constexpr input = FrozenString("  hello   world  ");
    auto constexpr result = input | ops::collapse_spaces;
    STATIC_CHECK(result.sv() == " hello world ");
  }

  SECTION("predicate based trim and collapse") {
    auto constexpr input = FrozenString("\t\n hello   \t world \r\n");

    // 全ての空白（タブ、改行含む）を削除・集約
    constexpr auto normalize = compose(
      ops::trim_if<detail::is_any_whitespace>,
      ops::collapse_spaces_if<detail::is_any_whitespace>
    );

    auto constexpr result = input | normalize;
    STATIC_CHECK(result.sv() == "hello world");
  }

  SECTION("custom char trim") {
    auto constexpr input = FrozenString("---hello---");
    auto constexpr result = input | ops::trim_if<detail::is_char<'-'>>;
    STATIC_CHECK(result.sv() == "hello");
  }
}
