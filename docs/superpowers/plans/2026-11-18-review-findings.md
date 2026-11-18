# Review Findings Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix all accepted review findings while preserving public API behavior except for making Glaze linkage explicit.

**Architecture:** Add one canonical UTF-8 sequence validator and route encoding, JSON, and Crush through it; apply local fixes to chrono and exception specifications; split the CMake Glaze integration into an optional target and export required constexpr limits. Every behavior change starts with a failing regression test and a focused green run before the next subsystem.

**Tech Stack:** C++23, Catch2 v3, CMake 3.25+, vcpkg manifest dependencies, CTest, std::expected.

---

### Task 1: Add canonical UTF-8 regression tests

**Files:**
- Modify: `test/test_encoding.cpp`
- Modify: `test/test_json_compress.cpp`
- Modify: `test/test_json_crush.cpp`
- Modify: `test/CMakeLists.txt`
- Create: `test/compile_fail/json_crush_invalid_utf8.cpp`
- Create: `test/compile_fail/json_uncrush_invalid_utf8.cpp`

- [ ] **Step 1: Write encoding tests**

Add `STATIC_CHECK`/`CHECK` cases for `C0 80`, `E0 80 80`, `ED A0 80`, `F0 80 80 80`, `F4 90 80 80`, truncated sequences, and lone continuation bytes. Require fail-soft APIs to consume one invalid byte at a time and preserve each original byte in reverse/codepoint conversion. Require strict conversion to report invalid.

- [ ] **Step 2: Write JSON validation and Crush tests**

In `test_json_compress.cpp`, assert that `validate_json` rejects raw `FF`, overlong UTF-8, surrogate UTF-8, above-maximum UTF-8, and truncation inside JSON strings, while `compress`/`decompress` continues to preserve already-formed `FrozenString` bytes without claiming malformed JSON is valid.

For compile-fail Crush cases, instantiate `crush<Invalid>()` and `uncrush<InvalidCrushed>()`, with `EXPECTED_TEXT` matching the new explicit diagnostic.

- [ ] **Step 3: Register compile-fail cases**

Add both new files through `add_compile_fail_test()` in `test/CMakeLists.txt`. Keep the existing `EXPECTED_TEXT` mechanism.

- [ ] **Step 4: Run focused tests to verify failure**

Run:

```bash
cmake --build build --target all_test --parallel
ctest --test-dir build --output-on-failure -R '^all_test$'
ctest --test-dir build --output-on-failure -R 'compile_fail_json_(crush|uncrush)_invalid_utf8'
```

Expected: the regression assertions fail and/or the compile-fail target unexpectedly compiles. Record the exact baseline failures.

---

### Task 2: Enforce canonical UTF-8 in encoding, JSON, and Crush

**Files:**
- Create or modify: `include/frozenchars/detail/utf8.hpp`
- Modify: `include/frozenchars/encoding.hpp:8-100`
- Modify: `include/frozenchars/json/detail/compress_detail.hpp:88-133`
- Modify: `include/frozenchars/json/detail/crush_detail.hpp:50-175`
- Modify: `include/frozenchars/json/crush.hpp:12-145`

- [ ] **Step 1: Implement a shared strict UTF-8 primitive**

Create a `constexpr` detail function that takes `std::string_view`, byte position, consumed-byte output, and codepoint output. It must:

- reject C0/C1 leads;
- require correct continuation bytes;
- reject E0 with a second byte below A0;
- reject ED with a second byte A0- BF;
- reject F0 with a second byte below 90;
- reject F4 with a second byte above 8F;
- reject F5-FF, surrogate code points, truncation, and values above 10FFFF;
- return `std::expected<char32_t, std::errc>` through an API that is usable in C++23 constant evaluation.

- [ ] **Step 2: Preserve fail-soft encoding behavior**

Replace `utf8_decode_at` sequence decoding with the strict primitive. On invalid input, set `consumed = 1` and set `codepoint` to the raw unsigned byte so existing re-encoding helpers preserve that byte instead of decoding overlong values as NUL or a scalar.

- [ ] **Step 3: Validate JSON raw string bytes**

In `parse_string`, for every unescaped byte >= 0x80, validate one complete sequence with the shared primitive, advance by its consumed length, and return `invalid_argument` on failure.

- [ ] **Step 4: Reject invalid Crush conversions explicitly**

Use the shared primitive in `decode_utf8`. Reject lone low/high surrogates in `utf16_to_utf8`. In `crushed_size`, `uncrushed_size`, `crush`, `uncrush`, and relevant helper conversions, check `expected` values explicitly and use a stable compile-time diagnostic such as `json::crush: invalid UTF-8/UTF-16 input` instead of dereferencing an unsuccessful result.

- [ ] **Step 5: Run focused tests**

Run:

```bash
cmake --build build --target all_test --parallel
ctest --test-dir build --output-on-failure -R 'all_test|compile_fail_json_(crush|uncrush)_invalid_utf8'
```

Expected: PASS.

- [ ] **Step 6: Commit**

```bash
git add include/frozenchars test/test_encoding.cpp test/test_json_compress.cpp test/test_json_crush.cpp test/compile_fail
git commit -m "fix: validate UTF-8 strictly"
```

---

### Task 3: Make runtime chrono validation match compile-time validation

**Files:**
- Modify: `test/test_chrono.cpp`
- Create: `test/compile_fail/chrono_parse_date_macro_invalid_calendar.cpp`
- Modify: `test/CMakeLists.txt`
- Modify: `include/frozenchars/chrono.hpp:75-195`

- [ ] **Step 1: Write failing chrono tests**

Add runtime rejection cases for:

- a lone `+` or `-` after a 19-character timestamp;
- `+24:00`, `+23:60`, `-99:99`;
- `Z` with offset fields still present or `+HH:MM` missing a sign;
- `Feb 30 2024`, `Apr 31 2024`, `Jan 00 2024`, `Jan 32 2024`;
- non-digits in day/year fields, including `Jan  : 2026`.

Add a compile-fail case for an invalid `parse_date_macro` calendar date.
Register it through `add_compile_fail_test()` in `test/CMakeLists.txt` and pin the new calendar-date diagnostic with `EXPECTED_TEXT`.

- [ ] **Step 2: Run focused tests to verify failure**

Run:

```bash
cmake --build build --target all_test --parallel
ctest --test-dir build --output-on-failure -R '^all_test$'
ctest --test-dir build --output-on-failure -R 'compile_fail_chrono_parse_date_macro_invalid_calendar'
```

Expected: malformed inputs currently produce values and at least one new assertion fails.

- [ ] **Step 3: Implement validation**

For both ISO overloads, enforce:

- length 20 terminator is only `Z`/`z`;
- length 25 terminator is `+`/`-`;
- offset digits, colon, hours <= 23, minutes <= 59.

For both `__DATE__` overloads, enforce the exact positions, decimal day/year digits, valid month, year >= 1970, and `year_month_day::ok()`.

- [ ] **Step 4: Run tests and commit**

Run:

```bash
ctest --test-dir build --output-on-failure -R 'all_test|compile_fail_chrono'
```

Expected: PASS.

```bash
git add include/frozenchars/chrono.hpp test/test_chrono.cpp test/compile_fail test/CMakeLists.txt
git commit -m "fix: validate runtime chrono inputs"
```

---

### Task 4: Repair throwing mapped-type exception contracts

**Files:**
- Modify: `test/test_frozen_map.cpp`
- Create: `test/test_container_noexcept.cpp` (if a focused file is clearer after inspecting fixtures)
- Modify: `include/frozenchars/map.hpp:675-830`
- Modify: `include/frozenchars/multimap.hpp:245-260`
- Modify: `include/frozenchars/trie_map.hpp:60-75`

- [ ] **Step 1: Add throwing type compile/runtime tests**

Define a test value type with throwing default construction, copy construction, and equality. Assert:

- default construction is not declared `noexcept`;
- constructing and propagating its exceptions works for map, multimap, and trie-map;
- `get_value_or` is not declared `noexcept` and propagates copy exceptions;
- equality is not declared `noexcept` and propagates equality exceptions;
- a genuinely nothrow type retains `noexcept(true)`.

- [ ] **Step 2: Run focused test to verify failure**

Run:

```bash
cmake --build build --target all_test --parallel
ctest --test-dir build --output-on-failure -R '^all_test$'
```

Expected: the new static assertions fail because the current APIs are unconditionally `noexcept`.

- [ ] **Step 3: Apply conditional noexcept**

Use `std::is_nothrow_default_constructible_v<T>`, `std::is_nothrow_copy_constructible_v<T>`, and:

```cpp
noexcept(std::declval<T const&>() == std::declval<T const&>())
```

for the actual affected operations only.

- [ ] **Step 4: Verify and commit**

Run `ctest --test-dir build --output-on-failure -R all_test`. Expected: PASS.

```bash
git add include/frozenchars/map.hpp include/frozenchars/multimap.hpp include/frozenchars/trie_map.hpp test
git commit -m "fix: make container noexcept conditional"
```

---

### Task 5: Decouple Glaze and export required compiler limits

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `cmake/frozencharsConfig.cmake.in`
- Modify: `test/CMakeLists.txt`
- Modify: `example/CMakeLists.txt`
- Modify: `README.md:2515-2545`
- Create: `test/package_consumer/CMakeLists.txt`
- Create: `test/package_consumer/core.cpp`
- Create: `test/package_consumer/glaze.cpp`
- Create: `test/package_consumer/verify_package.cmake`

- [ ] **Step 1: Add failing package-consumer fixtures**

The core consumer uses only `frozenchars/mod/core.hpp`, calls `find_package(frozenchars CONFIG REQUIRED)`, and links `frozenchars::frozenchars`. It must configure and compile without Glaze in its search path.

The Glaze consumer calls `find_package(frozenchars CONFIG REQUIRED COMPONENTS glaze)` and links `frozenchars::glaze`.

The script must configure/install the project twice into disposable prefixes: once with a poisoned or absent Glaze path for core, once with the normal manifest setup for Glaze. It must also compile a `json::crush` NTTP case that exceeds the unmodified GCC/Clang constexpr default.

Register `verify_package.cmake` as a `package_consumer` CTest on host platforms, passing the source directory, binary directory, generator, configuration, and compiler explicitly. Do not register this external-process test for WASI or Emscripten.

- [ ] **Step 2: Run fixture to verify failure**

Run `ctest --test-dir build --output-on-failure -R '^package_consumer$'` with the same compiler as `build/CMakeCache.txt`. Expected: FAIL because the current exported target either requires Glaze for core or fails to export the required constexpr limits.

- [ ] **Step 3: Implement target split**

Keep `frozenchars` as the only unconditional target. If Glaze is found, add an INTERFACE target named `frozenchars_glaze`, give it `EXPORT_NAME glaze`, add alias `frozenchars::glaze`, link it to `frozenchars` and `glaze::glaze`, and include it in the install export. Verify that the installed target is `frozenchars::glaze`, not `frozenchars::frozenchars_glaze`.

Move GCC/Clang/MSVC constexpr limits from directory-wide `add_compile_options()` to `target_compile_options(frozenchars INTERFACE ...)`. Keep `-Wall/-Wextra/-pedantic/-W4` local to the project build. Preserve CMake 3.25 generator-expression support and do not rely on newer export-name rewriting.

- [ ] **Step 4: Implement component-aware package config**

Always include `frozencharsTargets.cmake` and define `frozenchars_glaze_FOUND` from `FROZENCHARS_HAS_GLAZE_TARGET` generated into the config. For a requested `glaze` component, require Glaze with `find_dependency(glaze CONFIG)` and require the Glaze target. Leave an unavailable Glaze package harmless for core-only consumers.

- [ ] **Step 5: Update in-tree consumers and documentation**

Link Glaze-using test/example targets to `frozenchars::glaze`, otherwise retain `frozenchars::frozenchars`. Update the README installation section with both core and optional Glaze forms, including:

```cmake
find_package(frozenchars CONFIG REQUIRED COMPONENTS glaze)
target_link_libraries(myapp PRIVATE frozenchars::glaze)
```

- [ ] **Step 6: Verify package behavior**

Run:

```bash
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Expected: 53+ tests and both package scenarios pass. Inspect the installed `frozencharsConfig.cmake` and exported target to confirm core does not contain `glaze::glaze`.

- [ ] **Step 7: Commit**

```bash
git add CMakeLists.txt cmake test example README.md
git commit -m "fix: make glaze CMake integration optional"
```

---

### Task 6: Regenerate single header and verify the repository

**Files:**
- Modify: `single_include/frozenchars.hpp` (generated)
- Modify: `single_include/frozenchars_json.hpp` (generated)
- Modify: `README.md` (only if implementation docs require)
- Test: entire repository

- [ ] **Step 1: Regenerate amalgamation**

Run:

```bash
python3 tools/amalgamate.py
```

Expected: tracked single-header outputs include the UTF-8/chrono/container fixes in `single_include/frozenchars.hpp` and the JSON/Crush fixes in `single_include/frozenchars_json.hpp`, with no unrelated generated churn.

- [ ] **Step 2: Validate all surfaces**

Run:

```bash
bash test.sh
python3 tools/check_readme_examples.py
python3 tools/check_readme_anchors.py
python3 tools/amalgamate.py --check
git diff --check
```

Expected: PASS.

- [ ] **Step 3: Review the final diff**

Run the repository's code-review subagent over `bf01aec..HEAD`. Confirm every accepted finding has a regression test and no remaining unconditional `noexcept`, weak offset validation, invalid-UTF8 acceptance, or base-target Glaze dependency. Fix any concrete issue before declaring completion.

- [ ] **Step 4: Commit generated output**

```bash
git add single_include/frozenchars.hpp single_include/frozenchars_json.hpp README.md
git commit -m "build: refresh frozenchars single header"
```
