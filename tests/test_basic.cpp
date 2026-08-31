/**
 * @file tests/test_basic.cpp
 * @brief FROZENCHARS_FREESTANDING モードのコア機能検証。
 *
 * I/O ヘッダ非依存のコア API のみを使用し、wasm32-unknown-unknown などの
 * フリースタンディング環境でコンパイルできることを確認する。
 */
#include "frozenchars/string.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

// ---- concat ----
constexpr auto HELLO_WORLD = "hello"_fs + " "_fs + "world"_fs;
static_assert(HELLO_WORLD.sv() == "hello world");
static_assert(HELLO_WORLD.size() == 11);
static_assert(!HELLO_WORLD.empty());

// ---- FrozenString 基本操作 ----
constexpr auto FOO = "foo"_fs;
static_assert(FOO.size() == 3);
static_assert(FOO.data() != nullptr);

// ---- 比較演算子 ----
constexpr auto A = "abc"_fs;
constexpr auto B = "abc"_fs;
constexpr auto C = "xyz"_fs;
static_assert(A == B);
static_assert(A != C);
static_assert(A < C);
static_assert(C > A);

// ---- string_view 変換 ----
static_assert(static_cast<std::string_view>(A) == "abc");

// ---- operator+ (FrozenString + literal) ----
constexpr auto D = "prefix_"_fs + "suffix"_fs;
static_assert(D.sv() == "prefix_suffix");

// ---- freeze (文字列リテラル) ----
constexpr auto E = frozenchars::freeze("frozen");
static_assert(E.sv() == "frozen");
static_assert(E.size() == 6);

int main() { return 0; }
