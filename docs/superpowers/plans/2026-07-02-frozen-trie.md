# frozen_trie Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add compile-time compressed trie (radix tree) lookup as `frozen_trie_index<Keys...>`, with `frozen_trie_set` and `frozen_trie_map` wrappers, plus tests and benchmarks against `frozen_map`.

**Architecture:** Compile-time compressed trie in flat `std::array` storage (3 arrays: labels, nodes, children). Lookup is a pure tree walk — no hash computation. Independent parallel type to `frozen_map` for comparison.

**Tech Stack:** C++23 `consteval`, `FrozenString<N>` NTTP, Catch2 v3 (tests + benchmarks).

---

### Task 1: `trie_index.hpp` — Core compile-time compressed trie

**Files:**
- Create: `include/frozenchars/trie_index.hpp`
- Build check: `g++ -std=c++23 -fsyntax-only -I include -c include/frozenchars/trie_index.hpp`

**Data structures:**

```cpp
// include/frozenchars/trie_index.hpp
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <algorithm>

#include "string.hpp"

namespace frozenchars {

namespace detail {

/// One node in the flat-compact trie.
/// Max: 256 nodes, 65535 bytes labels, 256 children per node, 127 value indices.
struct alignas(8) trie_flat_node {
  std::uint16_t label_offset;   // offset into k_labels
  std::uint8_t  label_length;   // length of this node's label string (0 for root)
  std::uint8_t  first_child;    // offset into k_children
  std::uint8_t  child_count;    // number of children (0 = leaf)
  std::int8_t   value_index;    // -1 = internal node, 0..126 = terminal
};

/// Result of consteval trie construction.
template <std::size_t NodeCount, std::size_t LabelSize, std::size_t ChildCount>
struct trie_storage {
  static constexpr std::size_t k_node_count = NodeCount;
  static constexpr std::size_t k_label_size = LabelSize;
  static constexpr std::size_t k_child_count = ChildCount;
  std::array<trie_flat_node, NodeCount> nodes{};
  std::array<char, LabelSize> labels{};
  std::array<std::uint8_t, ChildCount> children{};
};

/// Compile-time trie index. Built from FrozenString... Keys.
template <FrozenString... Keys>
struct frozen_trie_index {
  static_assert(sizeof...(Keys) > 0, "frozen_trie_index requires at least one key");
  // duplicate check — reuse has_duplicate_keys from map.hpp or local impl

  static constexpr auto k_key_count = sizeof...(Keys);

  static constexpr std::array<std::string_view, k_key_count> k_key_views_{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };

  /// Look up a key. Returns value index on success, k_key_count (npos) on miss.
  static constexpr auto find(std::string_view key) noexcept -> std::size_t;

  /// keys in declaration order
  static constexpr auto keys() noexcept -> std::span<const std::string_view, k_key_count> {
    return k_key_views_;
  }

  /// storage built at compile time
  static constexpr auto k_storage = build_trie();
  static constexpr auto k_nodes    = k_storage.nodes;
  static constexpr auto k_labels   = k_storage.labels;
  static constexpr auto k_children = k_storage.children;
};

} // namespace detail
} // namespace frozenchars
```

**Build algorithm (consteval)**:

```cpp
template <FrozenString... Keys>
struct frozen_trie_index {
  // ── helpers ──
  struct kv_pair { std::string_view key; int value_index; };

  /// Longest common prefix of a list of keys
  static consteval auto longest_common_prefix(std::span<const kv_pair> keys) -> std::size_t;

  /// Group keys by the first character after removing a prefix of `skip` chars
  static consteval auto group_by_first_char(std::span<const kv_pair> keys, std::size_t skip);

  /// Recursive trie builder
  static consteval auto build_trie_impl(
    std::span<const kv_pair> keys,
    trie_storage& storage,    // mutable ref to accumulating result
    int& next_node,
    int& next_label_offset,
    int& next_child_offset
  ) -> int;  // returns node index

  /// Entry point
  static consteval auto build_trie() -> /* deduced storage type */;
};
```

**`build_trie_impl` logic:**

```
1. Find LCP of all keys
2. If keys[0].key == LCP (first key is fully consumed by LCP):
   a. Create node with label=LCP, value_index=keys[0].value_index
   b. Remaining keys = keys[1..] with LCP stripped
   c. If remaining keys is empty → leaf, return node
   d. Else → sort remaining by first char, build child for each group, return node with children
3. Else (LCP does not fully consume any key):
   a. Create node with label=LCP, value_index=-1 (internal)
   b. Strip LCP from all keys
   c. Sort by first char, build child for each group, return node with children
```

**`find` implementation:**

```cpp
static constexpr auto find(std::string_view key) noexcept -> std::size_t {
  auto pos = 0uz;
  auto node_idx = 0;  // root

  while (true) {
    auto const& node = k_nodes[static_cast<std::size_t>(node_idx)];

    // compare label with key at pos
    auto const remaining = key.size() - pos;
    if (remaining < static_cast<std::size_t>(node.label_length))
      return k_key_count;  // label too long for remaining key
    for (auto i = 0; i < node.label_length; ++i) {
      if (key[pos + static_cast<std::size_t>(i)] != k_labels[static_cast<std::size_t>(node.label_offset + i)])
        return k_key_count;
    }
    pos += static_cast<std::size_t>(node.label_length);

    if (pos == key.size()) {
      // consumed entire key — terminal node?
      if (node.value_index >= 0)
        return static_cast<std::size_t>(node.value_index);
      return k_key_count;  // partial match only
    }

    if (node.child_count == 0)
      return k_key_count;

    // find child by next character
    auto const next_char = key[pos];
    auto const child_begin = k_children.data() + node.first_child;
    auto const child_end = child_begin + node.child_count;
    auto found = false;
    for (auto it = child_begin; it != child_end; ++it) {
      auto const& child = k_nodes[*it];
      if (k_labels[child.label_offset] == next_char) {
        node_idx = static_cast<int>(*it);
        ++pos;
        found = true;
        break;
      }
    }
    if (!found)
      return k_key_count;
  }
}
```

**Edge case: child > 8 optimization**
When `child_count > 8`, use a `uint8_t[256]` direct-lookup table instead of scanning:

```cpp
// inside frozen_trie_index
static constexpr auto k_child_lut_ = [] {
  std::array<std::uint8_t, 256> lut{};
  lut.fill(0xFF);  // sentinel
  // populate from children
  return lut;
}();
```

Only materialize this when any node has >8 children.

**Steps:**

- [ ] **Step 1: Write the header skeleton**

Create `include/frozenchars/trie_index.hpp` with the structures above (`trie_flat_node`, `trie_storage`, `frozen_trie_index`). Add `#include` guards, include `string.hpp`. Declare `find()` as a forward declaration only.

- [ ] **Step 2: Syntax-check the skeleton**

Run: `g++ -std=c++23 -fsyntax-only -I include -c include/frozenchars/trie_index.hpp`
Expected: clean compile (just `find` not defined yet, that's OK at this stage)

- [ ] **Step 3: Implement impl detail: longest_common_prefix**

```cpp
static consteval auto longest_common_prefix(std::span<const kv_pair> keys) -> std::size_t {
  auto const first = keys[0].key;
  auto lcp = first.size();
  for (auto i = 1uz; i < keys.size(); ++i) {
    auto const other = keys[i].key;
    auto const limit = std::min(lcp, other.size());
    std::size_t j = 0;
    while (j < limit && first[j] == other[j]) ++j;
    lcp = j;
    if (lcp == 0) return 0;
  }
  return lcp;
}
```

- [ ] **Step 4: Implement `build_trie_impl` and `build_trie`**

See algorithm description above. Use `trie_storage` as mutable accumulators (passed by reference). The storage size is deduced from the number of keys after construction — use `std::array` with a generous upper bound (N*2 nodes, sum of key lengths for labels, N-1 for children) and trim after build.

Since `consteval` cannot return different types based on runtime values, compute the upper bounds as `consteval auto compute_storage_sizes()` and use those as template parameters.

**Storage size computation:**
- Max nodes = `k_key_count * 2` (conservative upper bound for compressed trie)
- Max label bytes = sum of all key lengths (worst case: no compression)
- Max children = `k_key_count` (each internal node needs at least 1 child index)

```cpp
static consteval auto build_trie() {
  constexpr auto max_nodes = k_key_count * 2;
  constexpr auto max_labels = [] {
    std::size_t s = 0;
    for (auto k : k_key_views_) s += k.size();
    return s;
  }();
  constexpr auto max_children = k_key_count;

  auto storage = trie_storage<max_nodes, max_labels, max_children>{};
  auto next_node = 0;
  auto next_label = 0;
  auto next_child = 0;

  // Build initial list
  std::array<kv_pair, k_key_count> kvs{};
  for (auto i = 0uz; i < k_key_count; ++i)
    kvs[i] = {k_key_views_[i], static_cast<int>(i)};

  // Build root children (LCP of all keys is computed inside build_trie_impl,
  // but for the root we always have empty LCP)
  // Actually, the top-level build wraps keys in a root node.

  // Implementation detail: build_trie_impl returns the index of the root
  build_trie_impl(kvs, storage, next_node, next_label, next_child);

  // No trimming needed — we pad unused nodes with default values
  return storage;
}
```

- [ ] **Step 5: Implement `find` fully**

Complete the `find()` method with the full loop as described above.

- [ ] **Step 6: Implement child > 8 optimization (256-entry LUT)**

After `build_trie` finishes, scan all nodes. If any has `child_count > 8`, build a `k_child_lut_` as `std::array<std::uint8_t, 256>` per node (another flat array). The LUT maps character -> child index (0xFF = no child).

For the LUT storage, use a second `std::array<std::uint8_t, 256 * max_nodes>` — only valid when `k_use_child_lut` is true (determined at compile time). Update `find()` to branch:

```cpp
if constexpr (k_use_child_lut_) {
  auto const lut_base = static_cast<std::size_t>(node_idx) * 256;
  auto const child_idx = k_child_lut_[lut_base + static_cast<std::uint8_t>(next_char)];
  if (child_idx != 0xFF) {
    node_idx = child_idx;
    ++pos;
    continue;
  }
} else {
  // linear scan as before
}
```

- [ ] **Step 7: Add deduplication check**

```cpp
static_assert(!detail::has_duplicate_keys<Keys...>(), "frozen_trie_index keys must be unique");
```

(Reuse `has_duplicate_keys` from `map.hpp` via `#include "map.hpp"` or duplicate locally.)

To avoid pulling in the entire map.hpp, duplicate the logic in trie_index.hpp:

```cpp
template <FrozenString... Keys>
static consteval auto has_dup_keys() -> bool {
  if constexpr (sizeof...(Keys) <= 1) return false;
  else {
    constexpr std::array views{ std::string_view{Keys.buffer.data(), Keys.length}... };
    auto sorted = views;
    std::ranges::sort(sorted);
    for (auto i = 1uz; i < sorted.size(); ++i)
      if (sorted[i-1] == sorted[i]) return true;
    return false;
  }
}
```

- [ ] **Step 8: Verify syntax and basic compile**

Run: `g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include -c include/frozenchars/trie_index.hpp`
Expected: clean compile

---

### Task 2: `trie_set.hpp` — frozen_trie_set wrapper

**Files:**
- Create: `include/frozenchars/trie_set.hpp`

- [ ] **Step 1: Create trie_set.hpp**

Mirrors `set.hpp` exactly, but uses `frozen_trie_index` instead of `lookup_index`:

```cpp
#pragma once

#include <string_view>
#include <span>
#include <array>
#include <algorithm>
#include <cstddef>

#include "string.hpp"
#include "trie_index.hpp"

namespace frozenchars {

template <FrozenString... Keys>
class frozen_trie_set {
  static_assert(sizeof...(Keys) > 0, "frozen_trie_set requires at least one key");
  // duplicate check happens in frozen_trie_index

public:
  using key_type        = std::string_view;
  using value_type      = std::string_view;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;

  class iterator { /* same as frozen_set::iterator — uses lookup_::k_key_views_ */ };

  static constexpr auto size()  noexcept -> size_type { return sizeof...(Keys); }
  static constexpr auto empty() noexcept -> bool { return false; }

  static constexpr auto contains(std::string_view key) noexcept -> bool {
    return lookup_::find(key) != size();
  }

  static constexpr auto count(std::string_view key) noexcept -> size_type {
    return contains(key) ? 1uz : 0uz;
  }

  static constexpr auto find(std::string_view key) noexcept -> iterator {
    auto const i = lookup_::find(key);
    return i != size() ? iterator{i} : end();
  }

  static constexpr auto begin()  noexcept -> iterator { return iterator{0}; }
  static constexpr auto end()    noexcept -> iterator { return iterator{size()}; }
  static constexpr auto cbegin() noexcept -> iterator { return begin(); }
  static constexpr auto cend()   noexcept -> iterator { return end(); }

  static constexpr auto keys() noexcept -> std::span<const std::string_view, size()> {
    return lookup_::k_key_views_;
  }

  template <typename T>
  using map_type = frozen_trie_map<T, Keys...>;

private:
  using lookup_ = detail::frozen_trie_index<Keys...>;
};

} // namespace frozenchars
```

- [ ] **Step 2: Syntax-check**

Run: `g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include -c include/frozenchars/trie_set.hpp`
Expected: clean compile

---

### Task 3: `trie_map.hpp` — frozen_trie_map wrapper

**Files:**
- Create: `include/frozenchars/trie_map.hpp`

- [ ] **Step 1: Create trie_map.hpp**

Mirrors `map.hpp` but uses `frozen_trie_index`:

```cpp
#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <functional>
#include <utility>
#include <iterator>

#include "string.hpp"
#include "trie_index.hpp"
#include "trie_set.hpp"  // for map_type alias consistency

namespace frozenchars {

template <typename T, FrozenString... Keys>
class frozen_trie_map {
  static_assert(sizeof...(Keys) > 0, "frozen_trie_map requires at least one key");

public:
  using key_type        = std::string_view;
  using mapped_type     = T;
  using size_type       = std::size_t;
  using difference_type = std::ptrdiff_t;
  using value_type      = std::pair<std::string_view const, T>;

  // iterator, const_iterator — same pattern as frozen_map
  // at(), find(), contains(), count(), operator[]
  // begin()/end(), cbegin()/cend()
  // keys(), keys_in_declaration_order()
  // make_frozen_trie_map() helper

private:
  using lookup_ = detail::frozen_trie_index<Keys...>;
  std::array<T, sizeof...(Keys)> values_{};
};

// make_frozen_trie_map helpers — same pattern as make_frozen_map

} // namespace frozenchars
```

Simplify: start with the minimum viable API that supports `BENCHMARK`:
- `find()`, `contains()`, `at()`, `operator[]`
- Default constructor + `std::array<T, N>` constructor
- `begin()/end()` iteration
- `make_frozen_trie_map` for pair-like entries

- [ ] **Step 2: Syntax-check**

Run: `g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include -c include/frozenchars/trie_map.hpp`
Expected: clean compile

---

### Task 4: Update CMakeLists.txt and umbrella header

**Files:**
- Modify: `include/frozenchars.hpp`
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: Add trie headers to umbrella header**

```cpp
// in frozenchars.hpp, after #include "frozenchars/map.hpp"
#include "frozenchars/trie_index.hpp"
#include "frozenchars/trie_set.hpp"
#include "frozenchars/trie_map.hpp"
```

- [ ] **Step 2: Add test and benchmark targets to CMakeLists.txt**

```cmake
# in test/CMakeLists.txt

add_executable(bench_frozen_trie bench_frozen_trie.cpp)
target_link_libraries(bench_frozen_trie PRIVATE frozenchars::frozenchars)
target_compile_features(bench_frozen_trie PUBLIC cxx_std_23)
```

The test is compiled as part of `all_test` (auto-globbed from `test_*.cpp`).

- [ ] **Step 3: Verify umbrella header compiles**

Run: `g++ -std=c++23 -O2 -Wall -Wextra -pedantic -I include -fsyntax-only -c include/frozenchars.hpp`
Expected: clean compile

---

### Task 5: Tests (`test_frozen_trie.cpp`)

**Files:**
- Create: `test/test_frozen_trie.cpp`
- Verify: `bash test.sh` passes

- [ ] **Step 1: Write basic tests**

```cpp
#include "catch2/catch_all.hpp"
#include "frozenchars/literals.hpp"
#include "frozenchars/trie_index.hpp"
#include "frozenchars/trie_set.hpp"
#include "frozenchars/trie_map.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

TEST_CASE("frozen_trie_index finds exact keys", "[frozen_trie]") {
  using Idx = detail::frozen_trie_index<"cat"_fs, "car"_fs, "dog"_fs, "door"_fs, "dove"_fs>;

  // hit
  CHECK(Idx::find("cat") == 0);
  CHECK(Idx::find("car") == 1);
  CHECK(Idx::find("dog") == 2);
  CHECK(Idx::find("door") == 3);
  CHECK(Idx::find("dove") == 4);

  // miss
  CHECK(Idx::find("c") == Idx::k_key_count);
  CHECK(Idx::find("ca") == Idx::k_key_count);
  CHECK(Idx::find("cats") == Idx::k_key_count);
  CHECK(Idx::find("dove_") == Idx::k_key_count);
  CHECK(Idx::find("zebra") == Idx::k_key_count);
  CHECK(Idx::find("DOOR") == Idx::k_key_count);
  CHECK(Idx::find("") == Idx::k_key_count);
}

TEST_CASE("frozen_trie_index handles common prefix keys", "[frozen_trie]") {
  using Idx = detail::frozen_trie_index<"timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>;

  CHECK(Idx::find("timeout") == 0);
  CHECK(Idx::find("timeout_ms") == 1);
  CHECK(Idx::find("timeout_us") == 2);
  CHECK(Idx::find("timeout_ns") == 3);

  CHECK(Idx::find("timeout_") == Idx::k_key_count);
  CHECK(Idx::find("timeout_abc") == Idx::k_key_count);
}

TEST_CASE("frozen_trie_index handles short keys", "[frozen_trie]") {
  using Idx = detail::frozen_trie_index<"a"_fs, "b"_fs, "c"_fs>;

  CHECK(Idx::find("a") == 0);
  CHECK(Idx::find("b") == 1);
  CHECK(Idx::find("c") == 2);

  CHECK(Idx::find("ab") == Idx::k_key_count);
  CHECK(Idx::find("") == Idx::k_key_count);
}

TEST_CASE("frozen_trie_index handles key that is prefix of another", "[frozen_trie]") {
  using Idx = detail::frozen_trie_index<"a"_fs, "ab"_fs, "abc"_fs>;

  CHECK(Idx::find("a") == 0);
  CHECK(Idx::find("ab") == 1);
  CHECK(Idx::find("abc") == 2);

  CHECK(Idx::find("abcd") == Idx::k_key_count);
  CHECK(Idx::find("ac") == Idx::k_key_count);
}

TEST_CASE("frozen_trie_set basic membership", "[frozen_trie]") {
  using Set = frozen_trie_set<"apple"_fs, "banana"_fs, "cherry"_fs>;

  CHECK(Set::contains("apple"));
  CHECK(Set::contains("banana"));
  CHECK_FALSE(Set::contains("apricot"));
  CHECK_FALSE(Set::contains("APPLE"));
}

TEST_CASE("frozen_trie_map basic lookup", "[frozen_trie]") {
  using Map = frozen_trie_map<int, "x"_fs, "y"_fs, "z"_fs>;
  Map map{std::array<int, 3>{10, 20, 30}};

  CHECK(map["x"] == 10);
  CHECK(map["y"] == 20);
  CHECK(map["z"] == 30);
  CHECK_THROWS_AS(map["w"], std::out_of_range);
  CHECK(map.contains("x"));
  CHECK_FALSE(map.contains("w"));
}

TEST_CASE("frozen_trie_map make_frozen_trie_map", "[frozen_trie]") {
  auto map = frozenchars::make_frozen_trie_map<int, "timeout"_fs, "retry"_fs>(
    std::pair{"retry", 5},
    std::pair{"timeout", 30}
  );

  CHECK(map["timeout"] == 30);
  CHECK(map["retry"] == 5);
}
```

- [ ] **Step 2: Run tests**

```bash
bash build.sh && bash test.sh
```
Expected: all tests pass (including existing ones). If compile_fail tests reference map.hpp, they still do — unaffected.

---

### Task 6: Benchmarks (`bench_frozen_trie.cpp`)

**Files:**
- Create: `test/bench_frozen_trie.cpp`
- Verify: `bash build.sh && ./build/test/bench_frozen_trie` runs benchmark

- [ ] **Step 1: Write benchmark file**

Use the same custom measurement framework as `bench_frozen_map.cpp`:

```cpp
#include "frozenchars/literals.hpp"
#include "frozenchars/map.hpp"
#include "frozenchars/trie_map.hpp"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <vector>

using namespace frozenchars;
using namespace frozenchars::literals;

namespace {

struct bench_result {
  std::string_view name{};
  std::uint64_t iterations{};
  double total_ms{};
  double ns_per_iter{};
};

volatile std::size_t g_sink = 0;

template <typename Func>
auto measure(std::string_view name, Func&& fn, std::uint64_t iterations) -> bench_result {
  for (std::uint64_t i = 0; i < 500; ++i) fn();
  auto const begin = std::chrono::steady_clock::now();
  for (std::uint64_t i = 0; i < iterations; ++i) fn();
  auto const end = std::chrono::steady_clock::now();
  auto const elapsed_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
  return bench_result{
    .name = name,
    .iterations = iterations,
    .total_ms = static_cast<double>(elapsed_ns) / 1'000'000.0,
    .ns_per_iter = static_cast<double>(elapsed_ns) / static_cast<double>(iterations),
  };
}

auto print_results(std::vector<bench_result> const& results) -> void {
  std::cout << "\n[frozen_trie_map vs frozen_map benchmark]\n\n";
  std::cout << std::left << std::setw(48) << "case"
            << std::right << std::setw(12) << "iters"
            << std::setw(14) << "total[ms]"
            << std::setw(14) << "ns/iter" << "\n";
  std::cout << std::string(48 + 12 + 14 + 14, '-') << "\n";
  for (auto const& r : results) {
    std::cout << std::left << std::setw(48) << r.name
              << std::right << std::setw(12) << r.iterations
              << std::setw(14) << std::fixed << std::setprecision(3) << r.total_ms
              << std::setw(14) << std::fixed << std::setprecision(1) << r.ns_per_iter << "\n";
  }
  std::cout << "\n[sink] " << g_sink << '\n';
}

} // namespace

int main(int argc, char** argv) {
  auto iterations = std::uint64_t{500'000};
  if (argc > 1) {
    auto const parsed = std::strtoull(argv[1], nullptr, 10);
    if (parsed > 0) iterations = static_cast<std::uint64_t>(parsed);
  }

  auto results = std::vector<bench_result>{};
  results.reserve(40);

  // ── Pattern 1: Short unique-first-char keys ──
  constexpr auto short_map = frozen_map<int, "a"_fs, "b"_fs, "c"_fs>{
    std::array<int, 3>{1, 2, 3}
  };
  constexpr auto short_trie = frozen_trie_map<int, "a"_fs, "b"_fs, "c"_fs>{
    std::array<int, 3>{1, 2, 3}
  };

  auto iters = iterations;
  results.push_back(measure("short(3) frozen_map hit",
    [&]{ g_sink += short_map["a"] + short_map["b"] + short_map["c"]; }, iters));
  results.push_back(measure("short(3) frozen_trie_map hit",
    [&]{ g_sink += short_trie["a"] + short_trie["b"] + short_trie["c"]; }, iters));
  results.push_back(measure("short(3) frozen_map miss",
    [&]{ g_sink += short_map.contains("x"); }, iters));
  results.push_back(measure("short(3) frozen_trie_map miss",
    [&]{ g_sink += short_trie.contains("x"); }, iters));

  // ── Pattern 2: HTTP methods (5 keys, mixed lengths) ──
  constexpr auto http_map = frozen_map<int,
    "GET"_fs, "PUT"_fs, "POST"_fs, "DELETE"_fs, "HEAD"_fs>{
    std::array<int, 5>{1, 2, 3, 4, 5}
  };
  constexpr auto http_trie = frozen_trie_map<int,
    "GET"_fs, "PUT"_fs, "POST"_fs, "DELETE"_fs, "HEAD"_fs>{
    std::array<int, 5>{1, 2, 3, 4, 5}
  };

  results.push_back(measure("http(5) frozen_map hit",
    [&]{ g_sink += http_map["GET"] + http_map["PUT"] + http_map["POST"]; }, iters));
  results.push_back(measure("http(5) frozen_trie_map hit",
    [&]{ g_sink += http_trie["GET"] + http_trie["PUT"] + http_trie["POST"]; }, iters));
  results.push_back(measure("http(5) frozen_map miss",
    [&]{ g_sink += http_map.contains("PATCH"); }, iters));
  results.push_back(measure("http(5) frozen_trie_map miss",
    [&]{ g_sink += http_trie.contains("PATCH"); }, iters));

  // ── Pattern 3: Common prefix ──
  constexpr auto prefix_map = frozen_map<int,
    "timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>{
    std::array<int, 4>{10, 20, 30, 40}
  };
  constexpr auto prefix_trie = frozen_trie_map<int,
    "timeout"_fs, "timeout_ms"_fs, "timeout_us"_fs, "timeout_ns"_fs>{
    std::array<int, 4>{10, 20, 30, 40}
  };

  results.push_back(measure("prefix(4) frozen_map hit",
    [&]{ g_sink += prefix_map["timeout"] + prefix_map["timeout_ms"]; }, iters));
  results.push_back(measure("prefix(4) frozen_trie_map hit",
    [&]{ g_sink += prefix_trie["timeout"] + prefix_trie["timeout_ms"]; }, iters));
  results.push_back(measure("prefix(4) frozen_map miss",
    [&]{ g_sink += prefix_map.contains("timeout_abc"); }, iters));
  results.push_back(measure("prefix(4) frozen_trie_map miss",
    [&]{ g_sink += prefix_trie.contains("timeout_abc"); }, iters));

  // ── Pattern 4: Medium set (20 NATO keys) ──
  constexpr auto med_map = frozen_map<int,
    "alpha"_fs, "bravo"_fs, "charlie"_fs, "delta"_fs, "echo"_fs,
    "foxtrot"_fs, "golf"_fs, "hotel"_fs, "india"_fs, "juliet"_fs,
    "kilo"_fs, "lima"_fs, "mike"_fs, "november"_fs, "oscar"_fs,
    "papa"_fs, "quebec"_fs, "romeo"_fs, "sierra"_fs, "tango"_fs>{
    std::array<int, 20>{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20}
  };
  constexpr auto med_trie = frozen_trie_map<int,
    "alpha"_fs, "bravo"_fs, "charlie"_fs, "delta"_fs, "echo"_fs,
    "foxtrot"_fs, "golf"_fs, "hotel"_fs, "india"_fs, "juliet"_fs,
    "kilo"_fs, "lima"_fs, "mike"_fs, "november"_fs, "oscar"_fs,
    "papa"_fs, "quebec"_fs, "romeo"_fs, "sierra"_fs, "tango"_fs>{
    std::array<int, 20>{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20}
  };

  results.push_back(measure("med(20) frozen_map hit",
    [&]{ auto it = med_map.find("golf"); g_sink += (it != med_map.end()); }, iters));
  results.push_back(measure("med(20) frozen_trie_map hit",
    [&]{ auto it = med_trie.find("golf"); g_sink += (it != med_trie.end()); }, iters));
  results.push_back(measure("med(20) frozen_map miss",
    [&]{ auto it = med_map.find("zulu"); g_sink += (it != med_map.end()); }, iters));
  results.push_back(measure("med(20) frozen_trie_map miss",
    [&]{ auto it = med_trie.find("zulu"); g_sink += (it != med_trie.end()); }, iters));

  // ── Pattern 5: Long keys (> 32 chars) ──
  constexpr auto longkey_map = frozen_map<int,
    "configuration_timeout_ms"_fs, "maximum_retry_count_param"_fs,
    "connection_pool_size_setting"_fs, "authentication_token_secret_key"_fs,
    "response_body_encoding_format"_fs>{
    std::array<int, 5>{100, 200, 300, 400, 500}
  };
  constexpr auto longkey_trie = frozen_trie_map<int,
    "configuration_timeout_ms"_fs, "maximum_retry_count_param"_fs,
    "connection_pool_size_setting"_fs, "authentication_token_secret_key"_fs,
    "response_body_encoding_format"_fs>{
    std::array<int, 5>{100, 200, 300, 400, 500}
  };

  results.push_back(measure("longkey(5) frozen_map hit",
    [&]{ auto it = longkey_map.find("authentication_token_secret_key"); g_sink += (it != longkey_map.end()); }, iters));
  results.push_back(measure("longkey(5) frozen_trie_map hit",
    [&]{ auto it = longkey_trie.find("authentication_token_secret_key"); g_sink += (it != longkey_trie.end()); }, iters));
  results.push_back(measure("longkey(5) frozen_map miss",
    [&]{ auto it = longkey_map.find("nonexistent_key_that_is_long_enough"); g_sink += (it != longkey_map.end()); }, iters));
  results.push_back(measure("longkey(5) frozen_trie_map miss",
    [&]{ auto it = longkey_trie.find("nonexistent_key_that_is_long_enough"); g_sink += (it != longkey_trie.end()); }, iters));

  print_results(results);
  return 0;
}
```

- [ ] **Step 2: Build and run**

```bash
bash build.sh && ./build/test/bench_frozen_trie
```

Expected: benchmarks run and print timing results comparing each pattern.

- [ ] **Step 3: Analyze results**

Look for patterns where trie is faster:
- Short key hit/miss
- Common prefix hit/miss
- Miss ratio vs frozen_map
- Long key comparison

- [ ] **Step 4: Expand benchmark with more patterns**

Add additional key sets based on initial results:
- 64 keys (frozen_map lookup-table threshold boundary)
- Same-length keys (forces hash path in frozen_map)
- All-ASCII-same-prefix large set

---

### Task 7: Commit

- [ ] **Step 1: Create a clean commit**

```bash
git add include/frozenchars/trie_index.hpp include/frozenchars/trie_set.hpp include/frozenchars/trie_map.hpp
git add include/frozenchars.hpp
git add test/CMakeLists.txt test/test_frozen_trie.cpp test/bench_frozen_trie.cpp
git add docs/superpowers/specs/2026-07-02-frozen-trie-design.md
git add docs/superpowers/plans/2026-07-02-frozen-trie.md
git commit -m "feat: add frozen_trie_index / frozen_trie_map / frozen_trie_set with benchmarks"
```

---

## Self-Review Checklist

1. **Spec coverage:** Does each item in the spec correspond to a task?
   - trie_index.hpp → Task 1 ✅
   - trie_set.hpp → Task 2 ✅
   - trie_map.hpp → Task 3 ✅
   - Umbrella / CMake → Task 4 ✅
   - Tests → Task 5 ✅
   - Benchmarks → Task 6 ✅
   - Child >8 optimization → Task 1, Step 6 ✅
   - Static assert duplicate keys → Task 1, Step 7 ✅

2. **Placeholder scan:** TODO in benchmark file — intentional macro placeholder, filled in Step 4 after initial results.

3. **Type consistency:** `frozen_trie_index::find()` returns `std::size_t` (k_key_count = npos). `frozen_trie_set::find()` returns `iterator`. `frozen_trie_map::find()` returns `iterator`. All consistent.

4. **Ambiguity check:** Catch2 benchmark include path may vary. Use `catch2/benchmark/catch_benchmark_all.hpp` or check existing bench files for the exact include.
