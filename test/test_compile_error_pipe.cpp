#include <frozenchars.hpp>
#include <frozenchars/detail/pipe.hpp>

// pipe_adaptor_base を継承していない型
struct BadAdaptor {};

int main() {
    // 意図的にコンセプト違反を起こす
    // コンパイルエラーで「指定された型は PipeAdaptor ではありません。...」が出れば成功
    auto result = frozenchars::compose(BadAdaptor{});
    return 0;
}
