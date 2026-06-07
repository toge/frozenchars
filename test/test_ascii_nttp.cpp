#include <catch2/catch_test_macros.hpp>
#include "frozenchars.hpp"

using namespace frozenchars;
namespace fops = frozenchars::ops;

// NTTP using encoded string
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
        
        // Encoded should only contain hex digits
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
        // This is the core requirement: Japanese characters via ASCII encoding in NTTP
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
