#pragma once

#include <concepts>
#include <type_traits>

namespace frozenchars {

/**
 * @brief 整数型を文字列化するためのタグ
 *
 * @tparam T 対象となる型
 */
template <typename T>
concept Integral = std::is_integral_v<T>;

/**
 * @brief 浮動小数点型を文字列化するためのタグ
 *
 * @tparam T 対象となる型
 */
template <typename T>
concept FloatingPoint = std::is_floating_point_v<T>;

/**
 * @brief 数値型（整数または浮動小数点）
 *
 * @tparam T 対象となる型
 */
template <typename T>
concept Numeric = Integral<T> || FloatingPoint<T>;

template <typename T>
concept ParseNumberTarget =
  std::same_as<std::remove_cv_t<T>, int>
  || std::same_as<std::remove_cv_t<T>, long>
  || std::same_as<std::remove_cv_t<T>, long long>
  || std::same_as<std::remove_cv_t<T>, unsigned int>
  || std::same_as<std::remove_cv_t<T>, unsigned long>
  || std::same_as<std::remove_cv_t<T>, unsigned long long>
  || std::same_as<std::remove_cv_t<T>, float>
  || std::same_as<std::remove_cv_t<T>, double>;

/**
 * @brief 整数値を16進数表現するためのタグ
 *
 */
struct Hex {
  long long value;
  constexpr Hex(Integral auto v)
  : value(v)
  {}
};

/**
 * @brief 整数値を2進数表現するためのタグ
 *
 */
struct Bin {
  long long value;
  constexpr Bin(Integral auto v)
  : value(v)
  {}
};

/**
 * @brief 整数値を8進数表現するためのタグ
 *
 */
struct Oct {
  long long value;
  constexpr Oct(Integral auto v)
  : value(v)
  {}
};

/**
 * @brief 浮動小数点数の精度指定用のタグ
 *
 */
struct Precision {
  double value;
  int precision;
  constexpr Precision(FloatingPoint auto v, int p = 2)
  : value(static_cast<double>(v)), precision(p)
  {}
};

} // namespace frozenchars
