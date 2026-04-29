#include <utility>

#include "frozenchars/literals.hpp"
#include "frozenchars/perfect_map.hpp"

using namespace frozenchars::literals;

struct Target {
  Target() = delete;
  explicit Target(int) = delete;
};

template <typename EntryLike>
concept valid_perfect_map_entry =
  requires(EntryLike&& entry) {
    frozenchars::make_perfect_map<Target, "timeout"_fs>(
      std::forward<EntryLike>(entry)
    );
  };

static_assert(
  valid_perfect_map_entry<std::pair<char const*, int>>,
  "perfect_map entry value must be constructible as mapped_type"
);
