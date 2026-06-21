#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

#include <glaze/glaze.hpp>
#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

using namespace frozenchars;
using namespace frozenchars::inja;
using namespace frozenchars::literals;

namespace test_detail {

struct profile_context {
  std::string city;
};

struct user_context {
  std::string name;
  std::int64_t age;
  profile_context profile;
};

struct root_context {
  std::string name{};
  std::vector<std::int64_t> items{};
  bool ok{};
  user_context user{};
  std::unordered_map<std::string, std::int64_t> obj{};
  std::vector<std::vector<std::int64_t>> rows{};
  std::string include_name{"dynamic"};
  std::int64_t n{};
};

struct empty_context {
};

struct x_context {
  std::int64_t x;
};

struct value_context {
  std::int64_t value;
};

struct opaque_payload {
  void* handle{};
};

struct partial_context {
  std::string name;
  opaque_payload opaque;
};

} // namespace test_detail

using namespace test_detail;

#if FROZENCHARS_HAS_GLAZE
static_assert(requires(root_context const& ctx) {
  glz::to_tie(ctx);
});
#endif

TEST_CASE("inja_value basic types", "[template_vm][value]") {
  auto const v_null = inja_value{};
  auto const v_num = inja_value{std::int64_t{42}};
  auto const v_str = inja_value{std::string{"hello"}};
  REQUIRE(is_null(v_null));
  REQUIRE(as_int(v_num) == 42);
  REQUIRE(as_string(v_str) == "hello");
}

TEST_CASE("template parser runs at constexpr", "[template_vm][parser]") {
  constexpr auto src = "A{{ x }}{% if ok %}B{% else %}C{% endif %}"_fs;
  constexpr auto count = [&] {
    auto constexpr program = frozenchars::inja::detail::parse_program<src>();
    return program.count;
  }();
  static_assert(count > 0);
  REQUIRE(count > 0);

  static_assert(frozenchars::inja::detail::is_simple_path_expression("user.profile.city"));
  static_assert(!frozenchars::inja::detail::is_simple_path_expression("upper(name)"));
  REQUIRE(frozenchars::inja::detail::is_simple_path_expression("name"));
  REQUIRE(!frozenchars::inja::detail::is_simple_path_expression("name..x"));
}

TEST_CASE("template parser marks simple-path metadata for if/for/include", "[template_vm][parser]") {
  constexpr auto src = "{% if user.profile.city %}X{% endif %}{% for v in user.profile %}{{ v }}{% endfor %}{% include include_name %}"_fs;
  constexpr auto program = frozenchars::inja::detail::parse_program<src>();

  auto has_if_simple = false;
  auto has_for_simple = false;
  auto has_include_simple = false;
  for (auto i = std::size_t{0}; i < program.count; ++i) {
    auto const& n = program.nodes[i];
    if (n.kind == node_kind::if_stmt) {
      has_if_simple = n.if_cond_is_simple_path;
    } else if (n.kind == node_kind::for_stmt) {
      has_for_simple = n.for_iter_is_simple_path;
    } else if (n.kind == node_kind::include_stmt) {
      has_include_simple = n.include_expr_is_simple_path;
    }
  }

  REQUIRE(has_if_simple);
  REQUIRE(has_for_simple);
  REQUIRE(has_include_simple);
}

TEST_CASE("split_variable_path enforces the small-buffer depth contract", "[template_vm][lookup]") {
  auto const segments = frozenchars::inja::detail::split_variable_path("a.b.c.d.e");
  REQUIRE(segments.size() == 5);
  REQUIRE(segments[0] == "a");
  REQUIRE(segments[4] == "e");

  REQUIRE_THROWS_WITH(
    frozenchars::inja::detail::split_variable_path("a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q"),
    "variable path too deep (max 16 segments)"
  );
}

TEST_CASE("render reports deep variable paths through lookup", "[template_vm][lookup]") {
  constexpr auto src = "{{ a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q }}"_fs;
  auto const root = [] {
    auto value = inja_value{std::int64_t{1}};
    for (auto const key : std::array{
           "q", "p", "o", "n", "m", "l", "k", "j", "i", "h", "g", "f", "e", "d", "c", "b", "a"
         }) {
      value = inja_value{inja_object{{std::string{key}, std::move(value)}}};
    }
    return value;
  }();

  REQUIRE_THROWS_WITH(render<src>(root), "variable path too deep (max 16 segments)");
}

TEST_CASE("template function extraction ignores comment blocks", "[template_vm][parser]") {
  constexpr auto src = "{# upper(x) #}{{ x }}"_fs;
  constexpr auto calls = extract_template_function_calls<src>();

  static_assert(calls.count == 0);
  REQUIRE(calls.count == 0);
}

TEST_CASE("template runtime renders core syntax", "[template_vm][runtime]") {
  constexpr auto src = "Hello {{ name }}{# comment #}{% if items %}:{% for x in items %}{{ x }}{% endfor %}{% endif %}"_fs;
  auto const ctx = root_context{
    .name = "A",
    .items = {1, 2},
  };
  auto const out = render<src>(ctx);
  REQUIRE(out == "Hello A:12");
}

TEST_CASE("template runtime supports for key, value", "[template_vm][runtime]") {
  constexpr auto src = "{% for k, v in user %}{{ k }}={{ v }};{% endfor %}"_fs;
  auto const ctx = root_context{
    .user = user_context{.name = "tom", .age = 18, .profile = profile_context{.city = "Tokyo"}},
  };
  auto const out = render<src>(ctx);
  REQUIRE(out.find("name=tom;") != std::string::npos);
  REQUIRE(out.find("age=18;") != std::string::npos);
}

TEST_CASE("template runtime supports for key, value on typed nested reflectable path", "[template_vm][runtime]") {
  constexpr auto src = "{% for k, v in user.profile %}{{ k }}={{ v }};{% endfor %}"_fs;
  auto const ctx = root_context{
    .user = user_context{.name = "tom", .age = 18, .profile = profile_context{.city = "Tokyo"}},
  };
  REQUIRE(render<src>(ctx) == "city=Tokyo;");
}

TEST_CASE("template runtime reuses array loop frame", "[template_vm][runtime]") {
  constexpr auto src = "{% for x in items %}{{ loop.index }},{{ x }}|{% endfor %}"_fs;
  auto const ctx = root_context{
    .items = {1, 2, 3},
  };
  REQUIRE(render<src>(ctx) == "0,1|1,2|2,3|");
}

TEST_CASE("template runtime sets loop flags", "[template_vm][runtime]") {
  constexpr auto src = "{% for x in items %}{{ loop.is_first }},{{ loop.is_last }}|{% endfor %}"_fs;
  auto const ctx = root_context{
    .items = {1, 2, 3},
  };
  REQUIRE(render<src>(ctx) == "true,false|false,false|false,true|");
}

TEST_CASE("template runtime supports nested for loops", "[template_vm][runtime]") {
  constexpr auto src = "{% for row in rows %}[{% for x in row %}{{ loop.index }}:{{ x }},{% endfor %}]{% endfor %}"_fs;
  auto const ctx = root_context{
    .rows = {{10, 20}, {30}},
  };
  REQUIRE(render<src>(ctx) == "[0:10,1:20,][0:30,]");
}

TEST_CASE("template runtime handles empty for iterables", "[template_vm][runtime]") {
  constexpr auto array_src = "A{% for x in items %}X{% endfor %}B"_fs;
  constexpr auto object_src = "C{% for k, v in obj %}Y{% endfor %}D"_fs;
  auto const array_ctx = root_context{};
  auto const object_ctx = root_context{};
  REQUIRE(render<array_src>(array_ctx) == "AB");
  REQUIRE(render<object_src>(object_ctx) == "CD");
}

TEST_CASE("template runtime short-circuit", "[template_vm][runtime]") {
  constexpr auto src = "{% if ok or missing.value %}yes{% else %}no{% endif %}"_fs;
  auto const ctx = root_context{
    .ok = true,
  };
  REQUIRE(render<src>(ctx) == "yes");
}

TEST_CASE("template runtime errors", "[template_vm][runtime_error]") {
  constexpr auto miss_var = "{{ missing }}"_fs;
  auto const empty = empty_context{};
  REQUIRE_THROWS_AS(render<miss_var>(empty), render_error);

  constexpr auto bad_mod = "{{ 1.5 % 2 }}"_fs;
  REQUIRE_THROWS_AS(render<bad_mod>(empty), render_error);

  constexpr auto bad_for = "{% for x in x %}{{ x }}{% endfor %}"_fs;
  auto const ctx = root_context{};
  REQUIRE_THROWS_AS(render<bad_for>(ctx), render_error);

  constexpr auto mod_zero = "{{ 10 % 0 }}"_fs;
  REQUIRE_THROWS_WITH(render<mod_zero>(empty), "modulo by zero");

  constexpr auto div_zero = "{{ 1.0 / 0.0 }}"_fs;
  REQUIRE_THROWS_WITH(render<div_zero>(empty), "division by zero");

  constexpr auto mod_ok = "{{ 10 % 3 }}"_fs;
  REQUIRE(render<mod_ok>(empty) == "1");

  constexpr auto div_ok = "{{ 9.0 / 4.0 }}"_fs;
  REQUIRE(render<div_ok>(empty) == "2.25");

  constexpr auto bad_path = "{{ name.first }}"_fs;
  auto const bad_path_ctx = root_context{.name = "Tom"};
  REQUIRE_THROWS_WITH(render<bad_path>(bad_path_ctx), "cannot resolve path: name.first");
}

TEST_CASE("append_value rejects non-finite doubles", "[template_vm][runtime_error]") {
  auto out = std::string{};

  REQUIRE_THROWS_AS(
    frozenchars::inja::detail::append_value(
      out, inja_value{std::numeric_limits<double>::quiet_NaN()}
    ),
    render_error
  );
  REQUIRE_THROWS_AS(
    frozenchars::inja::detail::append_value(
      out, inja_value{std::numeric_limits<double>::infinity()}
    ),
    render_error
  );
  REQUIRE_THROWS_AS(
    frozenchars::inja::detail::append_value(
      out, inja_value{-std::numeric_limits<double>::infinity()}
    ),
    render_error
  );
}

TEST_CASE("template_vm public API", "[template_vm][api]") {
  constexpr auto src = "X={{ x }}"_fs;
  auto const root = x_context{.x = 9};
  REQUIRE(render<src>(root) == "X=9");
}

TEST_CASE("template runtime options callback uses include name only", "[template_vm][api]") {
  auto options = runtime_options{};
  options.include_call = [](std::string_view include_name) -> std::string {
    return std::string{"<"} + std::string{include_name} + ">";
  };
  REQUIRE(options.include_call("name").find("name") != std::string::npos);
}

TEST_CASE("template runtime supports line statements", "[template_vm][line_statement]") {
  constexpr auto src = "## if ok\nYES\n## else\nNO\n## endif\n"_fs;
  auto const ctx = root_context{
    .ok = true,
  };
  auto const out = render<src>(ctx);
  REQUIRE(out.find("YES") != std::string::npos);
  REQUIRE(out.find("NO") == std::string::npos);
}

TEST_CASE("template runtime supports custom delimiters", "[template_vm][delimiters]") {
  constexpr auto expr_open = "[["_fs;
  constexpr auto expr_close = "]]"_fs;
  constexpr auto stmt_open = "(%"_fs;
  constexpr auto stmt_close = "%)"_fs;
  constexpr auto comment_open = "(#"_fs;
  constexpr auto comment_close = "#)"_fs;
  constexpr auto line_stmt_prefix = "%%"_fs;

  using custom_delims = delimiters<
    expr_open,
    expr_close,
    stmt_open,
    stmt_close,
    comment_open,
    comment_close,
    line_stmt_prefix>;

  constexpr auto src = "Hello [[ name ]]\n%% if ok\nOK\n%% endif\n(# hidden #)\n"_fs;
  auto const ctx = root_context{
    .name = "Tom",
    .ok = true,
  };
  auto const out = render<src, custom_delims>(ctx);
  REQUIRE(out.find("Hello Tom") != std::string::npos);
  REQUIRE(out.find("OK") != std::string::npos);
  REQUIRE(out.find("hidden") == std::string::npos);
}

TEST_CASE("template runtime supports else if chains", "[template_vm][else_if]") {
  constexpr auto src = "{% if n > 10 %}large{% else if n > 5 %}mid{% else %}small{% endif %}"_fs;

  auto const ctx_mid = root_context{.n = 7};
  REQUIRE(render<src>(ctx_mid) == "mid");

  auto const ctx_small = root_context{.n = 3};
  REQUIRE(render<src>(ctx_small) == "small");
}

TEST_CASE("template runtime resolves else if chains including tail else", "[template_vm][else_if]") {
  constexpr auto src = "{% if n == 1 %}A{% else if n == 2 %}B{% else %}C{% endif %}"_fs;

  REQUIRE(render<src>(root_context{.n = 1}) == "A");
  REQUIRE(render<src>(root_context{.n = 2}) == "B");
  REQUIRE(render<src>(root_context{.n = 3}) == "C");
}

TEST_CASE("template runtime supports set assignment", "[template_vm][set]") {
  constexpr auto src = "{% set title = upper(name) %}{{ title }}"_fs;
  auto const ctx = root_context{
    .name = "alice",
  };
  REQUIRE(render<src>(ctx) == "ALICE");
}

TEST_CASE("template runtime rejects set nested assignment", "[template_vm][set]") {
  constexpr auto src = "{% set user.name = \"Tom\" %}{{ user.name }}"_fs;
  auto const ctx = root_context{};
  REQUIRE_THROWS_WITH(render<src>(ctx), "set target must be local identifier");
}

TEST_CASE("template runtime supports include template registry", "[template_vm][include]") {
  constexpr auto src = "A{% include \"part\" %}B"_fs;
  auto const ctx = root_context{};

  auto options = runtime_options{};
  options.add_include("part", "X");

  REQUIRE(render<src>(ctx, std::cref(options)) == "AXB");
}

TEST_CASE("template runtime supports include callback", "[template_vm][include]") {
  constexpr auto src = "{% include include_name %}"_fs;
  auto const ctx = root_context{
    .include_name = "dynamic",
  };

  auto options = runtime_options{};
  options.include_call = [](std::string_view include_name) -> std::string {
    return std::string{"<"} + std::string{include_name} + ">";
  };

  REQUIRE(render<src>(ctx, std::cref(options)) == "<dynamic>");
}

TEST_CASE("template runtime supports custom function callbacks", "[template_vm][callback]") {
  constexpr auto src = "{{ twice(value) }}"_fs;
  auto const ctx = value_context{.value = 21};

  auto options = runtime_options{};
  options.add_function("twice", [](std::vector<inja_value> const& args) -> inja_value {
    if (args.size() != 1) {
      throw render_error{"twice() expects 1 argument"};
    }
    return inja_value{as_int(args[0]) * 2};
  });

  REQUIRE(render<src>(ctx, std::cref(options)) == "42");
}

TEST_CASE("typed root lookup does not require whole-tree conversion", "[template_vm][typed_root]") {
  constexpr auto src = "Hello {{ name }}"_fs;
  auto const ctx = partial_context{
    .name = "Tom",
    .opaque = opaque_payload{.handle = nullptr},
  };
  REQUIRE(render<src>(ctx) == "Hello Tom");
}

TEST_CASE("typed root reports conversion failure distinctly from undefined variable", "[template_vm][typed_root]") {
  constexpr auto src = "{{ opaque }}"_fs;
  auto const ctx = partial_context{
    .name = "Tom",
    .opaque = opaque_payload{.handle = nullptr},
  };
  REQUIRE_THROWS_WITH(render<src>(ctx), "value cannot be converted to inja_value");
}
