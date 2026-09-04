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
    REQUIRE(map.at("a")->get() == 1);
    REQUIRE(map.at("bb")->get() == 2);
    REQUIRE(map.at("ccc")->get() == 3);
    REQUIRE_FALSE(map.contains("b"));
    REQUIRE_FALSE(map.contains("aa"));
    REQUIRE_FALSE(map.contains("cccc"));
  }

  SECTION("Duplicate lengths") {
    using Map = frozen_map<int, "foo"_fs, "bar"_fs, "baz"_fs>;
    Map map{std::array{1, 2, 3}};
    REQUIRE(map.at("foo")->get() == 1);
    REQUIRE(map.at("bar")->get() == 2);
    REQUIRE(map.at("baz")->get() == 3);
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

TEST_CASE("frozen_map Optimization D: CHD path for more than 64 keys", "[frozen_map][opt]") {
  // 65 キー以上は単一シードの完全ハッシュではなく CHD 2 段ハッシュを使う経路の検証
  using Map = frozen_map<int,
    "key001"_fs, "key002"_fs, "key003"_fs, "key004"_fs, "key005"_fs,
    "key006"_fs, "key007"_fs, "key008"_fs, "key009"_fs, "key010"_fs,
    "key011"_fs, "key012"_fs, "key013"_fs, "key014"_fs, "key015"_fs,
    "key016"_fs, "key017"_fs, "key018"_fs, "key019"_fs, "key020"_fs,
    "key021"_fs, "key022"_fs, "key023"_fs, "key024"_fs, "key025"_fs,
    "key026"_fs, "key027"_fs, "key028"_fs, "key029"_fs, "key030"_fs,
    "key031"_fs, "key032"_fs, "key033"_fs, "key034"_fs, "key035"_fs,
    "key036"_fs, "key037"_fs, "key038"_fs, "key039"_fs, "key040"_fs,
    "key041"_fs, "key042"_fs, "key043"_fs, "key044"_fs, "key045"_fs,
    "key046"_fs, "key047"_fs, "key048"_fs, "key049"_fs, "key050"_fs,
    "key051"_fs, "key052"_fs, "key053"_fs, "key054"_fs, "key055"_fs,
    "key056"_fs, "key057"_fs, "key058"_fs, "key059"_fs, "key060"_fs,
    "key061"_fs, "key062"_fs, "key063"_fs, "key064"_fs, "key065"_fs,
    "key066"_fs, "key067"_fs, "key068"_fs, "key069"_fs, "key070"_fs,
    "key071"_fs, "key072"_fs, "key073"_fs, "key074"_fs, "key075"_fs,
    "key076"_fs, "key077"_fs, "key078"_fs, "key079"_fs, "key080"_fs,
    "key081"_fs, "key082"_fs, "key083"_fs, "key084"_fs, "key085"_fs,
    "key086"_fs, "key087"_fs, "key088"_fs, "key089"_fs, "key090"_fs,
    "key091"_fs, "key092"_fs, "key093"_fs, "key094"_fs, "key095"_fs,
    "key096"_fs, "key097"_fs, "key098"_fs, "key099"_fs, "a_much_longer_key_101"_fs>;

  auto values = std::array<int, 100>{};
  for (auto i = 0uz; i < 100; ++i) values[i] = static_cast<int>(i + 1);
  Map map{values};

  // 全キーヒット
  for (auto i = 0uz; i < 99; ++i) {
    CAPTURE(i);
    REQUIRE(map.at(Map::keys_in_declaration_order()[i])->get() == static_cast<int>(i + 1));
  }
  REQUIRE(map.at("a_much_longer_key_101")->get() == 100);

  // ミス: 長さ一致・長さ不一致の両方
  REQUIRE_FALSE(map.contains("key100"));  // 長さ一致でハッシュ検索まで到達
  REQUIRE_FALSE(map.contains("nope"));    // 長さ不一致で早期棄却
  REQUIRE_FALSE(map.contains(""));

  // constexpr でも同じ経路が動作する
  constexpr auto found = [] {
    Map m{};
    return m.find("key042") != m.end() && m.find("key100") == m.end();
  }();
  STATIC_CHECK(found);
}

TEST_CASE("frozen_map unique lengths detection", "[frozen_map][opt]") {
  // private 静的メンバは直接テストできないため、全キーで O(1) パスが動作することを確認する
  auto map = make_frozen_map<int, "a"_fs, "bb"_fs, "ccc"_fs>(
    std::pair{"a", 1},
    std::pair{"bb", 2},
    std::pair{"ccc", 3}
  );
  REQUIRE(map.at("a")->get() == 1);
  REQUIRE(map.at("bb")->get() == 2);
  REQUIRE(map.at("ccc")->get() == 3);
}
