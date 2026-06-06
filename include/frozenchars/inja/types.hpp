#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <functional>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202306L
#include <inplace_vector>
#endif

#if defined(__has_include) && __has_include(<glaze/glaze.hpp>)
#include <glaze/glaze.hpp>
#define FROZENCHARS_HAS_GLAZE 1
#else
#define FROZENCHARS_HAS_GLAZE 0
#endif

#include "../inja_function.hpp"
#include "../inja_value.hpp"
#include "../string.hpp"

namespace frozenchars::inja {

/**
 * @brief テンプレート区切り文字を保持する型。
 */
template <auto ExprOpen, auto ExprClose, auto StmtOpen, auto StmtClose, auto CommentOpen, auto CommentClose, auto LineStmtPrefix>
struct delimiters {
  static constexpr auto expression_open       = ExprOpen;
  static constexpr auto expression_close      = ExprClose;
  static constexpr auto statement_open        = StmtOpen;
  static constexpr auto statement_close       = StmtClose;
  static constexpr auto comment_open          = CommentOpen;
  static constexpr auto comment_close         = CommentClose;
  static constexpr auto line_statement_prefix = LineStmtPrefix;
};

inline constexpr auto default_expr_open        = FrozenString<3>{"{{"};
inline constexpr auto default_expr_close       = FrozenString<3>{"}}"};
inline constexpr auto default_stmt_open        = FrozenString<3>{"{%"};
inline constexpr auto default_stmt_close       = FrozenString<3>{"%}"};
inline constexpr auto default_comment_open     = FrozenString<3>{"{#"};
inline constexpr auto default_comment_close    = FrozenString<3>{"#}"};
inline constexpr auto default_line_stmt_prefix = FrozenString<3>{"##"};

using default_delimiters = delimiters<default_expr_open, default_expr_close, default_stmt_open, default_stmt_close, default_comment_open, default_comment_close, default_line_stmt_prefix>;

using function_callback = std::function<inja_value(std::vector<inja_value> const&)>;
using include_callback  = std::function<std::string(std::string_view)>;

/**
 * @brief テンプレート実行時の拡張オプション。
 */
struct runtime_options {
  std::unordered_map<std::string, function_callback, transparent_string_hash, std::equal_to<>> function_call{};
  std::unordered_map<std::string, std::string, transparent_string_hash, std::equal_to<>>       include_templates{};
  include_callback                                                                             include_call{};

  auto add_function(std::string name, function_callback callback) -> void { function_call.insert_or_assign(std::move(name), std::move(callback)); }

  auto reserve_functions(std::size_t count) -> void { function_call.reserve(count); }

  auto add_include(std::string name, std::string content) -> void { include_templates.insert_or_assign(std::move(name), std::move(content)); }

  auto reserve_includes(std::size_t count) -> void { include_templates.reserve(count); }
};

using runtime_options_ref = std::optional<std::reference_wrapper<runtime_options const>>;

namespace detail {

  template <typename T>
  concept is_environment_binding = frozenchars::inja::is_function_list<T> || frozenchars::inja::is_environment<T>;

  template <typename T>
  struct environment_traits;

  template <frozenchars::inja::is_function_list FunctionList>
  struct environment_traits<FunctionList> {
    using function_list_type = FunctionList;
    using constant_list_type = frozenchars::inja::empty_constant_list;
  };

  template <frozenchars::inja::is_environment Environment>
  struct environment_traits<Environment> {
    using function_list_type = typename Environment::function_list_type;
    using constant_list_type = typename Environment::constant_list_type;
  };

  template <typename T>
  using remove_cvref_t = std::remove_cvref_t<T>;

#if FROZENCHARS_HAS_GLAZE
  template <typename T>
  concept glaze_reflectable = requires {
    { glz::reflect<remove_cvref_t<T>>::size } -> std::convertible_to<std::size_t>;
    glz::reflect<remove_cvref_t<T>>::keys;
  };
#else
  template <typename T>
  concept glaze_reflectable = false;
#endif

  template <typename T>
  concept map_like = requires(remove_cvref_t<T> const& v) {
    typename remove_cvref_t<T>::key_type;
    typename remove_cvref_t<T>::mapped_type;
    { v.begin() };
    { v.end() };
  } && std::convertible_to<typename remove_cvref_t<T>::key_type, std::string_view>;

  template <typename T>
  concept range_like = std::ranges::input_range<remove_cvref_t<T>> && (!std::same_as<remove_cvref_t<T>, std::string>) && (!std::same_as<remove_cvref_t<T>, std::string_view>) && (!map_like<T>);

}  // namespace detail

/**
 * @brief テンプレートで保持する最大ノード数。
 */
constexpr auto MAX_NODES = std::size_t{1024 * 4};

/**
 * @brief 式のノードの種類。
 *
 * ノード定義から前置するため、ここで先に宣言する。
 */
enum class expr_kind : std::uint8_t {
  simple_path,  ///< 単一または複数セグメントの変数パス
  complex,      ///< 関数呼び出し・演算子を含む式
};

/**
 * @brief テンプレート構文ノードの種類。
 */
enum class node_kind : std::uint8_t { text, expr, if_stmt, else_stmt, endif_stmt, for_stmt, endfor_stmt, set_stmt, include_stmt };

/// パスセグメントの最大文字数（null 終端を除く）。
/// ノードごとに 4 セグメント保持するため、バイトコードサイズを抑える目的で 16 に制限。
constexpr auto MAX_SEG_LEN = std::size_t{15};

/**
 * @brief パース済みテンプレートの単一ノード。
 */
struct node {
  node_kind   kind{};
  std::size_t begin{};
  std::size_t end{};
  std::size_t else_index{std::numeric_limits<std::size_t>::max()};
  std::size_t end_index{std::numeric_limits<std::size_t>::max()};
  std::size_t aux_begin{std::numeric_limits<std::size_t>::max()};
  std::size_t aux_end{std::numeric_limits<std::size_t>::max()};
  std::size_t aux2_begin{std::numeric_limits<std::size_t>::max()};
  std::size_t aux2_end{std::numeric_limits<std::size_t>::max()};
  std::size_t aux3_begin{std::numeric_limits<std::size_t>::max()};
  std::size_t aux3_end{std::numeric_limits<std::size_t>::max()};
  bool        expr_is_simple_path{};
  bool        if_cond_is_simple_path{};
  bool        for_has_key{};
  bool        for_iter_is_simple_path{};
  bool        include_expr_is_simple_path{};
  bool        is_plain_else{};

  // コンパイル時にトークン化されたパスセグメント（最大 4 段、各 15 文字 + null）。
  // expr_e_kind == simple_path のときだけ有効。char 配列で 64 バイトに抑える。
  std::array<char, MAX_SEG_LEN + 1> path_seg_0{};
  std::array<char, MAX_SEG_LEN + 1> path_seg_1{};
  std::array<char, MAX_SEG_LEN + 1> path_seg_2{};
  std::array<char, MAX_SEG_LEN + 1> path_seg_3{};
  std::uint8_t                      path_depth{};

  // 式の分類
  expr_kind expr_e_kind{expr_kind::complex};
  expr_kind if_cond_e_kind{expr_kind::complex};
  expr_kind for_iter_e_kind{expr_kind::complex};
  expr_kind include_expr_e_kind{expr_kind::complex};

  // 実行時に解決される function pointer。RootContext 型に依存するため consteval
  // では埋め込めず、render 時に fill する。nullptr なら旧経路にフォールバック。
  // シグネチャ: void(void const* /*ctx*/, std::string& /*out*/)
  void (*simple_appender)(void const*, std::string&){};
};

/**
 * @brief consteval パースで生成されるバイトコード相当データ。
 */
struct bytecode {
  std::array<node, MAX_NODES> nodes{};
  std::size_t                 count{};
  std::size_t                 max_for_depth{};
};

struct loop_state {
  std::int64_t index{};
  std::int64_t index1{};
  bool         is_first{};
  bool         is_last{};
};

/**
 * @brief char 配列のセグメントを string_view に変換する（null 終端対応）。
 */
[[nodiscard]] constexpr auto seg_to_sv(std::array<char, MAX_SEG_LEN + 1> const& buf) -> std::string_view {
  auto len = std::size_t{0};
  while (len < buf.size() && buf[len] != '\0') {
    ++len;
  }
  return {buf.data(), len};
}

/**
 * @brief コンパイル時にトークン化された変数パス。
 *
 * セグメント列を NTTP として保持し、accessor に直接渡せる形にする。
 * 実行時にパスを分割する split_variable_path を排除するのが目的。
 */
template <fixed_string... Segments>
struct precomputed_path {
  static constexpr std::size_t depth = sizeof...(Segments);
};

struct local_binding {
  std::string name;
  inja_value  value;
};

struct local_frame {
  std::optional<local_binding> key{};
  std::optional<local_binding> value{};
  std::vector<local_binding>   assigned{};
  loop_state                   loop{};
};

enum class lookup_status : std::uint8_t {
  not_found,
  not_convertible,
};

using lookup_result = std::expected<inja_value, lookup_status>;

struct path_segments {
  static constexpr auto max_depth = std::size_t{16};

#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202306L
  using storage_type = std::inplace_vector<std::string_view, max_depth>;
#else
  using storage_type = std::array<std::string_view, max_depth>;
#endif

  storage_type storage{};
  std::size_t  size_{};

  auto push_back(std::string_view sv) -> void {
    if (size() >= max_depth) {
      throw render_error{"variable path too deep (max 16 segments)"};
    }
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202306L
    storage.push_back(sv);
#else
    storage[size_++] = sv;
#endif
  }

  [[nodiscard]] auto size() const -> std::size_t {
#if defined(__cpp_lib_inplace_vector) && __cpp_lib_inplace_vector >= 202306L
    return storage.size();
#else
    return size_;
#endif
  }

  [[nodiscard]] auto operator[](std::size_t index) const -> std::string_view { return storage[index]; }

  [[nodiscard]] auto front() const -> std::string_view { return storage.front(); }

  [[nodiscard]] auto data() const -> std::string_view const* { return storage.data(); }

  [[nodiscard]] auto span() const -> std::span<std::string_view const> { return {data(), size()}; }
};

#if FROZENCHARS_HAS_GLAZE
struct typed_object_view {
  void const* object{};
  std::size_t (*size_fn)(void const*){};
  void (*for_each_fn)(void const*, void*, void (*)(void*, std::string_view, inja_value&&)){};

  [[nodiscard]] auto size() const -> std::size_t { return size_fn(object); }

  template <typename Callback>
  auto for_each(Callback&& callback) const -> void {
    struct callback_state {
      Callback* callback_ptr;
    };
    auto state = callback_state{.callback_ptr = &callback};
    for_each_fn(object, &state, [](void* state_ptr, std::string_view key, inja_value&& value) {
      auto* state = static_cast<callback_state*>(state_ptr);
      (*state->callback_ptr)(key, std::move(value));
    });
  }
};
#else
struct typed_object_view {};
#endif

}  // namespace frozenchars::inja
