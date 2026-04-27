#pragma once

#include "frozen_string.hpp"
#include "freeze.hpp"
#include "detail/pipe.hpp"

#if __has_include(<ctre.hpp>)
#include <ctre.hpp>

namespace frozenchars {

namespace ops {

/**
 * @brief CTREの固定文字列をFrozenStringから生成するためのアダプタ
 *
 * CTREの固定文字列はコンパイル時に長さが決まっている必要がある。
 * FrozenStringのbufferサイズN-1と一致する必要があるため、一致しない場合は例外を投げる。
 */
struct to_ctre_t : frozenchars::detail::pipe_adaptor_tag {
  template <size_t N>
  consteval auto operator()(frozenchars::FrozenString<N> const& str) const {
    if (str.length != N - 1) {
      throw "FrozenStringのactual lengthがbufferサイズN-1と一致しません。to_ctre<expr>() を使用してください。";
    }
    return ctll::fixed_string<N - 1>(ctll::construct_from_pointer, str.data());
  }
};

/**
 * @brief CTRE内の固定文字列をFrozenStringから生成するためのアダプタ
 */
inline constexpr to_ctre_t to_ctre{};

} // namespace ops

/**
 * @brief CTREの固定文字列をFrozenStringから生成する
 *
 * CTREの固定文字列はコンパイル時に長さが決まっている必要がある。
 * FrozenStringのbufferサイズN-1と一致する必要がある。
 *
 * @tparam S FrozenStringリテラル
 * @return CTREの固定文字列
 */
template <auto S>
  requires detail::is_frozen_string_v<decltype(S)>
consteval auto to_ctre() noexcept {
  return ctll::fixed_string<S.length>(ctll::construct_from_pointer, S.data());
}

/**
 * @brief CTREのmatch関数をFrozenStringリテラルで呼び出す
 *
 * @tparam Pattern FrozenStringリテラル
 * @param text 検索対象の文字列
 * @return auto CTREのsearch関数の戻り値
 */
template <frozenchars::FrozenString Pattern>
auto ctre_match(auto&& text) noexcept {
  return ctre::match<frozenchars::to_ctre<Pattern>()>(text);
}

/**
 * @brief CTREのsearch関数をFrozenStringリテラルで呼び出す
 *
 * @tparam Pattern FrozenStringリテラル
 * @param text 検索対象の文字列
 * @return auto CTREのsearch関数の戻り値
 */
template <frozenchars::FrozenString Pattern>
auto ctre_search(auto&& text) noexcept {
  return ctre::search<frozenchars::to_ctre<Pattern>()>(text);
}

/**
 * @brief CTREのstarts_with関数をFrozenStringリテラルで呼び出す
 *
 * @tparam Pattern FrozenStringリテラル
 * @param text 検索対象の文字列
 * @return auto CTREのstarts_with関数の戻り値
 */
template <frozenchars::FrozenString Pattern>
auto ctre_starts_with(auto&& text) noexcept {
  return ctre::starts_with<frozenchars::to_ctre<Pattern>()>(text);
}

/**
 * @brief CTREのrange関数をFrozenStringリテラルで呼び出す
 *
 * @tparam Pattern FrozenStringリテラル
 * @param text 検索対象の文字列
 * @return auto CTREのrange関数の戻り値
 */
template <frozenchars::FrozenString Pattern>
auto ctre_range(auto&& text) noexcept {
  return ctre::range<frozenchars::to_ctre<Pattern>()>(text);
}

/**
 * @brief CTREのtokenize関数をFrozenStringリテラルで呼び出す
 *
 * @tparam Pattern FrozenStringリテラル
 * @param text 検索対象の文字列
 * @return auto CTREのtokenize関数の戻り値
 */
template <frozenchars::FrozenString Pattern>
auto ctre_tokenize(auto&& text) noexcept {
  return ctre::tokenize<frozenchars::to_ctre<Pattern>()>(text);
}

/**
 * @brief CTREのsplit関数をFrozenStringリテラルで呼び出す
 *
 * @tparam Pattern FrozenStringリテラル
 * @param text 検索対象の文字列
 * @return auto CTREのsplit関数の戻り値
 */
template <frozenchars::FrozenString Pattern>
auto ctre_split(auto&& text) noexcept {
  return ctre::split<frozenchars::to_ctre<Pattern>()>(text);
}

/**
 * @brief CTREのsearch_all関数をFrozenStringリテラルで呼び出す
 *
 * @tparam Pattern FrozenStringリテラル
 * @param text 検索対象の文字列
 * @return auto CTREのsearch_all関数の戻り値
 */
template <frozenchars::FrozenString Pattern>
auto ctre_search_all(auto&& text) noexcept {
  return ctre::search_all<frozenchars::to_ctre<Pattern>()>(text);
}

} // namespace frozenchars

#endif // __has_include (<ctll/fixed_string.hpp>)
