#pragma once

#include <compare>
#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace frozenchars::detail {

/**
 * @brief pair-like な参照値に対する operator-> 用プロキシ
 * @details operator* が値（std::pair<std::string_view, T&> など）を返すイテレータで
 *          it->key / it->value のメンバアクセスを提供する。
 * @tparam Ref イテレータの reference 型（.first / .second を持つ）
 */
template <typename Ref>
struct arrow_proxy {
  Ref ref_v;                                    ///< 参照値本体
  decltype(std::declval<Ref>().first)& key;     ///< ref_v.first への参照
  decltype(std::declval<Ref>().second)& value;  ///< ref_v.second への参照
  constexpr arrow_proxy(Ref r) : ref_v(r), key(ref_v.first), value(ref_v.second) {}
  constexpr auto operator->() noexcept -> arrow_proxy* { return this; }
  constexpr auto operator->() const noexcept -> arrow_proxy const* { return this; }
};

/**
 * @brief 添字ベースのランダムアクセスイテレータ
 * @details 位置は添字 1 つで表し、要素の取り出しは Access（呼び出し可能オブジェクト）に委譲する。
 *          Access が空クラスなら状態を持たず添字のみで動作する（frozen_set 系）。
 *          Access が所有側ポインタを持つ場合はそれを経由して値配列を参照する（frozen_map 系）。
 *          比較・距離はすべて添字で行い、Access は関与しない。
 *          reference が真の参照なら pointer は生ポインタ、そうでなければ arrow_proxy を返す。
 * @tparam Value iterator_traits::value_type
 * @tparam Access `Access const{}(std::size_t) -> reference` を提供する呼び出し可能型
 */
template <typename Value, typename Access>
class indexed_iterator {
public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type        = Value;
  using difference_type   = std::ptrdiff_t;
  using size_type         = std::size_t;
  using reference         = std::invoke_result_t<Access const&, size_type>;
  using arrow_proxy       = detail::arrow_proxy<reference>;
  using pointer           = std::conditional_t<std::is_reference_v<reference>,
                                               std::remove_reference_t<reference>*, arrow_proxy>;

  constexpr indexed_iterator() noexcept = default;
  /**
   * @param access 要素取り出しオブジェクト
   * @param index 添字
   */
  constexpr indexed_iterator(Access access, size_type index) noexcept : access_{access}, index_{index} {}
  /**
   * @param index 添字（Access が空クラスの場合のみ）
   */
  constexpr explicit indexed_iterator(size_type index) noexcept requires std::is_empty_v<Access> : index_{index} {}

  constexpr auto operator*() const noexcept -> reference { return access_(index_); }
  constexpr auto operator->() const noexcept -> pointer {
    if constexpr (std::is_reference_v<reference>) {
      return &access_(index_);
    } else {
      return pointer{access_(index_)};
    }
  }
  constexpr auto operator[](difference_type n) const noexcept -> reference {
    return access_(index_ + static_cast<size_type>(n));
  }

  constexpr auto operator++()    noexcept -> indexed_iterator& { ++index_; return *this; }
  constexpr auto operator++(int) noexcept -> indexed_iterator  { auto t = *this; ++index_; return t; }
  constexpr auto operator--()    noexcept -> indexed_iterator& { --index_; return *this; }
  constexpr auto operator--(int) noexcept -> indexed_iterator  { auto t = *this; --index_; return t; }
  constexpr auto operator+=(difference_type n) noexcept -> indexed_iterator& { index_ += static_cast<size_type>(n); return *this; }
  constexpr auto operator-=(difference_type n) noexcept -> indexed_iterator& { index_ -= static_cast<size_type>(n); return *this; }

  friend constexpr auto operator+(indexed_iterator it, difference_type n) noexcept -> indexed_iterator { return it += n; }
  friend constexpr auto operator+(difference_type n, indexed_iterator it) noexcept -> indexed_iterator { return it += n; }
  friend constexpr auto operator-(indexed_iterator it, difference_type n) noexcept -> indexed_iterator { return it -= n; }
  friend constexpr auto operator-(indexed_iterator a, indexed_iterator b) noexcept -> difference_type {
    return static_cast<difference_type>(a.index_) - static_cast<difference_type>(b.index_);
  }

  friend constexpr auto operator==(indexed_iterator const& a, indexed_iterator const& b) noexcept -> bool { return a.index_ == b.index_; }
  friend constexpr auto operator<=>(indexed_iterator const& a, indexed_iterator const& b) noexcept { return a.index_ <=> b.index_; }

private:
  [[no_unique_address]] Access access_{};  ///< 要素取り出しオブジェクト（空クラスならサイズ 0）
  size_type index_{0};                     ///< 現在の添字
};

} // namespace frozenchars::detail
