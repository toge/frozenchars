#pragma once

#include "types.hpp"
#include "chunk.hpp"
#include "escape.hpp"
#include "loop_state.hpp"
#include "resolve.hpp"
#include <expected>
#include <string_view>

namespace frozenchars::fast_inja::detail {

/// @brief vector-like 型の判定（std::string を除外）
template <class T>
concept is_vector_like =
    !std::is_arithmetic_v<T> &&
    !std::same_as<T, std::string> &&
    !std::same_as<T, std::string_view> &&
    requires(T const& v) {
      typename T::value_type;
      { v.size() } -> std::convertible_to<std::size_t>;
      { v[0] };
      { v.begin() };
      { v.end() };
    };

/// @brief render_chunks の前方宣言（再帰用）
template <class Mode, class Buffer, class T>
auto render_chunks(Buffer& out, parsed_template const& pt, T const& value, loop_state const* loop)
    -> std::expected<void, error_ctx>;

// -------- 各チャンク型のレンダリング実装 --------

/// @brief chunk_literal のレンダリング
template <class Mode, class Buffer, class T>
inline auto render_one(Buffer& out, chunk_literal const& c, T const& /*value*/, loop_state const* /*loop*/)
    -> std::expected<void, error_ctx> {
  out.append(std::string_view{c.text});
  return {};
}

/// @brief chunk_placeholder のレンダリング
template <class Mode, class Buffer, class T>
inline auto render_one(Buffer& out, chunk_placeholder const& c, T const& value, loop_state const* loop)
    -> std::expected<void, error_ctx> {
  if constexpr (std::is_same_v<Mode, mustache_tag>) {
    if (!c.raw) {
      std::string tmp;
      if (!resolve_value(tmp, std::string_view{c.key}, value, loop)) {
        return std::unexpected(error_ctx{.ec = error_code::unknown_key});
      }
      html_escape_into(out, std::string_view{tmp});
      return {};
    }
  }
  if (!resolve_value(out, std::string_view{c.key}, value, loop)) {
    return std::unexpected(error_ctx{.ec = error_code::unknown_key});
  }
  return {};
}

/// @brief chunk_section のレンダリング（vector または bool フィールド）
template <class Mode, class Buffer, class T>
inline auto render_one(Buffer& out, chunk_section const& c, T const& value, loop_state const* parent_loop)
    -> std::expected<void, error_ctx> {
  if constexpr (glz_reflectable<T>) {
    constexpr auto sz = static_cast<std::size_t>(glz::reflect<T>::size);
    bool found = false;
    std::expected<void, error_ctx> res{};
    auto tied = glz::to_tie(value);
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      (([&] {
        if (found) {
          return;
        }
        if (std::string_view{glz::reflect<T>::keys[I]} != std::string_view{c.key}) {
          return;
        }
        found = true;
        auto const& field = glz::get<I>(tied);
        using FT = std::remove_cvref_t<decltype(field)>;
        if constexpr (is_vector_like<FT>) {
          loop_state ls;
          ls.count = static_cast<std::uint32_t>(field.size());
          for (ls.index = 0; ls.index < ls.count; ++ls.index) {
            for (auto const& bpt : c.body) {
              res = render_chunks<Mode>(out, bpt, field[ls.index], &ls);
              if (!res) {
                return;
              }
            }
          }
        } else if constexpr (std::same_as<FT, bool>) {
          if (field) {
            for (auto const& bpt : c.body) {
              res = render_chunks<Mode>(out, bpt, value, parent_loop);
              if (!res) {
                return;
              }
            }
          }
        } else {
          res = std::unexpected(error_ctx{.ec = error_code::type_mismatch});
        }
      }()),
          ...);
    }(std::make_index_sequence<sz>{});

    if (!found) {
      return std::unexpected(error_ctx{.ec = error_code::unknown_key});
    }
    return res;
  } else {
    return std::unexpected(error_ctx{.ec = error_code::type_mismatch});
  }
}

/// @brief chunk_inverted のレンダリング（空または false のとき展開）
template <class Mode, class Buffer, class T>
inline auto render_one(Buffer& out, chunk_inverted const& c, T const& value, loop_state const* parent_loop)
    -> std::expected<void, error_ctx> {
  if constexpr (glz_reflectable<T>) {
    constexpr auto sz = static_cast<std::size_t>(glz::reflect<T>::size);
    bool found = false;
    std::expected<void, error_ctx> res{};
    auto tied = glz::to_tie(value);
    [&]<std::size_t... I>(std::index_sequence<I...>) {
      (([&] {
        if (found) {
          return;
        }
        if (std::string_view{glz::reflect<T>::keys[I]} != std::string_view{c.key}) {
          return;
        }
        found = true;
        auto const& field = glz::get<I>(tied);
        using FT = std::remove_cvref_t<decltype(field)>;
        if constexpr (is_vector_like<FT>) {
          if (field.empty()) {
            for (auto const& bpt : c.body) {
              res = render_chunks<Mode>(out, bpt, value, parent_loop);
              if (!res) {
                return;
              }
            }
          }
        } else if constexpr (std::same_as<FT, bool>) {
          if (!field) {
            for (auto const& bpt : c.body) {
              res = render_chunks<Mode>(out, bpt, value, parent_loop);
              if (!res) {
                return;
              }
            }
          }
        } else {
          res = std::unexpected(error_ctx{.ec = error_code::type_mismatch});
        }
      }()),
          ...);
    }(std::make_index_sequence<sz>{});

    if (!found) {
      return std::unexpected(error_ctx{.ec = error_code::unknown_key});
    }
    return res;
  } else {
    return std::unexpected(error_ctx{.ec = error_code::type_mismatch});
  }
}

/// @brief chunk_at_var のレンダリング（@index / @first / @last）
template <class Mode, class Buffer, class T>
inline auto render_one(Buffer& out, chunk_at_var const& c, T const& /*value*/, loop_state const* loop)
    -> std::expected<void, error_ctx> {
  if (!loop) {
    return {};
  }
  switch (c.var) {
    case chunk_at_var::kind::index:
      serialize_value(out, loop->index);
      break;
    case chunk_at_var::kind::first:
      serialize_value(out, loop->is_first());
      break;
    case chunk_at_var::kind::last:
      serialize_value(out, loop->is_last());
      break;
    case chunk_at_var::kind::root:
      break;
  }
  return {};
}

/// @brief chunk_at_section のレンダリング（@last / @first セクション）
template <class Mode, class Buffer, class T>
inline auto render_one(Buffer& out, chunk_at_section const& c, T const& value, loop_state const* loop)
    -> std::expected<void, error_ctx> {
  bool cond = false;
  if (loop) {
    switch (c.var) {
      case chunk_at_var::kind::last:
        cond = loop->is_last();
        break;
      case chunk_at_var::kind::first:
        cond = loop->is_first();
        break;
      case chunk_at_var::kind::index:
        cond = loop->index > 0;
        break;
      case chunk_at_var::kind::root:
        cond = false;
        break;
    }
  }
  if (c.inverted) {
    cond = !cond;
  }
  if (cond) {
    for (auto const& bpt : c.body) {
      auto r = render_chunks<Mode>(out, bpt, value, loop);
      if (!r) {
        return r;
      }
    }
  }
  return {};
}

/// @brief chunk_if のレンダリング（if/else）
template <class Mode, class Buffer, class T>
inline auto render_one(Buffer& out, chunk_if const& c, T const& value, loop_state const* loop)
    -> std::expected<void, error_ctx> {
  bool const cond = evaluate_if_expr(std::string_view{c.expr}, value, loop);
  auto const& branch = cond ? c.then_branch : c.else_branch;
  for (auto const& bpt : branch) {
    auto r = render_chunks<Mode>(out, bpt, value, loop);
    if (!r) {
      return r;
    }
  }
  return {};
}

/// @brief テンプレート内の全チャンクをレンダリングする
template <class Mode, class Buffer, class T>
auto render_chunks(Buffer& out, parsed_template const& pt, T const& value, loop_state const* loop)
    -> std::expected<void, error_ctx> {
  for (auto const& c : pt.chunks) {
    auto r = std::visit(
        [&](auto const& ck) -> std::expected<void, error_ctx> {
          return render_one<Mode>(out, ck, value, loop);
        },
        c);
    if (!r) {
      return r;
    }
  }
  return {};
}

} // namespace frozenchars::fast_inja::detail
