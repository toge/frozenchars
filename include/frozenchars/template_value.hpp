#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace frozenchars {

struct template_null final {};

struct template_value;
using template_array = std::vector<template_value>;
using template_object = std::unordered_map<std::string, template_value>;

struct template_value {
  using storage_type = std::variant<template_null, bool, std::int64_t, double, std::string, template_array, template_object>;
  storage_type storage{};

  template_value() : storage(template_null{}) {}
  template_value(std::nullptr_t) : storage(template_null{}) {}
  template_value(bool v) : storage(v) {}
  template_value(char const* v) : storage(std::string{v}) {}
  template_value(std::string v) : storage(std::move(v)) {}
  template_value(template_array v) : storage(std::move(v)) {}
  template_value(template_object v) : storage(std::move(v)) {}

  template <typename Int>
  requires(std::is_integral_v<std::remove_cvref_t<Int>> && !std::is_same_v<std::remove_cvref_t<Int>, bool>)
  template_value(Int v) : storage(static_cast<std::int64_t>(v)) {}

  template <typename Float>
  requires(std::is_floating_point_v<std::remove_cvref_t<Float>>)
  template_value(Float v) : storage(static_cast<double>(v)) {}
};

class template_render_error : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

[[nodiscard]] inline auto is_null(template_value const& v) -> bool {
  return std::holds_alternative<template_null>(v.storage);
}

[[nodiscard]] inline auto as_bool(template_value const& v) -> bool {
  if (auto const* p = std::get_if<bool>(&v.storage)) {
    return *p;
  }
  throw template_render_error{"value is not bool"};
}

[[nodiscard]] inline auto as_int(template_value const& v) -> std::int64_t {
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return *p;
  }
  throw template_render_error{"value is not int"};
}

[[nodiscard]] inline auto as_double(template_value const& v) -> double {
  if (auto const* p = std::get_if<double>(&v.storage)) {
    return *p;
  }
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return static_cast<double>(*p);
  }
  throw template_render_error{"value is not number"};
}

[[nodiscard]] inline auto as_string(template_value const& v) -> std::string const& {
  if (auto const* p = std::get_if<std::string>(&v.storage)) {
    return *p;
  }
  throw template_render_error{"value is not string"};
}

[[nodiscard]] inline auto as_array(template_value const& v) -> template_array const& {
  if (auto const* p = std::get_if<template_array>(&v.storage)) {
    return *p;
  }
  throw template_render_error{"value is not array"};
}

[[nodiscard]] inline auto as_object(template_value const& v) -> template_object const& {
  if (auto const* p = std::get_if<template_object>(&v.storage)) {
    return *p;
  }
  throw template_render_error{"value is not object"};
}

[[nodiscard]] inline auto make_template_array(std::initializer_list<template_value> items) -> template_value {
  return template_value{template_array{items}};
}

[[nodiscard]] inline auto make_template_object(std::initializer_list<std::pair<std::string, template_value>> items) -> template_value {
  auto out = template_object{};
  for (auto const& [k, v] : items) {
    out.insert_or_assign(k, v);
  }
  return template_value{std::move(out)};
}

[[nodiscard]] inline auto template_truthy(template_value const& v) -> bool {
  if (std::holds_alternative<template_null>(v.storage)) {
    return false;
  }
  if (auto const* p = std::get_if<bool>(&v.storage)) {
    return *p;
  }
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return *p != 0;
  }
  if (auto const* p = std::get_if<double>(&v.storage)) {
    return *p != 0.0;
  }
  if (auto const* p = std::get_if<std::string>(&v.storage)) {
    return !p->empty();
  }
  if (auto const* p = std::get_if<template_array>(&v.storage)) {
    return !p->empty();
  }
  return !std::get<template_object>(v.storage).empty();
}

[[nodiscard]] inline auto template_to_string(template_value const& v) -> std::string {
  if (std::holds_alternative<template_null>(v.storage)) {
    return "null";
  }
  if (auto const* p = std::get_if<bool>(&v.storage)) {
    return *p ? "true" : "false";
  }
  if (auto const* p = std::get_if<std::int64_t>(&v.storage)) {
    return std::to_string(*p);
  }
  if (auto const* p = std::get_if<double>(&v.storage)) {
    auto s = std::to_string(*p);
    while (s.size() > 2 && s.back() == '0' && s[s.size() - 2] != '.') {
      s.pop_back();
    }
    if (!s.empty() && s.back() == '.') {
      s.pop_back();
    }
    return s;
  }
  if (auto const* p = std::get_if<std::string>(&v.storage)) {
    return *p;
  }
  if (std::holds_alternative<template_array>(v.storage)) {
    return "[array]";
  }
  return "{object}";
}

} // namespace frozenchars
