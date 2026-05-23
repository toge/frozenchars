#include "catch2/catch_all.hpp"

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::inja;
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
  constexpr auto count = [&] {
    auto constexpr program = frozenchars::inja::detail::parse_program<src>();
    return program.count;
  }();
  static_assert(count > 0);
  REQUIRE(count > 0);
}

TEST_CASE("template runtime renders core syntax", "[template_vm][runtime]") {
  constexpr auto src = "Hello {{ name }}{# comment #}{% if items %}:{% for x in items %}{{ x }}{% endfor %}{% endif %}"_fs;
  auto const ctx = make_object({
    {"name", "A"},
    {"items", make_array({1, 2})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out == "Hello A:12");
}

TEST_CASE("template runtime supports for key, value", "[template_vm][runtime]") {
  constexpr auto src = "{% for k, v in user %}{{ k }}={{ v }};{% endfor %}"_fs;
  auto const ctx = make_object({
    {"user", make_object({{"name", "tom"}, {"age", 18}})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out.find("name=tom;") != std::string::npos);
  REQUIRE(out.find("age=18;") != std::string::npos);
}

TEST_CASE("template runtime reuses array loop frame", "[template_vm][runtime]") {
  constexpr auto src = "{% for x in items %}{{ loop.index }},{{ x }}|{% endfor %}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "0,1|1,2|2,3|");
}

TEST_CASE("template runtime sets loop flags", "[template_vm][runtime]") {
  constexpr auto src = "{% for x in items %}{{ loop.is_first }},{{ loop.is_last }}|{% endfor %}"_fs;
  auto const ctx = make_object({
    {"items", make_array({1, 2, 3})},
  });
  REQUIRE(render<src>(ctx) == "true,false|false,false|false,true|");
}

TEST_CASE("template runtime reuses object loop frame", "[template_vm][runtime]") {
  constexpr auto src = "{% for k, v in obj %}{{ k }}={{ v }};{% endfor %}"_fs;
  auto const ctx = make_object({
    {"obj", make_object({{"name", "tom"}, {"age", 18}})},
  });
  auto const out = render<src>(ctx);
  REQUIRE(out.find("name=tom;") != std::string::npos);
  REQUIRE(out.find("age=18;") != std::string::npos);
}

TEST_CASE("template runtime supports nested for loops", "[template_vm][runtime]") {
  constexpr auto src = "{% for row in rows %}[{% for x in row %}{{ loop.index }}:{{ x }},{% endfor %}]{% endfor %}"_fs;
  auto const ctx = make_object({
    {"rows", make_array({make_array({10, 20}), make_array({30})})},
  });
  REQUIRE(render<src>(ctx) == "[0:10,1:20,][0:30,]");
}

TEST_CASE("template runtime handles empty for iterables", "[template_vm][runtime]") {
  constexpr auto array_src = "A{% for x in items %}X{% endfor %}B"_fs;
  constexpr auto object_src = "C{% for k, v in obj %}Y{% endfor %}D"_fs;
  auto const array_ctx = make_object({{"items", make_array({})}});
  auto const object_ctx = make_object({{"obj", make_object({})}});
  REQUIRE(render<array_src>(array_ctx) == "AB");
  REQUIRE(render<object_src>(object_ctx) == "CD");
}

TEST_CASE("template runtime short-circuit", "[template_vm][runtime]") {
  constexpr auto src = "{% if ok or missing.value %}yes{% else %}no{% endif %}"_fs;
  auto const ctx = make_object({{"ok", true}});
  REQUIRE(render<src>(ctx) == "yes");
}

TEST_CASE("template runtime errors", "[template_vm][runtime_error]") {
  constexpr auto miss_var = "{{ missing }}"_fs;
  auto const empty = make_object({});
  REQUIRE_THROWS_AS(render<miss_var>(empty), render_error);

  constexpr auto bad_mod = "{{ 1.5 % 2 }}"_fs;
  REQUIRE_THROWS_AS(render<bad_mod>(empty), render_error);

  constexpr auto bad_for = "{% for x in x %}{{ x }}{% endfor %}"_fs;
  auto const ctx = make_object({{"x", 1}});
  REQUIRE_THROWS_AS(render<bad_for>(ctx), render_error);

  constexpr auto mod_zero = "{{ 10 % 0 }}"_fs;
  REQUIRE_THROWS_WITH(render<mod_zero>(empty), "modulo by zero");

  constexpr auto div_zero = "{{ 1.0 / 0.0 }}"_fs;
  REQUIRE_THROWS_WITH(render<div_zero>(empty), "division by zero");

  constexpr auto mod_ok = "{{ 10 % 3 }}"_fs;
  REQUIRE(render<mod_ok>(empty) == "1");

  constexpr auto div_ok = "{{ 9.0 / 4.0 }}"_fs;
  REQUIRE(render<div_ok>(empty) == "2.25");
}

TEST_CASE("template_vm public API", "[template_vm][api]") {
  constexpr auto src = "X={{ x }}"_fs;
  auto const root = make_frozen_map<inja_value, "x"_fs>(
    std::pair{"x", inja_value{9}}
  );
  REQUIRE(render<src>(make_object({{"x", 9}})) == "X=9");
}

TEST_CASE("inja::render accepts frozen_map root", "[template_vm][api]") {
  constexpr auto src = "X={{ x }}"_fs;
  REQUIRE(render<src>(make_object({{"x", 9}})) == "X=9");
}

TEST_CASE("template runtime options callback uses inja_object", "[template_vm][api]") {
  auto options = runtime_options{};
  options.include_call = [](std::string_view include_name, inja_object const&) -> std::string {
    return std::string{"<"} + std::string{include_name} + ">";
  };
  REQUIRE(options.include_call("name", inja_object{}).find("name") != std::string::npos);
}

TEST_CASE("template runtime supports line statements", "[template_vm][line_statement]") {
  constexpr auto src = "## if ok\nYES\n## else\nNO\n## endif\n"_fs;
  auto const ctx = make_object({{"ok", true}});
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
  auto const ctx = make_object({
    {"name", "Tom"},
    {"ok", true},
  });
  auto const out = render<src, custom_delims>(ctx);
  REQUIRE(out.find("Hello Tom") != std::string::npos);
  REQUIRE(out.find("OK") != std::string::npos);
  REQUIRE(out.find("hidden") == std::string::npos);
}

TEST_CASE("template runtime supports else if chains", "[template_vm][else_if]") {
  constexpr auto src = "{% if n > 10 %}large{% else if n > 5 %}mid{% else %}small{% endif %}"_fs;

  auto const ctx_mid = make_object({{"n", 7}});
  REQUIRE(render<src>(ctx_mid) == "mid");

  auto const ctx_small = make_object({{"n", 3}});
  REQUIRE(render<src>(ctx_small) == "small");
}

TEST_CASE("template runtime supports set assignment", "[template_vm][set]") {
  constexpr auto src = "{% set title = upper(name) %}{{ title }}"_fs;
  auto const ctx = make_object({{"name", "alice"}});
  REQUIRE(render<src>(ctx) == "ALICE");
}

TEST_CASE("template runtime supports set nested assignment", "[template_vm][set]") {
  constexpr auto src = "{% set user.name = \"Tom\" %}{{ user.name }}"_fs;
  auto const ctx = make_object({{"user", make_object({})}});
  REQUIRE(render<src>(ctx) == "Tom");
}

TEST_CASE("template runtime supports include template registry", "[template_vm][include]") {
  constexpr auto src = "A{% include \"part\" %}B"_fs;
  auto const ctx = make_object({});

  auto options = runtime_options{};
  options.add_include("part", "X");

  REQUIRE(render<src>(ctx, std::cref(options)) == "AXB");
}

TEST_CASE("template runtime supports include callback", "[template_vm][include]") {
  constexpr auto src = "{% include include_name %}"_fs;
  auto const ctx = make_object({{"include_name", "dynamic"}});

  auto options = runtime_options{};
  options.include_call = [](std::string_view include_name, inja_object const&) -> std::string {
    return std::string{"<"} + std::string{include_name} + ">";
  };

  REQUIRE(render<src>(ctx, std::cref(options)) == "<dynamic>");
}

TEST_CASE("template runtime supports custom function callbacks", "[template_vm][callback]") {
  constexpr auto src = "{{ twice(value) }}"_fs;
  auto const ctx = make_object({{"value", 21}});

  auto options = runtime_options{};
  options.add_function("twice", [](std::vector<inja_value> const& args) -> inja_value {
    if (args.size() != 1) {
      throw render_error{"twice() expects 1 argument"};
    }
    return inja_value{as_int(args[0]) * 2};
  });

  REQUIRE(render<src>(ctx, std::cref(options)) == "42");
}
