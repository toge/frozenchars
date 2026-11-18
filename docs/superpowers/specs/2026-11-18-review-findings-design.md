# レビュー指摘修正 設計

## 目的

`v0.8.2` の全体レビューで確認した、UTF-8/JSON 入力検証、日時変換、コンテナの例外契約、CMake 配布ターゲットの8件を修正する。
公開 API の戻り値型や主要ヘッダ構成は変更せず、既存利用者との互換性を維持する。

## UTF-8 と JSON

- UTF-8 の正当性判定を `detail` の共通ヘルパに集約する。
- 正当なシーケンスは最短形式で、サロゲート範囲と `U+10FFFF` 超過を拒否する。
- `utf8_decode_at` と Crush の UTF-8 デコーダは同じ制約を適用する。
- `utf8_to_codepoints` と reverse 操作の fail-soft 契約は、不正バイトを消費せず1バイトだけ保持して進める。
- JSON validator は文字列内の raw UTF-8 も検証する。
- Crush は不正な UTF-8、未対応のサロゲート、UTF-16 の不正ペアを `invalid_argument` として伝播する。

## 日時とコンテナ

- 実行時 ISO-8601 パーサは NTTP 版と同じ検証を行う。
- 長さ20の終端子は `Z`/`z` のみを受理し、長さ25のオフセットは符号、桁、コロン、時0-23、分0-59を検証する。
- `__DATE__` 形式は固定位置、10進数字、年下限、日数範囲に加え、完成した `year_month_day::ok()` を検証する。
- map、multimap、trie map のデフォルト構築、値コピー、等価比較に対応する `noexcept` は型の演算に応じて条件付きにする。

## CMake 配布

- `frozenchars::frozenchars` は Glaze に依存しない基底ターゲットにする。
- `frozenchars::glaze` を新設し、基底ターゲットと `glaze::glaze` に依存させる。
- Glaze を検出したビルドだけで、Glaze専用ターゲットを定義・exportする。
- インストール済み package config は利用可能なコンポーネントとして Glaze を必要時だけ探索する。
- GCC/Clang/MSVC 向け constexpr 上限は基底ターゲットの `INTERFACE_COMPILE_OPTIONS` として配布する。警告オプションは配布しない。
- インストール後の consumer test で、Glazeなしのcore利用とGlazeありの連携利用の双方を確認する。

## テストと検証

- 各修正に先行して、レビューで再現した入力を公開 API で検証する回帰テストを追加する。
- 不正UTF-8の1バイト単位保持、JSON rejection/preservation の区別、Crush round-trip corruption、タイムゾーン範囲、`Feb 30`、非数字、throwing mapped type、constexpr固定小数点外挙動を対象にする。
- CMake consumer test は一時 install prefix に対して `find_package(frozenchars CONFIG)` を行う。
- 変更後は対象テスト、全CTest、README例、anchor、amalgamation整合性、差分レビューを行う。

## 非目標

- 公開 API の名称変更、バージョン変更、UTF-8の別方式への全面移行、フォーマット仕様の再設計、C++標準の変更は行わない。
