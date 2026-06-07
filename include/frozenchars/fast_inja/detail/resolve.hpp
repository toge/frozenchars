#pragma once

#include "glz_dispatch.hpp"
#include "loop_state.hpp"
#include "serialize_value.hpp"
#include <string_view>

namespace frozenchars::fast_inja::detail {

/// @brief ネストパスの1ステップを解決する前方宣言
template <class Buffer, class T>
inline bool resolve_path_step(Buffer& out, std::string_view first, std::string_view rest,
                               T const& value, loop_state const* loop);

/// @brief キーを解決してバッファに書き込む（トップレベルエントリ）
/// @param out 出力バッファ
/// @param key フィールド名（@prefix, ネストパス, フラットいずれか）
/// @param value コンテキスト値
/// @param loop ループコンテキスト（ない場合は nullptr）
/// @return キーが見つかった場合 true
template <class Buffer, class T>
inline bool resolve_value(Buffer& out, std::string_view key, T const& value, loop_state const* loop) {
  // @prefix
  if (!key.empty() && key[0] == '@') {
    if (!loop) {
      return false;
    }
    if (key == "@index") {
      serialize_value(out, loop->index);
      return true;
    }
    if (key == "@first") {
      serialize_value(out, loop->is_first());
      return true;
    }
    if (key == "@last") {
      serialize_value(out, loop->is_last());
      return true;
    }
    return false;
  }

  // ネストパス（ドット区切り）
  auto const dot = key.find('.');
  if (dot != std::string_view::npos) {
    auto const first = key.substr(0, dot);
    auto const rest = key.substr(dot + 1);
    return resolve_path_step(out, first, rest, value, loop);
  }

  // フラットキー
  return write_value(out, key, value, loop);
}

/// @brief ネストパスの1ステップを解決する
template <class Buffer, class T>
inline bool resolve_path_step(Buffer& out, std::string_view first, std::string_view rest,
                               T const& value, loop_state const* loop) {
  if constexpr (glz_reflectable<T>) {
    constexpr auto count = static_cast<std::size_t>(glz::reflect<T>::size);
    bool found = false;
    auto tied = glz::to_tie(value);
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      (([&] {
        if (found) {
          return;
        }
        if (std::string_view{glz::reflect<T>::keys[I]} != first) {
          return;
        }
        auto const& field = glz::get<I>(tied);
        using FieldType = std::remove_cvref_t<decltype(field)>;
        if constexpr (glz_reflectable<FieldType>) {
          found = resolve_value(out, rest, field, loop);
        } else {
          found = false;
        }
      }()),
          ...);
    }(std::make_index_sequence<count>{});
    return found;
  } else {
    return false;
  }
}

/// @brief if 式を評価する
/// @param expr @last / @first / @index または フィールド名
/// @param value コンテキスト値
/// @param loop ループコンテキスト
/// @return 条件が真なら true
template <class T>
inline bool evaluate_if_expr(std::string_view expr, T const& value, loop_state const* loop) {
  if (!expr.empty() && expr[0] == '@') {
    if (!loop) {
      return false;
    }
    if (expr == "@last") {
      return loop->is_last();
    }
    if (expr == "@first") {
      return loop->is_first();
    }
    if (expr == "@index") {
      return loop->index > 0;
    }
    return false;
  }

  std::string tmp;
  if (!resolve_value(tmp, expr, value, loop)) {
    return false;
  }
  return !tmp.empty() && tmp != "false" && tmp != "0";
}

} // namespace frozenchars::fast_inja::detail
