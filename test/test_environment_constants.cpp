#include "catch2/catch_all.hpp"

#include <cctype>
#include <string>

#include "frozenchars.hpp"

using namespace frozenchars;
using namespace frozenchars::inja;
using namespace frozenchars::literals;

namespace {

auto shout_impl(std::vector<inja_value> const& args) -> inja_value {
  if (args.size() != 1) {
    throw render_error{"shout() expects 1 argument"};
  }
  if (!std::holds_alternative<std::string>(args[0].storage)) {
    throw render_error{"shout() expects string argument"};
  }
  auto out = std::get<std::string>(args[0].storage);
  for (auto& ch : out) {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }
  return inja_value{std::move(out)};
}

struct build_time_provider {
  static auto value() -> inja_value {
    return inja_value{"2026-05-24T00:00:00Z"};
  }
};

} // namespace

TEST_CASE("environment constants resolve when runtime variable is absent", "[environment][constants]") {
  using constants = constant_list<
    constant<"app_name", "FrozenApp">,
    constant<"version", "1.0">
  >;
  using env = environment<default_function_list, constants>;

  constexpr auto src = "{{ app_name }} {{ version }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src, env>(ctx) == "FrozenApp 1.0");
}

TEST_CASE("environment prefers runtime variable over compile-time constant", "[environment][constants]") {
  using constants = constant_list<constant<"version", "1.0">>;
  using env = environment<default_function_list, constants>;

  constexpr auto src = "{{ version }}"_fs;
  auto const ctx = make_object({
    {"version", "2.0"},
  });
  REQUIRE(render<src, env>(ctx) == "2.0");
}

TEST_CASE("environment constants support numeric and bool literals", "[environment][constants]") {
  using constants = constant_list<
    constant<"max_retries", 3>,
    constant<"pi", 3.14>,
    constant<"feature_on", true>
  >;
  using env = environment<default_function_list, constants>;

  constexpr auto src = "{{ max_retries }},{{ pi }},{{ feature_on }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src, env>(ctx) == "3,3.14,true");
}

TEST_CASE("environment integrates compile-time functions and constants", "[environment][integration]") {
  using functions = function_list<fn<"shout", shout_impl>>;
  using constants = constant_list<constant<"app_name", "FrozenApp">>;
  using env = environment<functions, constants>;

  constexpr auto src = "{{ shout(app_name) }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src, env>(ctx) == "FROZENAPP");
}

TEST_CASE("environment supports provider-based constant entries", "[environment][constants]") {
  using constants = constant_list<constant_provider<"build_time", build_time_provider>>;
  using env = environment<default_function_list, constants>;

  constexpr auto src = "{{ build_time }}"_fs;
  auto const ctx = make_object({});
  REQUIRE(render<src, env>(ctx) == "2026-05-24T00:00:00Z");
}

TEST_CASE("compile_template supports environment bindings", "[environment][compile]") {
  using constants = constant_list<constant<"app_name", "FrozenApp">>;
  using env = environment<default_function_list, constants>;
  constexpr auto src = "{{ app_name }}"_fs;

  constexpr auto program = compile_template<src, env>();
  static_assert(program.count > 0);
  REQUIRE(program.count > 0);
}
