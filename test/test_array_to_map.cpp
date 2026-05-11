#include "frozenchars/frozen_map.hpp"
#include "frozenchars/split.hpp"
#include "frozenchars/literals.hpp"
#include <iostream>

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

int main() {
    static constexpr auto keys = split_v<"apple,banana,cherry"_fs, detail::is_char<','>>;
    
    using Keys = key_list_from_array<keys>;
    using MapType = map_from_key_list<int, Keys>;
    
    MapType map{1, 2, 3};

    std::cout << "apple: " << map["apple"] << "\n";
    std::cout << "banana: " << map["banana"] << "\n";
    std::cout << "cherry: " << map["cherry"] << "\n";

    if (map.contains("banana") && map["apple"] == 1) {
        std::cout << "SUCCESS: Array to Map transformation works.\n";
        return 0;
    }
    return 1;
}
