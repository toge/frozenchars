#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::literals;

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
  constexpr auto count = [] {
    auto constexpr program = frozenchars::detail::parse_program<src>();
    return program.count;
  }();
  static_assert(count > 0);
  REQUIRE(count > 0);
}

TEST_CASE("template runtime renders core syntax", "[template_vm][runtime]") {
  constexpr auto src = "Hello {{ name }}{# comment #}{% if items %}:{% for x in items %}{{ x }}{% endfor %}{% endif %}"_fs;
  auto const ctx = make_template_object({
    {"name", "A"},
    {"items", make_template_array({1, 2})},
  });
  auto const out = render_template<src>(ctx);
  REQUIRE(out == "Hello A:12");
}

TEST_CASE("template runtime supports for key, value", "[template_vm][runtime]") {
  constexpr auto src = "{% for k, v in user %}{{ k }}={{ v }};{% endfor %}"_fs;
  auto const ctx = make_template_object({
    {"user", make_template_object({{"name", "tom"}, {"age", 18}})},
  });
  auto const out = render_template<src>(ctx);
  REQUIRE(out.find("name=tom;") != std::string::npos);
  REQUIRE(out.find("age=18;") != std::string::npos);
}

TEST_CASE("template runtime short-circuit", "[template_vm][runtime]") {
  constexpr auto src = "{% if ok or missing.value %}yes{% else %}no{% endif %}"_fs;
  auto const ctx = make_template_object({{"ok", true}});
  REQUIRE(render_template<src>(ctx) == "yes");
}

TEST_CASE("template runtime errors", "[template_vm][runtime_error]") {
  constexpr auto miss_var = "{{ missing }}"_fs;
  auto const empty = make_template_object({});
  REQUIRE_THROWS_AS(render_template<miss_var>(empty), template_render_error);

  constexpr auto bad_mod = "{{ 1.5 % 2 }}"_fs;
  REQUIRE_THROWS_AS(render_template<bad_mod>(empty), template_render_error);

  constexpr auto bad_for = "{% for x in x %}{{ x }}{% endfor %}"_fs;
  auto const ctx = make_template_object({{"x", 1}});
  REQUIRE_THROWS_AS(render_template<bad_for>(ctx), template_render_error);
}

TEST_CASE("template_vm public API", "[template_vm][api]") {
  constexpr auto src = "X={{ x }}"_fs;
  auto const root = make_frozen_map<inja_value, "x"_fs>(
    std::pair{"x", inja_value{9}}
  );
  REQUIRE(inja::template_vm<src>::render(root) == "X=9");
}

TEST_CASE("inja::render accepts frozen_map root", "[template_vm][api]") {
  constexpr auto src = "X={{ x }}"_fs;
  auto const root = make_frozen_map<inja_value, "x"_fs>(
    std::pair{"x", inja_value{9}}
  );
  REQUIRE(inja::render<src>(root) == "X=9");
}

TEST_CASE("template runtime options callback uses inja_object", "[template_vm][api]") {
  auto options = template_runtime_options{};
  options.include_callback = [](std::string_view include_name, inja_object const&) -> std::string {
    return std::string{"<"} + std::string{include_name} + ">";
  };
  REQUIRE(options.include_callback("name", inja_object{}).find("name") != std::string::npos);
}

TEST_CASE("template runtime supports line statements", "[template_vm][line_statement]") {
  constexpr auto src = "## if ok\nYES\n## else\nNO\n## endif\n"_fs;
  auto const ctx = make_template_object({{"ok", true}});
  auto const out = render_template<src>(ctx);
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

  using custom_delims = template_delimiters<
    expr_open,
    expr_close,
    stmt_open,
    stmt_close,
    comment_open,
    comment_close,
    line_stmt_prefix>;

  constexpr auto src = "Hello [[ name ]]\n%% if ok\nOK\n%% endif\n(# hidden #)\n"_fs;
  auto const ctx = make_template_object({
    {"name", "Tom"},
    {"ok", true},
  });
  auto const out = render_template<src, custom_delims>(ctx);
  REQUIRE(out.find("Hello Tom") != std::string::npos);
  REQUIRE(out.find("OK") != std::string::npos);
  REQUIRE(out.find("hidden") == std::string::npos);
}

TEST_CASE("template runtime supports else if chains", "[template_vm][else_if]") {
  constexpr auto src = "{% if n > 10 %}large{% else if n > 5 %}mid{% else %}small{% endif %}"_fs;

  auto const ctx_mid = make_template_object({{"n", 7}});
  REQUIRE(render_template<src>(ctx_mid) == "mid");

  auto const ctx_small = make_template_object({{"n", 3}});
  REQUIRE(render_template<src>(ctx_small) == "small");
}

TEST_CASE("template runtime supports set assignment", "[template_vm][set]") {
  constexpr auto src = "{% set title = upper(name) %}{{ title }}"_fs;
  auto const ctx = make_template_object({{"name", "alice"}});
  REQUIRE(render_template<src>(ctx) == "ALICE");
}

TEST_CASE("template runtime supports set nested assignment", "[template_vm][set]") {
  constexpr auto src = "{% set user.name = \"Tom\" %}{{ user.name }}"_fs;
  auto const ctx = make_template_object({{"user", make_template_object({})}});
  REQUIRE(render_template<src>(ctx) == "Tom");
}

TEST_CASE("template runtime supports include template registry", "[template_vm][include]") {
  constexpr auto src = "A{% include \"part\" %}B"_fs;
  auto const ctx = make_template_object({});

  auto options = template_runtime_options{};
  options.add_include_template("part", "X");

  REQUIRE(render_template<src>(ctx, options) == "AXB");
}

TEST_CASE("template runtime supports include callback", "[template_vm][include]") {
  constexpr auto src = "{% include include_name %}"_fs;
  auto const ctx = make_template_object({{"include_name", "dynamic"}});

  auto options = template_runtime_options{};
  options.include_callback = [](std::string_view include_name, inja_object const&) -> std::string {
    return std::string{"<"} + std::string{include_name} + ">";
  };

  REQUIRE(render_template<src>(ctx, options) == "<dynamic>");
}

TEST_CASE("template runtime supports custom function callbacks", "[template_vm][callback]") {
  constexpr auto src = "{{ twice(value) }}"_fs;
  auto const ctx = make_template_object({{"value", 21}});

  auto options = template_runtime_options{};
  options.add_function("twice", [](std::vector<inja_value> const& args) -> inja_value {
    if (args.size() != 1) {
      throw template_render_error{"twice() expects 1 argument"};
    }
    return inja_value{as_int(args[0]) * 2};
  });

  REQUIRE(render_template<src>(ctx, options) == "42");
}
