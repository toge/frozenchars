# Inja Engine Fixes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `include/frozenchars/inja_engine.hpp` の `else` チェーン、数値文字列化、glaze 連携、関数抽出、変数パス分割、ホットパス最適化、関連ドキュメントを既存公開 API を保ったまま修正する。

**Architecture:** 変更は既存の `inja_engine.hpp` 1 ファイルに集中させ、`consteval` パーサ / 抽出、ランタイム変換 / glaze lookup、変数パス / ループ実行の 3 ブロックで順に進める。各ブロックは `test/test_template_vm.cpp` に失敗テストを追加してから最小実装を入れ、最後に `template_vm` 系回帰とリポジトリ既存の総合確認を通す。

**Tech Stack:** C++26, header-only templates, Catch2, glaze, STL (`std::to_chars`, `std::span`, `std::inplace_vector`, `std::array`, `std::expected`)

---

## File Structure

- **Modify:** `include/frozenchars/inja_engine.hpp`
  - `runtime_options_ref` の Doxygen 補強
  - `detail::node`, `detail::parse_program`, `detail::extract_template_function_calls`
  - `detail::append_value`
  - glaze lookup helper 群 (`lookup_in_reflectable`, `lookup_typed_object_view_value`, `make_typed_object_view`)
  - `detail::split_variable_path` と関連 lookup / resolve helper
  - `detail::render_program_with_lookup` の array loop 分岐
- **Modify:** `test/test_template_vm.cpp`
  - parser / extractor / runtime_error / lookup / runtime の回帰追加
- **Reference only:** `docs/superpowers/specs/2026-05-31-inja-engine-fixes-design.md`
  - 要求仕様と非互換トレードオフの確認元

---

### Task 1: Lock down parser and function-extraction regressions

**Files:**
- Modify: `test/test_template_vm.cpp`
- Modify: `include/frozenchars/inja_engine.hpp`
- Test: `test/test_template_vm.cpp`

- [ ] **Step 1: Write the failing parser and extractor tests**

Add to `test/test_template_vm.cpp`:

```cpp
TEST_CASE("template runtime resolves else if chains including tail else", "[template_vm][else_if]") {
  constexpr auto src = "{% if n == 1 %}A{% else if n == 2 %}B{% else %}C{% endif %}"_fs;

  REQUIRE(render<src>(root_context{.n = 1}) == "A");
  REQUIRE(render<src>(root_context{.n = 2}) == "B");
  REQUIRE(render<src>(root_context{.n = 3}) == "C");
}

TEST_CASE("template function extraction ignores comment blocks", "[template_vm][parser]") {
  constexpr auto src = "{# upper(x) #}{{ x }}"_fs;
  constexpr auto calls = extract_template_function_calls<src>();

  static_assert(calls.count == 0);
  REQUIRE(calls.count == 0);
}
```

Keep the existing nearby comment documenting the duplicate-`else` compile-fail scenario; do not introduce a new compile-fail harness for this task.

- [ ] **Step 2: Run the focused template VM tests to confirm failure**

Run:

```bash
cmake -S . -B build && cmake --build build --target all_test --parallel 4 && ./build/test/all_test "[template_vm]"
```

Expected:
1. the new `else if` / `else` regression or extractor regression fails against the current implementation
2. no unrelated test target needs to change

- [ ] **Step 3: Implement the minimal parser / extractor fixes**

Update `include/frozenchars/inja_engine.hpp`:

```cpp
struct node {
  // ...
  bool include_expr_is_simple_path{};
  bool is_plain_else{};
};

auto process_statement = [&](std::string_view stmt, std::size_t begin, std::size_t end) consteval {
  auto const stmt_offset = static_cast<std::size_t>(stmt.data() - src.data());
  // ...
  if (stmt == "else" || stmt.starts_with("else if ")) {
    auto const else_idx = push_node(node_kind::else_stmt, begin, end);
    // ...
    if (!stmt.starts_with("else if ")) {
      program.nodes[else_idx].is_plain_else = true;
    }
    // duplicate else detection now checks program.nodes[tail_else].is_plain_else
  }
};

auto constexpr comment_open = Delims::comment_open.sv();
auto constexpr comment_close = Delims::comment_close.sv();
auto const comment_pos = src_sv.find(comment_open, pos);
```

While touching the same block:
1. replace `std::size_t{0}` counters in the affected `detail` helpers with `0uz`
2. remove the unused `always_false_v`
3. keep public API signatures unchanged

- [ ] **Step 4: Re-run the focused tests and make sure they pass**

Run:

```bash
cmake -S . -B build && cmake --build build --target all_test --parallel 4 && ./build/test/all_test "[template_vm]"
```

Expected: PASS for the newly added parser / extractor coverage and existing `template_vm` tests.

- [ ] **Step 5: Commit the parser / extractor slice**

```bash
git add include/frozenchars/inja_engine.hpp test/test_template_vm.cpp
git commit -m "fix: tighten inja parser and extraction"
```

### Task 2: Lock down numeric rendering and glaze preconditions

**Files:**
- Modify: `test/test_template_vm.cpp`
- Modify: `include/frozenchars/inja_engine.hpp`
- Test: `test/test_template_vm.cpp`

- [ ] **Step 1: Add the failing runtime-error and glaze capability checks**

Add to `test/test_template_vm.cpp`:

```cpp
#include <limits>

#if FROZENCHARS_HAS_GLAZE
static_assert(requires(root_context const& ctx) {
  glz::to_tie(ctx);
});
#endif

TEST_CASE("append_value rejects non-finite doubles", "[template_vm][runtime_error]") {
  auto out = std::string{};

  REQUIRE_THROWS_AS(
    frozenchars::inja::detail::append_value(
      out, inja_value{std::numeric_limits<double>::quiet_NaN()}
    ),
    render_error
  );
  REQUIRE_THROWS_AS(
    frozenchars::inja::detail::append_value(
      out, inja_value{std::numeric_limits<double>::infinity()}
    ),
    render_error
  );
}
```

If the `static_assert` fails, stop immediately and treat it as the spec-defined dependency blocker rather than adding a fallback `const_cast` path.

- [ ] **Step 2: Run the focused runtime-error tests**

Run:

```bash
cmake -S . -B build && cmake --build build --target all_test --parallel 4 && ./build/test/all_test "[template_vm][runtime_error]"
```

Expected:
1. the new non-finite `append_value` test fails before implementation
2. if glaze no longer supports `glz::to_tie(T const&)`, the build fails at compile time and work stops for dependency guidance

- [ ] **Step 3: Implement the minimal rendering / glaze / documentation fixes**

Update `include/frozenchars/inja_engine.hpp`:

```cpp
if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
  auto const [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), *p);
  if (ec != std::errc{}) {
    throw render_error{"integer to string conversion failed"};
  }
}

if (auto const* p = std::get_if<double>(&v.storage)) {
  if (!std::isfinite(*p)) {
    throw render_error{"double to string conversion failed"};
  }
  auto const [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), *p);
  if (ec != std::errc{}) {
    throw render_error{"double to string conversion failed"};
  }
}

auto tied = glz::to_tie(value);
```

Also:
1. replace the four `glz::to_tie(const_cast<...>)` call sites with direct const calls
2. add Japanese Doxygen comments around `runtime_options_ref`, `add_function`, `reserve_functions`, `add_include`, `reserve_includes`
3. document `std::cref(opts)` in the example comment

- [ ] **Step 4: Re-run the focused runtime-error tests**

Run:

```bash
cmake -S . -B build && cmake --build build --target all_test --parallel 4 && ./build/test/all_test "[template_vm][runtime_error]"
```

Expected: PASS for the new `append_value` regression and existing runtime error coverage.

- [ ] **Step 5: Commit the rendering / glaze slice**

```bash
git add include/frozenchars/inja_engine.hpp test/test_template_vm.cpp
git commit -m "fix: harden inja numeric rendering"
```

### Task 3: Replace variable-path heap allocation and finish loop hot-path cleanup

**Files:**
- Modify: `test/test_template_vm.cpp`
- Modify: `include/frozenchars/inja_engine.hpp`
- Test: `test/test_template_vm.cpp`

- [ ] **Step 1: Add the failing lookup-path regressions**

Add to `test/test_template_vm.cpp`:

```cpp
TEST_CASE("split_variable_path enforces the small-buffer depth contract", "[template_vm][lookup]") {
  auto const segments = frozenchars::inja::detail::split_variable_path("a.b.c.d.e");
  REQUIRE(segments.size() == 5);
  REQUIRE(segments[0] == "a");
  REQUIRE(segments[4] == "e");

  REQUIRE_THROWS_WITH(
    frozenchars::inja::detail::split_variable_path("a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q"),
    "variable path too deep (max 16 segments)"
  );
}

TEST_CASE("render reports deep variable paths through lookup", "[template_vm][lookup]") {
  constexpr auto src = "{{ a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q }}"_fs;
  auto const root = [] {
    auto value = inja_value{std::int64_t{1}};
    for (auto const key : std::array{
           "q", "p", "o", "n", "m", "l", "k", "j", "i", "h", "g", "f", "e", "d", "c", "b", "a"
         }) {
      value = inja_value{inja_object{{std::string{key}, std::move(value)}}};
    }
    return value;
  }();

  REQUIRE_THROWS_WITH(render<src>(root), "variable path too deep (max 16 segments)");
}
```

These tests give direct helper coverage plus one observable render-path check for the new depth cap.

- [ ] **Step 2: Run the focused lookup tests to confirm failure**

Run:

```bash
cmake -S . -B build && cmake --build build --target all_test --parallel 4 && ./build/test/all_test "[template_vm][lookup]"
```

Expected: FAIL because `split_variable_path` still returns `std::vector<std::string_view>` and has no 16-segment guard.

- [ ] **Step 3: Implement the small-buffer path type and loop cleanup**

Update `include/frozenchars/inja_engine.hpp`:

```cpp
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202306L
  #include <inplace_vector>
#endif

struct path_segments {
  static constexpr auto max_depth = 16uz;
  // inplace_vector or array-backed storage
  auto push_back(std::string_view sv) -> void;
  [[nodiscard]] auto size() const -> std::size_t;
  [[nodiscard]] auto operator[](std::size_t index) const -> std::string_view;
  [[nodiscard]] auto front() const -> std::string_view;
  [[nodiscard]] auto span() const -> std::span<std::string_view const>;
};

[[nodiscard]] inline auto split_variable_path(std::string_view name) -> path_segments;
```

Then thread the new type through:
1. `lookup_in_object_root`, `lookup_in_typed_root`, `lookup_typed_object_view_root`, `resolve_local`
2. any helper that currently expects `std::vector<std::string_view>` from `split_variable_path`
3. `render_program_with_lookup` array iteration, so `inja_value` elements are forwarded directly instead of going through `to_inja_value_or_throw`

Keep the depth cap and error string exactly as specified in the spec.

- [ ] **Step 4: Run focused and full regression commands**

Run:

```bash
cmake -S . -B build && cmake --build build --target all_test example_smoke --parallel 4 && ./build/test/all_test "[template_vm]" && ./test.sh
```

Expected:
1. PASS for the new lookup tests
2. PASS for the existing `template runtime supports include template registry`
3. PASS for the existing `template runtime supports custom delimiters`
4. PASS for the existing `typed root lookup does not require whole-tree conversion`
5. PASS for the repository-wide test script

- [ ] **Step 5: Commit the path / loop slice**

```bash
git add include/frozenchars/inja_engine.hpp test/test_template_vm.cpp
git commit -m "perf: optimize inja variable path lookup"
```
