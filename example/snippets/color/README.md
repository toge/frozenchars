# color スニペット

元 `include/frozenchars/color.hpp` / `mod/color.hpp` を本体から分離したサンプル。

## 機能

- `parse_hex_rgb("#RGB" | "#RRGGBB")` → `std::tuple<uint8_t,uint8_t,uint8_t>`
- `parse_hex_rgba("#RGBA" | "#RRGGBBAA")` → `std::tuple<uint8_t,uint8_t,uint8_t,uint8_t>`
- `to_bgr` / `to_bgra` / `to_abgr` — チャネル並び替え

すべて `consteval`。不正な入力はコンパイル時エラー。

## 使い方

```cpp
#include "example/snippets/color/color.hpp"

constexpr auto rgb = frozenchars::parse_hex_rgb("#1a2b3c");
constexpr auto bgr = frozenchars::to_bgr(rgb);
```

このヘッダは標準ライブラリのみに依存し、`frozenchars` 本体への依存はない。必要なプロジェクトへコピペして利用する。

## ビルド

```bash
g++ -std=c++23 -I example/snippets/color example/snippets/color/example_color.cpp && ./a.out
```
