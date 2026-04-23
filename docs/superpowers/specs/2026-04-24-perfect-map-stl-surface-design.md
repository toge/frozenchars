# PerfectMap STL-like surface design

## Problem

`frozenchars::PerfectMap<T, Keys...>` は固定キー集合に対する高速 lookup と
runtime での値更新はできるが、現状の public surface は最小限で、
STL 風コンテナとして扱うにはやや弱い。

特に不足しているのは次の点。

1. member types が不足しており generic code から扱いにくい
2. `find()` / `count()` がなく、`contains()` 以外の分岐が書きにくい
3. `begin()` / `end()` がなく、range-for や標準アルゴリズムに自然に乗らない

今回の追加は、PerfectMap のコア設計である「固定キー集合 + runtime 値更新」を
維持したまま、STL 風の read API を増やして扱いやすくすることを目的とする。

## Scope

### In scope

- member types の追加
- `empty()` / `max_size()` の追加
- `find()` / `count()` の追加
- `begin()` / `end()` / `cbegin()` / `cend()` の追加
- `iterator` / `const_iterator` の追加
- `for (auto&& [key, value] : map)` に寄せやすい iterator value 表現

### Out of scope

- `insert`, `erase`, `emplace`, `try_emplace` などキー集合を変える API
- `std::unordered_map` / `std::map` の完全互換
- ランタイムでのキー追加・削除
- 今回の段階での `keys()` / `values()` view 追加

## Approved API additions

PerfectMap に追加する public surface は次を最低ラインとする。

```cpp
template <typename T, FrozenString... Keys>
class PerfectMap {
 public:
  using key_type = std::string_view;
  using mapped_type = T;
  using value_type = /* pair-like proxy target */;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = /* pair-like proxy */;
  using const_reference = /* pair-like const proxy */;
  using iterator = /* custom iterator */;
  using const_iterator = /* custom const iterator */;

  static constexpr auto size() noexcept -> size_type;
  static constexpr auto max_size() noexcept -> size_type;
  static constexpr auto empty() noexcept -> bool;

  constexpr auto begin() noexcept -> iterator;
  constexpr auto end() noexcept -> iterator;
  constexpr auto begin() const noexcept -> const_iterator;
  constexpr auto end() const noexcept -> const_iterator;
  constexpr auto cbegin() const noexcept -> const_iterator;
  constexpr auto cend() const noexcept -> const_iterator;

  constexpr auto find(std::string_view key) noexcept -> iterator;
  constexpr auto find(std::string_view key) const noexcept -> const_iterator;
  constexpr auto count(std::string_view key) const noexcept -> size_type;
};
```

`contains()`, `at()`, `operator[]` は既存仕様を維持する。

## Design constraints

### Internal storage stays unchanged

- runtime state は引き続き slot 順の `std::array<T, N> values_` のみとする
- 固定キーは型側メタ情報として持ち、runtime に複製しない
- 完全ハッシュ導出と lookup 経路は既存の `find_seed`, `fnv1a_hash`, `slot_for` を再利用する

### No dynamic-key API

PerfectMap の価値は固定キー集合にあるため、キー集合を変える API は追加しない。
今回の拡張は「読みやすさ」と「generic STL-like access」の改善に限定する。

## Iterator and value representation

### Recommendation

内部の `values_` は今のまま維持し、iterator が
「固定キー metadata + 対応 value 参照」を束ねた軽量 proxy を返す。

この方針を採る理由:

1. runtime でキーを保持し直さずに済む
2. 既存の slot 順内部表現を壊さない
3. `find()` / `begin()` / `end()` を追加しやすい

### Proxy shape

- non-const iterator は `std::string_view key` と `T& value` を見せる proxy
- const iterator は `std::string_view key` と `T const& value` を見せる proxy
- 初手では `operator->()` の完璧再現までは無理に狙わない
- `operator*()` で pair-like proxy を返し、structured bindings に対応できる形を目標にする

### Pair-like compatibility

実装候補は次のいずれか。

1. public member を持つ簡素な proxy (`key`, `value`)
2. `tuple_size` / `tuple_element` / `get<I>` を備えた tuple-like proxy

今回の推奨は **tuple-like proxy**。
generic code や structured bindings との相性が良いため。

## Lookup integration

### find

- `find(key)` は既存 lookup path を使う
- 成功時は対応 slot を指す iterator を返す
- 失敗時は `end()` / `cend()` を返す

### count

- `count(key)` は `find(key) != end()` を `0` / `1` に落とす薄い wrapper にする
- 固定キー集合なので値域は `0` または `1` のみ

### contains / at / operator[]

追加 API 導入後も意味は変えない。
実装重複を避けるため、最終的には内部 lookup を
`find` または共通 helper に寄せる。

## Iteration order

- 走査順は **slot 順** とする
- 宣言順は保証しない
- これは既存 internal storage と整合し、追加メモリなしで iterator を実装しやすい

利用者が宣言順を期待しないよう、ドキュメントとテストで明示する。

## Testing

少なくとも次を追加する。

1. member types が存在し、基本的な型関係が成り立つこと
2. `empty()` / `max_size()` / `size()` の整合
3. `find()` 正常系: 既知キーで `end()` 以外を返すこと
4. `find()` 異常系: 未知キーで `end()` を返すこと
5. `count()` が `0` / `1` を返すこと
6. range-for で全要素を走査できること
7. structured bindings で `key` と `value` を読めること
8. const iterator と non-const iterator の両方でアクセスできること
9. slot 順 iteration が宣言順とは限らないことを明示すること

## Non-goals

- mutable associative container 化
- iterator invalidation rules の複雑化
- proxy を完全に `std::pair<const key_type, mapped_type>` と同一視できること
