#include "catch2/catch_all.hpp"
#include "frozenchars/map.hpp"
#include "frozenchars/split.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

/**
 * @brief split_v で分割したキー配列から frozen_map の型を構築するメタプログラミング手法のテスト。
 *   key_list / key_list_from_array / map_from_key_list の連携により、
 *   コンパイル時に文字列分割結果からマップ型を生成できることを確認する。
 */

/** @brief FrozenString のパックを保持する型リスト。テンプレートメタプログラミングの中間表現として使う。 */
template <FrozenString... Keys>
struct key_list {};

/** @brief コンパイル時配列の各要素を FrozenString として key_list に展開する実装ヘルパ。 */
template <auto Array, std::size_t... I>
constexpr auto make_key_list_impl(std::index_sequence<I...>) {
  return key_list<Array[I]...>{};
}

/** @brief コンパイル時配列から key_list 型を導出するエイリアステンプレート。 */
template <auto Array>
using key_list_from_array = decltype(make_key_list_impl<Array>(std::make_index_sequence<Array.size()>{}));

/** @brief key_list から frozen_map 型を導出するための部分特殊化ヘルパ。 */
template <typename T, typename KeyList>
struct map_helper;

template <typename T, FrozenString... Keys>
struct map_helper<T, key_list<Keys...>> {
  using type = frozen_map<T, Keys...>;
};

/** @brief key_list から frozen_map<T, Keys...> 型を導出するエイリアステンプレート。 */
template <typename T, typename KeyList>
using map_from_key_list = typename map_helper<T, KeyList>::type;

TEST_CASE("Array to Map transformation", "[frozen_map]") {
  static constexpr auto keys = split_v<"apple,banana,cherry"_fs, detail::is_char<','>>;

  using Keys = key_list_from_array<keys>;
  using MapType = map_from_key_list<int, Keys>;

  MapType map{1, 2, 3};

  REQUIRE(map["apple"] == 1);
  REQUIRE(map["banana"] == 2);
  REQUIRE(map["cherry"] == 3);
  REQUIRE(map.contains("banana"));
}
