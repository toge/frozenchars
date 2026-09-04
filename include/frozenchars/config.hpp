#pragma once

/**
 * @file frozenchars/config.hpp
 * @brief ビルドモード設定。
 *
 * frozenchars は既定で例外なし（glaze流儀）。実行時APIの失敗は
 * `std::expected<T, std::errc>` で返るため、`-fno-exceptions` でビルドできる。
 * コンパイル時評価での不正入力は従来どおりコンパイルエラーになる。
 * 本ライブラリの WASI 対応は wasi-sdk sysroot を用いた wasm32-wasip1 でのビルドを
 * 想定（wasm3 等で実行可能）。
 *
 * 例: clang++ --target=wasm32-wasip1 --sysroot=/opt/wasi-sdk/share/wasi-sysroot
 *       -fno-exceptions -I include -c src.cpp -o src.o
 */
#include <cstdlib>

/**
 * @brief consteval経路の失敗通知。
 *
 * 例外ありでは `throw msg`（不正入力はコンパイルエラーになり診断メッセージが残る）。
 * `-fno-exceptions`（`__cpp_exceptions` 未定義）では非constexprな [[noreturn]]
 * 関数の呼び出しになり、定数評価に触れるとコンパイルエラー、実行時に触れると
 * std::abort() する。
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

#ifdef FROZENCHARS_WASI_MINIMAL
#error "FROZENCHARS_WASI_MINIMAL was removed: frozenchars is now exception-free by default. See README."
#endif
