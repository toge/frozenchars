#include <catch2/catch_test_macros.hpp>
#include <frozenchars/fast_inja.hpp>
#include <glaze/glaze.hpp>
#include <vector>

namespace fj = frozenchars::fast_inja;

// ---- テスト用データ型 ----

struct FlatData {
  std::string name;
  int age{};
  bool active{};
};

template <>
struct glz::meta<FlatData> {
  static constexpr auto value = glz::object("name", &FlatData::name, "age", &FlatData::age, "active", &FlatData::active);
};

struct UserItem {
  std::string name;
};

struct UsersData {
  std::vector<UserItem> users;
  bool on{};
};

template <>
struct glz::meta<UserItem> {
  static constexpr auto value = glz::object("name", &UserItem::name);
};

template <>
struct glz::meta<UsersData> {
  static constexpr auto value = glz::object("users", &UsersData::users, "on", &UsersData::on);
};

struct TreeNode {
  std::string name;
  std::vector<TreeNode> children;
};

template <>
struct glz::meta<TreeNode> {
  static constexpr auto value = glz::object("name", &TreeNode::name, "children", &TreeNode::children);
};

struct IfData {
  std::string name;
  int age{};
};

struct IfUsersData {
  std::vector<IfData> users;
};

template <>
struct glz::meta<IfData> {
  static constexpr auto value = glz::object("name", &IfData::name, "age", &IfData::age);
};

template <>
struct glz::meta<IfUsersData> {
  static constexpr auto value = glz::object("users", &IfUsersData::users);
};

struct Address {
  std::string city;
  std::string country;
};

struct Founder {
  std::string name;
  Address address;
};

struct Company {
  std::string name;
  Founder founder;
};

struct WithAddress {
  Address address;
};

template <>
struct glz::meta<Address> {
  static constexpr auto value = glz::object("city", &Address::city, "country", &Address::country);
};

template <>
struct glz::meta<Founder> {
  static constexpr auto value = glz::object("name", &Founder::name, "address", &Founder::address);
};

template <>
struct glz::meta<Company> {
  static constexpr auto value = glz::object("name", &Company::name, "founder", &Company::founder);
};

template <>
struct glz::meta<WithAddress> {
  static constexpr auto value = glz::object("address", &WithAddress::address);
};

struct HtmlData {
  std::string html;
  std::string name;
};

template <>
struct glz::meta<HtmlData> {
  static constexpr auto value = glz::object("html", &HtmlData::html, "name", &HtmlData::name);
};

struct RuntimeData {
  std::string name;
  int value{};
};

template <>
struct glz::meta<RuntimeData> {
  static constexpr auto value = glz::object("name", &RuntimeData::name, "value", &RuntimeData::value);
};

struct AtUser {
  std::string name;
};

struct AtGroup {
  std::vector<AtUser> users;
};

struct AtGroupList {
  std::vector<AtGroup> groups;
};

template <>
struct glz::meta<AtUser> {
  static constexpr auto value = glz::object("name", &AtUser::name);
};

template <>
struct glz::meta<AtGroup> {
  static constexpr auto value = glz::object("users", &AtGroup::users);
};

template <>
struct glz::meta<AtGroupList> {
  static constexpr auto value = glz::object("groups", &AtGroupList::groups);
};

struct UsersCtx {
  std::vector<AtUser> users;
};

template <>
struct glz::meta<UsersCtx> {
  static constexpr auto value = glz::object("users", &UsersCtx::users);
};

// ---- expected 型テスト ----

TEST_CASE("expected typedef", "[expected]") {
  fj::expected<int> e = 42;
  REQUIRE(e.has_value());
  REQUIRE(*e == 42);

  fj::expected<int> err = std::unexpected(fj::error_ctx{.ec = fj::error_code::unknown_key});
  REQUIRE(!err.has_value());
  REQUIRE(err.error().ec == fj::error_code::unknown_key);
}

// ---- シリアライズテスト ----

TEST_CASE("serialize int positive", "[serialize]") {
  std::string out;
  fj::detail::serialize_value(out, 42);
  REQUIRE(out == "42");
}

TEST_CASE("serialize int zero", "[serialize]") {
  std::string out;
  fj::detail::serialize_value(out, 0);
  REQUIRE(out == "0");
}

TEST_CASE("serialize int negative", "[serialize]") {
  std::string out;
  fj::detail::serialize_value(out, -123);
  REQUIRE(out == "-123");
}

// ---- フィールド解決テスト ----

TEST_CASE("resolve string field", "[resolve]") {
  FlatData data{"alice", 30, true};
  std::string out;
  bool found = fj::detail::resolve_value(out, "name", data, nullptr);
  REQUIRE(found);
  REQUIRE(out == "alice");
}

TEST_CASE("resolve int field", "[resolve]") {
  FlatData data{"alice", 30, true};
  std::string out;
  bool found = fj::detail::resolve_value(out, "age", data, nullptr);
  REQUIRE(found);
  REQUIRE(out == "30");
}

TEST_CASE("resolve bool field", "[resolve]") {
  FlatData data{"alice", 30, true};
  std::string out;
  bool found = fj::detail::resolve_value(out, "active", data, nullptr);
  REQUIRE(found);
  REQUIRE(out == "true");
}

// ---- パーステスト ----

TEST_CASE("parse empty", "[parse]") {
  auto chunks = fj::detail::parse("");
  REQUIRE(chunks.size() == 0);
}

TEST_CASE("parse plain text", "[parse]") {
  auto chunks = fj::detail::parse("hello world");
  REQUIRE(chunks.size() == 1);
  REQUIRE(std::holds_alternative<fj::detail::chunk_literal>(chunks[0]));
  REQUIRE(std::get<fj::detail::chunk_literal>(chunks[0]).text == "hello world");
}

TEST_CASE("parse single placeholder", "[parse]") {
  auto chunks = fj::detail::parse("hello {{name}}!");
  REQUIRE(chunks.size() == 3);
  REQUIRE(std::holds_alternative<fj::detail::chunk_literal>(chunks[0]));
  REQUIRE(std::holds_alternative<fj::detail::chunk_placeholder>(chunks[1]));
  REQUIRE(std::holds_alternative<fj::detail::chunk_literal>(chunks[2]));
  REQUIRE(std::get<fj::detail::chunk_placeholder>(chunks[1]).key == "name");
  REQUIRE(std::get<fj::detail::chunk_placeholder>(chunks[1]).raw == false);
}

TEST_CASE("parse raw placeholder", "[parse]") {
  auto chunks = fj::detail::parse("{{{html}}}");
  REQUIRE(chunks.size() == 1);
  auto& ph = std::get<fj::detail::chunk_placeholder>(chunks[0]);
  REQUIRE(ph.key == "html");
  REQUIRE(ph.raw == true);
}

TEST_CASE("parse lone open brace", "[parse]") {
  auto chunks = fj::detail::parse("{");
  REQUIRE(chunks.size() == 1);
  REQUIRE(std::holds_alternative<fj::detail::chunk_literal>(chunks[0]));
  REQUIRE(std::get<fj::detail::chunk_literal>(chunks[0]).text == "{");
}

// ---- フラットレンダリングテスト ----

TEST_CASE("render plain text only", "[render]") {
  FlatData data{"alice", 30, true};
  auto result = fj::render_runtime("hello world", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "hello world");
}

TEST_CASE("render with placeholder", "[render]") {
  FlatData data{"alice", 30, true};
  auto result = fj::render_runtime("name={{name}}, age={{age}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "name=alice, age=30");
}

TEST_CASE("render unknown key", "[render]") {
  FlatData data{"alice", 30, true};
  auto result = fj::render_runtime("{{unknown}}", data);
  REQUIRE(!result.has_value());
  REQUIRE(result.error().ec == fj::error_code::unknown_key);
}

// ---- セクションテスト ----

TEST_CASE("parse simple section", "[parse][section]") {
  auto chunks = fj::detail::parse("<ul>{{#users}}<li>{{name}}</li>{{/users}}</ul>");
  REQUIRE(chunks.size() == 3);
  REQUIRE(std::holds_alternative<fj::detail::chunk_literal>(chunks[0]));
  REQUIRE(std::holds_alternative<fj::detail::chunk_section>(chunks[1]));
  REQUIRE(std::holds_alternative<fj::detail::chunk_literal>(chunks[2]));

  auto& sec = std::get<fj::detail::chunk_section>(chunks[1]);
  REQUIRE(sec.key == "users");
  REQUIRE(sec.body.size() == 3);
}

TEST_CASE("parse inverted section", "[parse][section]") {
  auto chunks = fj::detail::parse("{{^empty}}none{{/empty}}");
  REQUIRE(chunks.size() == 1);
  REQUIRE(std::holds_alternative<fj::detail::chunk_inverted>(chunks[0]));

  auto& inv = std::get<fj::detail::chunk_inverted>(chunks[0]);
  REQUIRE(inv.key == "empty");
  REQUIRE(inv.body.size() == 1);
}

TEST_CASE("section over vector", "[render][section]") {
  UsersData data{.users = {{"a"}, {"b"}}, .on = false};
  auto result = fj::render_runtime("{{#users}}<{{name}}>{{/users}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "<a><b>");
}

TEST_CASE("empty section", "[render][section]") {
  UsersData data{.users = {}, .on = false};
  auto result = fj::render_runtime("[{{#users}}{{name}}{{/users}}]", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "[]");
}

TEST_CASE("bool section true", "[render][section]") {
  UsersData data{.users = {}, .on = true};
  auto result = fj::render_runtime("{{#on}}YES{{/on}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "YES");
}

TEST_CASE("bool section false", "[render][section]") {
  UsersData data{.users = {}, .on = false};
  auto result = fj::render_runtime("{{#on}}YES{{/on}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "");
}

TEST_CASE("inverted section", "[render][section]") {
  UsersData data{.users = {}, .on = false};
  auto result = fj::render_runtime("{{^on}}NO{{/on}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "NO");
}

TEST_CASE("nested sections", "[render][section]") {
  TreeNode root{"a", {{"a1", {}}, {"a2", {}}}};
  auto result = fj::render_runtime("{{name}}>({{#children}}{{name}},{{/children}});", root);
  REQUIRE(result.has_value());
  REQUIRE(*result == "a>(a1,a2,);");
}

// ---- @vars テスト ----

TEST_CASE("loop state default", "[loop_state]") {
  fj::detail::loop_state ls;
  REQUIRE(ls.index == 0);
  REQUIRE(ls.count == 1);
  REQUIRE(ls.is_first());
  REQUIRE_FALSE(ls.is_last());
}

TEST_CASE("loop state middle", "[loop_state]") {
  fj::detail::loop_state ls;
  ls.index = 1;
  ls.count = 3;
  REQUIRE_FALSE(ls.is_first());
  REQUIRE_FALSE(ls.is_last());
}

TEST_CASE("loop state last", "[loop_state]") {
  fj::detail::loop_state ls;
  ls.index = 2;
  ls.count = 3;
  REQUIRE_FALSE(ls.is_first());
  REQUIRE(ls.is_last());
}

TEST_CASE("at index in section", "[render][at_vars]") {
  UsersCtx data{.users = {{"a"}, {"b"}, {"c"}}};
  auto result = fj::render_runtime("{{#users}}{{@index}}:{{name}};{{/users}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "0:a;1:b;2:c;");
}

TEST_CASE("at first in section", "[render][at_vars]") {
  UsersCtx data{.users = {{"a"}, {"b"}}};
  auto result = fj::render_runtime("{{#users}}[{{@first}}]{{name}};{{/users}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "[true]a;[false]b;");
}

TEST_CASE("at last in section", "[render][at_vars]") {
  UsersCtx data{.users = {{"a"}, {"b"}}};
  auto result = fj::render_runtime("{{#users}}[{{@last}}]{{name}};{{/users}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "[false]a;[true]b;");
}

TEST_CASE("nested at index", "[render][at_vars]") {
  AtGroupList data{.groups = {
                      AtGroup{.users = {{"x"}, {"y"}}},
                      AtGroup{.users = {{"z"}}},
                  }};
  auto result = fj::render_runtime("{{#groups}}[{{#users}}{{@index}}{{/users}}]{{/groups}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "[01][0]");
}

TEST_CASE("at last section", "[render][at_vars]") {
  UsersCtx data{.users = {{"a"}, {"b"}}};
  auto result = fj::render_runtime("{{#users}}{{name}}{{#@last}}.{{/@last}}{{/users}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "ab.");
}

TEST_CASE("at first section", "[render][at_vars]") {
  UsersCtx data{.users = {{"a"}, {"b"}, {"c"}}};
  auto result = fj::render_runtime(
      "{{#users}}{{#@first}}[{{/@first}}{{name}}{{#@first}}]{{/@first}};{{/users}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "[a];b;c;");
}

// ---- パステスト ----

TEST_CASE("resolve path simple", "[resolve][path]") {
  WithAddress data{.address = Address{.city = "Tokyo", .country = "JP"}};
  std::string out;
  bool found = fj::detail::resolve_value(out, "address.city", data, nullptr);
  REQUIRE(found);
  REQUIRE(out == "Tokyo");
}

TEST_CASE("resolve path 3 levels", "[resolve][path]") {
  Founder data{.name = "John", .address = Address{.city = "NYC", .country = "USA"}};
  std::string out;
  bool found = fj::detail::resolve_value(out, "address.city", data, nullptr);
  REQUIRE(found);
  REQUIRE(out == "NYC");
}

TEST_CASE("resolve path 4 levels", "[resolve][path]") {
  Company data{
      .name = "Acme",
      .founder = Founder{.name = "John", .address = Address{.city = "NYC", .country = "USA"}}};
  std::string out;
  bool found = fj::detail::resolve_value(out, "founder.address.country", data, nullptr);
  REQUIRE(found);
  REQUIRE(out == "USA");
}

TEST_CASE("resolve path unknown", "[resolve][path]") {
  WithAddress data{.address = Address{.city = "Tokyo", .country = "JP"}};
  std::string out;
  bool found = fj::detail::resolve_value(out, "address.bogus", data, nullptr);
  REQUIRE_FALSE(found);
}

TEST_CASE("resolve path partial unknown", "[resolve][path]") {
  WithAddress data{.address = Address{.city = "Tokyo", .country = "JP"}};
  std::string out;
  bool found = fj::detail::resolve_value(out, "bogus.city", data, nullptr);
  REQUIRE_FALSE(found);
}

TEST_CASE("render nested path", "[render][path]") {
  Company data{
      .name = "Acme",
      .founder = Founder{.name = "John", .address = Address{.city = "NYC", .country = "USA"}}};
  auto result = fj::render_runtime("{{name}} / {{founder.name}} / {{founder.address.city}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "Acme / John / NYC");
}

// ---- if/else テスト ----

TEST_CASE("parse if with else", "[parse][if]") {
  auto chunks = fj::detail::parse("{{#if x}}T{{else}}F{{/if}}");
  REQUIRE(chunks.size() == 1);
  REQUIRE(std::holds_alternative<fj::detail::chunk_if>(chunks[0]));

  auto& ci = std::get<fj::detail::chunk_if>(chunks[0]);
  REQUIRE(ci.expr == "x");
  REQUIRE(ci.then_branch.size() == 1);
  REQUIRE(ci.else_branch.size() == 1);

  auto const& then_lit = std::get<fj::detail::chunk_literal>(ci.then_branch[0].chunks[0]);
  REQUIRE(then_lit.text == "T");

  auto const& else_lit = std::get<fj::detail::chunk_literal>(ci.else_branch[0].chunks[0]);
  REQUIRE(else_lit.text == "F");
}

TEST_CASE("parse if no else", "[parse][if]") {
  auto chunks = fj::detail::parse("{{#if x}}T{{/if}}");
  REQUIRE(chunks.size() == 1);
  auto& ci = std::get<fj::detail::chunk_if>(chunks[0]);
  REQUIRE(ci.then_branch.size() == 1);
  REQUIRE(ci.else_branch.size() == 0);
}

TEST_CASE("if bool field true", "[render][if]") {
  IfData data{"alice", 20};
  auto result = fj::render_runtime("{{#if age}}adult{{/if}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "adult");
}

TEST_CASE("if bool field false", "[render][if]") {
  IfData data{"alice", 0};
  auto result = fj::render_runtime("{{#if age}}adult{{/if}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "");
}

TEST_CASE("if with at last true", "[render][if]") {
  IfUsersData data{.users = {{"a", 1}, {"b", 2}}};
  auto result = fj::render_runtime("{{#users}}{{name}}{{#if @last}}.{{/if}}{{/users}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "ab.");
}

TEST_CASE("if else bool field", "[render][if]") {
  IfData data{"alice", 0};
  auto result = fj::render_runtime("{{#if age}}A{{else}}B{{/if}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "B");
}

TEST_CASE("if else with section", "[render][if]") {
  IfUsersData data{.users = {{"a", 1}, {"b", 2}, {"c", 3}}};
  auto result = fj::render_runtime("{{#users}}{{name}}{{#if @last}}.{{else}},{{/if}}{{/users}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "a,b,c.");
}

// ---- エスケープテスト ----

TEST_CASE("html escape plain", "[escape]") {
  std::string out = "prefix:";
  fj::detail::html_escape_into(out, "hello");
  REQUIRE(out == "prefix:hello");
}

TEST_CASE("html escape no special", "[escape]") {
  std::string out;
  fj::detail::html_escape_into(out, "no special chars here");
  REQUIRE(out == "no special chars here");
}

TEST_CASE("html escape all chars", "[escape]") {
  std::string out;
  fj::detail::html_escape_into(out, "<>&\"'");
  REQUIRE(out == "&lt;&gt;&amp;&quot;&#x27;");
}

TEST_CASE("render stencil no escape", "[render][escape]") {
  HtmlData data{.html = "a&b", .name = "test"};
  auto result = fj::render_runtime<fj::stencil_tag>("{{html}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "a&b");
}

TEST_CASE("render mustache escape", "[render][escape]") {
  HtmlData data{.html = "<b>a&b</b>", .name = "test"};
  auto result = fj::render_runtime<fj::mustache_tag>("{{html}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "&lt;b&gt;a&amp;b&lt;/b&gt;");
}

TEST_CASE("render mustache raw no escape", "[render][escape]") {
  HtmlData data{.html = "<b>bold</b>", .name = "test"};
  auto result = fj::render_runtime<fj::mustache_tag>("{{{html}}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "<b>bold</b>");
}

// ---- ランタイム API テスト ----

TEST_CASE("render runtime simple", "[render][runtime]") {
  RuntimeData data{"world", 42};
  auto result = fj::render_runtime("hello {{name}}", data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "hello world");
}

TEST_CASE("render parsed simple", "[render][runtime]") {
  auto pt = fj::parse_template("{{name}}={{value}}");
  RuntimeData data{"answer", 42};
  auto result = fj::render_parsed(pt, data);
  REQUIRE(result.has_value());
  REQUIRE(*result == "answer=42");
}

TEST_CASE("render parsed reuse", "[render][runtime]") {
  auto pt = fj::parse_template("{{name}}");
  RuntimeData d1{"alice", 1};
  RuntimeData d2{"bob", 2};
  auto r1 = fj::render_parsed(pt, d1);
  auto r2 = fj::render_parsed(pt, d2);
  REQUIRE(*r1 == "alice");
  REQUIRE(*r2 == "bob");
}

TEST_CASE("render runtime error propagation", "[render][runtime]") {
  RuntimeData data{"test", 1};
  auto result = fj::render_runtime("{{name}} {{nonexistent}}", data);
  REQUIRE(!result.has_value());
  REQUIRE(result.error().ec == fj::error_code::unknown_key);
}
