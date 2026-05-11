#include <iostream>
#include "frozenchars/frozen_string.hpp"
#include "frozenchars/detail/split_impl.hpp"
#include "frozenchars/detail/char_utils.hpp"

using namespace frozenchars;

int main() {
    static constexpr auto fs = FrozenString<20>("apple,banana,cherry");
    
    constexpr auto count = detail::split_count_impl<detail::is_char<','>>(fs);
    
    std::cout << "Count with is_char: " << count << "\n";
    
    if (count == 3) {
        std::cout << "SUCCESS\n";
        return 0;
    }
    return 1;
}
