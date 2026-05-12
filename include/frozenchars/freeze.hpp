#pragma once

#include "frozen_string.hpp"
#include "detail/number_conv.hpp"
#include "detail/freeze_impl.hpp"
#include <array>
#include <cstddef>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

namespace frozenchars {

/*===============================================================================*\
 * 各種データ型に対応したfreeze関数の定義
\*===============================================================================*/

/*-------------------------------------------------------------------------------*\
 * FrozenString自体の場合はそのまま返す
\*/
template <size_t N>
auto consteval freeze(FrozenString<N> const& arg) noexcept {
  return arg;
}

/*-------------------------------------------------------------------------------*\
 * 文字列リテラル
\*/
template <size_t N>
auto consteval freeze(char const (&arg)[N]) noexcept {
  return FrozenString<N>{arg};
}

/*-------------------------------------------------------------------------------*\
 * 各種C文字列ポインタ
\*/
auto consteval freeze(char const* arg) noexcept {
  return detail::freeze_from_ptr(arg);
}
auto consteval freeze(char* arg) noexcept {
  return freeze(static_cast<char const*>(arg));
}
auto consteval freeze(signed char const* arg) noexcept {
  return detail::freeze_from_ptr(arg);
}
auto consteval freeze(signed char* arg) noexcept {
  return freeze(static_cast<signed char const*>(arg));
}
auto consteval freeze(unsigned char const* arg) noexcept {
  return detail::freeze_from_ptr(arg);
}
auto consteval freeze(unsigned char* arg) noexcept {
  return freeze(static_cast<unsigned char const*>(arg));
}

/*-------------------------------------------------------------------------------*\
 * 各種char型のspan
\*/
template <size_t Extent>
auto consteval freeze(std::span<char const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto consteval freeze(std::span<char, Extent> arg) noexcept {
  return freeze(std::span<char const, Extent>{arg});
}
template <size_t Extent>
auto consteval freeze(std::span<signed char const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto consteval freeze(std::span<signed char, Extent> arg) noexcept {
  return freeze(std::span<signed char const, Extent>{arg});
}
template <size_t Extent>
auto consteval freeze(std::span<unsigned char const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto consteval freeze(std::span<unsigned char, Extent> arg) noexcept {
  return freeze(std::span<unsigned char const, Extent>{arg});
}
template <size_t Extent>
auto consteval freeze(std::span<std::byte const, Extent> arg) noexcept {
  return detail::freeze_from_span(arg);
}
template <size_t Extent>
auto consteval freeze(std::span<std::byte, Extent> arg) noexcept {
  return freeze(std::span<std::byte const, Extent>{arg});
}

/*-------------------------------------------------------------------------------*\
 * 各種char型のarray
\*/
template <size_t N>
auto consteval freeze(std::array<char, N> const& arg) noexcept {
  return freeze(std::span<char const, N>{arg});
}
template <size_t N>
auto consteval freeze(std::array<signed char, N> const& arg) noexcept {
  return freeze(std::span<signed char const, N>{arg});
}
template <size_t N>
auto consteval freeze(std::array<unsigned char, N> const& arg) noexcept {
  return freeze(std::span<unsigned char const, N>{arg});
}
template <size_t N>
auto consteval freeze(std::array<std::byte, N> const& arg) noexcept {
  return freeze(std::span<std::byte const, N>{arg});
}

/*-------------------------------------------------------------------------------*\
 * 各種char型のvector
\*/
template <typename Alloc>
auto consteval freeze(std::vector<char, Alloc>& arg) noexcept {
  return freeze(std::span<char>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<char, Alloc> const& arg) noexcept {
  return freeze(std::span<char const>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<signed char, Alloc>& arg) noexcept {
  return freeze(std::span<signed char>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<signed char, Alloc> const& arg) noexcept {
  return freeze(std::span<signed char const>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<unsigned char, Alloc>& arg) noexcept {
  return freeze(std::span<unsigned char>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<unsigned char, Alloc> const& arg) noexcept {
  return freeze(std::span<unsigned char const>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<std::byte, Alloc>& arg) noexcept {
  return freeze(std::span<std::byte>{arg.data(), arg.size()});
}
template <typename Alloc>
auto consteval freeze(std::vector<std::byte, Alloc> const& arg) noexcept {
  return freeze(std::span<std::byte const>{arg.data(), arg.size()});
}

/*-------------------------------------------------------------------------------*\
 * nullptrは非対応とする
\*/
auto consteval freeze(decltype(nullptr)) noexcept = delete;

/**
 * @brief Binタグを受け取って整数を2進数表現の文字列に変換する
 *
 * @param arg 変換する整数を含む Bin タグ
 * @return auto 変換後の文字列
 */
auto consteval freeze(Bin const& arg) noexcept {
  auto const p = detail::to_bin_chars(arg.value);
  auto res = FrozenString<65>{};
  for (auto i = 0uz; i < p.second; ++i) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

/**
 * @brief Octタグを受け取って整数を8進数表現の文字列に変換する
 *
 * @param arg 変換する整数を含む Oct タグ
 * @return auto 変換後の文字列
 */
auto consteval freeze(Oct const& arg) noexcept {
  auto const p = detail::to_oct_chars(arg.value);
  auto res = FrozenString<23>{};
  for (auto i = 0uz; i < p.second; ++i) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

/**
 * @brief Hexタグを受け取って整数を16進数表現の文字列に変換する
 *
 * @param arg 変換する整数を含む Hex タグ
 * @return auto 変換後の文字列
 */
auto consteval freeze(Hex const& arg) noexcept {
  auto const p = detail::to_hex_chars(arg.value);
  auto res = FrozenString<17>{};
  for (auto i = 0uz; i < p.second; ++i) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

/**
 * @brief Precisionタグを受け取って浮動小数点数を指定精度の文字列に変換する
 *
 * @param arg 変換する浮動小数点数を含む Precision タグ
 * @return auto 変換後の文字列
 */
auto consteval freeze(Precision const& arg) noexcept {
  auto const p = detail::to_float_chars(arg.value, arg.precision);
  auto res = FrozenString<49>{};
  for (auto i = 0uz; i < p.second; ++i) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

/**
 * @brief 整数型を受け取って10進数表現の文字列に変換する
 *
 * @tparam T 整数型
 * @param arg 変換する整数
 * @return auto 変換後の文字列
 */
template <Integral T>
auto consteval freeze(T const& arg) noexcept {
  auto const p = detail::to_dec_chars(static_cast<long long>(arg));
  auto res = FrozenString<21>{};
  for (auto i = 0uz; i < p.second; ++i) {
    res.buffer[i] = p.first[i];
  }
  res.buffer[p.second] = '\0';
  res.length = p.second;
  return res;
}

template <FloatingPoint T>
auto consteval freeze(T const& arg) noexcept {
  return freeze(Precision(arg, 2)); // デフォルト精度は2
}

template <typename T>
  requires (std::is_convertible_v<T, std::string_view>
            && !Integral<std::remove_cvref_t<T>>
            && !FloatingPoint<std::remove_cvref_t<T>>
            && !std::same_as<std::remove_cvref_t<T>, Hex>
            && !std::same_as<std::remove_cvref_t<T>, Precision>)
auto consteval freeze(T const& arg) noexcept {
  auto const s = std::string_view{arg};
  return detail::freeze_from_sv(s);
}

/*-------------------------------------------------------------------------------*\
 * 未対応型はコンパイルエラー
\*/
template <typename T>
auto consteval freeze(T const&) noexcept = delete;

template <size_t N, size_t Count>
constexpr auto const& freeze(std::array<FrozenString<N>, Count> const& arr) noexcept {
  return arr;
}

} // namespace frozenchars
