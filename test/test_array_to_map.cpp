#include "catch2/catch_all.hpp"
#include "frozenchars/frozen_map.hpp"
#include "frozenchars/split.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

template <FrozenString... Keys>
struct key_list {};

template <auto Array, std::size_t... I>
consteval auto make_key_list_impl(std::index_sequence<I...>) {
    return key_list<Array[I]...>{};
}

template <auto Array>
using key_list_from_array = decltype(make_key_list_impl<Array>(std::make_index_sequence<Array.size()>{}));

template <typename T, typename KeyList>
struct map_helper;

template <typename T, FrozenString... Keys>
struct map_helper<T, key_list<Keys...>> {
    using type = frozen_map<T, Keys...>;
};

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
