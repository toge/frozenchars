# JSON 往復変換（decompress / uncrush）実装計画

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** `compress` / `crush` の復元 API（`decompress` / `uncrush`）を追加し、圧縮→復元の往復を保証する。

**Architecture:** 既存の「全体を走らせて実長を得る」consteval パターンに従う。core 実装は std::basic_string を返すヘルパーにし、サイズ計算版と FrozenString コピー版の2層にする。compress 出力を有効な JSON に変更し、既存パーサでそのまま読めるようにする。

**Tech Stack:** C++23 ヘッダオンリー、Catch2 v3、CTest。

## Global Constraints

- ヘッダオンリー維持（`include/` に `.cpp` を置かない）
- 命名: クラス・変数 = `lower_case`、関数 = `camelBack`、定数 = `UPPER_CASE`（`.clang-tidy`）
- フォーマット: LLVM ベース、2 スペース、200 列（`.clang-format`）
- コメントは日本語 Doxygen
- `_fs` リテラルの戻り値型は `FrozenString<文字数+1>`（終端 '\0' を含む）
- ビルド: `bash build.sh` → テスト: `bash test.sh`。単体テストは `./build/test/all_test "[json]"`

---

### Task 1: 往復テストを先に書く（TDD の失敗確認）

**Files:**
- Modify: `test/test_json_crush.cpp`
- Modify: `test/test_json_compress.cpp`

**Interfaces:**
- Produces: このタスクは `frozenchars::json::uncrush` / `frozenchars::json::decompress` の**意図する API**を定める（未実装なのでコンパイルエラーになる）

- [ ] **Step 1: crush 往復テストを書く**

`test/test_json_crush.cpp` に追加:

```cpp
TEST_CASE("uncrush roundtrip", "[json][crush]") {
  // シンプルなオブジェクト
  static constexpr auto rt1 = frozenchars::json::uncrush<
    frozenchars::json::crush<R"({"a":"x"})"_fs>()>();
  STATIC_REQUIRE(rt1.sv() == R"({"a":"x"})");

  // 繰り返し値（置換が発生する）
  static constexpr auto rt2 = frozenchars::json::uncrush<
    frozenchars::json::crush<R"({"a":"value","b":"value","c":"value","d":"value"})"_fs>()>();
  STATIC_REQUIRE(rt2.sv() == R"({"a":"value","b":"value","c":"value","d":"value"})");

  // "ab" の繰り返し（辞書破壊の回帰ケース）
  static constexpr auto rt3 = frozenchars::json::uncrush<
    frozenchars::json::crush<R"({"k":"abababab"})"_fs>()>();
  STATIC_REQUIRE(rt3.sv() == R"({"k":"abababab"})");

  // 空オブジェクト
  static constexpr auto rt4 = frozenchars::json::uncrush<
    frozenchars::json::crush<R"({})"_fs>()>();
  STATIC_REQUIRE(rt4.sv() == R"({})");

  // 配列
  static constexpr auto rt5 = frozenchars::json::uncrush<
    frozenchars::json::crush<R"([1,2,3,1,2,3])"_fs>()>();
  STATIC_REQUIRE(rt5.sv() == R"([1,2,3,1,2,3])");

  // サロゲートペア
  static constexpr auto rt6 = frozenchars::json::uncrush<
    frozenchars::json::crush<R"({"a":"😀","b":"😀"})"_fs>()>();
  STATIC_REQUIRE(rt6.sv() == R"({"a":"😀","b":"😀"})");
}
```

- [ ] **Step 2: compress 往復テストを書く**

`test/test_json_compress.cpp` に追加:

```cpp
TEST_CASE("decompress roundtrip", "[json][compress]") {
  // 意味的に同一（空白は正規化される）
  static constexpr auto rt1 = frozenchars::json::decompress<
    frozenchars::json::compress<R"({"a":"value"})"_fs>()>();
  STATIC_REQUIRE(rt1.sv() == R"({"a":"value"})");

  // 小数が失われない
  static constexpr auto rt2 = frozenchars::json::decompress<
    frozenchars::json::compress<R"({"x":1.5})"_fs>()>();
  STATIC_REQUIRE(rt2.sv() == R"({"x":1.5})");

  // エスケープされた引用符が壊れない
  static constexpr auto rt3 = frozenchars::json::decompress<
    frozenchars::json::compress<R"({"a":"he\"llo"})"_fs>()>();
  STATIC_REQUIRE(rt3.sv() == R"({"a":"he\"llo"})");

  // 空配列
  static constexpr auto rt4 = frozenchars::json::decompress<
    frozenchars::json::compress<R"([])"_fs>()>();
  STATIC_REQUIRE(rt4.sv() == R"([])");

  // ネスト
  static constexpr auto rt5 = frozenchars::json::decompress<
    frozenchars::json::compress<R"([{"name":"item","val":1},{"name":"item","val":1}])"_fs>()>();
  STATIC_REQUIRE(rt5.sv() == R"([{"name":"item","val":1},{"name":"item","val":1}])");
}

TEST_CASE("compress output is valid json", "[json][compress]") {
  constexpr auto compressed = frozenchars::json::compress<
    R"([{"name":"item","val":1},{"name":"item","val":1}])"_fs>();
  CHECK(frozenchars::json::validate_json(compressed.sv()));
}
```

- [ ] **Step 3: ビルドして失敗を確認**

Run: `cd build && cmake --build . --target all_test -j`
Expected: FAIL（`uncrush` / `decompress` が未定義）

- [ ] **Step 4: コミット**

```bash
git add test/test_json_crush.cpp test/test_json_compress.cpp
git commit -m "test: uncrush/decompress の往復テストを追加（現状コンパイルエラー）"
```

---

### Task 2: crush の辞書領域をデータ領域限定に修正

**Files:**
- Modify: `include/frozenchars/json/detail/crush_detail.hpp`

**Interfaces:**
- Consumes: なし（既存実装の修正）
- Produces: `build_initial_candidates` / `count_candidates` / `replace_all_with_char` に `size_t data_end` パラメータが追加される

- [ ] **Step 1: 置換ヘルパーにデータ領域限定を追加**

`replace_all_with_char` を置き換える:

```cpp
template <typename CharT>
[[nodiscard]] constexpr auto replace_all_with_char(
    std::basic_string<CharT> str,
    std::basic_string_view<CharT> const from,
    CharT const to,
    size_t const data_end) -> std::basic_string<CharT> {
  size_t pos = 0;
  while ((pos = str.find(from, pos)) != std::basic_string<CharT>::npos && pos < data_end) {
    str.replace(pos, from.size(), 1, to);
    data_end -= from.size() - 1;
    pos += 1;
  }
  return str;
}
```

- [ ] **Step 2: 候補構築をデータ領域限定に**

`build_initial_candidates` に `size_t const data_end` を追加し、2重ループの上限を変更:

```cpp
template <typename CharT>
[[nodiscard]] constexpr auto build_initial_candidates(
    std::basic_string_view<CharT> const string, int64_t const max_len = 50, size_t const data_end = std::basic_string_view<CharT>::npos) {
  ...
  auto const n = data_end == std::basic_string_view<CharT>::npos ? string.size() : data_end;
  ...
}
```

（ループは既存のまま `n` を上限に使う）

- [ ] **Step 3: 出現回数カウントをデータ領域限定に**

`count_candidates` に `size_t const data_end` を追加:

```cpp
template <typename CharT>
constexpr void count_candidates(
    std::basic_string_view<CharT> const str,
    std::vector<OrderedCandidate<CharT>>& candidates,
    size_t const data_end) {
  for (auto& c : candidates) {
    c.count = 0;
    size_t pos = 0;
    std::basic_string_view<CharT> cv{c.value.data(), c.value.size()};
    while ((pos = str.find(cv, pos)) != std::basic_string_view<CharT>::npos && pos + cv.size() <= data_end) {
      ++c.count;
      pos += c.value.size();
    }
  }
}
```

- [ ] **Step 4: メインループでデータ領域を追跡**

`js_crush_utf16` の while ループを書き換える:

```cpp
template <typename CharT>
[[nodiscard]] constexpr auto js_crush_utf16(
    std::basic_string<CharT> string, int64_t const max_len = 50) -> JSCrushResult<CharT> {
  std::basic_string<CharT> split_string;
  size_t data_end = string.size();  // データ領域の終端（辞書開始位置）
  int replace_pos = static_cast<int>(replacement_characters_utf16.size());

  while (true) {
    // 現在の文字列（辞書含む）に出現する文字を記録し、未使用の置換文字を選ぶ
    std::bitset<65536> present;
    for (auto c : string) {
      if (static_cast<uint16_t>(c) < 65536) present.set(static_cast<uint16_t>(c));
    }
    CharT replace_char = 0;
    while (replace_pos > 0) {
      auto const c = replacement_characters_utf16[--replace_pos];
      if (!present.test(static_cast<uint16_t>(c))) { replace_char = static_cast<CharT>(c); break; }
    }
    if (replace_char == 0) break;

    // 候補はデータ領域のみから構築・カウントする
    auto candidates = build_initial_candidates<CharT>(string, max_len, data_end);
    count_candidates<CharT>(string, candidates, data_end);

    int64_t rep_len = 1;
    int64_t delim_len = 1;
    size_t best_idx = 0;
    int64_t best_delta = 0;
    auto it = candidates.begin();
    while (it != candidates.end()) {
      int64_t delta = (it->count - 1) * static_cast<int64_t>(it->value.size()) - (it->count + 1) * rep_len;
      if (split_string.empty()) delta -= delim_len;
      if (delta <= 0) {
        it = candidates.erase(it);
      } else {
        if (delta > best_delta) { best_delta = delta; best_idx = std::distance(candidates.begin(), it); }
        ++it;
      }
    }
    if (best_delta <= 0 || candidates.empty()) break;

    // 置換はデータ領域のみに適用する（辞書は不変）
    auto const& best_sub = candidates[best_idx].value;
    auto replaced = replace_all_with_char<CharT>(string, best_sub, replace_char, data_end);
    string = std::move(replaced);
    // 置換でデータ領域が縮んだ分を補正してから辞書を追記する
    data_end -= best_sub.size() - 1;
    string.resize(data_end);
    string.push_back(replace_char);
    string.append(best_sub);
    data_end = string.size();
    split_string.insert(split_string.begin(), replace_char);
  }
  return {std::move(string), std::move(split_string)};
}
```

- [ ] **Step 5: ビルドと既存テストを確認**

Run: `cd build && cmake --build . --target all_test -j && ./build/test/all_test "[json][crush]"`
Expected: PASS（既存テストは出力形式を変えないため）

- [ ] **Step 6: コミット**

```bash
git add include/frozenchars/json/detail/crush_detail.hpp
git commit -m "fix: crush の置換をデータ領域限定にし辞書を不変化"
```

---

### Task 3: compress 出力を有効な JSON に変更

**Files:**
- Modify: `include/frozenchars/json/detail/compress_detail.hpp`
- Modify: `test/test_json_compress.cpp`

**Interfaces:**
- Consumes: なし
- Produces: `value_to_string` が JSON リテラルの原文を返すようになる。`compress_object` のキーがクォート付き原文になる。

- [ ] **Step 1: value_to_string を原文保持に変更**

```cpp
[[nodiscard]] constexpr auto value_to_string(json_value const& val) -> std::string {
  switch (val.type) {
  case json_type::null: return "null";
  case json_type::boolean: return val.bool_val ? "true" : "false";
  case json_type::number: return std::string(val.str_val);  // 原文（小数・指数を保持）
  case json_type::string: return std::string(val.str_val);  // クォート・エスケープ含む原文
  case json_type::array: {
    std::string r = "[";
    for (size_t i = 0; i < val.arr.size(); ++i) {
      if (i > 0) r += ",";
      r += value_to_string(val.arr[i]);
    }
    r += "]";
    return r;
  }
  case json_type::object: {
    std::string r = "{";
    for (size_t i = 0; i < val.keys.size(); ++i) {
      if (i > 0) r += ",";
      r += std::string(val.keys[i]);
      r += ":";
      r += value_to_string(val.arr[i]);
    }
    r += "}";
    return r;
  }
  }
  return {};
}
```

- [ ] **Step 2: compress_object のキーを原文に**

```cpp
[[nodiscard]] constexpr auto compress_object(CompressMemory& mem, json_value const& val) -> std::string {
  std::string result = "{";
  for (size_t i = 0; i < val.keys.size(); ++i) {
    if (i > 0) result += ",";
    result += std::string(val.keys[i]);  // クォート含む原文
    result += ":";
    result += compress_value(mem, val.arr[i]);
  }
  result += "}";
  return result;
}
```

- [ ] **Step 3: values 配列の出力を原文に**

`compress_to_string` のループを変更:

```cpp
  for (size_t i = 0; i < mem.values.size(); ++i) {
    if (i > 0) result += ",";
    result += mem.values[i];  // 既に有効な JSON リテラル（string はクォート含む）
  }
```

- [ ] **Step 4: root スカラー参照をクォート付きに**

`compress_value` のスカラー分岐で、`get_or_add_value` の戻り値（Base62 参照）をクォートで包む:

```cpp
  case json_type::string: {
    auto const s = value_to_string(val);
    auto const ref = get_or_add_value(mem, s);
    return "\"" + ref + "\"";
  }
```

※ `null` / `boolean` / `number` も同様に `"\"" + ref + "\""` を返す（root 木の中のスカラーは常に参照）。

- [ ] **Step 5: 既存テストのアサーションを修正**

`test_json_compress.cpp` の "compress with repeated values" を修正:

```cpp
TEST_CASE("compress with repeated values", "[json][compress]") {
  constexpr auto compressed = frozenchars::json::compress<
    R"([{"name":"item","val":1},{"name":"item","val":1}])"_fs>();
  auto const sv = compressed.sv();
  CHECK(sv.starts_with("{\"values\""));
  CHECK(sv.find("\"name\"") != std::string_view::npos);
  CHECK(sv.find("\"item\"") != std::string_view::npos);
}
```

- [ ] **Step 6: ビルドとテストを確認**

Run: `cd build && cmake --build . --target all_test -j && ./build/test/all_test "[json]" -a`
Expected: PASS（往復テストはまだ未実装なのでスキップされる）

- [ ] **Step 7: コミット**

```bash
git add include/frozenchars/json/detail/compress_detail.hpp test/test_json_compress.cpp
git commit -m "fix: compress 出力を有効な JSON に変更し数値原文を保持"
```

---

### Task 4: from_base62 と decompress の実装

**Files:**
- Modify: `include/frozenchars/json/detail/compress_detail.hpp`
- Modify: `include/frozenchars/json/compress.hpp`

**Interfaces:**
- Consumes: Task 3 の出力形式（有効な JSON、参照はクォート付き Base62）
- Produces: `frozenchars::json::decompress<Input>()` / `frozenchars::json::detail::decompressed_size<Input>()`

- [ ] **Step 1: from_base62 を追加**

`compress_detail.hpp` の `to_base62` の下に追加:

```cpp
/**
 * @brief Base62 文字列を符号なし整数に変換する
 *
 * @param s 変換する Base62 文字列
 * @return uint64_t 変換結果
 * @throw std::runtime_error 不正な文字を含む場合
 */
[[nodiscard]] constexpr auto from_base62(std::string_view const s) -> uint64_t {
  uint64_t value = 0;
  for (auto const c : s) {
    value *= 62;
    if (c >= '0' && c <= '9') value += static_cast<uint64_t>(c - '0');
    else if (c >= 'A' && c <= 'Z') value += static_cast<uint64_t>(c - 'A' + 10);
    else if (c >= 'a' && c <= 'z') value += static_cast<uint64_t>(c - 'a' + 36);
    else throw std::runtime_error("from_base62: invalid character");
  }
  return value;
}
```

- [ ] **Step 2: decompress のコア実装を追加**

`compress.hpp` の `namespace detail` に追加（既存の `compressed_size` の下）:

```cpp
/**
 * @brief 圧縮済み JSON を元の JSON テキストへ復元する
 *
 * @param input 圧縮済み JSON 文字列
 * @return std::string 復元した JSON テキスト
 */
[[nodiscard]] consteval auto decompress_to_string(std::string_view const input) -> std::string {
  auto const parsed = frozenchars::json::detail::parse_json(input);

  // values 配列（圧縮 JSON のルートは object であること）
  // {"values":[...],"root":...} 形式
  // 簡潔のため、root の再構築はローカルラムダで行う

  // 値テーブルの各要素を JSON リテラルテキストへ復元
  auto literal_of = [](json_value const& v) -> std::string {
    switch (v.type) {
    case json_type::null: return "null";
    case json_type::boolean: return v.bool_val ? "true" : "false";
    case json_type::number: return std::string(v.str_val);
    case json_type::string: return std::string(v.str_val);
    default: throw std::runtime_error("decompress: unexpected value table entry");
    }
  };

  // values 配列を探す
  std::vector<std::string> values;
  std::vector<json_value> const* root_val = nullptr;
  if (parsed.type == json_type::object) {
    for (size_t i = 0; i < parsed.keys.size(); ++i) {
      auto key = parsed.keys[i];
      if (key.size() >= 2 && key.front() == '"' && key.back() == '"') {
        key = key.substr(1, key.size() - 2);
      }
      if (key == "values") {
        for (auto const& e : parsed.arr[i].arr) values.push_back(literal_of(e));
      } else if (key == "root") {
        root_val = &parsed.arr[i];
      }
    }
  }
  if (root_val == nullptr) throw std::runtime_error("decompress: missing root");

  // root を再帰的に再構築。文字列は Base62 参照として values へ解決する
  auto rebuild = [&](auto&& self, json_value const& v) -> std::string {
    switch (v.type) {
    case json_type::object: {
      std::string r = "{";
      for (size_t i = 0; i < v.keys.size(); ++i) {
        if (i > 0) r += ",";
        r += std::string(v.keys[i]);
        r += ":";
        r += self(self, v.arr[i]);
      }
      r += "}";
      return r;
    }
    case json_type::array: {
      std::string r = "[";
      for (size_t i = 0; i < v.arr.size(); ++i) {
        if (i > 0) r += ",";
        r += self(self, v.arr[i]);
      }
      r += "]";
      return r;
    }
    case json_type::string: {
      // Base62 参照（クォート除去）
      auto ref = v.str_val;
      if (ref.size() >= 2 && ref.front() == '"' && ref.back() == '"') {
        ref = ref.substr(1, ref.size() - 2);
      }
      auto const idx = frozenchars::json::detail::from_base62(ref);
      if (idx >= values.size()) throw std::runtime_error("decompress: value index out of range");
      return values[idx];
    }
    default:
      throw std::runtime_error("decompress: unexpected node type");
    }
  };

  return rebuild(rebuild, *root_val);
}
```

- [ ] **Step 3: decompressed_size を追加**

```cpp
template <FrozenString Input>
[[nodiscard]] consteval auto decompressed_size() -> size_t {
  return decompress_to_string(Input.sv()).size() + 1;
}
```

- [ ] **Step 4: 公開 API decompress を追加**

`compress.hpp` の `compress` 関数の下:

```cpp
template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto decompress() -> FrozenString<detail::decompressed_size<Input>()> {
  constexpr auto N = detail::decompressed_size<Input>();
  auto const result = detail::decompress_to_string(Input.sv());
  auto res = FrozenString<N>{};
  for (size_t i = 0; i < result.size() && i < N - 1; ++i) {
    res.buffer[i] = result[i];
  }
  res.buffer[result.size()] = '\0';
  res.length = result.size();
  return res;
}
```

- [ ] **Step 5: ビルドと compress 往復テストを確認**

Run: `cd build && cmake --build . --target all_test -j && ./build/test/all_test "[json][compress]"`
Expected: PASS（decompress roundtrip / compress output is valid json が通る）

- [ ] **Step 6: コミット**

```bash
git add include/frozenchars/json/detail/compress_detail.hpp include/frozenchars/json/compress.hpp
git commit -m "feat: decompress API を追加"
```

---

### Task 5: uncrush の実装

**Files:**
- Modify: `include/frozenchars/json/crush.hpp`

**Interfaces:**
- Consumes: Task 2 のデータ領域限定 crush（辞書不変の出力形式）
- Produces: `frozenchars::json::uncrush<Input>()` / `frozenchars::json::detail::uncrushed_size<Input>()`

- [ ] **Step 1: uncrush のコア実装を追加**

`crush.hpp` の `namespace detail` に追加（`crushed_size` の下）:

```cpp
/**
 * @brief crush 済み文字列を元の UTF-8 JSON テキストへ復元する
 *
 * @param input crush 出力（末尾に '_' を含む）
 * @return std::string 復元した UTF-8 JSON テキスト
 * @throw std::runtime_error 末尾 '_' が無い、またはデリミタが不正な場合
 */
[[nodiscard]] consteval auto uncrush_to_string(std::string_view const input) -> std::string {
  if (input.empty() || input.back() != '_') {
    throw std::runtime_error("uncrush: missing trailing '_'");
  }
  auto const body_utf8 = input.substr(0, input.size() - 1);
  auto u16 = frozenchars::json::detail::utf8_to_utf16(body_utf8);

  // デリミタ U+0001 で body / split に分割
  std::u16string_view body = u16;
  std::u16string_view split;
  if (auto const delim = u16.rfind(frozenchars::json::detail::JSON_CRUSH_DELIMITER); delim != std::u16string::npos) {
    body = std::u16string_view(u16.data(), delim);
    split = std::u16string_view(u16.data() + delim + 1, u16.size() - delim - 1);
  }

  // split を正順（先頭 = 最後に使われた置換文字）に処理する。
  // 各置換文字は自分の辞書エントリ「置換文字+元文字列」を末尾に持つ。
  // 置換文字は「当時の文字列に出現しない」文字から選ばれるため、
  // rfind は常にその辞書エントリを一意に指す。
  auto data = std::u16string(body);
  for (auto const rc : split) {
    auto const p = data.rfind(rc);
    if (p == std::u16string::npos) {
      throw std::runtime_error("uncrush: replacement char not found");
    }
    auto const original = data.substr(p + 1);
    data.resize(p);
    size_t pos = 0;
    while ((pos = data.find(rc, pos)) != std::u16string::npos) {
      data.replace(pos, 1, original);
      pos += original.size();
    }
  }

  // JSON 構造文字を復元（swap の逆方向）
  auto const swapped_back = frozenchars::json::detail::json_crush_swap<char16_t>(data, false);
  return frozenchars::json::detail::utf16_to_utf8(swapped_back);
}
```

- [ ] **Step 2: uncrushed_size を追加**

```cpp
template <FrozenString Input>
[[nodiscard]] consteval auto uncrushed_size() -> size_t {
  return uncrush_to_string(Input.sv()).size() + 1;
}
```

- [ ] **Step 3: 公開 API uncrush を追加**

`crush.hpp` の `crush` 関数の下:

```cpp
template <FrozenString Input>
  requires(Input.length > 0)
[[nodiscard]] consteval auto uncrush() -> FrozenString<detail::uncrushed_size<Input>()> {
  constexpr auto N = detail::uncrushed_size<Input>();
  auto const result = detail::uncrush_to_string(Input.sv());
  auto res = FrozenString<N>{};
  for (size_t i = 0; i < result.size() && i < N - 1; ++i) {
    res.buffer[i] = result[i];
  }
  res.buffer[result.size()] = '\0';
  res.length = result.size();
  return res;
}
```

- [ ] **Step 4: ビルドと crush 往復テストを確認**

Run: `cd build && cmake --build . --target all_test -j && ./build/test/all_test "[json][crush]"`
Expected: PASS（uncrush roundtrip が通る）

- [ ] **Step 5: コミット**

```bash
git add include/frozenchars/json/crush.hpp
git commit -m "feat: uncrush API を追加"
```

---

### Task 6: 全テスト・README・CHANGELOG 更新

**Files:**
- Modify: `README.md`
- Modify: `CHANGELOG.md`

- [ ] **Step 1: 全テストを実行**

Run: `bash build.sh && bash test.sh && ./build/test/all_test`
Expected: 全パス（1706+ アサーション、compile_fail 含む）

- [ ] **Step 2: README に復元 API を追記**

`README.md` の JSON 圧縮記述（`include "frozenchars/json/compress.hpp"` の近く）に追加:

```markdown
- `json::decompress` — `json::compress` の結果を復元（意味的に同一の JSON を生成）
- `json::uncrush` — `json::crush` の結果を復元（バイト完全）
```

- [ ] **Step 3: CHANGELOG に追記**

```markdown
## [Unreleased]
### Added
- `json::decompress` / `json::uncrush`（圧縮結果の復元 API）
### Fixed
- `json::compress` の出力を有効な JSON に変更し、小数・指数・型情報を保持
- `json::crush` の辞書領域を置換対象から除外（圧縮結果の復元を保証）
```

- [ ] **Step 4: コミット**

```bash
git add README.md CHANGELOG.md
git commit -m "docs: decompress / uncrush を README・CHANGELOG に追記"
```

---

## Self-Review

- Spec coverage: 設計書の API（decompress / uncrush）、compress 形式変更、crush データ領域限定、from_base62、往復テスト、validate_json テスト、ドキュメント — 全項目にタスクあり。
- 設計書の「split を逆順に処理」は検証により誤りと判明。**正順（先頭=最新）が正しい**ため Task 5 に反映済み。
- 型整合: `build_initial_candidates` / `count_candidates` / `replace_all_with_char` の `data_end` パラメータは Task 2 で導入し、Task 2 内で完結。`decompress_to_string` / `uncrush_to_string` は Task 4/5 で導入し各タスク内で完結。
