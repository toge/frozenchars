#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/frozen_map.hpp"

using namespace frozenchars::literals;

struct Target {
  Target() = delete;
  explicit Target(int) = delete;
};

template <typename EntryLike>
concept valid_frozen_map_entry =
  requires(EntryLike&& entry) {
    frozenchars::make_frozen_map<Target, "timeout"_fs>(
      std::forward<EntryLike>(entry)
    );
  };

static_assert(
  valid_frozen_map_entry<std::pair<char const*, int>>,
  "frozen_map entry value must be constructible as mapped_type"
);
