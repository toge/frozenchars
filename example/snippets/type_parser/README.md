# type_parser スニペット

元 `include/frozenchars/type_parser.hpp` (426行) + `detail/type_mapping.hpp` (94行)
を本体から分離したサンプル。

## 機能

- `parse_to_tuple<"int, string?, [double,double]"_fs>()` → `std::tuple<...>`
- `parse_to_variant<...>()` → `std::variant<...>`
- `type_mapping<"int"_fs>::type` / `type_mapping_v` / `parse_to_tuple_t` 等

構文: `int`/`string`/`bool` 等の型名、カンマ区切り、`?` で `optional`、`[a,b]` でネスト tuple、`*`/`&`/`&&` サフィックス対応。詳細は元 README の `parse_to_tuple` 章を参照。

## 使い方

```cpp
#include "example/snippets/type_parser/type_parser.hpp"
using namespace frozenchars::literals;

using T = typename decltype(frozenchars::parse_to_tuple<"int, string?, bool"_fs>())::type;
// T = std::tuple<int, std::optional<std::string>, bool>
```

`frozenchars` 本体 (`string.hpp`/`string_ops.hpp`) に依存する。必要なプロジェクトへヘッダをコピペして利用する。

## ビルド

```bash
g++ -std=c++23 -I include example/snippets/type_parser/example_parse_to_tuple.cpp && ./a.out
```
