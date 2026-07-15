#include "catch2/catch_all.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

/** @brief 型名文字列から C++ 型へのコンパイル時マッピング (type_mapping_v) と、型リストのパース (parse_to_tuple_t / parse_to_variant_t) を検証する。 */
TEST_CASE("type_parser: type_mapping_v basic types") {
  STATIC_CHECK(std::is_same_v<type_mapping_v<"int"_fs>, int>);
  STATIC_CHECK(std::is_same_v<type_mapping_v<"bool"_fs>, bool>);
  STATIC_CHECK(std::is_same_v<type_mapping_v<"double"_fs>, double>);
  STATIC_CHECK(std::is_same_v<type_mapping_v<"string"_fs>, std::string>);
  STATIC_CHECK(std::is_same_v<type_mapping_v<"str"_fs>, std::string>);
  STATIC_CHECK(std::is_same_v<type_mapping_v<"sv"_fs>, std::string_view>);
  STATIC_CHECK(std::is_same_v<type_mapping_v<"uint32"_fs>, std::uint32_t>);
}

TEST_CASE("type_parser: parse_to_tuple_t single type") {
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"int"_fs>, std::tuple<int>>);
}

TEST_CASE("type_parser: parse_to_tuple_t multiple types") {
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"int, string, bool"_fs>, std::tuple<int, std::string, bool>>);
}

TEST_CASE("type_parser: parse_to_tuple_t optional") {
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"int, string?, bool"_fs>, std::tuple<int, std::optional<std::string>, bool>>);
}

TEST_CASE("type_parser: parse_to_tuple_t nested tuple") {
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"[int, bool]"_fs>, std::tuple<std::tuple<int, bool>>>);
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"int, [bool, double]"_fs>, std::tuple<int, std::tuple<bool, double>>>);
}

TEST_CASE("type_parser: parse_to_tuple_t nested optional tuple") {
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"[int, int]?"_fs>, std::tuple<std::optional<std::tuple<int, int>>>>);
}

TEST_CASE("type_parser: parse_to_tuple_t empty element is void") {
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"int,,bool"_fs>, std::tuple<int, void, bool>>);
}

TEST_CASE("type_parser: parse_to_tuple_t pointer and reference suffix") {
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"int*"_fs>, std::tuple<int*>>);
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"int&"_fs>, std::tuple<int&>>);
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"int&&"_fs>, std::tuple<int&&>>);
}

TEST_CASE("type_parser: parse_to_variant_t basic") {
  STATIC_CHECK(std::is_same_v<parse_to_variant_t<"int, string, bool"_fs>, std::variant<int, std::string, bool>>);
}

TEST_CASE("type_parser: parse_to_variant_t void maps to monostate") {
  STATIC_CHECK(std::is_same_v<parse_to_variant_t<"int,,bool"_fs>, std::variant<int, std::monostate, bool>>);
}

TEST_CASE("type_parser: whitespace is ignored") {
  STATIC_CHECK(std::is_same_v<parse_to_tuple_t<"  int ,  bool  "_fs>, std::tuple<int, bool>>);
}
