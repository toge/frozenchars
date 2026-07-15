#include <catch2/catch_test_macros.hpp>
#include "frozenchars.hpp"

/** @brief ASCII エンコードを介した非 ASCII 文字列の NTTP 利用テスト。 */

using namespace frozenchars;
namespace fops = frozenchars::ops;

/** @brief ASCII エンコード済み文字列を NTTP として受け取り、デコードして保持する struct */
template <FrozenString Encoded>
struct JapaneseText {
    static constexpr auto value = Encoded | fops::from_ascii;
};

TEST_CASE("ASCII conversion for NTTP with Japanese characters") {
  SECTION("Hex encoding/decoding via functions") {
    constexpr auto original = freeze("こんにちは");
    constexpr auto encoded = to_ascii(original);
    constexpr auto decoded = from_ascii(encoded);

    REQUIRE(decoded.sv() == original.sv());

    // エンコード結果は16進数文字のみで構成される
    for (char c : encoded.sv()) {
      bool is_hex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
      REQUIRE(is_hex);
    }
  }

  SECTION("Hex encoding/decoding via pipes") {
    constexpr auto original = freeze("こんにちは");
    constexpr auto encoded = original | fops::to_ascii;
    constexpr auto decoded = encoded | fops::from_ascii;

    REQUIRE(decoded.sv() == original.sv());
  }

  SECTION("NTTP usage with Japanese characters") {
    // 日本語文字を ASCII エンコードして NTTP に渡すのが本テストの核心要件
    using Msg = JapaneseText<freeze("プログラミング") | fops::to_ascii>;

    REQUIRE(Msg::value.sv() == "プログラミング");
  }

  SECTION("NTTP template version usage") {
    static constexpr auto original = freeze("日本語テスト");
    static constexpr auto encoded = to_ascii<original>();
    static constexpr auto decoded = from_ascii<encoded>();

    REQUIRE(decoded.sv() == original.sv());
    REQUIRE(decoded.size() == original.size());
  }
}
