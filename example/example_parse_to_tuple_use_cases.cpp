#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

namespace {

void print_section(std::string_view title) {
  std::cout << "\n=== " << title << " ===\n";
}

// 1. 基本的な型パース
// before: using T = std::tuple<int, std::string, bool>;  // 手書き
// after : 文字列で型リストを指定、コンパイル時に tuple へ展開
void use_case_1_basic_parsing() {
  print_section("1. Basic type parsing");

  using T = typename decltype(parse_to_tuple<"int, string, bool"_fs>())::type;
  static_assert(std::is_same_v<T, std::tuple<int, std::string, bool>>);

  std::cout << "  \"int, string, bool\" -> std::tuple<int, std::string, bool>\n";
  std::cout << "  tuple_size = " << std::tuple_size_v<T> << '\n';
}

// 2. optional (? 演算子)
// before: using T = std::tuple<int, std::optional<std::string>>;
// after :末尾に ? を付けるだけ
void use_case_2_optional() {
  print_section("2. Optional types with '?'");

  using T = typename decltype(parse_to_tuple<"int, string?"_fs>())::type;
  static_assert(std::is_same_v<T, std::tuple<int, std::optional<std::string>>>);

  std::cout << "  \"int, string?\" -> std::tuple<int, std::optional<std::string>>\n";

  // 複数の optional
  using T2 = typename decltype(parse_to_tuple<"int?, double?, bool"_fs>())::type;
  static_assert(std::is_same_v<T2, std::tuple<std::optional<int>, std::optional<double>, bool>>);
  std::cout << "  \"int?, double?, bool\" -> std::tuple<std::optional<int>, std::optional<double>, bool>\n";
}

// 3. ネストした tuple ([...] 構文)
// before: using T = std::tuple<int, std::tuple<double, double>>;
// after : [ ... ] で囲むだけ
void use_case_3_nested_tuples() {
  print_section("3. Nested tuples with '[...]'");

  using T = typename decltype(parse_to_tuple<"int, [double, double]"_fs>())::type;
  static_assert(std::is_same_v<T, std::tuple<int, std::tuple<double, double>>>);

  std::cout << "  \"int, [double, double]\" -> std::tuple<int, std::tuple<double, double>>\n";

  // ネスト + optional
  using T2 = typename decltype(parse_to_tuple<"int, [int, int]?"_fs>())::type;
  static_assert(std::is_same_v<T2, std::tuple<int, std::optional<std::tuple<int, int>>>>);
  std::cout << "  \"int, [int, int]?\"     -> std::tuple<int, std::optional<std::tuple<int, int>>>\n";
}

// 4. 型エイリアス定義
// before: using Point3D = std::tuple<double, double, double>;
// after : 文字列から型エイリアスを生成
void use_case_4_type_aliases() {
  print_section("4. Type alias creation");

  using Point2D = typename decltype(parse_to_tuple<"double, double"_fs>())::type;
  using Point3D = typename decltype(parse_to_tuple<"double, double, double"_fs>())::type;
  using Color   = typename decltype(parse_to_tuple<"uint8, uint8, uint8"_fs>())::type;

  static_assert(std::is_same_v<Point2D, std::tuple<double, double>>);
  static_assert(std::is_same_v<Point3D, std::tuple<double, double, double>>);
  static_assert(std::is_same_v<Color, std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>>);

  std::cout << "  Point2D = std::tuple<double, double>\n";
  std::cout << "  Point3D = std::tuple<double, double, double>\n";
  std::cout << "  Color   = std::tuple<uint8, uint8, uint8>\n";
}

// 5. std::variant 型の定義
// before: using MyVariant = std::variant<int, std::string, double>;
// after : parse_to_variant で直接 variant 型を生成
void use_case_5_variant_type() {
  print_section("5. std::variant type definition");

  // parse_to_variant で直接 variant 型を生成
  using V1 = typename decltype(parse_to_variant<"int, string, double"_fs>())::type;
  static_assert(std::is_same_v<V1, std::variant<int, std::string, double>>);
  std::cout << "  \"int, string, double\" -> std::variant<int, std::string, double>\n";

  // _t エイリアスを使った簡潔な書き方
  using V2 = parse_to_variant_t<"int, string, double"_fs>;
  static_assert(std::is_same_v<V2, std::variant<int, std::string, double>>);
  std::cout << "  (with _t alias: same result)\n";

  // optional を含む variant
  using V3 = parse_to_variant_t<"int?, string, bool"_fs>;
  static_assert(std::is_same_v<V3, std::variant<std::optional<int>, std::string, bool>>);
  std::cout << "  \"int?, string, bool\"  -> std::variant<std::optional<int>, std::string, bool>\n";

  // std::visit で variant の要素を走査
  V1 v{42};
  std::visit([](auto&& arg) {
    std::cout << "  variant value: " << arg << '\n';
  }, v);
}

// 6. 関数シグネチャの型定義
// before: void process(int id, std::string name, bool active);
// after : パラメータ型を文字列で定義
void use_case_6_function_signature() {
  print_section("6. Function parameter types");

  using Params = typename decltype(parse_to_tuple<"int, string, bool"_fs>())::type;
  static_assert(std::is_same_v<Params, std::tuple<int, std::string, bool>>);

  // std::apply で tuple の要素を関数引数に展開
  auto process = [](int id, std::string name, bool active) {
    std::cout << "  id=" << id << " name=" << name << " active=" << active << '\n';
  };

  Params params{42, "test", true};
  std::apply(process, params);
}

// 7. 空白・書式の柔軟性
// before: 厳密な书式指定が必要
// after : 空白やスペースを自由に又能
void use_case_7_formatting_flexibility() {
  print_section("7. Formatting flexibility (whitespace ignored)");

  // 空白を含む型文字列でも正しくパースされる
  using T1 = typename decltype(parse_to_tuple<" int , string , bool "_fs>())::type;
  using T2 = typename decltype(parse_to_tuple<"int,string,bool"_fs>())::type;
  using T3 = typename decltype(parse_to_tuple<"int , string , bool"_fs>())::type;

  static_assert(std::is_same_v<T1, std::tuple<int, std::string, bool>>);
  static_assert(std::is_same_v<T2, std::tuple<int, std::string, bool>>);
  static_assert(std::is_same_v<T3, std::tuple<int, std::string, bool>>);

  std::cout << "  \" int , string , bool \" -> OK (whitespace ignored)\n";
  std::cout << "  \"int,string,bool\"      -> OK (no spaces)\n";
  std::cout << "  \"int , string , bool\"  -> OK (mixed)\n";
}

// 8. void 型の扱い
// before: void 型を含む tuple は手書きで注意が必要
// after : 空要素や "void" で明示的に指定可能
void use_case_8_void_type() {
  print_section("8. void type handling");

  // "void" で明示的に指定
  using T1 = typename decltype(parse_to_tuple<"void"_fs>())::type;
  static_assert(std::is_same_v<T1, std::tuple<void>>);

  // 空要素（カンマのみ）は void として扱われる
  using T2 = typename decltype(parse_to_tuple<"int,,bool"_fs>())::type;
  static_assert(std::is_same_v<T2, std::tuple<int, void, bool>>);

  std::cout << "  \"void\"     -> std::tuple<void>\n";
  std::cout << "  \"int,,bool\" -> std::tuple<int, void, bool> (empty = void)\n";
}

// 9. ポインタ/参照型
// before: 手動で型を指定
// after : サフィックスでポインタ/参照型を指定
void use_case_9_pointer_reference() {
  print_section("9. Pointer and reference types");

  // ポインタ型
  using T1 = parse_to_tuple_t<"int*, string*"_fs>;
  static_assert(std::is_same_v<T1, std::tuple<int*, std::string*>>);
  std::cout << "  \"int*, string*\"     -> std::tuple<int*, std::string*>\n";

  // 参照型
  using T2 = parse_to_tuple_t<"int&, string&"_fs>;
  static_assert(std::is_same_v<T2, std::tuple<int&, std::string&>>);
  std::cout << "  \"int&, string&\"     -> std::tuple<int&, std::string&>\n";

  // 右辺値参照型
  using T3 = parse_to_tuple_t<"int&&, string&&"_fs>;
  static_assert(std::is_same_v<T3, std::tuple<int&&, std::string&&>>);
  std::cout << "  \"int&&, string&&\"   -> std::tuple<int&&, std::string&&>\n";

  // 混合
  using T4 = parse_to_tuple_t<"int*, string&, bool&&"_fs>;
  static_assert(std::is_same_v<T4, std::tuple<int*, std::string&, bool&&>>);
  std::cout << "  \"int*, string&, bool&&\" -> std::tuple<int*, std::string&, bool&&>\n";
}

} // namespace

int main() {
  use_case_1_basic_parsing();
  use_case_2_optional();
  use_case_3_nested_tuples();
  use_case_4_type_aliases();
  use_case_5_variant_type();
  use_case_6_function_signature();
  use_case_7_formatting_flexibility();
  use_case_8_void_type();
  use_case_9_pointer_reference();

  std::cout << "\nAll 9 parse_to_tuple use cases demonstrated.\n";
  return 0;
}
