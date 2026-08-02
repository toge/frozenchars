#include "catch2/catch_all.hpp"

#include <array>
#include <string_view>
#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/multimap.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("frozen_multimap basic shape and duplicate keys", "[frozen_multimap]") {
  frozen_multimap<int, "gzip"_fs, "deflate"_fs, "gzip"_fs, "br"_fs> map{
    std::array<int, 4>{1, 2, 3, 4}
  };

  REQUIRE(map.size() == 4);
  REQUIRE(map.count("gzip") == 2);
  REQUIRE(map.count("deflate") == 1);
  REQUIRE(map.count("zstd") == 0);
  REQUIRE(map.contains("gzip"));
  REQUIRE(map.contains("br"));
  REQUIRE_FALSE(map.contains("zstd"));
}

TEST_CASE("frozen_multimap iteration is sorted order", "[frozen_multimap]") {
  frozen_multimap<int, "gzip"_fs, "deflate"_fs, "gzip"_fs, "br"_fs> map{
    std::array<int, 4>{1, 2, 3, 4}
  };
  // ソート順: br, deflate, gzip, gzip（同一キー内は宣言順）
  auto const expected = std::array<std::string_view, 4>{"br", "deflate", "gzip", "gzip"};
  auto it = map.begin();
  for (auto const& key : expected) {
    REQUIRE(it->key == key);
    ++it;
  }
  REQUIRE(it == map.end());
}

TEST_CASE("frozen_multimap equal_range over duplicate keys", "[frozen_multimap]") {
  frozen_multimap<int, "gzip"_fs, "deflate"_fs, "gzip"_fs, "br"_fs> map{
    std::array<int, 4>{1, 2, 3, 4}
  };

  auto const [first, last] = map.equal_range("gzip");
  REQUIRE(first != last);
  auto sum = 0;
  for (auto it = first; it != last; ++it) {
    sum += it->value;
  }
  REQUIRE(sum == 4);  // 1 + 3

  auto const [missing_first, missing_last] = map.equal_range("zstd");
  REQUIRE(missing_first == missing_last);
}

TEST_CASE("frozen_multimap find / keys / keys_in_declaration_order", "[frozen_multimap]") {
  frozen_multimap<int, "gzip"_fs, "deflate"_fs, "gzip"_fs, "br"_fs> map{
    std::array<int, 4>{1, 2, 3, 4}
  };

  REQUIRE(map.find("deflate")->value == 2);
  REQUIRE(map.find("br")->value == 4);
  REQUIRE(map.find("zstd") == map.end());
  REQUIRE(map.keys()[0] == "br");
  REQUIRE(map.keys_in_declaration_order()[0] == "gzip");
  REQUIRE(map.keys_in_declaration_order()[2] == "gzip");
}

TEST_CASE("frozen_multimap values are mutable via iterator", "[frozen_multimap]") {
  auto map = frozen_multimap<int, "a"_fs, "b"_fs, "a"_fs>{std::array<int, 3>{1, 2, 3}};

  auto const [first, last] = map.equal_range("a");
  for (auto it = first; it != last; ++it) {
    it->value += 10;
  }
  REQUIRE(map.find("a")->value == 11);
}

TEST_CASE("frozen_multimap initializer_list constructor", "[frozen_multimap]") {
  frozen_multimap<int, "a"_fs, "b"_fs, "a"_fs> map{1, 2, 3};

  REQUIRE(map.count("a") == 2);
  REQUIRE(map.find("b")->value == 2);
}

TEST_CASE("frozen_multimap make_frozen_multimap with pair entries", "[frozen_multimap]") {
  constexpr auto map = make_frozen_multimap<int, "gzip"_fs, "deflate"_fs, "gzip"_fs>(
    std::pair{"gzip", 1},
    std::pair{"deflate", 2},
    std::pair{"gzip", 3}
  );

  STATIC_CHECK(map.count("gzip") == 2);
  STATIC_CHECK(map.count("deflate") == 1);
  STATIC_CHECK(map.equal_range("gzip").first->value == 1);
}

TEST_CASE("frozen_multimap at returns first value for key", "[frozen_multimap]") {
  frozen_multimap<int, "gzip"_fs, "deflate"_fs, "gzip"_fs, "br"_fs> map{
    std::array<int, 4>{1, 2, 3, 4}
  };

  REQUIRE(map.at("gzip") == 1);     // 重複キーはソート順で最初の値
  REQUIRE(map.at("br") == 4);
  REQUIRE(std::as_const(map).at("deflate") == 2);
  REQUIRE_THROWS_AS(map.at("zstd"), std::out_of_range);

  map.at("br") = 44;
  REQUIRE(map.at("br") == 44);
}

TEST_CASE("frozen_multimap contains_all", "[frozen_multimap]") {
  constexpr auto map = make_frozen_multimap<int, "a"_fs, "b"_fs, "a"_fs>(
    std::array<int, 3>{1, 2, 3});

  STATIC_CHECK(map.contains_all<"a"_fs, "b"_fs>());
  STATIC_CHECK(map.contains_all<>());
  STATIC_CHECK(!map.contains_all<"a"_fs, "z"_fs>());
}
