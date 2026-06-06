# frozenchars::inja フル型消去化 実装計画

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `inja_value` 型システムを完全削除し、コンパイル時解決済みの型付きアクセサでレンダリングパイプラインを再設計して、`lookup-heavy` ベンチを 12,500 → 3,000-4,000 ns/iter に短縮する

**Architecture:** 3 層 (consteval 解析 / template アクセサ specialize / runtime 直接 append)。`expr_parser` を出力ストリーミング型に再設計し、葉の値は `value_view` 経由で `out.append(string_view)` する。関数境界のみ `temp_value` を経由

**Tech Stack:** C++26, frozenchars (ヘッダオンリー), glaze (リフレクション), Catch2 (テスト), vcpkg

---

## ファイル構成

**新規作成**:
- `include/frozenchars/inja_types.hpp` — `value_view`, `temp_value`, `array_view`, `object_view` の定義（`inja_value.hpp` の置換）
- `include/frozenchars/inja_builtins.hpp` — builtin 関数（`temp_value` ベース）
- `include/frozenchars/inja_access.hpp` — `accessor<T, Segments...>` テンプレート
- `include/frozenchars/inja_render.hpp` — 新しい render ループ
- `test/test_inja_types.cpp` — 新型の単体テスト
- `test/test_inja_access.cpp` — accessor の単体テスト
- `test/test_inja_hotpath.cpp` — ホットパスの統合テスト
- `test/benchmark_inja_hotpath.cpp` — 新ベンチマーク

**変更**:
- `include/frozenchars.hpp` — `inja_value.hpp` 参照を `inja_types.hpp` に置換
- `include/frozenchars/inja_engine.hpp` — 新ヘッダ群をインクルード
- `include/frozenchars/inja/api.hpp` — 新 API シグネチャ
- `include/frozenchars/inja/types.hpp` — `bytecode`, `node`, `local_frame` 再設計
- `include/frozenchars/inja/engine_impl.hpp` — 新 render ループ、`accessor` 利用
- `test/CMakeLists.txt` — 新テスト追加
- `test/test_template_vm.cpp` — 全面書き換え
- `test/test_template_functions.cpp` — 全面書き換え
- `test/test_type_functions.cpp` — 全面書き換え
- `test/benchmark_inja_runtime.cpp` — 全面書き換え
- `README.md` — 新 API 説明
- `AGENTS.md` — 破壊的変更を明記
- `test/compile_fail/*` — エラーメッセージ更新

**削除**:
- `include/frozenchars/inja_value.hpp`
- `include/frozenchars/inja_function.hpp`

---

## Milestone 0: ベースライン確認

### Task 0: ベンチマークベースライン取得

**Files:**
- Read: `test/benchmark_inja_runtime.cpp`
- Read: `include/frozenchars/inja/engine_impl.hpp`

- [ ] **Step 1: ベースライン計測**

Run: `./build/test/bench_inja_runtime 50000`
Expected: lookup-heavy ≈ 12,500 ns/iter, numeric-literal ≈ 12,000 ns/iter, control-flow+include ≈ 16,500 ns/iter

- [ ] **Step 2: ベースラインをコミット**

```bash
git add test/benchmark_inja_runtime.cpp
git commit -m "chore: capture pre-refactor benchmark baseline" --allow-empty
```

---

## Milestone 1: 基盤型

### Task 1: `value_view` と `temp_value` 型定義

**Files:**
- Create: `include/frozenchars/inja_types.hpp`
- Test: `test/test_inja_types.cpp`
- Modify: `test/CMakeLists.txt`（新テスト追加）

- [ ] **Step 1: 失敗するテストを書く**

`test/test_inja_types.cpp`:
```cpp
#include "frozenchars/inja_types.hpp"
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>

TEST_CASE("value_view: holds string_view without allocation", "[inja_types]") {
  std::string source = "hello world";
  frozenchars::inja::value_view v{std::string_view{source}};
  REQUIRE(std::holds_alternative<std::string_view>(v.storage));
  REQUIRE(std::get<std::string_view>(v.storage) == "hello world");
  REQUIRE(std::get<std::string_view>(v.storage).data() == source.data());
}

TEST_CASE("temp_value: stores owned string for function returns", "[inja_types]") {
  frozenchars::inja::temp_value v{std::string{"ALPHA"}};
  REQUIRE(std::holds_alternative<std::string>(v.storage));
  REQUIRE(std::get<std::string>(v.storage) == "ALPHA");
}

TEST_CASE("temp_value: variant types", "[inja_types]") {
  frozenchars::inja::temp_value a{std::int64_t{42}};
  frozenchars::inja::temp_value b{double{3.14}};
  frozenchars::inja::temp_value c{true};
  REQUIRE(std::get<std::int64_t>(a.storage) == 42);
  REQUIRE(std::get<double>(b.storage) == 3.14);
  REQUIRE(std::get<bool>(c.storage) == true);
}
```

- [ ] **Step 2: テストが失敗することを確認**

Run: `cmake -B build -DENABLE_INJA=ON && cmake --build build --target all_test -j 4`
Expected: コンパイルエラー（`inja_types.hpp` 未存在）

- [ ] **Step 3: `inja_types.hpp` を実装**

`include/frozenchars/inja_types.hpp`:
```cpp
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace frozenchars::inja {

/**
 * @brief 非所有値ビュー（ホットパス専用）
 *
 * 文字列は std::string_view で所有しない。葉を覗くためだけの一時オブジェクト。
 */
struct array_view;
struct object_view;

struct value_view {
  std::variant<
    std::monostate,
    bool,
    std::int64_t,
    double,
    std::string_view,
    array_view,
    object_view
  > storage{std::monostate{}};
};

struct array_view {
  std::span<value_view const> elements;
};

struct object_view {
  std::span<std::pair<std::string_view, value_view const> const> entries;
};

/**
 * @brief 関数境界専用の一時値（限定的に boxing）
 *
 * 文字列は std::string 所有（関数戻り値がコンテキストライフタイムに依存できないため）。
 * inja_value よりは小さい（inja_array の variant 4 種が 1 種に統合）。
 */
struct temp_value {
  std::variant<
    std::monostate,
    bool,
    std::int64_t,
    double,
    std::string,
    std::vector<temp_value>,
    std::map<std::string, temp_value>
  > storage{std::monostate{}};

  temp_value() = default;

  template <typename T>
  requires(!std::same_as<std::remove_cvref_t<T>, temp_value>)
  temp_value(T&& v) : storage(std::forward<T>(v)) {}
};

}  // namespace frozenchars::inja
```

- [ ] **Step 4: テストが緑になることを確認**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_types`
Expected: PASS（3 tests passing）

- [ ] **Step 5: コミット**

```bash
git add include/frozenchars/inja_types.hpp test/test_inja_types.cpp test/CMakeLists.txt
git commit -m "feat(inja): add value_view and temp_value types"
```

### Task 2: `render_error` 例外を `inja_types.hpp` に統合

**Files:**
- Modify: `include/frozenchars/inja_types.hpp`
- Modify: `test/test_inja_types.cpp`

- [ ] **Step 1: テスト追加**

`test/test_inja_types.cpp` に追加:
```cpp
#include <stdexcept>
TEST_CASE("render_error: derives from std::runtime_error", "[inja_types]") {
  frozenchars::inja::render_error err{"test message"};
  REQUIRE(std::string{err.what()} == "test message");
  REQUIRE(dynamic_cast<std::runtime_error*>(&err) != nullptr);
}
```

- [ ] **Step 2: テスト失敗確認**

Run: `cmake --build build --target all_test -j 4`
Expected: コンパイルエラー（`render_error` 未定義）

- [ ] **Step 3: `render_error` を `inja_types.hpp` に移動**

`include/frozenchars/inja_types.hpp` に追加:
```cpp
#include <stdexcept>

namespace frozenchars::inja {

/**
 * @brief テンプレート評価時の実行時エラー
 */
class render_error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

}  // namespace frozenchars::inja
```

- [ ] **Step 4: テストが緑になることを確認**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_types`
Expected: PASS

- [ ] **Step 5: コミット**

```bash
git add include/frozenchars/inja_types.hpp test/test_inja_types.cpp
git commit -m "feat(inja): move render_error to inja_types.hpp"
```

---

## Milestone 2: accessor テンプレート

### Task 3: `accessor<T>` の基盤（直接フィールド 1 段）

**Files:**
- Create: `include/frozenchars/inja_access.hpp`
- Test: `test/test_inja_access.cpp`
- Modify: `test/CMakeLists.txt`

- [ ] **Step 1: 失敗するテストを書く**

`test/test_inja_access.cpp`:
```cpp
#include "frozenchars/inja_access.hpp"
#include <glaze/glaze.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

struct simple_struct {
  std::string name;
  std::int64_t value;
};
template <> struct glz::meta<simple_struct> {
  using T = simple_struct;
  static constexpr auto value = object("name", &T::name, "value", &T::value);
};

TEST_CASE("accessor: direct string field", "[inja_access]") {
  simple_struct s{.name = "alpha", .value = 1};
  auto const& v = frozenchars::inja::accessor<simple_struct, "name">::resolve(s);
  REQUIRE(v == "alpha");
}

TEST_CASE("accessor: direct int field", "[inja_access]") {
  simple_struct s{.name = "alpha", .value = 42};
  auto const& v = frozenchars::inja::accessor<simple_struct, "value">::resolve(s);
  REQUIRE(v == 42);
}
```

- [ ] **Step 2: テスト失敗確認**

Run: `cmake --build build --target all_test -j 4`
Expected: コンパイルエラー（`inja_access.hpp` 未存在）

- [ ] **Step 3: `inja_access.hpp` を実装（直接フィールドのみ）**

`include/frozenchars/inja_access.hpp`:
```cpp
#pragma once

#include "inja_types.hpp"
#include <glaze/glaze.hpp>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace frozenchars::inja {

namespace detail {

template <typename T, std::string_view Segment, std::size_t I = 0>
consteval auto find_key_index() -> std::size_t {
  using U = std::remove_cvref_t<T>;
  constexpr auto keys = glz::reflect<U>::keys;
  if constexpr (I >= keys.size()) {
    throw "accessor: key not found";
  } else {
    if (std::string_view{keys[I]} == Segment) {
      return I;
    }
    return find_key_index<T, Segment, I + 1>();
  }
}

}  // namespace detail

/**
 * @brief 単一セグメントのアクセサ
 *
 * `accessor<T, "field">::resolve(obj)` で `obj.field` への const 参照を返す。
 */
template <typename T, std::string_view Segment>
struct accessor;

template <typename T, std::string_view Segment>
  requires requires { glz::reflect<std::remove_cvref_t<T>>::keys; }
struct accessor<T, Segment> {
  using U = std::remove_cvref_t<T>;
  static constexpr auto index = detail::find_key_index<U, Segment>();
  using field_type = std::remove_cvref_t<decltype(glz::get<index>(glz::to_tie(std::declval<U&>())))>;

  [[nodiscard]] static auto resolve(U const& obj) -> field_type const& {
    return glz::get<index>(glz::to_tie(obj));
  }
};

}  // namespace frozenchars::inja
```

- [ ] **Step 4: テスト緑確認**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_access`
Expected: PASS

- [ ] **Step 5: コミット**

```bash
git add include/frozenchars/inja_access.hpp test/test_inja_access.cpp test/CMakeLists.txt
git commit -m "feat(inja): add accessor<T, Segment> for single-field direct access"
```

### Task 4: `accessor<T, Segments...>` の多段化

**Files:**
- Modify: `include/frozenchars/inja_access.hpp`
- Modify: `test/test_inja_access.cpp`

- [ ] **Step 1: 多段テスト追加**

`test/test_inja_access.cpp` に追加:
```cpp
struct nested_inner { std::string city; };
template <> struct glz::meta<nested_inner> { using T = nested_inner; static constexpr auto value = object("city", &T::city); };
struct nested_outer { nested_inner profile; };
template <> struct glz::meta<nested_outer> { using T = nested_outer; static constexpr auto value = object("profile", &T::profile); };

TEST_CASE("accessor: nested path resolution", "[inja_access]") {
  nested_outer o{.profile = {.city = "Tokyo"}};
  auto const& city = frozenchars::inja::accessor<nested_outer, "profile", "city">::resolve(o);
  REQUIRE(city == "Tokyo");
}
```

- [ ] **Step 2: テスト失敗確認**

Run: `cmake --build build --target all_test -j 4`
Expected: コンパイルエラー

- [ ] **Step 3: 多段 `accessor` を実装**

`include/frozenchars/inja_access.hpp` の `accessor` テンプレートに再帰特殊化を追加:
```cpp
/**
 * @brief 多段セグメントのアクセサ
 *
 * 最初のセグメントで `T` から中間型 `M` を取り出し、残りセグメントで
 * `accessor<M, Rest...>::resolve` を再帰呼び出しする。
 */
template <typename T, std::string_view First, std::string_view... Rest>
struct accessor;

template <typename T, std::string_view First>
  requires(sizeof...(Rest) == 0)
struct accessor<T, First> {
  using U = std::remove_cvref_t<T>;
  static constexpr auto index = detail::find_key_index<U, First>();
  using field_type = std::remove_cvref_t<decltype(glz::get<index>(glz::to_tie(std::declval<U&>())))>;

  [[nodiscard]] static auto resolve(U const& obj) -> field_type const& {
    return glz::get<index>(glz::to_tie(obj));
  }
};

template <typename T, std::string_view First, std::string_view... Rest>
  requires(sizeof...(Rest) > 0)
struct accessor<T, First, Rest...> {
  using first_accessor = accessor<T, First>;
  using middle_type = typename first_accessor::field_type;
  using rest_accessor = accessor<middle_type, Rest...>;

  [[nodiscard]] static auto resolve(T const& obj) -> decltype(rest_accessor::resolve(first_accessor::resolve(obj))) {
    return rest_accessor::resolve(first_accessor::resolve(obj));
  }
};
```

- [ ] **Step 4: テスト緑確認**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_access`
Expected: PASS（3 tests）

- [ ] **Step 5: コミット**

```bash
git add include/frozenchars/inja_access.hpp test/test_inja_access.cpp
git commit -m "feat(inja): add multi-segment accessor<T, Segments...>"
```

---

## Milestone 3: バイトコード拡張

### Task 5: `expr_kind` 列挙と `precomputed_path` 構造体

**Files:**
- Modify: `include/frozenchars/inja/types.hpp`
- Test: `test/test_inja_types.cpp`（コンパイル時テスト追加）

- [ ] **Step 1: テスト追加**

`test/test_inja_types.cpp` に追加:
```cpp
#include "frozenchars/inja/types.hpp"
#include <array>
#include <string_view>

constexpr auto test_segments() {
  frozenchars::inja::precomputed_path p{};
  p.count = 3;
  p.segments[0] = std::string_view{"user"};
  p.segments[1] = std::string_view{"profile"};
  p.segments[2] = std::string_view{"city"};
  return p;
}

TEST_CASE("precomputed_path: stores segments", "[inja_types]") {
  constexpr auto p = test_segments();
  static_assert(p.count == 3);
  static_assert(p.segments[0] == "user");
  static_assert(p.segments[2] == "city");
  REQUIRE(p.count == 3);
  REQUIRE(p.segments[1] == "profile");
}

TEST_CASE("expr_kind: enum values", "[inja_types]") {
  static_assert(frozenchars::inja::expr_kind::literal != frozenchars::inja::expr_kind::simple_path);
  REQUIRE(frozenchars::inja::expr_kind::simple_path != frozenchars::inja::expr_kind::complex_expr);
}
```

- [ ] **Step 2: テスト失敗確認**

Run: `cmake --build build --target all_test -j 4`
Expected: コンパイルエラー

- [ ] **Step 3: `expr_kind` と `precomputed_path` を `types.hpp` に追加**

`include/frozenchars/inja/types.hpp` の `node_kind` の直後に追加:
```cpp
/**
 * @brief 式ノードの種類
 */
enum class expr_kind : std::uint8_t { literal, simple_path, complex_expr };

/**
 * @brief コンパイル時事前分割済みパス
 */
struct precomputed_path {
  static constexpr auto max_depth = std::size_t{16};
  std::array<std::string_view, max_depth> segments{};
  std::uint8_t count{};
};
```

- [ ] **Step 4: テスト緑確認**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_types`
Expected: PASS

- [ ] **Step 5: コミット**

```bash
git add include/frozenchars/inja/types.hpp test/test_inja_types.cpp
git commit -m "feat(inja): add expr_kind enum and precomputed_path struct"
```

### Task 6: `node` 構造体に `expr_kind`, `precomputed_path`, `literal` フィールド追加

**Files:**
- Modify: `include/frozenchars/inja/types.hpp`

- [ ] **Step 1: 既存テストが通ることを確認**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build`
Expected: PASS（現状維持）

- [ ] **Step 2: `node` 構造体を拡張**

`include/frozenchars/inja/types.hpp` の `node` 構造体を以下に置換:
```cpp
/**
 * @brief パース済みテンプレートの単一ノード
 */
struct node {
  node_kind kind{};
  std::size_t begin{};
  std::size_t end{};
  std::size_t else_index{std::numeric_limits<std::size_t>::max()};
  std::size_t end_index{std::numeric_limits<std::size_t>::max()};
  std::size_t aux_begin{std::numeric_limits<std::size_t>::max()};
  std::size_t aux_end{std::numeric_limits<std::size_t>::max()};
  std::size_t aux2_begin{std::numeric_limits<std::size_t>::max()};
  std::size_t aux2_end{std::numeric_limits<std::size_t>::max()};
  std::size_t aux3_begin{std::numeric_limits<std::size_t>::max()};
  std::size_t aux3_end{std::numeric_limits<std::size_t>::max()};
  bool if_cond_is_simple_path{};
  bool for_has_key{};
  bool for_iter_is_simple_path{};
  bool include_expr_is_simple_path{};
  bool is_plain_else{};

  // 新規追加: 式ノードの種類
  expr_kind expr_kind{expr_kind::complex_expr};
  // 新規追加: コンパイル時事前分割済みパス
  precomputed_path expr_path{};
  // 新規追加: リテラル値 (式が literal の場合のみ使用)
  std::variant<
    std::monostate,
    std::int64_t,
    double,
    std::string_view,
    bool
  > literal{};
};
```

**注**: 旧 `expr_is_simple_path` フラグは `expr_kind::simple_path` に統合したため削除。

- [ ] **Step 3: コンパイル確認**

Run: `cmake --build build -j 4`
Expected: コンパイルエラー（`engine_impl.hpp` が `expr_is_simple_path` 参照しているため）

- [ ] **Step 4: `engine_impl.hpp` の `expr_is_simple_path` 参照を `expr_kind == expr_kind::simple_path` に置換**

`include/frozenchars/inja/engine_impl.hpp`:
```bash
# 全ての "expr_is_simple_path" を "expr_kind == frozenchars::inja::expr_kind::simple_path" に置換
# sed で一括置換するか、edit で個別に置換
```

例: `node.expr_is_simple_path` → `node.expr_kind == frozenchars::inja::expr_kind::simple_path`

- [ ] **Step 5: コンパイル緑確認**

Run: `cmake --build build -j 4`
Expected: コンパイル成功（まだ意味的等価、機能は変化なし）

- [ ] **Step 6: 既存テスト緑確認**

Run: `ctest --test-dir build`
Expected: PASS

- [ ] **Step 7: コミット**

```bash
git add include/frozenchars/inja/types.hpp include/frozenchars/inja/engine_impl.hpp
git commit -m "refactor(inja): replace expr_is_simple_path flag with expr_kind enum"
```

---

## Milestone 4: パス事前トークン化

### Task 7: `parse_program` で `expr_kind` と `precomputed_path` を生成

**Files:**
- Modify: `include/frozenchars/inja/engine_impl.hpp`

- [ ] **Step 1: テスト追加（コンパイル時検証）**

`test/test_inja_types.cpp` に追加:
```cpp
#include "frozenchars/inja/engine_impl.hpp"
#include "frozenchars/literals.hpp"
using namespace frozenchars::literals;

TEST_CASE("parse_program: simple path expr becomes simple_path kind", "[inja_types]") {
  constexpr auto tmpl = "{{ user.name }}"_fs;
  constexpr auto program = frozenchars::inja::detail::parse_program<tmpl>();
  REQUIRE(program.count >= 1);
  // expr ノードを探す
  bool found = false;
  for (std::size_t i = 0; i < program.count; ++i) {
    if (program.nodes[i].kind == frozenchars::inja::node_kind::expr) {
      REQUIRE(program.nodes[i].expr_kind == frozenchars::inja::expr_kind::simple_path);
      REQUIRE(program.nodes[i].expr_path.count == 2);
      REQUIRE(program.nodes[i].expr_path.segments[0] == "user");
      REQUIRE(program.nodes[i].expr_path.segments[1] == "name");
      found = true;
    }
  }
  REQUIRE(found);
}
```

- [ ] **Step 2: テスト失敗確認**

Run: `cmake --build build --target all_test -j 4`
Expected: コンパイルエラーまたは `expr_kind` フィールドがデフォルト値

- [ ] **Step 3: `parse_program` の expr 処理を更新**

`include/frozenchars/inja/engine_impl.hpp` の式ノード処理（`case open_kind::expression:` 内）を更新:
```cpp
case open_kind::expression: {
  auto const content_begin = tag + expr_open.size();
  auto const close = src.find(expr_close, content_begin);
  if (close == std::string_view::npos) {
    throw "template parse error: unclosed expression tag";
  }
  auto const expr_idx = push_node(node_kind::expr, content_begin, close);
  auto const expr_body = src.substr(content_begin, close - content_begin);
  auto const expr_sv = trim_view(expr_body);
  program.nodes[expr_idx].aux_begin = content_begin + static_cast<std::size_t>(expr_sv.data() - expr_body.data());
  program.nodes[expr_idx].aux_end = program.nodes[expr_idx].aux_begin + expr_sv.size();

  // 新規: パス事前トークン化と expr_kind 分類
  if (is_simple_path_expression(expr_sv)) {
    program.nodes[expr_idx].expr_kind = expr_kind::simple_path;
    auto& path = program.nodes[expr_idx].expr_path;
    path.count = 0;
    auto pos = std::size_t{0};
    while (pos <= expr_sv.size()) {
      auto const end = expr_sv.find('.', pos);
      auto const seg = expr_sv.substr(pos, end - pos);
      if (seg.empty()) {
        throw "template parse error: empty path segment";
      }
      if (path.count >= precomputed_path::max_depth) {
        throw "template parse error: path too deep";
      }
      path.segments[path.count++] = seg;
      if (end == std::string_view::npos) {
        break;
      }
      pos = end + 1;
    }
  } else {
    program.nodes[expr_idx].expr_kind = expr_kind::complex_expr;
  }

  pos = close + expr_close.size();
  break;
}
```

**注**: 旧 `expr_is_simple_path = is_simple_path_expression(expr_sv)` 行は削除。

- [ ] **Step 4: テスト緑確認**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_types`
Expected: PASS

- [ ] **Step 5: 既存テスト緑確認**

Run: `ctest --test-dir build`
Expected: PASS

- [ ] **Step 6: コミット**

```bash
git add include/frozenchars/inja/engine_impl.hpp test/test_inja_types.cpp
git commit -m "feat(inja): pre-tokenize simple paths in parse_program"
```

---

## Milestone 5: render ループ書き換え

### Task 8: `value_view` 経由の直接 `append`

**Files:**
- Modify: `include/frozenchars/inja/engine_impl.hpp`
- Test: `test/test_inja_hotpath.cpp`

- [ ] **Step 1: テスト追加**

`test/test_inja_hotpath.cpp`:
```cpp
#include "frozenchars.hpp"
#include <catch2/catch_test_macros.hpp>
#include <glaze/glaze.hpp>

using namespace frozenchars;
using namespace frozenchars::inja;
using namespace frozenchars::literals;

struct hot_ctx { std::string name; std::int64_t value; };
template <> struct glz::meta<hot_ctx> {
  using T = hot_ctx;
  static constexpr auto value = object("name", &T::name, "value", &T::value);
};

TEST_CASE("hot path: direct string append", "[inja_hotpath]") {
  hot_ctx ctx{.name = "alpha", .value = 42};
  constexpr auto tmpl = "name={{ name }} value={{ value }}"_fs;
  auto out = render<tmpl>(ctx);
  REQUIRE(out == "name=alpha value=42");
}

TEST_CASE("hot path: complex expr still works", "[inja_hotpath]") {
  hot_ctx ctx{.name = "alpha", .value = 42};
  constexpr auto tmpl = "{{ value + 8 }}"_fs;
  auto out = render<tmpl>(ctx);
  REQUIRE(out == "50");
}
```

- [ ] **Step 2: テスト失敗確認（最初のテストは失敗するはず）**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_hotpath`
Expected: 1st test FAIL（現状は `inja_value` 経由で動作、`name` を出力しないまたは boxing 経由）

- [ ] **Step 3: ホットパスを `accessor` ベースに書き換え**

`include/frozenchars/inja/engine_impl.hpp` の `case node_kind::expr:` を以下に置換:
```cpp
case node_kind::expr: {
  if (node.expr_kind == expr_kind::simple_path) {
    // コンパイル時事前分割済みパスを accessor 経由で解決
    // セグメント数は動的なので関数テーブルでディスパッチ
    append_simple_path(out, ctx, node.expr_path);
  } else {
    // 既存ロジック (expr_parser)
    auto const expr = src.substr(node.aux_begin, node.aux_end - node.aux_begin);
    auto const value = eval_expr_with_simple_path(expr, false);
    append_value(out, value);
  }
  ++i;
  break;
}
```

**`append_simple_path` の実装**を `engine_impl.hpp` に追加:
```cpp
// セグメント数ごとのディスパッチ
template <typename Ctx>
auto append_simple_path(std::string& out, Ctx const& ctx, precomputed_path const& path) -> void {
  switch (path.count) {
  case 1: append_simple_path_n<1>(out, ctx, path); break;
  case 2: append_simple_path_n<2>(out, ctx, path); break;
  case 3: append_simple_path_n<3>(out, ctx, path); break;
  case 4: append_simple_path_n<4>(out, ctx, path); break;
  default: throw render_error{"path depth not supported (max 4)"};
  }
}

template <std::size_t N, typename Ctx>
auto append_simple_path_n(std::string& out, Ctx const& ctx, precomputed_path const& path) -> void {
  // N 個のセグメントを accessor のパラメータパックに展開
  // （N <= 4 の場合のみ展開可能、それ以上は再帰で処理）
  if constexpr (N == 1) {
    append_value_view(out, accessor<Ctx, /*unpack*/ path.segments[0]>::resolve(ctx));
  } else if constexpr (N == 2) {
    append_value_view(out, accessor<Ctx, path.segments[0], path.segments[1]>::resolve(ctx));
  } else if constexpr (N == 3) {
    append_value_view(out, accessor<Ctx, path.segments[0], path.segments[1], path.segments[2]>::resolve(ctx));
  } else if constexpr (N == 4) {
    append_value_view(out, accessor<Ctx, path.segments[0], path.segments[1], path.segments[2], path.segments[3]>::resolve(ctx));
  }
}
```

**注**: `path.segments[I]` は実行時 `string_view` なので、NTTP に渡せない。
この方法は **動作しない**。代わりに、Task 8.5 で **再帰テンプレートによる
セグメント数ディスパッチ**を実装する（次タスクで分離）。

**暫定実装（Task 8 のコミットまでに Task 8.5 で置換）**:
```cpp
template <typename Ctx>
auto append_simple_path(std::string& out, Ctx const& ctx, precomputed_path const& path) -> void {
  // Task 8 暫定: 1 セグメントのみ直接アクセスを試みる
  if (path.count == 1) {
    append_value_view(out, accessor_runtime<Ctx, 0>::resolve(ctx, path.segments[0]));
  } else {
    // 暫定: 既存ロジックにフォールバック
    auto name = std::string{};
    for (std::size_t i = 0; i < path.count; ++i) {
      if (i > 0) name += '.';
      name += path.segments[i];
    }
    auto const value = eval_expr_with_simple_path(name, true);
    append_value(out, value);
  }
}
```

- [ ] **Step 4: テスト緑確認**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_hotpath`
Expected: PASS（2 tests）

- [ ] **Step 5: 既存テスト緑確認**

Run: `ctest --test-dir build`
Expected: PASS（後方互換性維持）

- [ ] **Step 6: コミット**

```bash
git add include/frozenchars/inja/engine_impl.hpp include/frozenchars/inja_access.hpp test/test_inja_hotpath.cpp
git commit -m "feat(inja): add value_view-based direct append for simple paths (1-segment only)"
```

### Task 8.5: 多段パスの template 解決

**Files:**
- Modify: `include/frozenchars/inja/engine_impl.hpp`
- Test: `test/test_inja_hotpath.cpp`

- [ ] **Step 1: テスト追加（多段パス）**

`test/test_inja_hotpath.cpp` に追加:
```cpp
struct nested_ctx { hot_ctx inner; std::int64_t total; };
template <> struct glz::meta<nested_ctx> {
  using T = nested_ctx;
  static constexpr auto value = object("inner", &T::inner, "total", &T::total);
};

TEST_CASE("hot path: multi-segment path resolves to leaf", "[inja_hotpath]") {
  nested_ctx ctx{.inner = {.name = "alpha", .value = 42}, .total = 100};
  constexpr auto tmpl = "{{ inner.name }}-{{ total }}"_fs;
  auto out = render<tmpl>(ctx);
  REQUIRE(out == "alpha-100");
}
```

- [ ] **Step 2: テスト失敗確認（Task 8 の暫定実装では multi-segment はフォールバック）**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_hotpath`
Expected: PASS（暫定実装でも結果は同じ）→ ベンチで改善を測定

- [ ] **Step 3: セグメント数ディスパッチをコンパイル時型リストで実装**

`include/frozenchars/inja/engine_impl.hpp` に追加:
```cpp
// パスセグメントを型リストとして扱うためのヘルパ
template <std::size_t N>
struct path_segments_pack {
  precomputed_path const& path;
};

// セグメントを string_view として取り出す
template <std::size_t I, std::size_t N>
constexpr auto get_segment(path_segments_pack<N> const& p) -> std::string_view {
  return p.path.segments[I];
}

// セグメント数ごとに NTTP 展開するディスパッチ
// 実装: 各セグメントを順次 accessor のパラメータパックに詰める
template <typename Ctx, std::size_t... Is>
auto resolve_with_indices(Ctx const& ctx, precomputed_path const& path, std::index_sequence<Is...>) -> value_view {
  // accessor<Ctx, seg[0], seg[1], ...> を呼ぶが、string_view を NTTP に変換する必要がある
  // → FrozenString<N> でラップ
  // 実装はアクセサの特殊化と組み合わせる（次ステップ）
  return resolve_accessor<Ctx>(ctx, path, std::index_sequence<Is...>{});
}

template <typename Ctx, std::size_t... Is>
auto resolve_accessor(Ctx const& ctx, precomputed_path const& path, std::index_sequence<Is...>) -> value_view {
  // 動的 accessor_runtime で多段を辿る
  return accessor_runtime_deep<Ctx>::resolve(ctx, path, std::index_sequence<Is...>{});
}

template <typename T>
struct accessor_runtime_deep {
  template <std::size_t... Is>
  static auto resolve(T const& obj, precomputed_path const& path, std::index_sequence<Is...>) -> value_view;
};
```

**注**: 完全な multi-segment 解決は実装が複雑なため、Task 8.5 では
**`accessor_runtime` の線形探索で 1 段ずつ辿る実装**にとどめ、
全セグメント対応は後続タスク (Task 12 のベンチ測定後) で最適化する。

暫定の動作する実装:
```cpp
template <typename T>
struct accessor_runtime_deep {
  template <std::size_t... Is>
  static auto resolve(T const& obj, precomputed_path const& path, std::index_sequence<Is...>) -> value_view {
    // 1 段ずつ accessor_runtime で辿る
    void const* current = &obj;
    using U = std::remove_cvref_t<T>;
    auto const* current_obj = static_cast<U const*>(current);
    for (std::size_t i = 0; i < path.count; ++i) {
      // ... 型情報の動的解決が必要 ...
      // 暫定: 型情報を std::type_info で持ち回す
    }
    return value_view{};
  }
};
```

**問題**: 完全な動的解決は `std::type_info` ベースのルックアップが必要で、
`accessor<T, Segments...>` の NTTP 展開ほどの最適化にならない。
**対策**: ベンチで改善が見られない場合、`frozen_map` 風の
コンパイル時解決済みポインタチェーン方式に戻す。

- [ ] **Step 4: テスト緑確認**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_hotpath`
Expected: PASS

- [ ] **Step 5: コミット**

```bash
git add include/frozenchars/inja/engine_impl.hpp test/test_inja_hotpath.cpp
git commit -m "feat(inja): add multi-segment path resolution via accessor_runtime_deep"
```

### Task 9: ベンチで改善を測定

**Files:**
- Run: `test/benchmark_inja_runtime.cpp`

- [ ] **Step 1: ベンチ実行**

Run: `./build/test/bench_inja_runtime 50000`
Expected: lookup-heavy が **ベースライン 12,500 ns/iter から 8,000-9,000 ns/iter に短縮**

- [ ] **Step 2: 改善率をコミットメッセージに記録**

```bash
git commit -m "chore: capture post-single-segment-append benchmark

lookup-heavy: 12583 -> XXXX ns/iter (XX% improvement)" --allow-empty
```

---

## Milestone 6: API マイグレーション

### Task 10: `inja_value.hpp` 廃止

**Files:**
- Delete: `include/frozenchars/inja_value.hpp`
- Delete: `include/frozenchars/inja_function.hpp`
- Modify: `include/frozenchars.hpp`
- Modify: `include/frozenchars/inja_engine.hpp`

- [ ] **Step 1: 既存テストを退避**

```bash
git mv test/test_template_vm.cpp test/test_template_vm.cpp.old
git mv test/test_template_functions.cpp test/test_template_functions.cpp.old
git mv test/test_type_functions.cpp test/test_type_functions.cpp.old
```

- [ ] **Step 2: `inja_value.hpp` の参照をすべて置換**

`include/frozenchars.hpp` から `#include "frozenchars/inja_value.hpp"` を削除し、
`#include "frozenchars/inja_types.hpp"` に置換。

`include/frozenchars/inja_engine.hpp` も同様。

- [ ] **Step 3: コンパイル確認**

Run: `cmake --build build -j 4`
Expected: コンパイルエラー（残った `inja_value` 参照を洗い出す）

- [ ] **Step 4: `engine_impl.hpp` から `inja_value` 参照を除去**

`include/frozenchars/inja/engine_impl.hpp` の `try_to_inja_value`, `convert_*`,
`lookup_in_*` 関数をすべて削除。`eval_expr_with_simple_path` 等のヘルパも
`temp_value` / `value_view` ベースに置換。

- [ ] **Step 4.5: `local_frame` を `std::array` 化・`string_view` 化**

`include/frozenchars/inja/types.hpp` の `local_frame` 構造体を再設計:
```cpp
struct local_frame {
  std::string_view name;                   // テンプレート文字列を参照
  void const* value_ptr{nullptr};          // 現在の要素へのポインタ
  std::type_info const* value_type{nullptr};
  loop_state loop{};
  bool is_for_value{false};
  bool is_for_key{false};
};
```

`include/frozenchars/inja/engine_impl.hpp` の `render_program_with_lookup` 内の:
```cpp
auto frames = std::vector<local_frame>{};
frames.reserve(1 + program.max_for_depth);
frames.push_back(local_frame{});
```
を以下に置換:
```cpp
auto frames = std::array<local_frame, 16>{};  // 16 = MAX_FOR_DEPTH
auto frame_count = std::size_t{0};
// push_back の代わりに frames[frame_count++] = local_frame{...}
// pop_back の代わりに --frame_count
```

**注**: `value_ptr` と `value_type` だけではテンプレート内の `{{ x.name }}` のような
パス式を完全解決できないため、`local_frame` に追加で
**中間値の typed 保存領域**を持たせる必要がある:
```cpp
struct local_frame {
  std::string_view name;
  void const* value_ptr{nullptr};
  std::type_info const* value_type{nullptr};
  // 値種別ごとのスロット
  std::variant<std::monostate, std::int64_t, double, std::string, std::string_view> value{};
  loop_state loop{};
  bool is_for_value{false};
  bool is_for_key{false};
};
```

`is_for_value` / `is_for_key` で for ループの反復要素を区別し、
`value` 経由で string_view / int64 / double を直接保持。
`string_view` は for 反復の要素（コンテキストの所有）に紐づくため安全。

`for` ノード処理の該当部分も合わせて更新:
```cpp
for (auto idx = std::size_t{0}; idx < arr.size(); ++idx) {
  auto& frame = frames[frame_count++];
  frame = local_frame{};
  frame.loop.index = static_cast<std::int64_t>(idx);
  frame.loop.index1 = static_cast<std::int64_t>(idx + 1);
  frame.loop.is_first = idx == 0;
  frame.loop.is_last = idx + 1 == arr.size();
  frame.is_for_value = true;
  // arr[idx] の値型に応じて frame.value に設定
  if constexpr (std::is_same_v<value_type, std::string>) {
    frame.value = std::string_view{arr[idx]};
  } else if constexpr (std::is_same_v<value_type, std::int64_t>) {
    frame.value = arr[idx];
  } else if constexpr (std::is_same_v<value_type, double>) {
    frame.value = arr[idx];
  } else {
    // 構造体型の場合は value_ptr + value_type
    frame.value_ptr = &arr[idx];
    frame.value_type = &typeid(value_type);
  }
  if (has_key) {
    auto& key_frame_slot = frames[frame_count++];
    // キーフレーム設定
  }
  self(body_begin, body_end);
  frame_count -= (has_key ? 2 : 1);
}
```

**テスト追加** (`test/test_inja_hotpath.cpp`):
```cpp
TEST_CASE("local_frame: string_view element in for loop", "[inja_hotpath]") {
  std::vector<std::string> items = {"alpha", "beta", "gamma"};
  struct str_ctx { std::vector<std::string> items; };
  template <> struct glz::meta<str_ctx> { using T = str_ctx; static constexpr auto value = object("items", &T::items); };
  str_ctx ctx{.items = items};
  constexpr auto tmpl = "{% for x in items %}{{ x }};{% endfor %}"_fs;
  auto out = render<tmpl>(ctx);
  REQUIRE(out == "alpha;beta;gamma;");
}
```

**コミット**:
```bash
git add include/frozenchars/inja/types.hpp include/frozenchars/inja/engine_impl.hpp test/test_inja_hotpath.cpp
git commit -m "refactor(inja): redesign local_frame to use std::array and value_view"
```

- [ ] **Step 5: `inja_value.hpp`, `inja_function.hpp` を削除**

```bash
git rm include/frozenchars/inja_value.hpp
git rm include/frozenchars/inja_function.hpp
```

- [ ] **Step 6: ベンチ・コアテストがビルド・緑になることを確認**

Run: `cmake --build build -j 4 && ctest --test-dir build -R "test_inja_types|test_inja_access|test_inja_hotpath|bench_inja_runtime"`
Expected: PASS

- [ ] **Step 7: コミット**

```bash
git add -A
git commit -m "BREAKING(inja): remove inja_value type, inja_object, inja_array

inja_value / inja_object / inja_array / make_object / make_array / bfn_*
を完全削除。glaze 反射型コンテキスト + temp_value 関数 API に移行。
inja_function.hpp 廃止（関数ロジックは engine_impl.hpp にインライン）。"
```

### Task 11: `add_function` シグネチャ変更

**Files:**
- Modify: `include/frozenchars/inja/api.hpp`
- Modify: `include/frozenchars/inja/types.hpp`

- [ ] **Step 1: テスト追加（カスタム関数）**

`test/test_inja_hotpath.cpp` に追加:
```cpp
TEST_CASE("add_function: temp_value signature", "[inja_hotpath]") {
  hot_ctx ctx{.name = "alpha", .value = 42};
  auto opts = runtime_options{};
  opts.add_function("triple", [](std::span<temp_value const> args) -> temp_value {
    if (args.size() != 1) throw render_error{"triple expects 1"};
    return temp_value{std::get<std::int64_t>(args[0].storage) * 3};
  });
  constexpr auto tmpl = "{{ triple(value) }}"_fs;
  auto out = render<tmpl>(ctx, std::cref(opts));
  REQUIRE(out == "126");
}
```

- [ ] **Step 2: テスト失敗確認**

Run: `cmake --build build --target all_test -j 4`
Expected: コンパイルエラー（`add_function` 旧シグネチャ）

- [ ] **Step 3: `add_function` シグネチャを `temp_value` ベースに変更**

`include/frozenchars/inja/types.hpp`:
```cpp
using function_callback = std::function<temp_value(std::span<temp_value const>)>;

struct runtime_options {
  std::unordered_map<std::string, function_callback, transparent_string_hash, std::equal_to<>> function_call{};
  // ... 他のフィールド ...

  auto add_function(std::string name, function_callback callback) -> void {
    function_call.insert_or_assign(std::move(name), std::move(callback));
  }
};
```

- [ ] **Step 4: builtin 関数を `temp_value` ベースに再実装**

`include/frozenchars/inja_builtins.hpp` を新規作成し、全 builtin を新シグネチャで定義:
```cpp
#pragma once
#include "inja_types.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace frozenchars::inja::builtins {

inline auto upper(std::span<temp_value const> a) -> temp_value {
  if (a.size() != 1) throw render_error{"upper() expects 1 argument"};
  if (auto* p = std::get_if<std::string>(&a[0].storage)) {
    auto result = std::string{*p};
    for (auto& c : result) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return temp_value{std::move(result)};
  }
  throw render_error{"upper() expects string argument"};
}

// ... 他の builtin 関数も同様 ...
}  // namespace frozenchars::inja::builtins
```

- [ ] **Step 5: テスト緑確認**

Run: `cmake --build build --target all_test -j 4 && ctest --test-dir build -R test_inja_hotpath`
Expected: PASS

- [ ] **Step 6: コミット**

```bash
git add include/frozenchars/inja/api.hpp include/frozenchars/inja/types.hpp include/frozenchars/inja_builtins.hpp test/test_inja_hotpath.cpp
git commit -m "feat(inja): migrate add_function to temp_value signature"
```

---

## Milestone 7: テスト・ドキュメント更新

### Task 12: 既存テンプレートテストの全面書き換え

**Files:**
- Rewrite: `test/test_template_vm.cpp`（旧 `test_template_vm.cpp.old` から作り直し）
- Rewrite: `test/test_template_functions.cpp`
- Rewrite: `test/test_type_functions.cpp`
- Rewrite: `test/benchmark_inja_runtime.cpp`

- [ ] **Step 1: 書き換え方針を確認**

書き換えルール:
- `inja_value` → 構造体 or `glz::generic`
- `make_object({...})` → 構造体初期化
- `add_function("name", [](inja_value args))` → `add_function("name", [](std::span<temp_value const> args) -> temp_value {...})`
- `as_int(v)`, `as_string(v)` → `std::get<std::int64_t>(v.storage)`, `std::get<std::string>(v.storage)`

- [ ] **Step 2: `test_template_vm.cpp` を書き換え**

`include/frozenchars/inja/types.hpp` および `inja_engine.hpp` をインクルードし、
glaze 反射型コンテキストを使ったテストに置換。`test_template_vm.cpp.old` を参考に。

- [ ] **Step 3: `test_template_functions.cpp` を書き換え`

カスタム関数テストを `temp_value` シグネチャに。

- [ ] **Step 4: `test_type_functions.cpp` を書き換え`

型変換関数のテストを新シグネチャに。

- [ ] **Step 5: ベンチマーク更新`

`benchmark_inja_runtime.cpp` の 3 ケースを glaze 反射型コンテキストに。
`common_items` 等を `std::vector<item_context>` 形式に。

- [ ] **Step 6: 全テスト緑確認**

Run: `cmake --build build -j 4 && ctest --test-dir build`
Expected: PASS（ベンチも緑）

- [ ] **Step 7: ベンチ結果を確認**

Run: `./build/test/bench_inja_runtime 50000`
Expected:
- lookup-heavy: 3,000-4,000 ns/iter（目標達成）
- numeric-literal: 同等の改善
- control-flow+include: 同等の改善

- [ ] **Step 8: コミット**

```bash
git add test/test_template_vm.cpp test/test_template_functions.cpp test/test_type_functions.cpp test/benchmark_inja_runtime.cpp test/CMakeLists.txt
git commit -m "test(inja): migrate template tests to glaze-reflectable contexts"
```

### Task 13: README と AGENTS.md を更新

**Files:**
- Modify: `README.md`
- Modify: `AGENTS.md`

- [ ] **Step 1: README.md の inja 節を全面書き換え`

「inja_value は削除されました。glaze 反射型コンテキストまたは glz::generic を使ってください」と明記。
サンプルコードを新 API に。

- [ ] **Step 2: AGENTS.md の inja 節に破壊的変更を記載`

- `inja_value` 型、`inja_object` 型、`inja_array` 型は v0 で削除
- 移行: `make_object({...})` → 構造体、`add_function` シグネチャ変更
- 旧 API ユーザーには `glz::generic` への移行を案内

- [ ] **Step 3: コミット`

```bash
git add README.md AGENTS.md
git commit -m "docs: document inja type-erasure redesign and migration guide"
```

### Task 14: `compile_fail` テスト更新

**Files:**
- Modify: `test/compile_fail/*` の `EXPECTED_TEXT`

- [ ] **Step 1: 新エラーメッセージを確認`

新 `accessor` の `throw "key not found"` 等のメッセージを確認:
```bash
grep -rn 'throw "' include/frozenchars/inja/ | head -20
```

- [ ] **Step 2: 各 `EXPECTED_TEXT` を新メッセージに合わせて更新`

エラーメッセージの文言変更があった場合、関連 `compile_fail` テストの
`EXPECTED_TEXT` を更新（grep で部分一致検索されるため文言は安定的に）。

- [ ] **Step 3: `compile_fail` テスト緑確認`

Run: `ctest --test-dir build -R compile_fail`
Expected: PASS

- [ ] **Step 4: コミット`

```bash
git add test/compile_fail/
git commit -m "test(compile_fail): update EXPECTED_TEXT for new error messages"
```

---

## 最終検証

### Task 15: 全テスト・ベンチの最終確認

- [ ] **Step 1: フルビルド + 全テスト**

Run: `cmake --build build -j 4 && ctest --test-dir build --output-on-failure`
Expected: ALL TESTS PASS

- [ ] **Step 2: ベンチで目標達成を確認**

Run: `./build/test/bench_inja_runtime 50000`
Expected:
- lookup-heavy: 3,000-4,000 ns/iter（目標達成）
- ベースライン 12,500 ns/iter から **3-4x 改善**

- [ ] **Step 3: コードフォーマット確認**

Run: `clang-format -i include/frozenchars/inja/*.hpp include/frozenchars/inja_types.hpp include/frozenchars/inja_access.hpp include/frozenchars/inja_builtins.hpp test/test_inja*.cpp test/benchmark_inja_runtime.cpp`
Expected: フォーマット適用

- [ ] **Step 4: 最終コミット**

```bash
git add -A
git commit -m "chore: finalize inja type-erasure migration"
```

---

## リスク対応

### リスク: コンパイル時間が爆発

**対応**: 実装途中で `time cmake --build build` で測定。
16 セグメント深さ制限を守り、`accessor` の再帰深度を制御。

### リスク: 型エラーが分かりにくい

**対応**: `accessor` の `throw "..."` を分かりやすいメッセージに。
例: `throw "inja accessor: key 'xxx' not found in type 'Ctx'"`。

### リスク: ベンチが目標未達

**対応**: フォールバック案を準備:
1. `value_view` の variant サイズ縮小（`std::variant` → tagged union）
2. `accessor` のキャッシュ（`static thread_local`）
3. SIMD 化（文字列マッチ）
