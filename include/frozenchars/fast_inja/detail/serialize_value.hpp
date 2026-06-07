#pragma once

#include <array>
#include <charconv>
#include <concepts>
#include <string>
#include <string_view>

namespace frozenchars::fast_inja::detail {

/// @brief シリアライズ可能な型の判定
/// @details std::vector<U> などのコンテナ型を serialize_value から除外するガード
template <class T>
inline constexpr bool serializable_v =
    std::integral<T> || std::floating_point<T> ||
    std::same_as<T, std::string> || std::same_as<T, std::string_view>;

/// @brief 整数型（bool除く）をバッファに変換して追記する
template <class Buffer, class T>
  requires std::integral<T> && (!std::same_as<T, bool>)
inline void serialize_value(Buffer& out, T value) {
  std::array<char, 32> buf{};
  auto result = std::to_chars(buf.data(), buf.data() + buf.size(), value);
  out.append(std::string_view{buf.data(), static_cast<std::size_t>(result.ptr - buf.data())});
}

/// @brief bool型をバッファに変換して追記する
template <class Buffer>
inline void serialize_value(Buffer& out, bool b) {
  out.append(b ? std::string_view{"true"} : std::string_view{"false"});
}

/// @brief string_view型をバッファに追記する
template <class Buffer>
inline void serialize_value(Buffer& out, std::string_view s) {
  out.append(s);
}

/// @brief string型をバッファに追記する
template <class Buffer>
inline void serialize_value(Buffer& out, std::string const& s) {
  out.append(std::string_view{s});
}

/// @brief 浮動小数点型をバッファに変換して追記する
template <class Buffer, class T>
  requires std::floating_point<T>
inline void serialize_value(Buffer& out, T value) {
  std::array<char, 64> buf{};
  auto result = std::to_chars(buf.data(), buf.data() + buf.size(), value);
  out.append(std::string_view{buf.data(), static_cast<std::size_t>(result.ptr - buf.data())});
}

} // namespace frozenchars::fast_inja::detail
