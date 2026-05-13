#pragma once

#include "frozen_string.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <concepts>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <cstring>
#include <ranges>
#include <span>

#if defined(__SSE4_2__)
#include <nmmintrin.h>
#elif defined(__aarch64__) && defined(__ARM_FEATURE_CRC32)
#include <arm_acle.h>
#endif

namespace frozenchars {

template <typename T>
struct frozen_map_entry {
  std::string_view key;
  T value;
};

namespace detail {

template <typename Self, typename T>
using forward_like_t = decltype(std::forward_like<Self>(std::declval<T&>()));

/**
 * @brief ソフトウェア版 CRC32C (Castagnoli) 実装 (Constexpr用)
 */
namespace crc32c {

static constexpr std::uint32_t k_polynomial = 0x82F63B78;

consteval auto make_table() {
  std::array<std::uint32_t, 256> table{};
  for (std::uint32_t i = 0; i < 256; ++i) {
    std::uint32_t res = i;
    for (int j = 0; j < 8; ++j) res = (res >> 1) ^ (res & 1 ? k_polynomial : 0);
    table[i] = res;
  }
  return table;
}

static constexpr auto k_table = make_table();
constexpr auto update_byte(std::uint32_t crc, std::uint8_t byte) noexcept -> std::uint32_t {
  return (crc >> 8) ^ k_table[(crc ^ byte) & 0xFF];
}

constexpr auto hash_software(std::string_view key, std::uint32_t seed) noexcept -> std::uint32_t {
  std::uint32_t crc = seed;
  for (auto const c : key) crc = update_byte(crc, static_cast<std::uint8_t>(c));
  return crc;
}

} // namespace crc32c

/**
 * @brief ハッシュアルゴリズム: ハードウェア命令（SSE4.2 / ARM CRC32）を使用、そうでなければソフトウェア実装
 */
constexpr auto hash_impl(std::string_view key, std::uint32_t seed) noexcept -> std::uint64_t {
  auto h = [key, seed] {
    if (!std::is_constant_evaluated()) {
#if defined(__SSE4_2__)
      std::uint64_t crc = seed;
      auto const n = key.size();
      auto const d = key.data();
      std::size_t i = 0;
      for (; i + 8 <= n; i += 8) {
          std::uint64_t chunk;
          std::memcpy(&chunk, d + i, 8);
          crc = _mm_crc32_u64(crc, chunk);
      }
      for (; i < n; ++i) crc = _mm_crc32_u8(static_cast<std::uint32_t>(crc), static_cast<std::uint8_t>(d[i]));
      return crc;
#elif defined(__aarch64__) && defined(__ARM_FEATURE_CRC32)
      std::uint32_t crc = seed;
      auto const n = key.size();
      auto const d = reinterpret_cast<const std::uint8_t*>(key.data());
      std::size_t i = 0;
      for (; i + 8 <= n; i += 8) {
          std::uint64_t chunk;
          std::memcpy(&chunk, d + i, 8);
          crc = __crc32cd(crc, chunk);
      }
      for (; i < n; ++i) crc = __crc32cb(crc, d[i]);
      return static_cast<std::uint64_t>(crc);
#endif
    }
    return static_cast<std::uint64_t>(crc32c::hash_software(key, seed));
  }();
  // Finalizer to break CRC32 linearity and effectively use the seed for same-length strings
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccdull;
  h ^= h >> 33;
  h *= 0xc4ceb9fe1a85ec53ull;
  h ^= h >> 33;
  return h;
}

inline constexpr auto k_fnv_offset_basis = 14695981039346656037ull;
inline constexpr auto k_fnv_prime = 1099511628211ull;

constexpr auto fnv1a_hash(std::string_view key, std::uint32_t seed) noexcept -> std::uint64_t {
  auto hash = k_fnv_offset_basis ^ static_cast<std::uint64_t>(seed);
  for (auto const ch : key) hash = (hash ^ static_cast<unsigned char>(ch)) * k_fnv_prime;
  return hash;
}

consteval auto next_pow2(std::size_t n) -> std::size_t {
  std::size_t res = 1; while (res < n) res <<= 1; return res;
}

template <FrozenString... Keys>
consteval auto has_duplicate_keys() -> bool {
  if constexpr (sizeof...(Keys) == 0) return false;
  else {
    constexpr std::array key_views{ std::string_view{Keys.buffer.data(), Keys.length}... };
    for (auto i = 0uz; i < key_views.size(); ++i)
      for (auto j = i + 1; j < key_views.size(); ++j)
        if (key_views[i] == key_views[j]) return true;
    return false;
  }
}

/**
 * @brief テーブルサイズに応じた適切なインデックス型を選択する
 */
template <std::size_t TableSize>
using index_type_t = std::conditional_t<(TableSize < 127), std::int8_t,
                    std::conditional_t<(TableSize < 32767), std::int16_t, std::int32_t>>;

/**
 * @brief ルックアップテーブルのメタデータとシード値の探索結果
 */
template <std::size_t TableSize, std::size_t KeyCount>
struct lookup_seed_result {
  using index_t = index_type_t<KeyCount>;
  std::uint32_t seed;              ///< 衝突を回避するためのハッシュシード
  std::array<index_t, TableSize> table; ///< インデックス値を格納するルックアップテーブル
};

/**
 * @brief 完全衝突ゼロのルックアップテーブルを構築するためのシード値を探索する
 * キー数が多い場合、コンパイル時の計算量制限を回避するためにこの関数の呼び出しはスキップされる必要がある。
 */
template <std::size_t TableSize, FrozenString... Keys>
consteval auto find_lookup_seed() {
  using result_t = lookup_seed_result<TableSize, sizeof...(Keys)>;
  using index_t = typename result_t::index_t;
  constexpr std::array key_views{ std::string_view{Keys.buffer.data(), Keys.length}... };
  constexpr auto mask = TableSize - 1;
  for (auto seed = 0u; seed < 1'000'000u; ++seed) {
    std::array<index_t, TableSize> table; table.fill(static_cast<index_t>(-1));
    auto collision = false;
    for (auto i = 0uz; i < sizeof...(Keys); ++i) {
      auto const slot = static_cast<std::size_t>(hash_impl(key_views[i], seed) & mask);
      if (table[slot] != static_cast<index_t>(-1)) { collision = true; break; }
      table[slot] = static_cast<index_t>(i);
    }
    if (!collision) return result_t{seed, table};
  }
  throw "frozen_map seed search exhausted";
}

template <std::uint32_t MaxSeedExclusive, FrozenString... Keys>
consteval auto find_seed() -> std::uint32_t {
  static_assert(!has_duplicate_keys<Keys...>(), "frozen_map keys must be unique");
  if constexpr (MaxSeedExclusive == 0) throw "frozen_map seed search exhausted";
  else {
    constexpr std::array key_views{ std::string_view{Keys.buffer.data(), Keys.length}... };
    for (auto seed = 0u; seed < MaxSeedExclusive; ++seed) {
      std::array<bool, sizeof...(Keys)> used_slots{};
      auto collision = false;
      for (auto const key_view : key_views) {
        auto const slot = static_cast<std::size_t>(fnv1a_hash(key_view, seed) % sizeof...(Keys));
        if (used_slots[slot]) { collision = true; break; }
        used_slots[slot] = true;
      }
      if (!collision) return seed;
    }
    throw "frozen_map seed search exhausted";
  }
}

template <typename EntryLike>
concept PairLikeEntry = requires {
  typename std::tuple_size<std::remove_cvref_t<EntryLike>>::type;
  requires std::tuple_size_v<std::remove_cvref_t<EntryLike>> == 2;
};

template <std::size_t Index, typename EntryLike>
constexpr decltype(auto) pair_like_get(EntryLike&& entry) {
  using std::get; return get<Index>(std::forward<EntryLike>(entry));
}

template <typename T> struct is_std_map : std::false_type {};
template <typename K, typename V, typename C, typename A> struct is_std_map<std::map<K, V, C, A>> : std::true_type {};
template <typename T> inline constexpr bool is_std_map_v = is_std_map<std::remove_cvref_t<T>>::value;
template <typename T> struct is_std_unordered_map : std::false_type {};
template <typename K, typename V, typename H, typename P, typename A> struct is_std_unordered_map<std::unordered_map<K, V, H, P, A>> : std::true_type {};
template <typename T> inline constexpr bool is_std_unordered_map_v = is_std_unordered_map<std::remove_cvref_t<T>>::value;
template <typename T> struct is_std_array : std::false_type {};
template <typename T, std::size_t N> struct is_std_array<std::array<T, N>> : std::true_type {};
template <typename T> inline constexpr bool is_std_array_v = is_std_array<std::remove_cvref_t<T>>::value;
template <typename T> inline constexpr bool always_false_v = false;

template <typename Result, typename ValueArg>
concept is_frozen_map_associative_result = (is_std_map_v<Result> || is_std_unordered_map_v<Result>) &&
    std::default_initializable<std::remove_cvref_t<Result>> &&
    std::constructible_from<typename std::remove_cvref_t<Result>::key_type, std::string_view> &&
    std::constructible_from<typename std::remove_cvref_t<Result>::mapped_type, ValueArg> &&
    requires(std::remove_cvref_t<Result>& result, typename std::remove_cvref_t<Result>::key_type key, typename std::remove_cvref_t<Result>::mapped_type value) {
      result.emplace(std::move(key), std::move(value));
    };

template <typename Result, typename ValueArg>
concept frozen_map_associative_result = is_frozen_map_associative_result<Result, ValueArg>;

template <typename Result, std::size_t ExpectedSize, typename ValueArg>
concept is_frozen_map_array_result = is_std_array_v<Result> && std::tuple_size_v<std::remove_cvref_t<Result>> == ExpectedSize &&
    PairLikeEntry<typename std::remove_cvref_t<Result>::value_type> &&
    std::constructible_from<typename std::remove_cvref_t<Result>::value_type, std::string_view, ValueArg>;

template <typename Result, std::size_t ExpectedSize, typename ValueArg>
concept frozen_map_array_result = is_frozen_map_array_result<Result, ExpectedSize, ValueArg>;

template <typename Result, std::size_t ExpectedSize, typename ValueArg>
concept frozen_map_result = frozen_map_associative_result<Result, ValueArg> || frozen_map_array_result<Result, ExpectedSize, ValueArg>;

} // namespace detail

template <typename T, FrozenString... Keys>
class frozen_map {
public:
  using key_type = std::string_view;
  using mapped_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using value_type = std::pair<std::string_view const, T>;
  using reference = std::pair<std::string_view, T&>;
  using const_reference = std::pair<std::string_view, T const&>;

  template <typename Ref>
  struct arrow_proxy {
    Ref ref_v;
    std::string_view& key;
    decltype(std::declval<Ref>().second)& value;
    constexpr arrow_proxy(Ref r) : ref_v(r), key(ref_v.first), value(ref_v.second) {}
    constexpr auto operator->() noexcept -> arrow_proxy* { return this; }
    constexpr auto operator->() const noexcept -> const arrow_proxy* { return this; }
  };

  template <typename Owner, typename Ref>
  class iterator_base {
  public:
    using iterator_category = std::random_access_iterator_tag;
    using value_type = frozen_map::value_type;
    using difference_type = std::ptrdiff_t;

    struct arrow_proxy_impl {
        Ref ref_v;
        std::string_view& key;
        decltype(std::declval<Ref>().second)& value;
        constexpr arrow_proxy_impl(Ref r) : ref_v(r), key(ref_v.first), value(ref_v.second) {}
        constexpr auto operator->() noexcept -> arrow_proxy_impl* { return this; }
        constexpr auto operator->() const noexcept -> const arrow_proxy_impl* { return this; }
    };
    using arrow_proxy = arrow_proxy_impl;
    using pointer = arrow_proxy;
    using reference = Ref;

    constexpr iterator_base() noexcept = default;
    constexpr iterator_base(Owner* owner, size_type index) noexcept : owner_{owner}, index_{index} {}
    constexpr auto operator*() const noexcept -> reference { return reference{owner_->key_views_[index_], owner_->values_[index_]}; }
    constexpr auto operator->() const noexcept -> pointer { return pointer{operator*()}; }
    constexpr auto operator++() noexcept -> iterator_base& { ++index_; return *this; }
    constexpr auto operator++(int) noexcept -> iterator_base { auto tmp = *this; ++index_; return tmp; }
    constexpr auto operator--() noexcept -> iterator_base& { --index_; return *this; }
    constexpr auto operator--(int) noexcept -> iterator_base { auto tmp = *this; --index_; return tmp; }
    constexpr auto operator+=(difference_type n) noexcept -> iterator_base& { index_ += n; return *this; }
    constexpr auto operator-=(difference_type n) noexcept -> iterator_base& { index_ -= n; return *this; }
    friend constexpr auto operator+(iterator_base a, difference_type n) noexcept -> iterator_base { return a += n; }
    friend constexpr auto operator+(difference_type n, iterator_base a) noexcept -> iterator_base { return a += n; }
    friend constexpr auto operator-(iterator_base a, difference_type n) noexcept -> iterator_base { return a -= n; }
    friend constexpr auto operator-(iterator_base a, iterator_base b) noexcept -> difference_type { return static_cast<difference_type>(a.index_) - static_cast<difference_type>(b.index_); }
    friend constexpr bool operator==(iterator_base const& a, iterator_base const& b) noexcept { return a.index_ == b.index_; }
    friend constexpr auto operator<=>(iterator_base const& a, iterator_base const& b) noexcept { return a.index_ <=> b.index_; }
  private:
    Owner* owner_{nullptr}; size_type index_{0};
  };

  using iterator = iterator_base<frozen_map, reference>;
  using const_iterator = iterator_base<frozen_map const, const_reference>;

  static_assert(sizeof...(Keys) > 0, "frozen_map requires at least one key");
  static_assert(!detail::has_duplicate_keys<Keys...>(), "frozen_map keys must be unique");
  static constexpr auto size() noexcept -> size_type { return sizeof...(Keys); }
  static constexpr auto max_size() noexcept -> size_type { return size(); }
  [[nodiscard]] static constexpr auto empty() noexcept -> bool { return false; }
  /**
   * @brief 辞書順にソートされたキー配列を取得する
   */
  [[nodiscard]] static constexpr auto keys() noexcept -> std::span<const std::string_view, size()> {
    return sorted_key_views_;
  }
  [[nodiscard]] constexpr auto find(std::string_view key) noexcept -> iterator {
    auto const i = find_index_raw(key);
    return i != size() ? iterator{this, i} : end();
  }
  [[nodiscard]] constexpr auto find(std::string_view key) const noexcept -> const_iterator {
    auto const i = find_index_raw(key);
    return i != size() ? const_iterator{this, i} : end();
  }
  [[nodiscard]] constexpr auto count(std::string_view key) const noexcept -> size_type {
    return find_index_raw(key) != size() ? 1uz : 0uz;
  }
  constexpr auto begin() noexcept -> iterator { return iterator{this, 0}; }
  constexpr auto end() noexcept -> iterator { return iterator{this, size()}; }
  constexpr auto begin() const noexcept -> const_iterator { return const_iterator{this, 0}; }
  constexpr auto end() const noexcept -> const_iterator { return const_iterator{this, size()}; }
  constexpr auto cbegin() const noexcept -> const_iterator { return begin(); }
  constexpr auto cend() const noexcept -> const_iterator { return end(); }
  constexpr frozen_map() noexcept requires std::default_initializable<T> = default;
  constexpr explicit frozen_map(std::array<T, size()> values) noexcept(std::is_nothrow_move_constructible_v<T>) : values_{std::move(values)} {}
  constexpr explicit frozen_map(std::initializer_list<T> values) requires std::constructible_from<T, T const&> : values_{copy_initializer_list(values)} {}
  constexpr explicit frozen_map(std::array<frozen_map_entry<T>, size()> entries) : values_{reorder_entries(std::move(entries))} {}
  [[nodiscard]] constexpr auto contains(std::string_view key) const noexcept -> bool {
    return find_index_raw(key) != size();
  }
  [[nodiscard]] constexpr auto at(std::string_view key) -> T& {
    auto const i = find_index_raw(key);
    if (i != size()) [[likely]] return values_[i];
    throw std::out_of_range("frozen_map key not found");
  }
  [[nodiscard]] constexpr auto at(std::string_view key) const -> T const& {
    auto const i = find_index_raw(key);
    if (i != size()) [[likely]] return values_[i];
    throw std::out_of_range("frozen_map key not found");
  }
  [[nodiscard]] constexpr auto get(std::string_view key) noexcept -> std::optional<std::reference_wrapper<T>> { if (auto const index = find_index_opt(key); index) [[likely]] return values_[*index]; return std::nullopt; }
  [[nodiscard]] constexpr auto get(std::string_view key) const noexcept -> std::optional<std::reference_wrapper<T const>> { if (auto const index = find_index_opt(key); index) [[likely]] return values_[*index]; return std::nullopt; }
  constexpr auto operator[](std::string_view key) -> T& { return at(key); }
  constexpr auto operator[](std::string_view key) const -> T const& { return at(key); }
  template <typename Result> requires detail::frozen_map_result<Result, size(), detail::forward_like_t<frozen_map const&, mapped_type>>
  [[nodiscard]] constexpr auto to() const& -> Result { return to_result<Result>(*this); }
  template <typename Result> requires detail::frozen_map_result<Result, size(), detail::forward_like_t<frozen_map&&, mapped_type>>
  [[nodiscard]] constexpr auto to() && -> Result { return to_result<Result>(std::move(*this)); }
private:
  /// キー文字列のビュー配列
  static constexpr std::array<std::string_view, size()> key_views_{ std::string_view{Keys.buffer.data(), Keys.length}... };

  /// 全キーの最大長
  static constexpr auto k_max_key_len_ = std::max({Keys.length...});

  /**
   * @brief 有効なキー長を示すビットセット（ブール配列）
   * @details インデックス i が true であるとき、長さ i のキーが少なくとも1つ存在することを意味する。
   *          サイズは k_max_key_len_ + 1。
   */
  static constexpr auto valid_lengths_ = [] {
    std::array<bool, k_max_key_len_ + 1> table{};
    ((table[Keys.length] = true), ...);
    return table;
  }();

  /**
   * @brief 全キーの長さが互いに異なるかどうかのフラグ
   * @note true の場合、長さからインデックスを O(1) で引けるため、ハッシュ計算が不要となる。
   */
  static constexpr auto all_lengths_unique_ = [] {
    std::array<std::size_t, size()> lens{ Keys.length... };
    std::ranges::sort(lens);
    return std::ranges::adjacent_find(lens) == lens.end();
  }();

  /**
   * @brief キー長 → キー配列インデックスのルックアップテーブル
   * @details all_lengths_unique_ == true の場合のみ有効。
   *          インデックス i に対して、長さ i のキーが存在すればそのインデックスを、
   *          存在しなければ static_cast<index_t>(-1) を格納する。
   *          サイズは k_max_key_len_ + 1。
   */
  static constexpr auto length_to_index_ = [] {
    using index_t = detail::index_type_t<size()>;
    std::array<index_t, k_max_key_len_ + 1> table{};
    table.fill(static_cast<index_t>(-1));
    std::size_t idx = 0;
    (([&]{ table[Keys.length] = static_cast<index_t>(idx++); }()), ...);
    return table;
  }();
  /// 辞書順にソートされたキー文字列のビュー配列
  static constexpr std::array<std::string_view, size()> sorted_key_views_ = [] {
    auto res = key_views_;
    std::ranges::sort(res);
    return res;
  }();
  /// 高速ルックアップテーブルを使用するかどうかのしきい値。11キー以上はコンパイル時間を考慮し線形探索にフォールバック。
  static constexpr auto k_lookup_threshold = 10uz;
  /// ルックアップテーブルを使用するかどうかのフラグ
  static constexpr auto use_lookup_table_ = (size() <= k_lookup_threshold);
  /// ルックアップテーブルのサイズ（2のべき乗）。使用しない場合は最小サイズ。
  static constexpr auto table_size_ = use_lookup_table_ ? detail::next_pow2(size() * 2) : 1uz;
  /// スロット選択用のマスク
  static constexpr auto mask_ = table_size_ - 1;

  /**
   * @brief ルックアップ用のメタデータを生成する。キー数が多い場合は探索をスキップする。
   */
  static consteval auto make_lookup_metadata() {
    if constexpr (use_lookup_table_) {
      return detail::find_lookup_seed<table_size_, Keys...>();
    } else {
      using index_t = detail::index_type_t<size()>;
      return detail::lookup_seed_result<table_size_, size()>{ 0, { static_cast<index_t>(-1) } };
    }
  }
  static constexpr auto metadata_ = make_lookup_metadata();
  static constexpr auto k_seed = metadata_.seed;
  static constexpr auto lookup_table_ = metadata_.table;

  /// 全てのキーが16文字以下であるかどうかのフラグ
  static constexpr bool all_keys_short = ((Keys.length <= 16) && ...);
  /// 短いキー比較の最適化用構造体
  struct alignas(16) PaddedKey { std::uint64_t data[2]{0, 0}; std::uint8_t len = 0; };
  /**
   * @brief 短いキー比較を高速化するためのパディング済みキーデータを生成する
   */
  static consteval auto make_padded_keys() {
    std::array<PaddedKey, size()> res{}; std::size_t idx = 0;
    ([&]{ res[idx].len = static_cast<std::uint8_t>(Keys.length);
        for (std::size_t i = 0; i < Keys.length; ++i) res[idx].data[i / 8] |= (static_cast<std::uint64_t>(static_cast<unsigned char>(Keys.buffer[i])) << ((i % 8) * 8));
        idx++; }(), ...); return res;
  }
  static constexpr auto padded_keys_ = make_padded_keys();

  /**
   * @brief 与えられたキーが指定インデックスのキーと一致するか判定する
   * 短いキー最適化が有効な場合はパディング済みデータで比較を行う。
   */
  [[nodiscard]] static constexpr auto key_equals(std::string_view key, size_type index) noexcept -> bool {
    if constexpr (all_keys_short) {
      if (key.size() > 16) return false;
      auto const& pk = padded_keys_[index];
      if (key.size() != pk.len) return false;
      std::uint64_t low = 0, high = 0;
      auto const d = key.data(); auto const n = key.size();
      if (!std::is_constant_evaluated()) {
        if (n >= 8) { std::memcpy(&low, d, 8); if (n > 8) std::memcpy(&high, d + 8, n - 8); }
        else std::memcpy(&low, d, n);
      } else {
        for (std::size_t i = 0; i < n; ++i) {
          if (i < 8) low |= (static_cast<std::uint64_t>(static_cast<unsigned char>(d[i])) << (i * 8));
          else high |= (static_cast<std::uint64_t>(static_cast<unsigned char>(d[i])) << ((i - 8) * 8));
        }
      }
      return (low == pk.data[0] && high == pk.data[1]);
    }
    return key_views_[index] == key;
  }

  /**
   * @brief キーに対応するインデックスをセンチネル値で返す内部実装
   * @param key 検索するキー
   * @return 見つかった場合はそのインデックス（0 以上 size() 未満）、
   *         見つからない場合は size()（センチネル値）
   * @note この関数は std::optional を生成しないため、ホットパスでの使用に適している。
   *       呼び出し元は戻り値 == size() を「見つからない」と解釈すること。
   */
  [[nodiscard]] static constexpr auto find_index_raw(std::string_view key) noexcept -> size_type {
    if constexpr (use_lookup_table_) {
      auto const len = key.size();

      // ② ハッシュレスルックアップ（全キー長が一意の場合）
      if constexpr (all_lengths_unique_) {
        if (len > k_max_key_len_) return size();
        auto const index = length_to_index_[len];
        if (index == static_cast<decltype(index)>(-1)) return size();
        if (key_equals(key, static_cast<size_type>(index))) [[likely]]
          return static_cast<size_type>(index);
        return size();
      }

      // ① 長さフィルタリング（ハッシュ計算より先に実行）
      if (len > k_max_key_len_ || !valid_lengths_[len]) return size();

      auto const h = detail::hash_impl(key, k_seed);
      auto const slot = static_cast<std::size_t>(h & mask_);
      auto const index = lookup_table_[slot];
      if (index == static_cast<decltype(index)>(-1)) return size();
      if (key_equals(key, static_cast<size_type>(index))) [[likely]]
        return static_cast<size_type>(index);
      return size();
    } else {
      for (auto const i : std::views::iota(0uz, size())) {
        if (key_equals(key, i)) return i;
      }
      return size();
    }
  }

  [[nodiscard]] static constexpr auto find_index_opt(std::string_view key) noexcept
      -> std::optional<size_type> {
    auto const i = find_index_raw(key);
    return i != size() ? std::optional<size_type>{i} : std::nullopt;
  }
  static constexpr auto copy_initializer_list(std::initializer_list<T> values) -> std::array<T, size()> requires std::constructible_from<T, T const&> {
    if (values.size() != size()) throw std::invalid_argument("frozen_map size mismatch: expected " + std::to_string(size()) + " values (one per key), got " + std::to_string(values.size()));
    return [&]<std::size_t... I>(std::index_sequence<I...>) { return std::array<T, size()>{ *(values.begin() + I)... }; }(std::make_index_sequence<size()>{});
  }
  static constexpr auto reorder_entries(std::array<frozen_map_entry<T>, size()> entries) -> std::array<T, size()> {
    auto const get_value = [&](size_type index) -> T&& {
      for (auto& entry : entries) if (entry.key == key_views_[index]) return std::move(entry.value);
      throw std::invalid_argument("missing key");
    };
    return [&]<std::size_t... I>(std::index_sequence<I...>) { return std::array<T, size()>{ get_value(I)... }; }(std::make_index_sequence<size()>{});
  }
  template <typename Self> static constexpr decltype(auto) forward_mapped(Self&& self, size_type index) { return std::forward_like<Self>(self.values_[index]); }
  template <typename Result, typename Self, std::size_t... Index> static constexpr auto to_array_result(Self&& self, std::index_sequence<Index...>) -> Result { return Result{ typename Result::value_type{ key_views_[Index], forward_mapped<Self>(std::forward<Self>(self), Index) }... }; }
  template <typename Result, typename Self> static constexpr auto to_associative_result(Self&& self) -> Result { Result result{}; for (auto index = 0uz; index < size(); ++index) result.emplace(typename Result::key_type{key_views_[index]}, typename Result::mapped_type{forward_mapped<Self>(std::forward<Self>(self), index)}); return result; }
  template <typename Result, typename Self> static constexpr auto to_result(Self&& self) -> Result {
    using value_arg = detail::forward_like_t<Self, mapped_type>;
    if constexpr (detail::frozen_map_array_result<Result, size(), value_arg>) return to_array_result<Result>(std::forward<Self>(self), std::make_index_sequence<size()>{});
    else if constexpr (detail::frozen_map_associative_result<Result, value_arg>) return to_associative_result<Result>(std::forward<Self>(self));
    else static_assert(detail::always_false_v<Result>, "frozen_map::to<Result>() supports std::map, std::unordered_map, and std::array<pair-like, size()> results only");
  }
  std::array<T, size()> values_{};
};

template <typename T, FrozenString... Keys, typename... Entries>
  requires((detail::PairLikeEntry<Entries> && ...) &&
           (std::convertible_to<decltype(detail::pair_like_get<0>(std::declval<Entries>())), std::string_view> && ...) &&
           (std::convertible_to<decltype(detail::pair_like_get<1>(std::declval<Entries>())), T> && ...))
constexpr auto make_frozen_map(Entries&&... entries) -> frozen_map<T, Keys...> {
  static_assert(sizeof...(Keys) == sizeof...(Entries), "make_frozen_map requires exactly one entry per key");
  return frozen_map<T, Keys...>{std::array<frozen_map_entry<T>, sizeof...(Keys)>{
    frozen_map_entry<T>{detail::pair_like_get<0>(std::forward<Entries>(entries)), detail::pair_like_get<1>(std::forward<Entries>(entries))}...
  }};
}

template <typename T, FrozenString... Keys>
constexpr auto make_frozen_map(std::array<T, sizeof...(Keys)> values) -> frozen_map<T, Keys...> {
  return frozen_map<T, Keys...>{std::move(values)};
}

template <std::size_t N, typename V>
struct kv {
  FrozenString<N> key; V value;
  constexpr kv(char const (&k)[N], V v) : key{k}, value{std::move(v)} {}
};
template <std::size_t N, typename V> kv(char const (&)[N], V) -> kv<N, V>;

template <typename T, kv... KVs>
constexpr auto make_frozen_map_kv() -> frozen_map<T, KVs.key...> {
  return frozen_map<T, KVs.key...>{ std::array<T, sizeof...(KVs)>{ KVs.value... } };
}

} // namespace frozenchars

namespace std {

template <typename T>
struct tuple_size<frozenchars::frozen_map_entry<T>> : std::integral_constant<std::size_t, 2> {};

template <std::size_t I, typename T>
struct tuple_element<I, frozenchars::frozen_map_entry<T>> { using type = std::conditional_t<I == 0, std::string_view, T>; };

template <std::size_t N, typename V>
struct tuple_size<frozenchars::kv<N, V>> : std::integral_constant<std::size_t, 2> {};

template <std::size_t I, std::size_t N, typename V>
struct tuple_element<I, frozenchars::kv<N, V>> { using type = std::conditional_t<I == 0, std::string_view, V>; };

} // namespace std
