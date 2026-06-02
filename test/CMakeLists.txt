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
target_compile_features(all_test PUBLIC ${STD_CPP})

add_test(NAME all_test COMMAND all_test -r junit)

add_executable(example_smoke ../example.cpp)
target_link_libraries(example_smoke PRIVATE frozenchars::frozenchars)
target_compile_features(example_smoke PUBLIC ${STD_CPP})
add_test(NAME example_smoke COMMAND example_smoke)

add_executable(example_render_with_output_buffer ../example_render_with_output_buffer.cpp)
target_link_libraries(example_render_with_output_buffer PRIVATE frozenchars::frozenchars)
target_compile_features(example_render_with_output_buffer PUBLIC ${STD_CPP})
add_test(NAME example_render_with_output_buffer COMMAND example_render_with_output_buffer)

add_executable(bench_inja_runtime benchmark_inja_runtime.cpp)
target_link_libraries(bench_inja_runtime PRIVATE frozenchars::frozenchars)
target_compile_features(bench_inja_runtime PUBLIC ${STD_CPP})

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
  NAME join_lines_sep_too_long
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/join_lines_sep_too_long.cpp
  EXPECTED_TEXT "join_lines: separator exceeds MAX_SEP_LEN"
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

# example_split executable
add_executable(example_split ../example_split.cpp)
target_compile_features(example_split PRIVATE ${STD_CPP})
target_link_libraries(example_split PRIVATE frozenchars::frozenchars)
add_test(NAME example_split COMMAND example_split)
