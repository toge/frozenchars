#pragma once

#include "frozen_string.hpp"

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

namespace frozenchars {

template <typename T>
struct frozen_map_entry;

namespace detail {

inline constexpr auto k_fnv_offset_basis = 14695981039346656037ull;
inline constexpr auto k_fnv_prime = 1099511628211ull;

constexpr auto fnv1a_hash(std::string_view key, std::uint32_t seed) noexcept
    -> std::uint64_t {
  auto hash = k_fnv_offset_basis ^ static_cast<std::uint64_t>(seed);
  for (auto const ch : key) {
    hash ^= static_cast<unsigned char>(ch);
    hash *= k_fnv_prime;
  }
  return hash;
}

template <FrozenString... Keys>
consteval auto has_duplicate_keys() -> bool {
  constexpr std::array key_views{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };

  for (auto i = 0uz; i < key_views.size(); ++i) {
    for (auto j = i + 1; j < key_views.size(); ++j) {
      if (key_views[i] == key_views[j]) {
        return true;
      }
    }
  }
  return false;
}

template <std::uint32_t MaxSeedExclusive, FrozenString... Keys>
consteval auto find_seed() -> std::uint32_t {
  static_assert(!has_duplicate_keys<Keys...>(),
    "frozen_map keys must be unique");
  static_assert(MaxSeedExclusive > 0,
    "frozen_map seed search exhausted");

  constexpr std::array key_views{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };
  constexpr auto key_count = key_views.size();

  for (auto seed = 0u; seed < MaxSeedExclusive; ++seed) {
    std::array<bool, key_count> used_slots{};
    auto collision = false;

    for (auto const key_view : key_views) {
      auto const slot = fnv1a_hash(key_view, seed) % key_count;
      if (used_slots[slot]) {
        collision = true;
        break;
      }
      used_slots[slot] = true;
    }

    if (!collision) {
      return seed;
    }
  }

  throw "frozen_map seed search exhausted";
}

template <typename EntryLike>
concept PairLikeEntry = requires {
  typename std::tuple_size<std::remove_cvref_t<EntryLike>>::type;
  requires std::tuple_size_v<std::remove_cvref_t<EntryLike>> == 2;
};

template <std::size_t Index, typename EntryLike>
constexpr decltype(auto) pair_like_get(EntryLike&& entry) {
  using std::get;
  return get<Index>(std::forward<EntryLike>(entry));
}

template <typename T, typename EntryLike>
concept frozen_map_entry_like =
    PairLikeEntry<EntryLike> &&
    requires(EntryLike&& entry) {
      { pair_like_get<0>(std::forward<EntryLike>(entry)) }
          -> std::convertible_to<std::string_view>;
      { pair_like_get<1>(std::forward<EntryLike>(entry)) }
          -> std::convertible_to<T>;
    };

template <typename T, typename EntryLike>
  requires frozen_map_entry_like<T, EntryLike>
constexpr auto to_frozen_map_entry(EntryLike&& entry) -> frozen_map_entry<T> {
  return frozen_map_entry<T>{
    std::string_view{pair_like_get<0>(std::forward<EntryLike>(entry))},
    T{pair_like_get<1>(std::forward<EntryLike>(entry))}
  };
}

template <typename Result>
inline constexpr auto always_false_v = false;

template <typename Result>
struct is_std_array : std::false_type {
};

template <typename Value, std::size_t N>
struct is_std_array<std::array<Value, N>> : std::true_type {
};

template <typename Result>
inline constexpr auto is_std_array_v =
    is_std_array<std::remove_cvref_t<Result>>::value;

template <typename Result>
struct is_std_map : std::false_type {
};

template <typename Key, typename Mapped, typename Compare, typename Allocator>
struct is_std_map<std::map<Key, Mapped, Compare, Allocator>> : std::true_type {
};

template <typename Result>
inline constexpr auto is_std_map_v =
    is_std_map<std::remove_cvref_t<Result>>::value;

template <typename Result>
struct is_std_unordered_map : std::false_type {
};

template <typename Key, typename Mapped, typename Hash, typename KeyEqual, typename Allocator>
struct is_std_unordered_map<std::unordered_map<Key, Mapped, Hash, KeyEqual, Allocator>>
  : std::true_type {
};

template <typename Result>
inline constexpr auto is_std_unordered_map_v =
    is_std_unordered_map<std::remove_cvref_t<Result>>::value;

template <typename Self, typename T>
using forward_like_t = decltype(std::forward_like<Self>(std::declval<T&>()));

template <typename Result, typename ValueArg>
concept frozen_map_associative_result =
    (is_std_map_v<Result> || is_std_unordered_map_v<Result>) &&
    std::default_initializable<std::remove_cvref_t<Result>> &&
    std::constructible_from<typename std::remove_cvref_t<Result>::key_type, std::string_view> &&
    std::constructible_from<typename std::remove_cvref_t<Result>::mapped_type, ValueArg> &&
    requires(std::remove_cvref_t<Result>& result,
        typename std::remove_cvref_t<Result>::key_type key,
        typename std::remove_cvref_t<Result>::mapped_type value) {
      result.emplace(std::move(key), std::move(value));
    };

template <typename Result, std::size_t ExpectedSize, typename ValueArg>
concept frozen_map_array_result =
    is_std_array_v<Result> &&
    std::tuple_size_v<std::remove_cvref_t<Result>> == ExpectedSize &&
    PairLikeEntry<typename std::remove_cvref_t<Result>::value_type> &&
    std::constructible_from<typename std::remove_cvref_t<Result>::value_type,
        std::string_view,
        ValueArg>;

template <typename Result, std::size_t ExpectedSize, typename ValueArg>
concept frozen_map_result =
    frozen_map_associative_result<Result, ValueArg> ||
    frozen_map_array_result<Result, ExpectedSize, ValueArg>;

} // namespace detail

template <typename T>
struct frozen_map_entry {
  std::string_view key;
  T value;
};

/**
 * @brief コンパイル時キー/値エントリを表す補助型
 * @tparam T 値型
 * @tparam N キー文字列長 (終端 '\0' を含む)
 * @note `make_frozen_map_kv()` / `make_kv_map()` のテンプレート実引数として使う。
 * @note 値もテンプレート実引数として保持するため、主に整数・enum などの
 *       compile-time 値向け。実行時値や move-only 値は `make_frozen_map()` を使う。
 */
template <typename T, std::size_t N>
struct kv {
  using frozen_map_kv_tag = void;
  using mapped_type = T;

  FrozenString<N> key{};
  T value{};

  consteval kv(char const (&key_literal)[N], T value_in) noexcept(
      std::is_nothrow_move_constructible_v<T>)
  : key{key_literal}, value{std::move(value_in)} {
  }
};

template <std::size_t N, typename T>
kv(char const (&)[N], T) -> kv<T, N>;

template <typename T>
struct frozen_map_reference {
  std::string_view key;
  T& value;

  constexpr operator std::pair<std::string_view, T&>() const noexcept {
    return {key, value};
  }

  constexpr operator std::pair<std::string_view, T const&>() const noexcept {
    return {key, value};
  }
};

template <typename T>
struct frozen_map_const_reference {
  std::string_view key;
  T const& value;

  constexpr operator std::pair<std::string_view, T const&>() const noexcept {
    return {key, value};
  }
};

template <std::size_t I, typename T>
constexpr decltype(auto) get(frozen_map_reference<T> ref) noexcept {
  static_assert(I < 2);
  if constexpr (I == 0) {
    return ref.key;
  } else {
    return (ref.value);
  }
}

template <std::size_t I, typename T>
constexpr decltype(auto) get(frozen_map_const_reference<T> ref) noexcept {
  static_assert(I < 2);
  if constexpr (I == 0) {
    return ref.key;
  } else {
    return (ref.value);
  }
}

template <typename T, FrozenString... Keys>
class frozen_map {
 private:
  template <typename Ref>
  struct arrow_proxy_t;

 public:
  /**
   * @note キーはコンパイル時に固定される。バリューは実行時に更新可能。
   *       begin()/end() による反復順はキー宣言順ではなくハッシュスロット順。
   */
  using key_type = std::string_view;
  using mapped_type = T;
  using value_type = std::pair<key_type, mapped_type>;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = frozen_map_reference<mapped_type>;
  using const_reference = frozen_map_const_reference<mapped_type>;

  class const_iterator;

  class iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;
    using value_type = frozen_map::value_type;
    using difference_type = frozen_map::difference_type;
    using reference = frozen_map::reference;
    using pointer = void;

    using arrow_proxy = frozen_map::arrow_proxy_t<frozen_map::reference>;

    constexpr iterator() noexcept = default;

    constexpr auto operator==(iterator const& other) const noexcept -> bool {
      return owner_ == other.owner_ && index_ == other.index_;
    }
    constexpr operator const_iterator() const noexcept;

    /**
     * @brief 現在位置のキーと値を参照する
     * @return reference キーと値への proxy
     */
    constexpr auto operator*() const noexcept -> reference {
      return reference{owner_->slot_key_views_[index_], owner_->values_[index_]};
    }

    /**
     * @brief 現在位置のキーと値へ矢印演算子でアクセスする
     * @return arrow_proxy key/value proxy
     */
    constexpr auto operator->() const noexcept -> arrow_proxy {
      return arrow_proxy{**this};
    }

    /**
     * @brief イテレータを前進させる
     * @return iterator& 更新後のイテレータ
     */
    constexpr auto operator++() noexcept -> iterator& {
      ++index_;
      return *this;
    }

    /**
     * @brief イテレータを前進させる
     * @return iterator 更新前のイテレータ
     */
    constexpr auto operator++(int) noexcept -> iterator {
      auto const current = *this;
      ++(*this);
      return current;
    }

   private:
     friend class frozen_map;
     constexpr iterator(frozen_map* owner, size_type index) noexcept
     : owner_{owner}, index_{index} {
     }

     frozen_map* owner_{nullptr};
     size_type index_{0};
   };

  class const_iterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;
    using value_type = frozen_map::value_type;
    using difference_type = frozen_map::difference_type;
    using reference = frozen_map::const_reference;
    using pointer = void;

    using arrow_proxy = frozen_map::arrow_proxy_t<frozen_map::const_reference>;

    constexpr const_iterator() noexcept = default;

    constexpr auto operator==(const_iterator const& other) const noexcept -> bool {
      return owner_ == other.owner_ && index_ == other.index_;
    }

    /**
     * @brief 現在位置のキーと値を参照する const 版
     * @return const_reference キーと値への proxy
     */
    constexpr auto operator*() const noexcept -> reference {
      return reference{owner_->slot_key_views_[index_], owner_->values_[index_]};
    }

    /**
     * @brief 現在位置のキーと値へ矢印演算子でアクセスする const 版
     * @return arrow_proxy key/value proxy
     */
    constexpr auto operator->() const noexcept -> arrow_proxy {
      return arrow_proxy{**this};
    }

    /**
     * @brief イテレータを前進させる
     * @return const_iterator& 更新後のイテレータ
     */
    constexpr auto operator++() noexcept -> const_iterator& {
      ++index_;
      return *this;
    }

    /**
     * @brief イテレータを前進させる
     * @return const_iterator 更新前のイテレータ
     */
    constexpr auto operator++(int) noexcept -> const_iterator {
      auto const current = *this;
      ++(*this);
      return current;
    }

   private:
     friend class frozen_map;
     friend class iterator;
     constexpr const_iterator(frozen_map const* owner, size_type index) noexcept
     : owner_{owner}, index_{index} {
     }

     frozen_map const* owner_{nullptr};
     size_type index_{0};
   };

  /**
   * @brief 固定キー集合の要素数を返す
   * @return std::size_t キー数
   */
  static_assert(sizeof...(Keys) > 0, "frozen_map requires at least one key");

  [[nodiscard]] static constexpr auto size() noexcept -> size_type {
    return sizeof...(Keys);
  }

  [[nodiscard]] static constexpr auto max_size() noexcept -> size_type {
    return size();
  }

  /**
   * @brief 空コンテナかどうかを返す
   * @return bool 常に false
   */
  [[nodiscard]] static constexpr auto empty() noexcept -> bool {
    return false;
  }

  /**
   * @brief キーに対応する要素を検索する
   * @param key 検索対象キー
   * @return iterator 見つかった要素、未存在なら end()
   */
  [[nodiscard]] constexpr auto find(std::string_view key) noexcept -> iterator {
    if (auto const slot = find_slot(key); slot.has_value()) {
      return iterator{this, *slot};
    }
    return end();
  }

  /**
   * @brief キーに対応する要素を検索する const 版
   * @param key 検索対象キー
   * @return const_iterator 見つかった要素、未存在なら end()
   */
  [[nodiscard]] constexpr auto find(std::string_view key) const noexcept -> const_iterator {
    if (auto const slot = find_slot(key); slot.has_value()) {
      return const_iterator{this, *slot};
    }
    return end();
  }

  /**
   * @brief キーの出現数を返す
   * @param key 検索対象キー
   * @return size_type 0 または 1
   */
  [[nodiscard]] constexpr auto count(std::string_view key) const noexcept -> size_type {
    return find_slot(key).has_value() ? 1uz : 0uz;
  }

  /**
   * @brief 先頭要素を指す iterator を返す
   * @return iterator slot 順先頭
   * @note 反復順序はキー宣言順ではなくハッシュスロット順。
   *       同じキー集合に対しては決定論的だが、宣言順を保証しない。
   */
  constexpr auto begin() noexcept -> iterator {
    return iterator{this, 0};
  }

  /**
   * @brief 終端 iterator を返す
   * @return iterator past-the-end iterator
   */
  constexpr auto end() noexcept -> iterator {
    return iterator{this, size()};
  }

  /**
   * @brief 先頭要素を指す const_iterator を返す
   * @return const_iterator slot 順先頭
   * @note 反復順序はキー宣言順ではなくハッシュスロット順。
   *       同じキー集合に対しては決定論的だが、宣言順を保証しない。
   */
  constexpr auto begin() const noexcept -> const_iterator {
    return const_iterator{this, 0};
  }

  /**
   * @brief 終端 const_iterator を返す
   * @return const_iterator past-the-end iterator
   */
  constexpr auto end() const noexcept -> const_iterator {
    return const_iterator{this, size()};
  }

  /**
   * @brief 先頭要素を指す const_iterator を返す
   * @return const_iterator slot 順先頭
   * @note 反復順序はキー宣言順ではなくハッシュスロット順。
   *       同じキー集合に対しては決定論的だが、宣言順を保証しない。
   */
  constexpr auto cbegin() const noexcept -> const_iterator {
    return begin();
  }

  /**
   * @brief 終端 const_iterator を返す
   * @return const_iterator past-the-end iterator
   */
  constexpr auto cend() const noexcept -> const_iterator {
    return end();
  }

  /**
   * @brief 値をデフォルト構築してマップを初期化する
   */
  constexpr frozen_map() noexcept
    requires std::default_initializable<T> = default;

  /**
   * @brief 宣言順の値配列からマップを初期化する
   * @param values キー宣言順に並んだ値配列
   */
  constexpr explicit frozen_map(std::array<T, size()> values) noexcept(
      std::is_nothrow_move_constructible_v<T>)
  : values_{reorder_to_slots(std::move(values), std::make_index_sequence<size()>{})} {
  }

  /**
   * @brief 宣言順の値リストからマップを初期化する
   * @param values キー宣言順に並んだ値リスト
   * @throw std::invalid_argument 要素数がキー数と一致しない場合
   * @note braced-init-list から自然に書ける ergonomic overload。
   */
  constexpr explicit frozen_map(std::initializer_list<T> values)
    requires std::constructible_from<T, T const&>
  : values_{materialize_staged_values(
      stage_initializer_list_values(values),
      std::make_index_sequence<size()>{})} {
  }

  /**
   * @brief キー付きの値配列からマップを初期化する
   * @param entries キーと値の組を含む配列
   */
  constexpr explicit frozen_map(std::array<frozen_map_entry<T>, size()> entries)
  : values_{materialize_staged_values(
      stage_keyed_entries(std::move(entries)),
      std::make_index_sequence<size()>{})} {
  }

  /**
   * @brief キーが存在するか判定する
   * @param key 検索対象キー
   * @return bool 存在する場合 true
   */
  [[nodiscard]] constexpr auto contains(std::string_view key) const noexcept -> bool {
    return find_slot(key).has_value();
  }

  /**
   * @brief キーに対応する値を返す。未存在なら std::out_of_range を投げる。
   * @param key 検索対象キー
   * @return T& 値参照
   */
  [[nodiscard]] constexpr auto at(std::string_view key) -> T& {
    if (auto const slot = find_slot(key); slot.has_value()) [[likely]] {
      return values_[*slot];
    }
    throw std::out_of_range("frozen_map key not found");
  }

  /**
   * @brief キーに対応する値を返す。未存在なら std::out_of_range を投げる。
   * @param key 検索対象キー
   * @return T const& 値参照
   */
  [[nodiscard]] constexpr auto at(std::string_view key) const -> T const& {
    if (auto const slot = find_slot(key); slot.has_value()) [[likely]] {
      return values_[*slot];
    }
    throw std::out_of_range("frozen_map key not found");
  }

  /**
   * @brief キーに対応する値を返す。未存在なら std::nullopt。
   * @param key 検索対象キー
   * @return std::optional<std::reference_wrapper<T>> 値参照
   */
  [[nodiscard]] constexpr auto get(std::string_view key) noexcept
      -> std::optional<std::reference_wrapper<T>> {
    if (auto const slot = find_slot(key); slot.has_value()) [[likely]] {
      return values_[*slot];
    }
    return std::nullopt;
  }

  /**
   * @brief キーに対応する値を返す。未存在なら std::nullopt。
   * @param key 検索対象キー
   * @return std::optional<std::reference_wrapper<T const>> 値参照
   */
  [[nodiscard]] constexpr auto get(std::string_view key) const noexcept
      -> std::optional<std::reference_wrapper<T const>> {
    if (auto const slot = find_slot(key); slot.has_value()) [[likely]] {
      return values_[*slot];
    }
    return std::nullopt;
  }

  /**
   * @brief at() と同義。std::map との一貫性のためのエイリアス。
   * @param key 検索対象キー
   * @return T& 値参照
   */
  constexpr auto operator[](std::string_view key) -> T& {
    return at(key);
  }

  /**
   * @brief at() と同義。std::map との一貫性のためのエイリアス。
   * @param key 検索対象キー
   * @return T const& 値参照
   */
  constexpr auto operator[](std::string_view key) const -> T const& {
    return at(key);
  }

  template <typename Result>
    requires detail::frozen_map_result<
        Result,
        size(),
        detail::forward_like_t<frozen_map const&, mapped_type>>
  [[nodiscard]] constexpr auto to() const& -> Result {
    return to_result<Result>(*this);
  }

  template <typename Result>
    requires detail::frozen_map_result<
        Result,
        size(),
        detail::forward_like_t<frozen_map&&, mapped_type>>
  [[nodiscard]] constexpr auto to() && -> Result {
    return to_result<Result>(std::move(*this));
  }

 private:
  template <typename Ref>
  struct arrow_proxy_t {
    Ref ref;
    constexpr auto operator->() noexcept -> Ref* {
      return std::addressof(ref);
    }
    constexpr auto operator->() const noexcept -> Ref const* {
      return std::addressof(ref);
    }
  };

  static constexpr std::array<std::string_view, size()> key_views_{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };

  static constexpr auto k_seed = detail::find_seed<1'000'001, Keys...>();

  static constexpr auto slot_for(std::string_view key) noexcept -> std::size_t {
    return detail::fnv1a_hash(key, k_seed) % size();
  }

  static constexpr auto find_slot(std::string_view key) noexcept
      -> std::optional<size_type> {
    auto const slot = slot_for(key);
    if (slot_key_views_[slot] == key) [[likely]] {
      return slot;
    }
    return std::nullopt;
  }

  static consteval auto make_slot_key_views() -> std::array<std::string_view, size()> {
    std::array<std::string_view, size()> slot_key_views{};
    for (auto i = 0uz; i < size(); ++i) {
      slot_key_views[slot_for(key_views_[i])] = key_views_[i];
    }
    return slot_key_views;
  }

  static constexpr auto slot_key_views_ = make_slot_key_views();
  // Iteration order is slot order, not declaration order.

  static consteval auto make_key_order_to_slot() -> std::array<std::size_t, size()> {
    std::array<std::size_t, size()> key_order_to_slot{};
    for (auto i = 0uz; i < size(); ++i) {
      key_order_to_slot[i] = slot_for(key_views_[i]);
    }
    return key_order_to_slot;
  }

  static consteval auto make_slot_to_key_order() -> std::array<std::size_t, size()> {
    auto const key_order_to_slot = make_key_order_to_slot();
    std::array<std::size_t, size()> slot_to_key_order{};
    for (auto i = 0uz; i < size(); ++i) {
      slot_to_key_order[key_order_to_slot[i]] = i;
    }
    return slot_to_key_order;
  }

  template <std::size_t... SlotIndex>
  static constexpr auto reorder_to_slots(
      std::array<T, size()> values,
      std::index_sequence<SlotIndex...>) noexcept(std::is_nothrow_move_constructible_v<T>)
      -> std::array<T, size()> {
    constexpr auto slot_to_key_order = make_slot_to_key_order();
    return {std::move(values[slot_to_key_order[SlotIndex]])...};
  }

  static constexpr auto stage_initializer_list_values(std::initializer_list<T> values)
      -> std::array<std::optional<T>, size()>
    requires std::constructible_from<T, T const&> {
    if (values.size() != size()) {
      throw std::invalid_argument(
        "frozen_map initializer_list size mismatch: expected " +
        std::to_string(size()) +
        " values (one per key), got " +
        std::to_string(values.size())
      );
    }

    constexpr auto key_order_to_slot = make_key_order_to_slot();
    std::array<std::optional<T>, size()> staged{};
    auto index = 0uz;
    for (auto const& value : values) {
      staged[key_order_to_slot[index]].emplace(value);
      ++index;
    }

    return staged;
  }

  static constexpr auto stage_keyed_entries(std::array<frozen_map_entry<T>, size()> entries)
      -> std::array<std::optional<T>, size()> {
    std::array<std::optional<T>, size()> staged{};
    std::array<bool, size()> seen{};

    for (auto& entry : entries) {
      auto const slot = find_slot(entry.key);
      if (!slot.has_value()) {
        throw std::invalid_argument("frozen_map key not found during initialization");
      }
      if (seen[*slot]) {
        throw std::invalid_argument("frozen_map key specified more than once");
      }
      staged[*slot].emplace(std::move(entry.value));
      seen[*slot] = true;
    }

    for (auto const present : seen) {
      if (!present) {
        throw std::invalid_argument("frozen_map requires every key to be initialized");
      }
    }

    return staged;
  }

  template <std::size_t... Index>
  static constexpr auto materialize_staged_values(
      std::array<std::optional<T>, size()> staged,
      std::index_sequence<Index...>) noexcept(std::is_nothrow_move_constructible_v<T>)
      -> std::array<T, size()> {
    return {std::move(*staged[Index])...};
  }

  template <typename Self>
  static constexpr decltype(auto) forward_mapped(Self&& self, size_type index) {
    return std::forward_like<Self>(self.values_[index]);
  }

  template <typename Result, typename Self, std::size_t... Index>
  static constexpr auto to_array_result(
      Self&& self,
      std::index_sequence<Index...>) -> Result {
    return Result{
      typename Result::value_type{
        slot_key_views_[Index],
        forward_mapped<Self>(std::forward<Self>(self), Index)
      }...
    };
  }

  template <typename Result, typename Self>
  static constexpr auto to_associative_result(Self&& self) -> Result {
    Result result{};
    for (auto index = 0uz; index < size(); ++index) {
      std::ignore = result.emplace(
        typename Result::key_type{slot_key_views_[index]},
        typename Result::mapped_type{forward_mapped<Self>(std::forward<Self>(self), index)}
      );
    }
    return result;
  }

  template <typename Result, typename Self>
  static constexpr auto to_result(Self&& self) -> Result {
    using value_arg = detail::forward_like_t<Self, mapped_type>;

    if constexpr (detail::frozen_map_array_result<Result, size(), value_arg>) {
      return to_array_result<Result>(
        std::forward<Self>(self),
        std::make_index_sequence<size()>{}
      );
    } else if constexpr (detail::frozen_map_associative_result<Result, value_arg>) {
      return to_associative_result<Result>(std::forward<Self>(self));
    } else {
      static_assert(
        detail::always_false_v<Result>,
        "frozen_map::to<Result>() supports std::map, std::unordered_map, and "
        "std::array<pair-like, size()> results only"
      );
    }
  }

  std::array<T, size()> values_{};
};

template <typename T, FrozenString... Keys>
constexpr frozen_map<T, Keys...>::iterator::operator
    frozen_map<T, Keys...>::const_iterator() const noexcept {
  return typename frozen_map<T, Keys...>::const_iterator{owner_, index_};
}

/**
 * @brief pair-like なエントリ列から frozen_map を構築する
 * @tparam T 値型
 * @tparam Keys 固定キー列
 * @param entries キーと値の組
 * @return frozen_map<T, Keys...> 初期化済みマップ
 */
template <typename T, FrozenString... Keys, typename... EntryLike>
  requires (detail::frozen_map_entry_like<T, EntryLike> && ...)
constexpr auto make_frozen_map(EntryLike&&... entries) -> frozen_map<T, Keys...> {
  static_assert(sizeof...(EntryLike) == sizeof...(Keys),
    "make_frozen_map requires exactly one entry per key");

  return frozen_map<T, Keys...>{
    std::array<frozen_map_entry<T>, sizeof...(Keys)>{
      detail::to_frozen_map_entry<T>(std::forward<EntryLike>(entries))...
    }
  };
}

namespace detail {

template <typename Entry>
concept frozen_map_kv_like = requires {
  typename std::remove_cvref_t<Entry>::frozen_map_kv_tag;
  typename std::remove_cvref_t<Entry>::mapped_type;
};

} // namespace detail

/**
 * @brief コンパイル時キー/値エントリ列から frozen_map を構築する
 * @tparam T 値型
 * @tparam Entries `kv{"key", value}` 形式の固定エントリ列
 * @return frozen_map<T, Entries.key...> 初期化済みマップ
 * @note `{"key", value}` のような裸の braced-init-list を
 *       テンプレート実引数に置くことは難しいため、`kv{"key", value}` を使う。
 * @note 値も NTTP に載るため、この API は compile-time 値向け。
 *       実行時値・非 structural type・move-only 型は `make_frozen_map()` を使う。
 */
template <typename T, auto... Entries>
  requires ((detail::frozen_map_kv_like<decltype(Entries)> &&
              std::constructible_from<
                  T,
                  typename std::remove_cvref_t<decltype(Entries)>::mapped_type>) && ...)
constexpr auto make_frozen_map_kv() -> frozen_map<T, Entries.key...> {
  return frozen_map<T, Entries.key...>{
    std::array<frozen_map_entry<T>, sizeof...(Entries)>{
      frozen_map_entry<T>{Entries.key.sv(), T{Entries.value}}...
    }
  };
}

} // namespace frozenchars

namespace std {

template <typename T>
struct common_reference<
    frozenchars::frozen_map_reference<T>&&,
    std::pair<std::string_view, T>&> {
  using type = std::pair<std::string_view, T&>;
};

template <typename T>
struct common_reference<
    std::pair<std::string_view, T>&,
    frozenchars::frozen_map_reference<T>&&> {
  using type = std::pair<std::string_view, T&>;
};

template <typename T>
struct common_reference<
    frozenchars::frozen_map_reference<T>&&,
    std::pair<std::string_view, T> const&> {
  using type = std::pair<std::string_view, T const&>;
};

template <typename T>
struct common_reference<
    std::pair<std::string_view, T> const&,
    frozenchars::frozen_map_reference<T>&&> {
  using type = std::pair<std::string_view, T const&>;
};

template <typename T>
struct common_reference<
    frozenchars::frozen_map_const_reference<T>&&,
    std::pair<std::string_view, T>&> {
  using type = std::pair<std::string_view, T const&>;
};

template <typename T>
struct common_reference<
    std::pair<std::string_view, T>&,
    frozenchars::frozen_map_const_reference<T>&&> {
  using type = std::pair<std::string_view, T const&>;
};

template <typename T>
struct common_reference<
    frozenchars::frozen_map_const_reference<T>&&,
    std::pair<std::string_view, T> const&> {
  using type = std::pair<std::string_view, T const&>;
};

template <typename T>
struct common_reference<
    std::pair<std::string_view, T> const&,
    frozenchars::frozen_map_const_reference<T>&&> {
  using type = std::pair<std::string_view, T const&>;
};

template <typename T>
struct tuple_size<frozenchars::frozen_map_reference<T>>
  : integral_constant<std::size_t, 2> {
};

template <typename T>
struct tuple_size<frozenchars::frozen_map_const_reference<T>>
  : integral_constant<std::size_t, 2> {
};

template <typename T>
struct tuple_element<0, frozenchars::frozen_map_reference<T>> {
  using type = std::string_view;
};

template <typename T>
struct tuple_element<1, frozenchars::frozen_map_reference<T>> {
  using type = T&;
};

template <typename T>
struct tuple_element<0, frozenchars::frozen_map_const_reference<T>> {
  using type = std::string_view;
};

template <typename T>
struct tuple_element<1, frozenchars::frozen_map_const_reference<T>> {
  using type = T const&;
};

} // namespace std
