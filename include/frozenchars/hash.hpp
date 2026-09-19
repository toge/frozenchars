#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

#include "string.hpp"

namespace frozenchars {

namespace detail {

/**
 * @brief FNV-1a のビット幅ごとのパラメータ
 *
 * @tparam BitWidth ハッシュ値のビット幅（32 または 64）
 */
template <std::size_t BitWidth>
struct fnv1a_params;

template <>
struct fnv1a_params<32> {
  static constexpr std::uint32_t OFFSET_BASIS = 2166136261u;  ///< 32bit の offset basis
  static constexpr std::uint32_t PRIME = 16777619u;           ///< 32bit の FNV prime
};

template <>
struct fnv1a_params<64> {
  static constexpr std::uint64_t OFFSET_BASIS = 14695981039346656037ull;  ///< 64bit の offset basis
  static constexpr std::uint64_t PRIME = 1099511628211ull;                ///< 64bit の FNV prime
};

/**
 * @brief ハッシュ値の型から FNV-1a パラメータを選択する
 *
 * @tparam SizeT ハッシュ値の型
 */
template <typename SizeT>
struct fnv1a_constants : fnv1a_params<sizeof(SizeT) * 8> {};

} // namespace detail

/**
 * @brief FNV-1a ハッシュを計算する
 *
 * コンパイル時・実行時の両方で同じ値になる。文字列スイッチの case ラベルや、
 * 複数フィールドをまとめた複合キーに利用できる。
 *
 * @tparam SizeT ハッシュ値の型（既定: std::size_t。std::uint32_t で 32bit 版になる）
 * @param str 対象文字列
 * @return SizeT ハッシュ値
 */
template <typename SizeT = std::size_t>
[[nodiscard]] constexpr auto fnv1a(std::string_view const str) noexcept -> SizeT {
  using constants = detail::fnv1a_constants<SizeT>;

  auto hash = static_cast<SizeT>(constants::OFFSET_BASIS);
  for (auto const c : str) {
    // 符号付き char でも一貫した値になるよう unsigned char を経由する
    hash ^= static_cast<SizeT>(static_cast<unsigned char>(c));
    hash *= constants::PRIME;
  }
  return hash;
}

/**
 * @brief FrozenString の FNV-1a ハッシュを計算する
 *
 * @tparam N 文字列の長さ (終端文字'\0'を含む)
 * @param str 対象文字列
 * @return std::size_t ハッシュ値
 */
template <size_t N>
[[nodiscard]] constexpr auto fnv1a(FrozenString<N> const& str) noexcept -> std::size_t {
  return fnv1a(str.sv());
}

/**
 * @brief 文字列リテラル / FrozenString のコンパイル時ハッシュ値
 *
 * @tparam S 対象の FrozenString（NTTP）
 *
 * @code
 * static_assert(hash_v<"hello"> == fnv1a("hello"));
 * @endcode
 */
template <FrozenString S>
inline constexpr std::size_t hash_v = fnv1a(S.sv());

/**
 * @brief constexpr 文脈で使える std::hash 互換のハッシュ関数オブジェクト
 *
 * @tparam T ハッシュ対象の型（fnv1a にそのまま渡せること）
 */
template <typename T>
struct constexpr_hash {
  /**
   * @brief ハッシュ値を計算する
   *
   * @param value 対象の値
   * @return std::size_t ハッシュ値
   */
  [[nodiscard]] constexpr auto operator()(T const& value) const noexcept -> std::size_t {
    return fnv1a(value);
  }
};

/**
 * @brief 2 つのハッシュ値を結合する
 *
 * @param h1 1 つ目のハッシュ値
 * @param h2 2 つ目のハッシュ値
 * @return std::size_t 結合後のハッシュ値
 */
[[nodiscard]] constexpr auto hash_combine(std::size_t const h1, std::size_t const h2) noexcept -> std::size_t {
  // Boost 流の結合（黄金比由来の定数を加えて撹拌する）
  return h1 ^ (h2 + 0x9e3779b9uz + (h1 << 6) + (h1 >> 2));
}

/**
 * @brief 3 つ以上のハッシュ値を左から順に結合する
 *
 * @tparam Rest 残りのハッシュ値の型
 * @param h1 1 つ目のハッシュ値
 * @param h2 2 つ目のハッシュ値
 * @param rest 3 つ目以降のハッシュ値
 * @return std::size_t 結合後のハッシュ値
 */
template <typename... Rest>
[[nodiscard]] constexpr auto hash_combine(std::size_t const h1, std::size_t const h2, Rest... rest) noexcept -> std::size_t {
  return hash_combine(hash_combine(h1, h2), rest...);
}

} // namespace frozenchars

namespace frozenchars::literals {

/**
 * @brief 文字列リテラルのコンパイル時 FNV-1a ハッシュ値を得る
 *
 * @param str 対象の文字列リテラル
 * @param len 文字数
 * @return std::size_t ハッシュ値
 *
 * @code
 * switch (fnv1a(runtime_string)) {
 *   case "GET"_hash:  ...
 *   case "POST"_hash: ...
 * }
 * @endcode
 */
[[nodiscard]] auto consteval operator""_hash(char const* str, std::size_t const len) noexcept -> std::size_t {
  return frozenchars::fnv1a(std::string_view{str, len});
}

} // namespace frozenchars::literals
