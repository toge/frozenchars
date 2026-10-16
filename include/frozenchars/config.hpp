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
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

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

/**
 * @brief 実行時に起動する前提条件違反の通知。
 *
 * @param msg 文字列リテラル（診断メッセージ）
 */
[[noreturn]] inline void precondition_failed(char const* msg) noexcept {
  std::fputs("frozenchars: precondition failed: ", stderr);
  std::fputs(msg, stderr);
  std::fputc('\n', stderr);
  std::abort();
}
} // namespace frozenchars::detail

#ifdef __cpp_exceptions
#define FROZENCHARS_CONSTEVAL_FAIL(msg) throw(msg)
#else
#define FROZENCHARS_CONSTEVAL_FAIL(msg) ::frozenchars::detail::consteval_fail(msg)
#endif

/**
 * @brief 実行時に必ず作動する前提条件チェック。
 *
 * cond が偽の場合:
 * - 定数評価中: constexpr ではない consteval_fail() を呼ぶためコンパイルエラーになる。
 *   診断には展開後の行（診断メッセージを含む）が出力される。
 * - 実行時: 標準エラーにメッセージを出して std::abort()
 *
 * NDEBUG でも無効化されない点が FROZENCHARS_ASSERT と異なる。operator[] の
 * 未検出キー検出のように、Release ビルドで未定義動作に落入ることを許さない
 * 箇所で使う。
 *
 * @param cond 真となるべき条件式
 * @param msg 文字列リテラル（診断メッセージ）
 */
#ifndef FROZENCHARS_PRECONDITION
#define FROZENCHARS_PRECONDITION(cond, msg)                                                             \
  do {                                                                                                  \
    if (!static_cast<bool>(cond)) [[unlikely]] {                                                        \
      if (std::is_constant_evaluated()) {                                                               \
        ::frozenchars::detail::consteval_fail(msg);                                                    \
      } else {                                                                                          \
        ::frozenchars::detail::precondition_failed(msg);                                               \
      }                                                                                                 \
    }                                                                                                   \
  } while (false)
#endif

/**
 * @brief 実行時・コンパイル時の前提条件チェック（assert 相当）。
 *
 * cond が偽の場合:
 * - 実行時: assert により中止（NDEBUG 定義時は無効化。通常の assert と同様）。
 * - 定数評価中: assert の条件評価が定数式評価に失敗しコンパイルエラー
 *   （診断のソース表示にメッセージが現れる）。
 *
 * @param cond 真となるべき条件式
 * @param msg 文字列リテラル（診断メッセージ）
 */
#ifndef FROZENCHARS_ASSERT
#define FROZENCHARS_ASSERT(cond, msg) assert((cond) && (msg))
#endif

#ifdef FROZENCHARS_WASI_MINIMAL
#error "FROZENCHARS_WASI_MINIMAL was removed: frozenchars is now exception-free by default. See README."
#endif
