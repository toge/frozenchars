#pragma once

/**
 * @file frozenchars/config.hpp
 * @brief ビルドモード設定。
 *
 * FROZENCHARS_WASI_MINIMAL が定義されると、ライブラリ内の全ての例外送出
 * (FROZENCHARS_THROW) が std::abort() に置き換わり、-fno-exceptions でも
 * ビルドできる「例外なしモード」になる。コンパイル時評価での不正入力は
 * 従来どおりコンパイルエラーになる。wasm32-wasip1 / wasm32-emscripten は
 * WASI/hosted とみなすため自動では有効にならず、WASI 上で
 * 最小構成を検証する場合は手動で `-DFROZENCHARS_WASI_MINIMAL` を指定する。
 * 本ライブラリの WASI 対応は wasi-sdk sysroot を用いた wasm32-wasip1 でのビルドを
 * 想定（wasm3 等で実行可能）。`<iostream>` は wasip1/wasip2 では WASI 経由で
 * 利用可能なため無効化しない。
 *
 * 例: clang++ --target=wasm32-wasip1 --sysroot=/opt/wasi-sdk/share/wasi-sysroot
 *       -fno-exceptions -DFROZENCHARS_WASI_MINIMAL=1 -I include -c src.cpp -o src.o
 */
#if !defined(FROZENCHARS_WASI_MINIMAL) && defined(__wasm__) && !defined(__wasi__) && !defined(__EMSCRIPTEN__)
#  define FROZENCHARS_WASI_MINIMAL 1
#endif

#include <cstdlib>

/**
 * @brief consteval経路の失敗通知。
 *
 * 例外ありでは `throw msg`（従来どおり、不正入力はコンパイルエラーになり
 * 診断メッセージが残る）。`-fno-exceptions`（`__cpp_exceptions` 未定義）では
 * 非constexprな [[noreturn]] 関数の呼び出しになり、定数評価に触れると
 * コンパイルエラー、実行時に触れると std::abort() する。
 */
namespace frozenchars::detail {
[[noreturn]] inline void consteval_fail(char const* msg) noexcept {
  (void)msg;
  std::abort();
}
} // namespace frozenchars::detail

#ifdef __cpp_exceptions
#define FROZENCHARS_CONSTEVAL_FAIL(msg) throw(msg)
#else
#define FROZENCHARS_CONSTEVAL_FAIL(msg) ::frozenchars::detail::consteval_fail(msg)
#endif

/**
 * @brief 例外送出の統一マクロ。
 *
 * hosted (既定) では `throw expr` に展開する。FROZENCHARS_WASI_MINIMAL 定義時は
 * expr を評価せず `detail::fail()` を呼ぶ。fail() は非 constexpr のため
 * コンパイル時評価では従来どおりコンパイルエラーになり、実行時は std::abort() する。
 * これにより -fno-exceptions でもライブラリ全体がビルドできる。
 */
#ifndef FROZENCHARS_WASI_MINIMAL
#  include <stdexcept>
#  define FROZENCHARS_THROW(expr) throw expr
#else
#  include <cstdlib>
namespace frozenchars::detail {
[[noreturn]] inline void fail() noexcept { std::abort(); }
} // namespace frozenchars::detail
#  define FROZENCHARS_THROW(expr) ::frozenchars::detail::fail()
#endif
