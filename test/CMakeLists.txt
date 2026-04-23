find_package(Catch2 REQUIRED CONFIG)
find_package(ctre REQUIRED CONFIG)

file(GLOB test_src test_*.cpp)

add_executable(all_test main.cpp ${test_src})

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

include(${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/run_compile_fail.cmake)

assert_static_perfect_map_compile_fail(
  NAME static_perfect_map_duplicate_keys
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/static_perfect_map_duplicate_keys.cpp
  EXPECTED_TEXT "StaticPerfectMap keys must be unique"
)

assert_static_perfect_map_compile_fail(
  NAME static_perfect_map_empty_keys
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/static_perfect_map_empty_keys.cpp
  EXPECTED_TEXT "StaticPerfectMap requires at least one key"
)

assert_static_perfect_map_compile_fail(
  NAME static_perfect_map_seed_limit_zero
  SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/compile_fail/static_perfect_map_seed_limit_zero.cpp
  EXPECTED_TEXT "StaticPerfectMap seed search exhausted"
)
