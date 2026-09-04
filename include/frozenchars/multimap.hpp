#pragma once

#include <algorithm>
#include <array>
#include <concepts>
#include <cstddef>
#include <expected>
#include <functional>
#include <initializer_list>
#include <optional>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#include "map.hpp"

namespace frozenchars {

namespace detail {

/**
 * @brief frozen_multimap 用の静的インデックス（宣言順キー + ソート済みビュー + 二分探索）
 *
 * frozen_map の lookup_index は重複キーで衝突ゼロのシード探索が破綻するため、
 * マルチマップ専用に宣言順のキー配列とソート済みキー配列だけを持つ。
 *
 * @tparam Keys キー列（FrozenString NTTP、重複可）
 */
template <FrozenString... Keys>
struct multimap_index {
  static constexpr auto size() noexcept -> std::size_t { return sizeof...(Keys); }

  // 宣言順のキービュー（values_ のインデックス空間と一致）
  static constexpr std::array<std::string_view, size()> key_views_{
    std::string_view{Keys.buffer.data(), Keys.length}...
  };

  // 辞書順にソートしたキービュー（lower_bound / upper_bound 用）
  static constexpr std::array<std::string_view, size()> sorted_key_views_ = [] {
    auto res = key_views_;
    std::ranges::sort(res);
    return res;
  }();

  // ソート位置 → 宣言順インデックス（sorted_key_views_[i] は key_views_[sorted_indices_[i]]）
  // 同一キー内は宣言順を保証する全順序（安定ソートと同等）で並べる
  static constexpr std::array<std::size_t, size()> sorted_indices_ = [] {
    std::array<std::size_t, size()> order{};
    for (auto i = 0uz; i < size(); ++i) {
      order[i] = i;
    }
    std::ranges::sort(order, [](auto const a, auto const b) {
      return key_views_[a] == key_views_[b] ? a < b : key_views_[a] < key_views_[b];
    });
    return order;
  }();

  // key 以上の最初のソート位置
  [[nodiscard]] static constexpr auto first_pos(std::string_view key) noexcept -> std::size_t {
    return static_cast<std::size_t>(std::ranges::lower_bound(sorted_key_views_, key) - sorted_key_views_.begin());
  }
  // key より大きい最初のソート位置
  [[nodiscard]] static constexpr auto last_pos(std::string_view key) noexcept -> std::size_t {
    return static_cast<std::size_t>(std::ranges::upper_bound(sorted_key_views_, key) - sorted_key_views_.begin());
  }
};

} // namespace detail

/**
 * @brief 重複キーを許容する、キー固定の連想コンテナ（frozen multimap）
 *
 * キーは FrozenString NTTP で固定。同じキーを複数宣言でき、値は宣言順の配列に格納される。
 * 探索はソート済みキービューへの二分探索で O(log n)、重複キーは [first, last) の範囲で返る。
 * イテレータはソート順（キー昇順、同一キー内は宣言順）で走査する。
 *
 * @tparam T 値の型
 * @tparam Keys キー列（FrozenString NTTP、重複可）
 */
template <typename T, FrozenString... Keys>
class frozen_multimap {
  static_assert(sizeof...(Keys) > 0, "frozen_multimap requires at least one key");

public:
  using key_type = std::string_view;
  using mapped_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using value_type = std::pair<std::string_view const, T>;
  using reference = std::pair<std::string_view, T&>;
  using const_reference = std::pair<std::string_view, T const&>;

  /**
   * @brief イテレータ用の要素取り出し（ソート順のキー + ソート位置→宣言順で引いた値）
   *
   * @tparam Owner 所有側の frozen_multimap 型（const 修飾込み）
   * @tparam Ref 参照の値型（pair-like）
   */
  template <typename Owner, typename Ref>
  struct entry_access {
    Owner* owner{};
    constexpr auto operator()(size_type i) const noexcept -> Ref {
      return Ref{lookup_::sorted_key_views_[i], owner->values_[lookup_::sorted_indices_[i]]};
    }
  };

  using iterator = detail::indexed_iterator<value_type, entry_access<frozen_multimap, reference>>;
  using const_iterator = detail::indexed_iterator<value_type, entry_access<frozen_multimap const, const_reference>>;

  /** @brief キーの総数を返す（重複を含む） */
  static constexpr auto size() noexcept -> size_type { return sizeof...(Keys); }
  /** @brief 格納可能な最大要素数（size() と同じ） */
  static constexpr auto max_size() noexcept -> size_type { return size(); }
  /** @brief 常に false（frozen_multimap は少なくとも1つのキーを要求する） */
  [[nodiscard]] static constexpr auto empty() noexcept -> bool { return false; }
  /**
   * @brief 辞書順にソートされたキー配列（重複を含む）を取得する
   */
  [[nodiscard]] static constexpr auto keys() noexcept -> std::span<const std::string_view, size()> {
    return lookup_::sorted_key_views_;
  }
  /**
   * @brief 宣言順のキー配列を取得する
   */
  [[nodiscard]] static constexpr auto keys_in_declaration_order() noexcept
      -> std::span<std::string_view const, size()> {
    return lookup_::key_views_;
  }
  /**
   * @brief キーに対応する要素数を返す（重複を加算）
   *
   * @param key 調べるキー
   * @return size_type キーに対応する要素数
   */
  [[nodiscard]] constexpr auto count(std::string_view key) const noexcept -> size_type {
    return lookup_::last_pos(key) - lookup_::first_pos(key);
  }
  /**
   * @brief キーが存在するかを判定する
   *
   * @param key 調べるキー
   * @return bool 存在すれば true
   */
  [[nodiscard]] constexpr auto contains(std::string_view key) const noexcept -> bool {
    return std::ranges::binary_search(lookup_::sorted_key_views_, key);
  }
  /**
   * @brief 複数のキーが全て存在するかを一括判定する (consteval)
   * @details 空パックに対しては true (vacuous truth) を返す。
   */
  template <FrozenString... QueryKeys>
  [[nodiscard]] static consteval auto contains_all() noexcept -> bool {
    return ((contains_impl<QueryKeys>()) && ... && true);
  }
  /**
   * @brief キーに対応する最初の要素を探索する
   *
   * @param key 探索するキー
   * @return iterator 見つかった要素、未検出は end()
   */
  [[nodiscard]] constexpr auto find(std::string_view key) noexcept -> iterator {
    auto const pos = lookup_::first_pos(key);
    return pos < size() && lookup_::sorted_key_views_[pos] == key ? iterator{{this}, pos} : end();
  }
  /**
   * @brief キーに対応する最初の要素を探索する（const 版）
   *
   * @param key 探索するキー
   * @return const_iterator 見つかった要素、未検出は end()
   */
  [[nodiscard]] constexpr auto find(std::string_view key) const noexcept -> const_iterator {
    auto const pos = lookup_::first_pos(key);
    return pos < size() && lookup_::sorted_key_views_[pos] == key ? const_iterator{{this}, pos} : end();
  }
  /**
   * @brief キーに対応する要素範囲 [first, last) を返す
   *
   * @param key 探索するキー
   * @return std::pair<iterator, iterator> 要素範囲、未検出は [end(), end())
   */
  [[nodiscard]] constexpr auto equal_range(std::string_view key) noexcept -> std::pair<iterator, iterator> {
    auto const first = lookup_::first_pos(key);
    auto const last = lookup_::last_pos(key);
    if (first < size() && lookup_::sorted_key_views_[first] == key) {
      return {iterator{{this}, first}, iterator{{this}, last}};
    }
    return {end(), end()};
  }
  /**
   * @brief キーに対応する要素範囲 [first, last) を返す（const 版）
   *
   * @param key 探索するキー
   * @return std::pair<const_iterator, const_iterator> 要素範囲、未検出は [end(), end())
   */
  [[nodiscard]] constexpr auto equal_range(std::string_view key) const noexcept
      -> std::pair<const_iterator, const_iterator> {
    auto const first = lookup_::first_pos(key);
    auto const last = lookup_::last_pos(key);
    if (first < size() && lookup_::sorted_key_views_[first] == key) {
      return {const_iterator{{this}, first}, const_iterator{{this}, last}};
    }
    return {end(), end()};
  }
  /**
   * @brief キーに対応する最初の値への参照を取得する（未検出は expected で報告）
   *
   * 重複キーの場合はソート順で最初に現れる値を返す。
   *
   * @param key 探索するキー
   * @return std::expected<std::reference_wrapper<T>, std::errc> 最初の値への参照。未検出は std::errc::invalid_argument
   */
  [[nodiscard]] constexpr auto at(std::string_view key) noexcept
    -> std::expected<std::reference_wrapper<T>, std::errc> {
    auto const pos = lookup_::first_pos(key);
    if (pos < size() && lookup_::sorted_key_views_[pos] == key) [[likely]] {
      return std::ref(values_[lookup_::sorted_indices_[pos]]);
    }
    return std::unexpected(std::errc::invalid_argument);
  }
  /**
   * @brief キーに対応する最初の値への参照を取得する（const 版、未検出は expected で報告）
   *
   * @param key 探索するキー
   * @return std::expected<std::reference_wrapper<T const>, std::errc> 最初の値への参照。未検出は std::errc::invalid_argument
   */
  [[nodiscard]] constexpr auto at(std::string_view key) const noexcept
    -> std::expected<std::reference_wrapper<T const>, std::errc> {
    auto const pos = lookup_::first_pos(key);
    if (pos < size() && lookup_::sorted_key_views_[pos] == key) [[likely]] {
      return std::cref(values_[lookup_::sorted_indices_[pos]]);
    }
    return std::unexpected(std::errc::invalid_argument);
  }
  /// @brief 先頭イテレータを返す（ソート順）
  constexpr auto begin() noexcept -> iterator { return iterator{{this}, 0}; }
  /// @brief 末尾イテレータを返す
  constexpr auto end() noexcept -> iterator { return iterator{{this}, size()}; }
  /// @brief 先頭イテレータを返す（const 版）
  constexpr auto begin() const noexcept -> const_iterator { return const_iterator{{this}, 0}; }
  /// @brief 末尾イテレータを返す（const 版）
  constexpr auto end() const noexcept -> const_iterator { return const_iterator{{this}, size()}; }
  /// @brief 先頭イテレータを返す（const_iterator）
  constexpr auto cbegin() const noexcept -> const_iterator { return begin(); }
  /// @brief 末尾イテレータを返す（const_iterator）
  constexpr auto cend() const noexcept -> const_iterator { return end(); }

  /**
   * @brief デフォルトコンストラクタ（値型がデフォルト構築可能な場合のみ）
   */
  constexpr frozen_multimap() noexcept requires std::default_initializable<T> = default;
  /**
   * @brief 値配列から構築する（宣言順に対応）
   * @param values 宣言順に対応する値の配列
   */
  constexpr explicit frozen_multimap(std::array<T, size()> values) noexcept(std::is_nothrow_move_constructible_v<T>) : values_{std::move(values)} {}
  /**
   * @brief 初期化リストから構築する（要素数はキー数と一致が必要）
   * @param values 宣言順に対応する値の初期化リスト
   */
  constexpr explicit frozen_multimap(std::initializer_list<T> values) requires std::constructible_from<T, T const&> : values_{copy_initializer_list(values)} {}
  /**
   * @brief キー・値エントリ配列から構築する（重複キーは宣言順スロットへ割り当て、欠落キーは例外）
   * @param entries キー・値ペアの配列
   */
  constexpr explicit frozen_multimap(std::array<frozen_map_entry<T>, size()> entries) : values_{reorder_entries(std::move(entries))} {}

  /**
   * @brief 初期化リストから構築する（要素数はキー数と一致が必要）
   * @param values 宣言順に対応する値の初期化リスト
   * @return std::expected<frozen_multimap, std::errc> 構築結果。要素数不一致は std::errc::invalid_argument
   */
  static constexpr auto try_make(std::initializer_list<T> values) noexcept(
    std::is_nothrow_copy_constructible_v<T>&& std::is_nothrow_move_constructible_v<T>)
    -> std::expected<frozen_multimap, std::errc> requires std::constructible_from<T, T const&> {
    if (values.size() != size()) return std::unexpected(std::errc::invalid_argument);
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return frozen_multimap{std::array<T, size()>{*(values.begin() + I)...}};
    }(std::make_index_sequence<size()>{});
  }
  /**
   * @brief キー・値エントリ配列から構築する（重複キーは宣言順スロットへ割り当て）
   * @param entries キー・値ペアの配列
   * @return std::expected<frozen_multimap, std::errc> 構築結果。未知キー・欠落は std::errc::invalid_argument
   */
  static constexpr auto try_make(std::array<frozen_map_entry<T>, size()> entries) noexcept(
    std::is_nothrow_move_constructible_v<T>)
    -> std::expected<frozen_multimap, std::errc> {
    auto values = std::array<std::optional<T>, size()>{};
    for (auto& entry : entries) {
      auto placed = false;
      for (auto slot = 0uz; slot < size(); ++slot) {
        if (values[slot].has_value()) {
          continue;
        }
        if (lookup_::key_views_[slot] == entry.key) {
          values[slot].emplace(std::move(entry.value));
          placed = true;
          break;
        }
      }
      if (!placed) {
        return std::unexpected(std::errc::invalid_argument);
      }
    }
    for (auto const& slot : values) {
      if (!slot.has_value()) {
        return std::unexpected(std::errc::invalid_argument);
      }
    }
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return frozen_multimap{std::array<T, size()>{std::move(*values[I])...}};
    }(std::make_index_sequence<size()>{});
  }

private:
  using lookup_ = detail::multimap_index<Keys...>;

  /**
   * @brief 単一 NTTP キーがマップ内に存在するか (contains_all 内部用)
   */
  template <FrozenString Key>
  [[nodiscard]] static consteval auto contains_impl() noexcept -> bool {
    return ((Key.sv() == std::string_view{Keys.buffer.data(), Keys.length}) || ... || false);
  }

  // 初期化リストから値配列を構築（要素数検証付き）
  static constexpr auto copy_initializer_list(std::initializer_list<T> values) -> std::array<T, size()> requires std::constructible_from<T, T const&> {
    if (values.size() != size()) FROZENCHARS_CONSTEVAL_FAIL("frozen_multimap size mismatch: expected one value per key");
    return [&]<std::size_t... I>(std::index_sequence<I...>) { return std::array<T, size()>{ *(values.begin() + I)... }; }(std::make_index_sequence<size()>{});
  }
  // エントリ配列をキー一致の未使用宣言順スロットへ順次配置（重複キーは宣言順に割り当て、欠落キーは例外）
  static constexpr auto reorder_entries(std::array<frozen_map_entry<T>, size()> entries) -> std::array<T, size()> {
    auto values = std::array<std::optional<T>, size()>{};
    for (auto& entry : entries) {
      for (auto slot = 0uz; slot < size(); ++slot) {
        if (values[slot].has_value()) {
          continue;
        }
        if (lookup_::key_views_[slot] == entry.key) {
          values[slot].emplace(std::move(entry.value));
          break;
        }
      }
    }
    for (auto const& slot : values) {
      if (!slot.has_value()) {
        FROZENCHARS_CONSTEVAL_FAIL("frozen_multimap missing key");
      }
    }
    return [&]<std::size_t... I>(std::index_sequence<I...>) {
      return std::array<T, size()>{ std::move(*values[I])... };
    }(std::make_index_sequence<size()>{});
  }

  std::array<T, size()> values_{};  // 宣言順の値の格納（インデックスは key_views_ と一致）
};

/**
 * @brief pair-like 要素群から frozen_multimap を構築する
 *
 * @tparam T 値の型
 * @tparam Keys キー列（FrozenString NTTP、重複可）
 * @tparam Entries 要素の型パラメータパック
 * @param entries キー・値ペアの要素群
 * @return frozen_multimap<T, Keys...> 構築されたマルチマップ
 */
template <typename T, FrozenString... Keys, typename... Entries>
  requires((detail::PairLikeEntry<Entries> && ...) &&
           (std::convertible_to<decltype(detail::pair_like_get<0>(std::declval<Entries>())), std::string_view> && ...) &&
           (std::convertible_to<decltype(detail::pair_like_get<1>(std::declval<Entries>())), T> && ...))
constexpr auto make_frozen_multimap(Entries&&... entries) -> frozen_multimap<T, Keys...> {
  static_assert(sizeof...(Keys) == sizeof...(Entries), "make_frozen_multimap requires exactly one entry per key");
  auto arr = std::array<frozen_map_entry<T>, sizeof...(Keys)>{
    frozen_map_entry<T>{detail::pair_like_get<0>(std::forward<Entries>(entries)), detail::pair_like_get<1>(std::forward<Entries>(entries))}...
  };
  return frozen_multimap<T, Keys...>{std::move(arr)};
}

/**
 * @brief 値配列から frozen_multimap を構築する
 *
 * @tparam T 値の型
 * @tparam Keys キー列（FrozenString NTTP、重複可）
 * @param values 宣言順に対応する値の配列
 * @return frozen_multimap<T, Keys...> 構築されたマルチマップ
 */
template <typename T, FrozenString... Keys>
constexpr auto make_frozen_multimap(std::array<T, sizeof...(Keys)> values) -> frozen_multimap<T, Keys...> {
  return frozen_multimap<T, Keys...>{std::move(values)};
}

} // namespace frozenchars
