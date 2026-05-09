#pragma once

#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace frozenchars {

template <size_t N>
struct FrozenString;

/**
 * @brief パイプアダプタのベースクラス
 * ユーザー定義のアダプタはこのクラスを継承する必要があります。
 */
struct pipe_adaptor_base {};

/**
 * @brief パイプアダプタのコンセプト（内部用）
 */
template <typename T>
concept IsPipeAdaptor = std::derived_from<std::remove_cvref_t<T>, pipe_adaptor_base>;

/**
 * @brief パイプアダプタのコンセプト（診断メッセージ付き）
 */
template <typename T>
concept PipeAdaptor = IsPipeAdaptor<T> || []<typename U>() {
  static_assert(IsPipeAdaptor<U>, "指定された型は PipeAdaptor ではありません。pipe_adaptor_base を継承していることを確認してください。");
  return false;
}.template operator()<T>();

namespace detail {

/**
 * @brief 内部用のアダプタタグ
 * 後方互換性のために維持されています。
 */
struct pipe_adaptor_tag : pipe_adaptor_base {};

} // namespace detail

/**
 * @brief 合成されたアダプタ
 */
template <PipeAdaptor... Adaptors>
struct composed_adaptor : pipe_adaptor_base {
  std::tuple<Adaptors...> adaptors;

  consteval explicit composed_adaptor(Adaptors... args) : adaptors(std::move(args)...) {}

  template <size_t N>
  consteval auto operator()(FrozenString<N> const& str) const {
    return apply_impl<0>(str);
  }

private:
  template <size_t I, size_t N>
  consteval auto apply_impl(FrozenString<N> const& str) const {
    if constexpr (I == sizeof...(Adaptors)) {
      return str;
    } else {
      return apply_impl<I + 1>(std::get<I>(adaptors)(str));
    }
  }
};

/**
 * @brief 複数のアダプタを合成して1つのアダプタを作成します。
 * アダプタは左から右の順に適用されます。
 *
 * @param adaptors 合成するアダプタ
 * @return 合成されたアダプタ
 */
template <PipeAdaptor... Adaptors>
consteval auto compose(Adaptors... adaptors) {
  return composed_adaptor<Adaptors...>(std::move(adaptors)...);
}

/**
 * @brief FrozenString に対してアダプタを適用するパイプ演算子
 */
template <size_t N, PipeAdaptor Adaptor>
auto consteval operator|(FrozenString<N> const& lhs, Adaptor const& rhs) noexcept(noexcept(rhs(lhs))) {
  return rhs(lhs);
}

} // namespace frozenchars
