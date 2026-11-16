#include "catch2/catch_all.hpp"
#include "frozenchars/literals.hpp"
#include "frozenchars/map.hpp"
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

TEST_CASE("frozen_map Optimization D: binary search path for 65-256 keys", "[frozen_map][opt]") {
  // 65〜256 キーはソート済みキーへの二分探索を使う経路の検証
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

TEST_CASE("frozen_map Optimization E: CHD path for more than 256 keys", "[frozen_map][opt]") {
  // 256 キー超は CHD 2 段ハッシュを使う経路の検証
  using Map = frozen_map<int, "k000"_fs, "k001"_fs, "k002"_fs, "k003"_fs, "k004"_fs, "k005"_fs, "k006"_fs, "k007"_fs, "k008"_fs, "k009"_fs, "k010"_fs, "k011"_fs, "k012"_fs, "k013"_fs, "k014"_fs,
                         "k015"_fs, "k016"_fs, "k017"_fs, "k018"_fs, "k019"_fs, "k020"_fs, "k021"_fs, "k022"_fs, "k023"_fs, "k024"_fs, "k025"_fs, "k026"_fs, "k027"_fs, "k028"_fs, "k029"_fs, "k030"_fs,
                         "k031"_fs, "k032"_fs, "k033"_fs, "k034"_fs, "k035"_fs, "k036"_fs, "k037"_fs, "k038"_fs, "k039"_fs, "k040"_fs, "k041"_fs, "k042"_fs, "k043"_fs, "k044"_fs, "k045"_fs, "k046"_fs,
                         "k047"_fs, "k048"_fs, "k049"_fs, "k050"_fs, "k051"_fs, "k052"_fs, "k053"_fs, "k054"_fs, "k055"_fs, "k056"_fs, "k057"_fs, "k058"_fs, "k059"_fs, "k060"_fs, "k061"_fs, "k062"_fs,
                         "k063"_fs, "k064"_fs, "k065"_fs, "k066"_fs, "k067"_fs, "k068"_fs, "k069"_fs, "k070"_fs, "k071"_fs, "k072"_fs, "k073"_fs, "k074"_fs, "k075"_fs, "k076"_fs, "k077"_fs, "k078"_fs,
                         "k079"_fs, "k080"_fs, "k081"_fs, "k082"_fs, "k083"_fs, "k084"_fs, "k085"_fs, "k086"_fs, "k087"_fs, "k088"_fs, "k089"_fs, "k090"_fs, "k091"_fs, "k092"_fs, "k093"_fs, "k094"_fs,
                         "k095"_fs, "k096"_fs, "k097"_fs, "k098"_fs, "k099"_fs, "k100"_fs, "k101"_fs, "k102"_fs, "k103"_fs, "k104"_fs, "k105"_fs, "k106"_fs, "k107"_fs, "k108"_fs, "k109"_fs, "k110"_fs,
                         "k111"_fs, "k112"_fs, "k113"_fs, "k114"_fs, "k115"_fs, "k116"_fs, "k117"_fs, "k118"_fs, "k119"_fs, "k120"_fs, "k121"_fs, "k122"_fs, "k123"_fs, "k124"_fs, "k125"_fs, "k126"_fs,
                         "k127"_fs, "k128"_fs, "k129"_fs, "k130"_fs, "k131"_fs, "k132"_fs, "k133"_fs, "k134"_fs, "k135"_fs, "k136"_fs, "k137"_fs, "k138"_fs, "k139"_fs, "k140"_fs, "k141"_fs, "k142"_fs,
                         "k143"_fs, "k144"_fs, "k145"_fs, "k146"_fs, "k147"_fs, "k148"_fs, "k149"_fs, "k150"_fs, "k151"_fs, "k152"_fs, "k153"_fs, "k154"_fs, "k155"_fs, "k156"_fs, "k157"_fs, "k158"_fs,
                         "k159"_fs, "k160"_fs, "k161"_fs, "k162"_fs, "k163"_fs, "k164"_fs, "k165"_fs, "k166"_fs, "k167"_fs, "k168"_fs, "k169"_fs, "k170"_fs, "k171"_fs, "k172"_fs, "k173"_fs, "k174"_fs,
                         "k175"_fs, "k176"_fs, "k177"_fs, "k178"_fs, "k179"_fs, "k180"_fs, "k181"_fs, "k182"_fs, "k183"_fs, "k184"_fs, "k185"_fs, "k186"_fs, "k187"_fs, "k188"_fs, "k189"_fs, "k190"_fs,
                         "k191"_fs, "k192"_fs, "k193"_fs, "k194"_fs, "k195"_fs, "k196"_fs, "k197"_fs, "k198"_fs, "k199"_fs, "k200"_fs, "k201"_fs, "k202"_fs, "k203"_fs, "k204"_fs, "k205"_fs, "k206"_fs,
                         "k207"_fs, "k208"_fs, "k209"_fs, "k210"_fs, "k211"_fs, "k212"_fs, "k213"_fs, "k214"_fs, "k215"_fs, "k216"_fs, "k217"_fs, "k218"_fs, "k219"_fs, "k220"_fs, "k221"_fs, "k222"_fs,
                         "k223"_fs, "k224"_fs, "k225"_fs, "k226"_fs, "k227"_fs, "k228"_fs, "k229"_fs, "k230"_fs, "k231"_fs, "k232"_fs, "k233"_fs, "k234"_fs, "k235"_fs, "k236"_fs, "k237"_fs, "k238"_fs,
                         "k239"_fs, "k240"_fs, "k241"_fs, "k242"_fs, "k243"_fs, "k244"_fs, "k245"_fs, "k246"_fs, "k247"_fs, "k248"_fs, "k249"_fs, "k250"_fs, "k251"_fs, "k252"_fs, "k253"_fs, "k254"_fs,
                         "k255"_fs, "k256"_fs, "k257"_fs, "k258"_fs, "k259"_fs, "k260"_fs, "k261"_fs, "k262"_fs, "k263"_fs, "k264"_fs, "k265"_fs, "k266"_fs, "k267"_fs, "k268"_fs, "k269"_fs, "k270"_fs,
                         "k271"_fs, "k272"_fs, "k273"_fs, "k274"_fs, "k275"_fs, "k276"_fs, "k277"_fs, "k278"_fs, "k279"_fs, "k280"_fs, "k281"_fs, "k282"_fs, "k283"_fs, "k284"_fs, "k285"_fs, "k286"_fs,
                         "k287"_fs, "k288"_fs, "k289"_fs, "k290"_fs, "k291"_fs, "k292"_fs, "k293"_fs, "k294"_fs, "k295"_fs, "k296"_fs, "k297"_fs, "k298"_fs, "k299"_fs, "a_much_longer_key_999"_fs>;

  auto values = std::array<int, 301>{};
  for (auto i = 0uz; i < 301; ++i)
    values[i] = static_cast<int>(i + 1);
  Map map{values};

  // 全キーヒット
  for (auto i = 0uz; i < 300; ++i) {
    CAPTURE(i);
    REQUIRE(map.at(Map::keys_in_declaration_order()[i])->get() == static_cast<int>(i + 1));
  }
  REQUIRE(map.at("a_much_longer_key_999")->get() == 301);

  // ミス: 長さ一致・長さ不一致の両方
  REQUIRE_FALSE(map.contains("k999")); // 長さ一致でハッシュ検索まで到達
  REQUIRE_FALSE(map.contains("zz"));   // 長さ不一致で早期棄却
  REQUIRE_FALSE(map.contains(""));

  // constexpr でも同じ経路が動作する
  constexpr auto found = [] {
    Map m{};
    return m.find("k042") != m.end() && m.find("k999") == m.end();
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
