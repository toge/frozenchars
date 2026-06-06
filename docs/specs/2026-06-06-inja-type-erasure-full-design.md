# frozenchars::inja フル型消去化 設計書

## 概要

`frozenchars::inja` のレンダリング性能を `glz::stencil` レベルまで引き上げるため、
`inja_value` 動的型システムを完全廃止し、コンパイル時に解決済みの型付きアクセサで
レンダリングパイプラインを再設計する。

### 動機

- 現状、`inja_value` への boxing（`std::string` のヒープコピー、`std::variant` 構築）が
  レンダリングコストの大半を占める
- `split_variable_path` のランタイム呼び出し、`lookup_in_reflectable` の O(N) 線形探索、
  `local_frame` の `std::vector` push/pop など、ホットパスに多くの無駄がある
- `glz::stencil` は同じユースケースで 5-10 倍速いため、アーキテクチャレベルで追いつく

### 目標

- `lookup-heavy` ベンチ（glaze 反射型コンテキスト）で **現状 12,500 ns/iter → 3,000-4,000 ns/iter**
- `inja_value` 型を完全削除（ユーザーコード含む）
- 後方互換性は **完全に捨てる**（破壊的変更を許容）

### 非目標

- 動的オブジェクト（実行時にキー追加）のサポート
- 既存テストの維持（全面書き換え）
- 段階的移行パス（一度で全面置換）

## アーキテクチャ

3 層構造:

```
┌─────────────────────────────────────────┐
│  consteval 層: parse_program<src>()     │
│  テンプレート文字列 → 拡張バイトコード      │
│  (パス式を consteval でセグメント列に分解) │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  template 層: render<src>(Ctx const&)   │
│  Ctx 型のメタを見てアクセサを specialize  │
│  パス → コンパイル時インデックス列        │
└─────────────────────────────────────────┘
                  ↓
┌─────────────────────────────────────────┐
│  runtime 層: バイトコード walk + 直接    │
│  出力バッファに append (string_view)     │
│  関数境界のみ temp_value を経由          │
└─────────────────────────────────────────┘
```

評価器（`expr_parser`）は **「式を評価して出力に直接書き込む」** ストリーミング型に再設計。
`inja_value` を介さない。

## コンポーネント

### 1. `value_view` — 非所有値ビュー（ホットパス専用）

```cpp
struct value_view {
  std::variant<
    std::monostate,
    bool,
    std::int64_t,
    double,
    std::string_view,    // 文字列は所有しない
    array_view,           // std::span<value_view const>
    object_view           // 型消去キー → value_view
  > storage;
};
```

文字列は `std::string_view` で参照のみ。葉を覗くためだけの一時オブジェクト。

### 2. `temp_value` — 関数境界専用（限定的に boxing）

```cpp
struct temp_value {
  std::variant<
    std::monostate, bool, std::int64_t, double,
    std::string,                     // 関数戻り値用 (所有)
    std::vector<temp_value>,          // 配列
    std::map<std::string, temp_value> // オブジェクト
  > storage;
};

using function_callback = std::function<temp_value(std::span<temp_value const>)>;
```

`inja_value` との差分:
- 文字列は `std::string` 所有（関数戻り値がコンテキストライフタイムに依存できないため）
- `inja_array` の variant 4 種（`vector<inja_value>` / `vector<int64>` / `vector<double>` / `vector<string>`）が 1 種に統合
- `inja_object` の `std::unordered_map` を `std::map` に（順序安定、ベンチではアロケータ特性有利）

### 3. `accessor<T, Segments>` — パス → 値アクセサ

```cpp
template <typename T, std::size_t N>
struct accessor;  // Segments は type-level list (template parameter pack) で持つ

// accessor<lookup_context, "user", "profile", "city">::resolve(ctx)
//   → const std::string&
// accessor<lookup_context, "items">::resolve(ctx)
//   → const std::vector<item_context>&
```

**NTTP 表現の選択**: `std::array<std::string_view, N>` を NTTP に使うと
C++20 の class-type NTTP が必要で配列の比較が非自前。代わりに
**可変長テンプレートパラメータパック**で各セグメントを NTTP として渡す
（既存の `fn<"name", ...>` パターンと統一）。

各セグメントは `if constexpr` で `glz::reflect<T>::keys` を線形探索して
`std::size_t` index に確定。`glz::get<index>(glz::to_tie(value))` で次レベルへ。
**最後が string なら string_view を返す**。

### 4. `bytecode` 拡張

`node` 構造体に追加:

```cpp
enum class expr_kind : std::uint8_t { literal, simple_path, complex_expr };

struct precomputed_path {
  std::array<std::string_view, 16> segments{};
  std::uint8_t count{};
};

struct node {
  node_kind kind;
  // ... 既存フィールド ...
  expr_kind expr_kind{literal};   // 既存 expr_is_simple_path を置換
  precomputed_path path{};        // 新規
  // 新規: リテラル値 (整数/浮動小数/文字列/真偽値/null)
  // variant で持つ
  std::variant<
    std::monostate,
    std::int64_t,
    double,
    std::string_view,
    bool
  > literal{};
};
```

`expr_is_simple_path` フラグは `expr_kind::simple_path` に統合。

### 5. 関数ディスパッチの新シグネチャ

```cpp
using function_callback = std::function<temp_value(std::span<temp_value const>)>;
```

builtin 関数は全て新シグネチャでラップ。例:

```cpp
inline auto bfn_upper(std::span<temp_value const> args) -> temp_value {
  if (args.size() != 1) {
    throw render_error{"upper() expects 1 argument"};
  }
  if (auto* p = std::get_if<std::string>(&args[0].storage)) {
    return temp_value{fn_upper(std::string_view{*p})};
  }
  throw render_error{"upper() expects string argument"};
}
```

`function_list<fn<"upper", bfn_upper>, ...>` の `dispatch` メソッドは
`std::span<temp_value const>` を新シグネチャとして扱う。

### 6. `local_frame` の再設計

```cpp
struct local_frame {
  std::string_view name;          // string_view (テンプレート文字列を参照)
  void const* value_ptr;          // 現在の要素へのポインタ
  std::type_info const* value_type;
  // ... loop state ...
};
```

`std::string name` を `std::string_view` に。`inja_value value` を `void const*` ポインタに。
`std::vector<local_frame>` を **固定サイズ `std::array<local_frame, 16>`** に変更
（`max_for_depth` で上限を把握、16 は緩めの安全マージン）。

### 7. 削除する型・API

以下を **完全削除**:

- `inja_value` 構造体
- `inja_null`, `inja_array`, `inja_object`
- `make_array`, `make_object`, `array`, `object`
- `as_int`, `as_string`, `as_bool`, `as_double`, `as_array`, `as_object`, `is_null`
- `truthy`, `is_empty_value`, `values_equal`
- `to_string`
- `bfn_upper`, `bfn_lower`, `bfn_capitalize`, `bfn_replace`, `bfn_length`, `bfn_first`,
  `bfn_last`, `bfn_join`, `bfn_sort`, `bfn_range`, `bfn_abs`, `bfn_round`, `bfn_max`,
  `bfn_min`, `bfn_even`, `bfn_odd`, `bfn_divisibleBy`, `bfn_int`, `bfn_float`,
  `bfn_isString`, `bfn_isArray`, `bfn_isNumber`, `bfn_isObject`, `bfn_isBoolean`,
  `bfn_isFloat`, `bfn_isInteger`, `bfn_isNone`, `bfn_isEmpty`, `bfn_default`, `bfn_at`,
  `bfn_exists`, `bfn_existsIn`, `bfn_as_int_array`, `bfn_as_double_array`,
  `bfn_as_string_array`
- `try_to_inja_value`, `to_inja_value_or_throw`
- `convert_map_like`, `convert_range_like`, `convert_reflectable`
- `lookup_in_object_root`, `lookup_in_inja_value`
- `lookup_in_typed_value`, `lookup_in_reflectable`, `lookup_in_typed_root`
- `lookup_typed_object_view_root`, `typed_object_view`
- `equals_value`, `less_value`, `try_as_double`
- `runtime_options::add_function(std::function<inja_value(...)>)` 旧シグネチャ
- `inja_function.hpp` 全体（`fn_upper`, `fn_lower` 等の全自由関数）

`bfn_*` のうちbuiltin 関数の本体は `engine_impl.hpp` 内に `temp_value` ベースで
インライン実装。`inja_function.hpp` 自体を廃止し、関数ロジックは
`engine_impl.hpp` または新規 `inja_builtins.hpp` に集約する。

## データフロー

### パス式: `{{ user.profile.city }}`

```
[consteval] parse_program<src>():
  node { kind=expr, expr_kind=simple_path, path.segments=["user","profile","city"] }

[template instantiation] render<src>(ctx):
  accessor_chain<lookup_context, ["user","profile","city"]> を specialize
    - if constexpr: "user" → glz::reflect<lookup_context>::keys 線形探索 → 0
    - if constexpr: "profile" → glz::reflect<user_context>::keys → 0
    - if constexpr: "city" → glz::reflect<profile_context>::keys → 0

[runtime] render loop:
  case node_kind::expr:
    if (node.expr_kind == simple_path):
      // accessor は const std::string& を返す特殊化
      out.append(accessor<...>::resolve(ctx));
      // 関数呼び出し 1 + glz::get 3 + string::append 1
      // アロケーション 0
```

### 数値式: `{{ item.value * 2 }}`

```
- item.value: accessor 経由で const std::int64_t& への参照
- 2: consteval で literal int64 = 2
- 評価: 中間値 typed (std::int64_t a, std::int64_t b)
- 計算: a * b → std::int64_t
- append: std::to_chars → string_view → out.append
```

**変更点**: 中間値は typed ローカル変数として保持。`temp_value` にするのは関数境界のみ。

### 関数呼び出し: `{{ upper(x.name) }}`

```
- x.name: accessor 経由で const std::string& → string_view 化
- temp_value{string_view} 構築（引数用）
- upper(args) 呼び出し:
    temp_value bfn_upper(std::span<temp_value const> args) {
      // args[0] = temp_value{string_view{"alpha"}}
      // → std::string{"ALPHA"} を作って返す
      return temp_value{fn_upper(std::get<std::string_view>(args[0].storage))};
    }
- 戻り値 temp_value から std::string 取り出し → out.append
```

### for ループ: `{% for item in items %}{{ item.name }}{% endfor %}`

```
- items: accessor<Ctx, ["items"]>::resolve(ctx)
    → const std::vector<item_context>&
- 要素型: decltype(items)::value_type = item_context (consteval で判明)
- 反復: 要素 item_context& ごとに frame push
  - frame.name = "item" (string_view, テンプレート文字列)
  - frame.value_ptr = &element
  - frame.value_type = &typeid(item_context)
- body 評価:
  - item.name: accessor<item_context, ["name"]>::resolve(element)
    → const std::string& → out.append
```

`local_frame` の `name` は `std::string_view` なので push/pop はポインタと view のコピーのみ。

## 動的コンテキスト対応

**`inja_value` / `inja_object` 廃止後の動的オブジェクト**:

- **第一選択**: `glz::generic`（glaze 同梱の動的 JSON 風オブジェクト）。
  `glz::meta<glz::generic>::keys` 経由でフィールドアクセス可能。
- **第二選択**: ユーザー定義 `std::map<std::string, T>` に `glz::meta` を特殊化。
  ベンチ用途で `std::map` の `std::unordered_map` 比オーバーヘッドは許容。
- **非サポート**: 型が `glaze_reflectable` でも `glz::generic` でもない場合は
  テンプレートインスタンス化時に `static_assert` でコンパイルエラー。
  エラーメッセージで「`inja_value` は v1 で削除されました。glaze 反射型または
  `glz::generic` を使ってください」と案内。

## エラー処理

- **Parse エラー**: consteval `throw "..."` 文字列リテラル（既存パターン踏襲）
- **Render エラー**: `render_error` 例外（`std::runtime_error` 派生）
  - 「未定義パス: `user.profile.city`」
  - 「型不一致: expected int64, got string」
  - 「関数の引数数: `upper` expects 1 argument」
- **関数ユーザーエラー**: ユーザー関数が投げた例外をそのまま伝播

## テスト

### 既存テストの移行

全面書き換え:
- `test/test_template_vm.cpp`
- `test/test_template_functions.cpp`
- `test/test_type_functions.cpp`
- `test/test_environment_constants.cpp`
- `test/test_parser.cpp`（パーサ部分）
- `test/benchmark_inja_runtime.cpp`（ベンチ）

`inja_value` → 構造体 or `glz::generic`。`make_object({...})` → 構造体初期化。
`add_function` シグネチャ変更。

### 新規テスト

- `test/test_template_direct_append.cpp`:
  `{{ path }}` が出力に 1 回の `append` で反映される動作のドキュメント的テスト
  （実装詳細を直接テストせず、出力内容の正しさで検証）
- `test/test_template_path_pre_resolution.cpp`:
  パスセグメントが `consteval` でトークン化されることのテスト
  （consteval 関数経由で確認）
- `test/test_template_glaze_fast_path.cpp`:
  構造体コンテキストでベンチが想定範囲内に収まる検証
- `test/compile_fail/`: パスが存在しない型で `static_assert` エラーになるケース
  - 旧 `EXPECTED_TEXT` ファイルを全面更新

### ベンチマーク

- 既存 `benchmark_inja_runtime.cpp` の 3 ケースを維持
- 目標: 現状の **1/3 以下** の ns/iter
- 追加で `glaze-typed` 全ケースの測定

## リスク評価

| リスク | 影響 | 対策 |
|--------|------|------|
| ヘッダオンリーのコンパイル時間増大 | 中 | 段階移行で都度計測 |
| `glz::get<idx>` の deep template instantiation | 低 | `if constexpr` 連鎖でインライン化 |
| 既存ユーザーコード全壊 | 大 | `AGENTS.md` / `README.md` に大きな変更点明記。v0 リリース扱い |
| `temp_value` の boxing が予想より重い | 中 | ベンチで計測、必要なら typed dispatch に再設計 |
| for ループの `void const*` キャストで型安全性低下 | 低 | `std::type_info` で型チェック |
| `std::map<std::string, temp_value>` の `std::map` 選択 | 中 | ベンチで `std::unordered_map` と比較 |

## 実装順

1. `value_view`, `temp_value` 型定義（`inja_value.hpp` 廃止、新規 `inja_types.hpp` 作成）
2. `accessor<T, Segments>` テンプレート実装
3. `bytecode` 拡張（`expr_kind`, `precomputed_path`, `literal` フィールド追加）
4. `parse_program` のパス事前トークン化
5. `expr_parser` を出力ストリーミング型に再設計
6. `render` ループのホットパス書き換え
7. `local_frame` の再設計（`std::array` 化、`string_view` 化）
8. builtin 関数の新シグネチャへの移行
9. 公開 API の `inja_value` 削除（`inja_value.hpp`, `inja_function.hpp` 廃止）
10. テスト・ベンチ更新
11. README, AGENTS.md 更新
12. `compile_fail` テスト更新

## 検証

- 各実装ステップでビルド確認（`bash build.sh`）
- ベンチで段階的に性能向上を確認
- 全テスト緑で完了
