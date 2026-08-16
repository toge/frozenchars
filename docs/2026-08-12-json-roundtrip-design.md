# JSON 往復変換（decompress / uncrush）設計

- 日付: 2026-08-12
- 対象: `include/frozenchars/json/` 配下

## 背景と目的

`frozenchars::json` にはコンパイル時圧縮 API（`compress` / `crush` / `crush_compress`）があるが、
復元 API が存在せず、圧縮結果を元に戻す手段がない。また、現行実装には以下の欠陥がある。

- compress の出力が非標準形式（キー無クォート、null が `_`）
- number が `int64_t` に丸められ、小数・指数が失われる（`1.5` → `1`）
- crush の辞書領域（末尾に連結した「置換文字＋元文字列」）が後続の置換で破壊されるケースがあり、
  `"ab"` を4回含む入力などが往復不能

本設計では、復元 API（`decompress` / `uncrush`）を追加し、同時に上記の欠陥を修正して
**圧縮→復元の往復を保証**する。

## API

すべて `consteval`、入出力は `FrozenString`。実行時 API は提供しない（スコープ外）。

```cpp
// compress.hpp に追加
template <FrozenString Input> requires(Input.length > 0)
consteval auto decompress() -> FrozenString<detail::decompressed_size<Input>()>;

// crush.hpp に追加
template <FrozenString Input> requires(Input.length > 0)
consteval auto uncrush() -> FrozenString<detail::uncrushed_size<Input>()>;
```

サイズ計算ヘルパ（`decompressed_size` / `uncrushed_size`）は既存の
`compressed_size` / `crushed_size` と同じ「全体を走らせて実長を得る」パターンに従う。

`crush_compress` の逆変換は専用 API を作らず、`decompress` ∘ `uncrush` の合成で対応する。

## compress 出力形式の変更

出力を**そのまま `parse_json` で読める有効な JSON** にする。変更は
`compress_detail.hpp` の `value_to_string` とキー処理に限定する（パーサ自体は変更しない）。

| 要素 | 変更前 | 変更後 |
|---|---|---|
| null | `_` | `null` |
| number | `int64_to_string(num_val)`（小数喪失） | `str_val` 原文 |
| string | クォートを剥がして再付加 | `str_val` 原文（クォート・エスケープ含む） |
| object キー | クォート剥がし | `keys[i]` 原文（クォート保持） |
| root のスカラー参照 | 裸の `0` / `a` | クォート付き `"0"` / `"a"` |

変更後の例:

```json
{"values":["item",1],"root":[{"name":"0","val":"1"}]}
```

- 数値の原文保持により、小数・指数・型（数値 `1` と文字列 `"1"` の区別）が保存される。
- 文字列エスケープは解釈せず**原文をバイト単位で保持**する。これにより
  `{"a":"he\"llo"}` のような入力も破壊されない。
- 既存テスト `test_json_compress.cpp` のアサーションを1箇所修正する
  （`"name:"` を検索 → `"\"name\""` を検索）。

## crush の辞書領域修正

現行 crush は置換対象を文字列全体にしているため、辞書エントリ（末尾の
「置換文字＋元部分文字列」）のマーカー文字が後続の置換で破壊される。対策として、
js_crush の置換を**データ領域（辞書開始位置より前）に限定**する。

- 置換・出現回数カウントはデータ領域のみを対象にする。
- 辞書は追記後不変（凍結）。
- 出力形式は変更しない。既存テストはそのまま通過する。

## uncrush アルゴリズム

1. 末尾の `_` を剥がす（無ければ throw）
2. UTF-8 → UTF-16
3. デリミタ U+0001 で body / split に分割
4. split を**正順**に処理する:
   - 各置換文字 c について `rfind(c)` で辞書エントリを特定
   - 末尾の部分文字列（c の次から末尾まで）を取得
   - 本体を辞書手前で切り詰め、c を取得した部分文字列へ全面置換
   - 正順である理由: 置換文字は「当時の文字列全体に出現しない」文字から選ばれるため、
     最新の置換文字から展開すれば、展開結果に含まれる古い置換文字は後段で正しく展開される
5. `json_crush_swap(forward=false)` で構造文字を復元（既存実装は順序が鏡像のため
   数学的に正しい逆変換。検証済み）
6. UTF-16 → UTF-8

## decompress アルゴリズム

1. 圧縮済み JSON（有効な JSON）を既存 `parse_json` でパース
2. `values` 配列から各値の JSON テキストを復元:
   - string / number は `str_val` 原文
   - bool / null はリテラル
3. root を再帰的に再構築:
   - スカラー文字列は Base62 参照として `from_base62` で復号し `values[idx]` に置換
   - キーは原文のまま

`to_base62` の逆関数 `from_base62` を compress_detail.hpp に追加する。

## テスト

- compress → decompress の往復テスト
  - オブジェクト・配列・小数（`1.5`）・エスケープ（`"he\"llo"`）・空構造・ネスト
  - `validate_json(compress出力)` が true になること
- crush → uncrush の往復テスト
  - バイト完全一致
  - `"ab"` を4回含む入力の回帰ケース
  - サロゲートペア（😀）を含む入力
- 不正な圧縮データ（末尾 `_` なし）が throw すること

## 既知の制約（仕様）

- compress → decompress は**意味的**に同一（空白は正規化される。バイト完全ではない）
- crush → uncrush は U+0001 を含まない入力でバイト完全（crush が入力をフィルタするため）
- 不正な圧縮データは throw（consteval 文脈ではコンパイルエラーになる）

## 変更ファイル

- `include/frozenchars/json/detail/compress_detail.hpp` — 出力形式変更、`from_base62` 追加
- `include/frozenchars/json/detail/crush_detail.hpp` — データ領域限定の置換
- `include/frozenchars/json/compress.hpp` — `decompress` / `decompressed_size` 追加
- `include/frozenchars/json/crush.hpp` — `uncrush` / `uncrushed_size` 追加
- `test/test_json_compress.cpp` — 出力形式アサーション修正、往復テスト追加
- `test/test_json_crush.cpp` — 往復テスト追加
- `README.md` / `CHANGELOG.md` — ドキュメント更新
