#pragma once

#include <string_view>

namespace frozenchars::fast_inja::detail {

/// @brief HTMLエスケープをバッファに書き込む
/// @details <, >, &, ', " をHTMLエンティティに変換する
template <class Buffer>
inline void html_escape_into(Buffer& out, std::string_view s) {
  for (auto const c : s) {
    switch (c) {
      case '<':
        out.append("&lt;");
        break;
      case '>':
        out.append("&gt;");
        break;
      case '&':
        out.append("&amp;");
        break;
      case '"':
        out.append("&quot;");
        break;
      case '\'':
        out.append("&#x27;");
        break;
      default:
        out.push_back(c);
        break;
    }
  }
}

} // namespace frozenchars::fast_inja::detail
