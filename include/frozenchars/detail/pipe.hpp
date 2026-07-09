#pragma once

#include <concepts>
#include <cstddef>
#include <string_view>
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
 * @brief パイプアダプタのコンセプト
 */
template <typename T>
concept PipeAdaptor = std::derived_from<std::remove_cvref_t<T>, pipe_adaptor_base>;

/**
 * @brief 合成されたアダプタ
 */
template <PipeAdaptor... Adaptors>
struct composed_adaptor : pipe_adaptor_base {
  std::tuple<Adaptors...> adaptors;

  consteval explicit composed_adaptor(Adaptors... args) : adaptors(std::move(args)...) {}

  template <size_t N>
  [[nodiscard]] consteval auto operator()(FrozenString<N> const& str) const {
    return apply_impl<0>(str);
  }

private:
  template <size_t I, size_t N>
  [[nodiscard]] consteval auto apply_impl(FrozenString<N> const& str) const {
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
[[nodiscard]] consteval auto compose(Adaptors... adaptors) {
  return composed_adaptor<Adaptors...>(std::move(adaptors)...);
}

/**
 * @brief FrozenString に対してアダプタを適用するパイプ演算子
 */
template <size_t N, PipeAdaptor Adaptor>
[[nodiscard]] auto consteval operator|(FrozenString<N> const& lhs, Adaptor const& rhs) noexcept(noexcept(rhs(lhs))) {
  return rhs(lhs);
}

template <PipeAdaptor Adaptor>
[[nodiscard]] constexpr auto operator|(std::string_view lhs, Adaptor const& rhs) noexcept(noexcept(rhs(lhs))) {
  return rhs(lhs);
}

/**
 * @brief 文字列リテラルに対してアダプタを適用するパイプ演算子
 */
template <size_t N, PipeAdaptor Adaptor>
[[nodiscard]] auto consteval operator|(char const (&lhs)[N], Adaptor const& rhs) noexcept(noexcept(rhs(FrozenString<N>{lhs}))) {
  return rhs(FrozenString<N>{lhs});
}

} // namespace frozenchars
