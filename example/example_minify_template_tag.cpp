/// @file example_minify_template_tag.cpp
/// @brief minify_html が Mustache/injamm テンプレートタグ {{ }} を破壊しないことの動作確認
///
/// 修正前は以下の不具合があった:
///   - {{ の直前の空白が原因で { { のようにタグが分割される
///   - }} 直後の空白処理が不整合でタグ内部の空白が落ちる
/// 本プログラムは修正後の正しい挙動を可視化し、期待値と一致することを
/// static_assert + 実行時チェックで確認する。

#include <frozenchars.hpp>
#include <iostream>
#include <string_view>

using namespace frozenchars;
using namespace frozenchars::literals;

/// @brief 入力と期待出力を比較して結果を報告する
constexpr auto expect(std::string_view name, std::string_view got, std::string_view want) noexcept -> bool {
  auto const ok = got == want;
  std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << "\n";
  std::cout << "  in : [" << name << "]\n";
  std::cout << "  out: [" << got << "]\n";
  std::cout << "  want: [" << want << "]\n";
  return ok;
}

int main() {
  auto all_ok = true;

  {
    // ケース1: 前後に改行/空白を含むタグ。内部に空白が混入せず完全な形で残る
    constexpr auto in  = "\n{{#items}}\n  <tr><td>{{id}}</td></tr>\n{{/items}}\n"_fs;
    constexpr auto got = minify_html(in);
    constexpr auto want = "{{#items}}<tr><td>{{id}}{{/items}}"_fs;
    static_assert(got.sv() == want.sv());
    all_ok &= expect("ケース1: 改行前後のタグ", got.sv(), want.sv());
  }

  {
    // ケース2: 属性値内・テキスト内の {{}} が保持される
    constexpr auto in  = "<span class=\"{{cat_color}}\">{{category}}</span>"_fs;
    constexpr auto got = minify_html(in);
    constexpr auto want = "<span class={{cat_color}}>{{category}}</span>"_fs;
    static_assert(got.sv() == want.sv());
    all_ok &= expect("ケース2: 属性値内/テキスト内のタグ", got.sv(), want.sv());
  }

  {
    // ケース3: 前後空白なしのタグは変化しない（回帰）
    constexpr auto in  = "{{#a}}x{{/a}}"_fs;
    constexpr auto got = minify_html(in);
    constexpr auto want = "{{#a}}x{{/a}}"_fs;
    static_assert(got.sv() == want.sv());
    all_ok &= expect("ケース3: 前後空白なし", got.sv(), want.sv());
  }

  {
    // ケース4: タグ内部の空白は透過コピーで保持される
    constexpr auto in  = "a {{ x }} b"_fs;
    constexpr auto got = minify_html(in);
    constexpr auto want = "a{{ x }} b"_fs;
    static_assert(got.sv() == want.sv());
    all_ok &= expect("ケース4: タグ内部の空白保持", got.sv(), want.sv());
  }

  {
    // ケース5: minify_xml も同じ保護区間ロジックで {{}} を保持する
    constexpr auto in  = "<root>\n  {{val}}\n</root>"_fs;
    constexpr auto got = minify_xml(in);
    constexpr auto want = "<root>{{val}}</root>"_fs;
    static_assert(got.sv() == want.sv());
    all_ok &= expect("ケース5: minify_xml のタグ保護", got.sv(), want.sv());
  }

  if (!all_ok) {
    std::cout << "FAIL: 期待挙動と一致しないケースがあります\n";
    return 1;
  }
  std::cout << "ALL PASS: minify がテンプレートタグ {{}} を破壊しないことを確認\n";
  return 0;
}
