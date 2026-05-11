#include <iostream>
#include <string_view>
#include "frozenchars/frozen_string.hpp"
#include "frozenchars/detail/split_impl.hpp"

using namespace frozenchars;

struct comma_delim {
    constexpr bool operator()(char c) const noexcept { return c == ','; }
};

int main() {
    constexpr auto fs = FrozenString<20>("apple,banana,cherry");
    
    constexpr auto count = detail::split_count_impl<comma_delim{}>(fs);
    constexpr auto max_l = detail::max_token_len_impl<comma_delim{}>(fs);

    std::cout << "String: " << fs.sv() << "\n";
    std::cout << "Length: " << fs.length << "\n";
    std::cout << "Count: " << count << "\n";
    std::cout << "Max Len: " << max_l << "\n";

    if (count == 3 && max_l == 6) {
        std::cout << "Logic is CORRECT.\n";
    } else {
        std::cout << "Logic is WRONG.\n";
    }
    return 0;
}
