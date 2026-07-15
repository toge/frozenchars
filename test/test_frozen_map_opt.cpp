#include "catch2/catch_all.hpp"
#include "frozenchars/map.hpp"
#include "frozenchars/literals.hpp"
#include <string_view>

using namespace frozenchars;
using namespace frozenchars::literals;

/**
 * @brief frozen_map の最適化パス（長さフィルタリング・ハッシュレスルックアップ・constexpr find_index_raw）の動作検証。
 *   各最適化が正しく有効になり、ヒット／ミスともに期待通りの結果を返すことを確認する。
 */

TEST_CASE("frozen_map Optimization A: Length filtering", "[frozen_map][opt]") {
  using Map = frozen_map<int, "foo"_fs, "bar"_fs>;
  Map map{std::array{1, 2}};

  // 長さ3は有効だが、長さ4は登録されていない
  REQUIRE(map.contains("foo"));
  REQUIRE(map.contains("bar"));
  REQUIRE_FALSE(map.contains("quxx")); // 長さ4 → 早期終了
  REQUIRE_FALSE(map.contains("f"));    // 長さ1 → 早期終了
}

TEST_CASE("frozen_map Optimization B: Hashless lookup", "[frozen_map][opt]") {
  SECTION("Unique lengths") {
    using Map = frozen_map<int, "a"_fs, "bb"_fs, "ccc"_fs>;

    // 内部フラグ all_lengths_unique_ を直接確認はできないが、挙動で検証する
    Map map{std::array{1, 2, 3}};
    REQUIRE(map.at("a") == 1);
    REQUIRE(map.at("bb") == 2);
    REQUIRE(map.at("ccc") == 3);
    REQUIRE_FALSE(map.contains("b"));
    REQUIRE_FALSE(map.contains("aa"));
    REQUIRE_FALSE(map.contains("cccc"));
  }

  SECTION("Duplicate lengths") {
    using Map = frozen_map<int, "foo"_fs, "bar"_fs, "baz"_fs>;
    Map map{std::array{1, 2, 3}};
    REQUIRE(map.at("foo") == 1);
    REQUIRE(map.at("bar") == 2);
    REQUIRE(map.at("baz") == 3);
    REQUIRE_FALSE(map.contains("qux"));
  }
}

TEST_CASE("frozen_map Optimization C: find_index_raw constexpr", "[frozen_map][opt]") {
  constexpr auto result = [] {
    auto m = frozen_map<int, "foo"_fs, "bar"_fs, "baz"_fs>{std::array{1, 2, 3}};
    return m.find("bar") != m.end();
  }();
  static_assert(result);
  REQUIRE(result);
}

TEST_CASE("frozen_map unique lengths detection", "[frozen_map][opt]") {
  // private 静的メンバは直接テストできないため、全キーで O(1) パスが動作することを確認する
  auto map = make_frozen_map<int, "a"_fs, "bb"_fs, "ccc"_fs>(
    std::pair{"a", 1},
    std::pair{"bb", 2},
    std::pair{"ccc", 3}
  );
  REQUIRE(map.at("a") == 1);
  REQUIRE(map.at("bb") == 2);
  REQUIRE(map.at("ccc") == 3);
}
