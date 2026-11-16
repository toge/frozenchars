#include "frozenchars/encoding.hpp"
#include "frozenchars/literals.hpp"

using namespace frozenchars::literals;

// \U 表記でもサロゲートは不正なコードポイントとして診断される
constexpr auto invalid_codepoint = frozenchars::unescape_c<"\\U0000D800"_fs>();
