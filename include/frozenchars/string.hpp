#pragma once

#include "config.hpp"
#include "concepts.hpp"
#include "detail/pipe.hpp"

#include <array>
#include <cstddef>
#include <ostream>
#include <span>
#include <string_view>

namespace frozenchars {

/**
 * @brief 静的文字列
 * このライブラリでの基本的な文字列型
 * @tparam N 文字列の最大長 (終端文字'\0'を含む)
 */
template <size_t N>
struct FrozenString {
  static_assert(N > 0, "FrozenString requires N > 0");
  std::array<char, N> buffer{};  ///< 文字列本体（終端 '\0' 込み、未使用領域はゼロ初期化）
  size_t length = 0;             ///< 終端 '\0' を除く文字列長

  /**
   * @brief デフォルトコンストラクタ
   */
  constexpr FrozenString() = default;

  /**
   * @brief 文字列リテラルからのコンストラクタ
   * @param str 文字列リテラル
   *
   * @note 契約: 入力 `str` の [0, N-1) には終端 '\\0' を含まないこと。
   * 埋め込まれた '\\0' は長さ `length = N-1` には反映されず、buffer にそのままコピーされる。
   * 比較は buffer 全体と length で行うため、埋め込み '\\0' を含むキー検索は想定外。
   */
  constexpr FrozenString(char const (&str)[N]) noexcept {
    for (size_t i = 0; i < N - 1; ++i) {
      buffer[i] = str[i];
    }
    buffer[N - 1] = '\0';
    length = N - 1;
  }

  /**
   * @brief 他の FrozenString からのコンストラクタ
   */
  template <size_t M>
  constexpr FrozenString(FrozenString<M> const& other) noexcept {
    static_assert(M <= N, "FrozenString size mismatch");
    for (size_t i = 0; i < other.length; ++i) {
      buffer[i] = other.buffer[i];
    }
    buffer[other.length] = '\0';
    length = other.length;
  }

  /**
   * @brief string_view を取得する
   * @return std::string_view
   */
  [[nodiscard]] constexpr auto sv() const noexcept -> std::string_view {
    return {buffer.data(), length};
  }

  /**
   * @brief データポインタを取得する
   */
  [[nodiscard]] constexpr auto data() noexcept -> char* {
    return buffer.data();
  }

  /**
   * @brief データポインタを取得する (const)
   */
  [[nodiscard]] constexpr auto data() const noexcept -> char const* {
    return buffer.data();
  }

  /**
   * @brief 文字列長を取得する
   */
  [[nodiscard]] constexpr auto size() const noexcept -> size_t {
    return length;
  }

  /**
   * @brief 文字列が空か判定する
   */
  [[nodiscard]] constexpr auto empty() const noexcept -> bool {
    return length == 0;
  }

  /**
   * @brief 先頭要素を返す
   * @pre !empty()
   */
  [[nodiscard]] constexpr auto front() const noexcept -> char {
    return buffer[0];
  }

  /**
   * @brief 末尾要素を返す
   * @pre !empty()
   */
  [[nodiscard]] constexpr auto back() const noexcept -> char {
    return buffer[length - 1];
  }

  /**
   * @brief 指定インデックスの文字を返す
   * @param i インデックス
   * @pre i < size()
   */
  [[nodiscard]] constexpr auto operator[](size_t i) const noexcept -> char {
    return buffer[i];
  }

  /**
   * @brief 先頭イテレータを返す
   */
  [[nodiscard]] constexpr auto begin() noexcept -> char* {
    return buffer.data();
  }

  /**
   * @brief 末尾イテレータを返す
   */
  [[nodiscard]] constexpr auto end() noexcept -> char* {
    return buffer.data() + length;
  }

  /**
   * @brief 先頭イテレータを返す
   */
  [[nodiscard]] constexpr auto begin() const noexcept -> char const* {
    return buffer.data();
  }

  /**
   * @brief 末尾イテレータを返す
   */
  [[nodiscard]] constexpr auto end() const noexcept -> char const* {
    return buffer.data() + length;
  }

  /**
   * @brief 互換性のための string_view 変換
   */
  [[nodiscard]] constexpr operator std::string_view() const noexcept {
    return sv();
  }

  /**
   * @brief 読み取り専用 span への変換
   */
  [[nodiscard]] constexpr operator std::span<char const>() const noexcept {
    return {buffer.data(), length};
  }

  /**
   * @brief 可変 span への変換
   */
  [[nodiscard]] constexpr operator std::span<char>() noexcept {
    return {buffer.data(), length};
  }

  /**
   * @brief 同一サイズの FrozenString との比較演算子 (hidden friends)
   *
   * buffer 全体と length を比較。未使用バッファ領域は常にゼロ初期化されるため
   * 内容比較と等価でありかつ NTTP 構造等価性・構造順序と完全に一致する。
   */
  [[nodiscard]] friend constexpr auto operator==(FrozenString const& lhs, FrozenString const& rhs) noexcept -> bool {
    return lhs.buffer == rhs.buffer && lhs.length == rhs.length;
  }

  /**
   * @brief 非等値比較
   */
  [[nodiscard]] friend constexpr auto operator!=(FrozenString const& lhs, FrozenString const& rhs) noexcept -> bool {
    return !(lhs == rhs);
  }

  /**
   * @brief 辞書順比較（小なり）
   */
  [[nodiscard]] friend constexpr auto operator<(FrozenString const& lhs, FrozenString const& rhs) noexcept -> bool {
    return lhs.sv() < rhs.sv();
  }

  /**
   * @brief 辞書順比較（以下）
   */
  [[nodiscard]] friend constexpr auto operator<=(FrozenString const& lhs, FrozenString const& rhs) noexcept -> bool {
    return !(rhs < lhs);
  }

  /**
   * @brief 辞書順比較（大なり）
   */
  [[nodiscard]] friend constexpr auto operator>(FrozenString const& lhs, FrozenString const& rhs) noexcept -> bool {
    return rhs < lhs;
  }

  /**
   * @brief 辞書順比較（以上）
   */
  [[nodiscard]] friend constexpr auto operator>=(FrozenString const& lhs, FrozenString const& rhs) noexcept -> bool {
    return !(lhs < rhs);
  }
};

/**
 * @brief FrozenString同士を結合する
 */
template <size_t N1, size_t N2>
constexpr auto operator+(FrozenString<N1> const& lhs, FrozenString<N2> const& rhs) noexcept -> FrozenString<N1 + N2 - 1> {
  auto res = FrozenString<N1 + N2 - 1>{};
  for (size_t i = 0; i < lhs.length; ++i) {
    res.buffer[i] = lhs.buffer[i];
  }
  for (size_t i = 0; i < rhs.length; ++i) {
    res.buffer[lhs.length + i] = rhs.buffer[i];
  }
  res.buffer[lhs.length + rhs.length] = '\0';
  res.length = lhs.length + rhs.length;
  return res;
}

/**
 * @brief FrozenStringと文字列リテラルを結合する
 */
template <size_t N1, size_t N2>
constexpr auto operator+(FrozenString<N1> const& lhs, char const (&rhs)[N2]) noexcept {
  return lhs + FrozenString<N2>{rhs};
}
template <size_t N1, size_t N2>
constexpr auto operator+(char const (&lhs)[N1], FrozenString<N2> const& rhs) noexcept {
  return FrozenString<N1>{lhs} + rhs;
}

/**
 * @brief ostream 出力
 */
template <size_t N>
std::ostream& operator<<(std::ostream& os, FrozenString<N> const& str) {
  return os << str.sv();
}

namespace detail {

/**
 * @brief 型 T が FrozenString か判定するトレイト
 */
template <typename T>
struct is_frozen_string : std::false_type {};

template <std::size_t N>
struct is_frozen_string<FrozenString<N>> : std::true_type {};

/// @brief is_frozen_string の簡易定数版
template <typename T>
inline constexpr bool is_frozen_string_v = is_frozen_string<std::remove_cvref_t<T>>::value;

} // namespace detail

/**
 * @brief 型 T が FrozenString であることを要求するコンセプト（NTTP の requires 節用）
 */
template <typename T>
concept FrozenStringLike = detail::is_frozen_string_v<T>;

/**
 * @brief 文字列リテラルから FrozenString を作成する
 */
template <size_t N>
[[nodiscard]] constexpr auto freeze(char const (&str)[N]) noexcept -> FrozenString<N> {
  return FrozenString<N>{str};
}

} // namespace frozenchars
