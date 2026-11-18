# レビュー指摘修正 設計

## 目的

`v0.8.2` の全体レビューで確認した、UTF-8/JSON 入力検証、日時変換、コンテナの例外契約、CMake 配布ターゲットの8件を修正する。
公開 API の戻り値型や主要ヘッダ構成は変更せず、既存利用者との互換性を維持する。

## UTF-8 と JSON

- UTF-8 の正当性判定を `detail` の共通ヘルパに集約する。
- 正当なシーケンスは最短形式で、サロゲート範囲と `U+10FFFF` 超過を拒否する。
- `utf8_decode_at` と Crush の UTF-8 デコーダは同じ制約を適用する。
- `utf8_to_codepoints` と reverse 操作の fail-soft 契約は、`E0 80 80` や `ED A0 80` を一つのコードポイントへ変換せず、先頭1バイトを保持して次の位置を処理する。したがって出力は入力の不正バイトをbyte-for-byteで保持する。
- JSON validator は文字列内の raw UTF-8 も検証する。
- Crush の内部変換は不正な UTF-8、未対応のサロゲート、UTF-16 の不正ペアを `invalid_argument` として返す。公開 `consteval` API はその結果を `static_assert` で明示的に診断し、compile-fail testで確認する。

## 日時とコンテナ

- 実行時 ISO-8601 パーサは NTTP 版と同じ検証を行う。
- 長さ20の終端子は `Z`/`z` のみを受理し、長さ25のオフセットは符号、桁、コロン、時0-23、分0-59を検証する。
- `__DATE__` 形式は固定位置、10進数字、年下限、日数範囲に加え、完成した `year_month_day::ok()` を検証する。
- コンテナの無条件 `noexcept` を演算別に条件化する。default constructor は `is_nothrow_default_constructible_v<T>`、`get_value_or` は `is_nothrow_copy_constructible_v<T>`、`operator==` は値型 `operator==` の `noexcept` 式を使う。
- この対象は `frozen_map` の3箇所と、同じdefault constructorを持つ `frozen_multimap`・`frozen_trie_map`。後二者に同名の `get_value_or`/`operator==` はない。

## CMake 配布

- `frozenchars::frozenchars` は Glaze に依存しない基底ターゲットにする。
- `frozenchars::glaze` を新設し、基底ターゲットと `glaze::glaze` に依存させる。
- Glaze連携ヘッダは基底ターゲットに含めない要件ではないが、その利用者が明示的に `frozenchars::glaze` をリンクする契約にする。
- Glaze を検出したビルドだけで、Glaze専用ターゲットを定義・exportする。検出できない場合も基底ターゲットのinstall/consumeは成功する。
- package config 自体は常にcoreだけ読み込み、`find_package(frozenchars CONFIG COMPONENTS glaze)` の場合だけGlazeを `find_dependency` して、`frozenchars::glaze` のexportを確認する。
- GCC/Clang/MSVC 向け constexpr 上限は基底ターゲットの `INTERFACE_COMPILE_OPTIONS` として配布する。警告オプションは配布しない。
- in-tree test、Glaze利用example、READMEのCMake例を `frozenchars::glaze` に切り替える。
- インストール後の consumer test で、Glazeなしのcore利用と、`COMPONENTS glaze` を伴う連携利用を確認する。
- install prefixを使ったconsumerで既定上限を超える `json::crush` をコンパイルし、各compilerへconstexprオプションが伝播することを確認する。

## テストと検証

- 各修正に先行して、レビューで再現した入力を公開 API で検証する回帰テストを追加する。
- 不正UTF-8の1バイト単位保持、JSON rejection/preservation の区別、Crush compile failure、タイムゾーン範囲、`Feb 30`、非数字、throwing mapped typeを対象にする。
- CMake consumer test は一時 install prefix に対して `find_package(frozenchars CONFIG)` を行う。
- 変更後は対象テスト、全CTest、README例、anchor、amalgamation整合性、差分レビューを行う。

## 非目標

- 公開 API の名称変更、バージョン変更、UTF-8の別方式への全面移行、フォーマット仕様の再設計、C++標準の変更は行わない。
