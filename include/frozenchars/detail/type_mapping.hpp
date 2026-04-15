#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>

namespace frozenchars {
template <size_t N> struct FrozenString;
}

namespace frozenchars::detail {

/**
 * @brief 型情報を保持するための単純な構造体
 */
template <typename T>
struct type_identity {
  using type = T;
};

struct unknown_type {};

/**
 * @brief 文字列トークンを対応する型に変換する
 *
 * @tparam S 判定対象の FrozenString
 */
template <auto S>
consteval auto map_string_to_type() {
  auto constexpr s = S.sv();
  if constexpr (s == "bool") return type_identity<bool>{};
  else if constexpr (s == "char") return type_identity<char>{};
  else if constexpr (s == "int") return type_identity<int>{};
  else if constexpr (s == "uint" || s == "unsigned") return type_identity<unsigned int>{};
  else if constexpr (s == "long") return type_identity<long>{};
  else if constexpr (s == "ulong") return type_identity<unsigned long>{};
  else if constexpr (s == "float") return type_identity<float>{};
  else if constexpr (s == "double") return type_identity<double>{};
  else if constexpr (s == "string" || s == "str") return type_identity<std::string>{};
  else if constexpr (s == "string_view" || s == "sv") return type_identity<std::string_view>{};
  else if constexpr (s == "void") return type_identity<void>{};
  else if constexpr (s == "size_t" || s == "sz") return type_identity<std::size_t>{};
  // 固定幅整数
  else if constexpr (s == "int8_t" || s == "int8") return type_identity<std::int8_t>{};
  else if constexpr (s == "int16_t" || s == "int16") return type_identity<std::int16_t>{};
  else if constexpr (s == "int32_t" || s == "int32") return type_identity<std::int32_t>{};
  else if constexpr (s == "int64_t" || s == "int64") return type_identity<std::int64_t>{};
  else if constexpr (s == "uint8_t" || s == "uint8") return type_identity<std::uint8_t>{};
  else if constexpr (s == "uint16_t" || s == "uint16") return type_identity<std::uint16_t>{};
  else if constexpr (s == "uint32_t" || s == "uint32") return type_identity<std::uint32_t>{};
  else if constexpr (s == "uint64_t" || s == "uint64") return type_identity<std::uint64_t>{};
  else return type_identity<unknown_type>{};
}

/**
 * @brief 閉じ括弧 ']' を探す。括弧の深さを考慮する。
 */
template <auto S>
consteval std::size_t find_closing_bracket() {
  auto constexpr sv = S.sv();
  std::size_t depth = 0;
  for (std::size_t i = 0; i < sv.size(); ++i) {
    if (sv[i] == '[') ++depth;
    else if (sv[i] == ']') {
      if (--depth == 0) return i;
    }
  }
  return std::string_view::npos;
}

/**
 * @brief トップレベルのカンマ ',' を探す。括弧の深さを考慮する。
 */
template <auto S>
consteval std::size_t find_top_level_comma() {
  auto constexpr sv = S.sv();
  std::size_t depth = 0;
  for (std::size_t i = 0; i < sv.size(); ++i) {
    if (sv[i] == '[') ++depth;
    else if (sv[i] == ']') --depth;
    else if (sv[i] == ',' && depth == 0) return i;
  }
  return std::string_view::npos;
}

} // namespace frozenchars::detail
