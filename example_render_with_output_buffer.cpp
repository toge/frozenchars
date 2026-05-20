#include "frozenchars.hpp"
#include <iostream>
#include <string>

// カスタム出力バッファクラスの例
class StringBuffer {
public:
  void append(std::string_view text) {
    buffer_.append(text);
  }

  [[nodiscard]] auto result() const -> std::string {
    return buffer_;
  }

private:
  std::string buffer_;
};

int main() {
  // テンプレート例
  constexpr auto tmpl = frozenchars::FrozenString{"Hello {{ name }}! Count: {{ count }}"};

  // コンテキスト作成
  auto context = frozenchars::make_template_object({
    {"name", frozenchars::template_value{"World"}},
    {"count", frozenchars::template_value{42.0}},
  });

  // 方法1: 従来の render_template（std::string を返す）
  try {
    auto result1 = frozenchars::render_template<tmpl>(context);
    std::cout << "Method 1 (std::string): " << result1 << '\n';
  } catch (std::exception const& e) {
    std::cerr << "Error in method 1: " << e.what() << '\n';
  }

  // 方法2: カスタム出力バッファを使用（std::expected を返す）
  auto buffer = StringBuffer{};
  auto result2 = frozenchars::render_template<tmpl>(context, buffer);

  if (result2.has_value()) {
    std::cout << "Method 2 (OutputBuffer): " << buffer.result() << '\n';
  } else {
    std::cerr << "Error in method 2: " << result2.error() << '\n';
  }

  // エラーケースのテスト: root がオブジェクトでない場合
  auto non_obj = frozenchars::template_value{123.0};
  auto buffer_err = StringBuffer{};
  auto result_err = frozenchars::render_template<tmpl>(non_obj, buffer_err);

  if (!result_err.has_value()) {
    std::cout << "Expected error: " << result_err.error() << '\n';
  }

  return 0;
}
