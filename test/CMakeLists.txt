find_package(Catch2 REQUIRED CONFIG)
find_package(ctre REQUIRED CONFIG)

file(GLOB test_src test_*.cpp)
list(FILTER test_src EXCLUDE REGEX "test_split_(direct|logic|tester|v_debug)\\.cpp$")

add_executable(all_test ${test_src})

target_link_libraries(all_test PRIVATE
  frozenchars::frozenchars
  Catch2::Catch2
  Catch2::Catch2WithMain
  ctre::ctre
)
target_compile_features(all_test PUBLIC cxx_std_23)

add_test(NAME all_test COMMAND all_test -r junit)

add_executable(example_smoke ../example.cpp)
target_link_libraries(example_smoke PRIVATE frozenchars::frozenchars)
target_compile_features(example_smoke PUBLIC cxx_std_23)
add_test(NAME example_smoke COMMAND example_smoke)

add_executable(bench_frozen_map bench_frozen_map.cpp)
target_link_libraries(bench_frozen_map PRIVATE frozenchars::frozenchars)
target_compile_features(bench_frozen_map PUBLIC cxx_std_23)

add_executable(bench_frozen_set bench_frozen_set.cpp)
target_link_libraries(bench_frozen_set PRIVATE frozenchars::frozenchars)
target_compile_features(bench_frozen_set PUBLIC cxx_std_23)

# wildcards benchmark dependency: fetched at configure time
# (https://github.com/zemasoft/wildcards)
include(FetchContent)
cmake_policy(SET CMP0169 OLD)
FetchContent_Declare(
  wildcards
  GIT_REPOSITORY https://github.com/zemasoft/wildcards.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE
)
FetchContent_Populate(wildcards)
# Patch is_alt bug: return-type mismatch in cfg_HAS_CONSTEXPR14 +
# !cfg_HAS_FULL_FEATURED_CONSTEXPR14 path (upstream has
# `return throw_logic_error(p, ...)` returning PatternIterator from bool fn).
set(_wc_patched "${wildcards_SOURCE_DIR}/single_include/wildcards.hpp")
file(READ "${_wc_patched}" _wc_content)
string(REPLACE
  "break;\ncase is_alt_state::escape:\nstate = is_alt_state::next;\nbreak;\ndefault:\n#if cfg_HAS_FULL_FEATURED_CONSTEXPR14\nthrow std::logic_error(\n\"The program execution should never end up here throwing this exception\");\n#else\nreturn throw_logic_error(\np, \"The program execution should never end up here throwing this exception\");\n#endif"
  "break;\ncase is_alt_state::escape:\nstate = is_alt_state::next;\nbreak;\ndefault:\n#if cfg_HAS_FULL_FEATURED_CONSTEXPR14\nthrow std::logic_error(\n\"The program execution should never end up here throwing this exception\");\n#else\nthrow_logic_error(\np, \"The program execution should never end up here throwing this exception\");\nreturn false;\n#endif"
  _wc_content "${_wc_content}")
file(WRITE "${_wc_patched}" "${_wc_content}")
unset(_wc_patched)
unset(_wc_content)
add_library(wildcards INTERFACE)
target_include_directories(wildcards INTERFACE ${wildcards_SOURCE_DIR}/single_include)

add_executable(bench_wildcard bench_wildcard.cpp)
target_link_libraries(bench_wildcard PRIVATE frozenchars::frozenchars wildcards)
target_compile_features(bench_wildcard PUBLIC cxx_std_23)

include(${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/run_compile_fail.cmake)

assert_frozen_map_compile_fail(
  NAME frozen_map_duplicate_keys
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_map_duplicate_keys.cpp
  EXPECTED_TEXT "frozen_map keys must be unique"
)

assert_frozen_map_compile_fail(
  NAME frozen_map_empty_keys
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_map_empty_keys.cpp
  EXPECTED_TEXT "frozen_map requires at least one key"
)

assert_frozen_map_compile_fail(
  NAME frozen_map_make_frozen_map_wrong_arity
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_map_make_frozen_map_wrong_arity.cpp
  EXPECTED_TEXT "make_frozen_map requires exactly one entry per key"
)

assert_frozen_map_compile_fail(
  NAME frozen_map_make_frozen_map_bad_key
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_map_make_frozen_map_bad_key.cpp
  EXPECTED_TEXT "frozen_map entry key must be convertible to std::string_view"
)

assert_frozen_map_compile_fail(
  NAME frozen_map_make_frozen_map_bad_value
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_map_make_frozen_map_bad_value.cpp
  EXPECTED_TEXT "frozen_map entry value must be constructible as mapped_type"
)

assert_frozen_map_compile_fail(
  NAME frozen_set_duplicate_keys
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_set_duplicate_keys.cpp
  EXPECTED_TEXT "frozen_set keys must be unique"
)

assert_frozen_map_compile_fail(
  NAME frozen_set_empty_keys
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/frozen_set_empty_keys.cpp
  EXPECTED_TEXT "frozen_set requires at least one key"
)

assert_frozen_map_compile_fail(
  NAME join_lines_sep_too_long
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/join_lines_sep_too_long.cpp
  EXPECTED_TEXT "join_lines: separator exceeds MAX_SEP_LEN"
  EXPECTED_TEXTS "expression did not evaluate to a constant"
)

assert_frozen_map_compile_fail(
  NAME to_sv_missing_arg
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/to_sv_missing_arg.cpp
  EXPECTED_TEXT "format"
)

assert_frozen_map_compile_fail(
  NAME to_sv_type_mismatch
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/to_sv_type_mismatch.cpp
  EXPECTED_TEXT "format"
)

assert_frozen_map_compile_fail(
  NAME parse_to_tuple_void_pointer
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/parse_to_tuple_void_pointer.cpp
  EXPECTED_TEXT "Unknown type name"
)

# example_split executable
add_executable(example_split ../example_split.cpp)
target_compile_features(example_split PRIVATE cxx_std_23)
target_link_libraries(example_split PRIVATE frozenchars::frozenchars)
add_test(NAME example_split COMMAND example_split)

add_executable(example_frozen_map_use_cases ../example_frozen_map_use_cases.cpp)
target_compile_features(example_frozen_map_use_cases PRIVATE cxx_std_23)
target_link_libraries(example_frozen_map_use_cases PRIVATE frozenchars::frozenchars)
add_test(NAME example_frozen_map_use_cases COMMAND example_frozen_map_use_cases)

add_executable(example_parse_to_tuple_use_cases ../example_parse_to_tuple_use_cases.cpp)
target_compile_features(example_parse_to_tuple_use_cases PRIVATE cxx_std_23)
target_link_libraries(example_parse_to_tuple_use_cases PRIVATE frozenchars::frozenchars)
add_test(NAME example_parse_to_tuple_use_cases COMMAND example_parse_to_tuple_use_cases)
