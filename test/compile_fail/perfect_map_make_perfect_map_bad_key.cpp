#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/perfect_map.hpp"

using namespace frozenchars::literals;

struct BadKey {};

template <typename EntryLike>
concept valid_perfect_map_entry =
  requires(EntryLike&& entry) {
    frozenchars::make_perfect_map<int, "timeout"_fs>(
      std::forward<EntryLike>(entry)
    );
  };

static_assert(
  valid_perfect_map_entry<std::pair<BadKey, int>>,
  "PerfectMap entry key must be convertible to std::string_view"
);
