#pragma once

#include "loop_state.hpp"
#include "serialize_value.hpp"
#include <string_view>
#include <utility>

#if defined(__has_include) && __has_include(<glaze/glaze.hpp>)
#include <glaze/glaze.hpp>
#define FROZENCHARS_FAST_INJA_HAS_GLAZE 1
#else
#define FROZENCHARS_FAST_INJA_HAS_GLAZE 0
#endif

namespace frozenchars::fast_inja::detail {

#if FROZENCHARS_FAST_INJA_HAS_GLAZE

/// @brief glz::meta リフレクション可能な型の判定
template <class T>
concept glz_reflectable = requires {
  glz::reflect<T>::size;
};

/// @brief glz::meta 反射を使ってフィールドキーを検索し、バッファに書き込む
/// @param out 出力バッファ
/// @param key 検索するフィールド名
/// @param value コンテキスト値
/// @return キーが見つかった場合 true、見つからない場合 false
template <class Buffer, class T>
inline bool write_value(Buffer& out, std::string_view key, T const& value, loop_state const* /*loop*/) {
  if constexpr (glz_reflectable<T>) {
    constexpr auto count = static_cast<std::size_t>(glz::reflect<T>::size);
    bool found = false;
    auto tied = glz::to_tie(value);
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      (([&] {
        if (found) {
          return;
        }
        if (std::string_view{glz::reflect<T>::keys[I]} != key) {
          return;
        }
        auto const& field = glz::get<I>(tied);
        using FieldType = std::remove_cvref_t<decltype(field)>;
        if constexpr (serializable_v<FieldType>) {
          serialize_value(out, field);
          found = true;
        }
      }()),
          ...);
    }(std::make_index_sequence<count>{});
    return found;
  } else if constexpr (serializable_v<T>) {
    serialize_value(out, value);
    return true;
  } else {
    return false;
  }
}

#else

/// @brief glaze が利用できない場合の glz_reflectable
template <class>
inline constexpr bool glz_reflectable = false;

/// @brief glaze が利用できない場合のフォールバック
template <class Buffer, class T>
inline bool write_value(Buffer& out, std::string_view key, T const& value, loop_state const* /*loop*/) {
  if constexpr (serializable_v<T>) {
    serialize_value(out, value);
    return true;
  } else {
    return false;
  }
}

#endif

} // namespace frozenchars::fast_inja::detail
