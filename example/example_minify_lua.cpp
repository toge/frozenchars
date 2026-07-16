/// @file example_minify_lua.cpp
/// @brief Lua/Luau minify の実行例

#include <cstdio>

#include "frozenchars.hpp"

int main()
{
  using namespace frozenchars;
  using namespace frozenchars::literals;
  namespace fops = frozenchars::ops;

  constexpr auto src = "--!strict\nlocal x = 1    -- side comment\nlocal y = 2"_fs;

  // 通常（ディレクティブもコメントも除去）
  constexpr auto compact = minify_lua(src);
  std::printf("none     : %s\n", compact.sv().data());

  // ディレクティブ保持
  constexpr auto kept = minify_lua(src, minify_lua_opt::keep_directives);
  std::printf("keep_dir : %s\n", kept.sv().data());

  // パイプ演算子
  constexpr auto piped = src | fops::minify_lua;
  std::printf("pipe     : %s\n", piped.sv().data());
}
