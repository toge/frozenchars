/**
 * @file test/smoke_wasi_minimal.cpp
 * @brief FROZENCHARS_WASI_MINIMAL モードの検証。
 *
 * -fno-exceptions 付きでビルドされる。FROZENCHARS_THROW を使う全モジュール
 * (数値変換・コンテナ・正規表現・フォーマット・json) が例外なしで
 * コンパイルできることを確認する。
 */
#include "frozenchars.hpp"
#include "frozenchars/json/compress.hpp"
#include "frozenchars/json/crush.hpp"

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

// ---- FROZENCHARS_THROW を含むモジュールが例外なしでコンパイル・評価できる ----
static_assert(*frozenchars::parse_number<int>("42"_fs) == 42);
static_assert(frozenchars::hex_decode("41"_fs).sv() == "A");
constexpr frozenchars::frozen_map<int, "a"_fs, "b"_fs> MAP{1, 2};
static_assert(MAP.at("b")->get() == 2);
static_assert(frozenchars::frozen_regex<"(a|b)c">::keys().size() == 2);
static_assert(frozenchars::frozen_format<"{}-{}"_fs>(1, "x").sv() == "1-x");
static_assert(frozenchars::json::compress<R"({"a":[1,2]})"_fs>().size() > 0);

int main() {
  // 実行時パスも例外なしでリンクできること (見つかるキーのみ使用)
  return MAP.at("a")->get() == 1 ? 0 : 1;
}
