#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_map.hpp"

using namespace frozenchars::literals;

struct BadKey {};

template <typename EntryLike>
concept valid_frozen_map_entry =
  requires(EntryLike&& entry) {
    frozenchars::make_frozen_map<int, "timeout"_fs>(
      std::forward<EntryLike>(entry)
    );
  };

static_assert(
  valid_frozen_map_entry<std::pair<BadKey, int>>,
  "frozen_map entry key must be convertible to std::string_view"
);
