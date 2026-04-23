function(assert_perfect_map_compile_fail)
  cmake_parse_arguments(ARG "" "NAME;SOURCE;EXPECTED_TEXT" "" ${ARGN})

  file(SHA256 "${ARG_SOURCE}" TEST_HASH)
  set(TEST_SOURCE_DIR "${CMAKE_CURRENT_BINARY_DIR}/compile_fail/${TEST_HASH}")
  set(TEST_BINARY_DIR "${TEST_SOURCE_DIR}/build")

  file(MAKE_DIRECTORY "${TEST_SOURCE_DIR}")
  file(WRITE "${TEST_SOURCE_DIR}/CMakeLists.txt" "
cmake_minimum_required(VERSION 3.25)
project(perfect_map_compile_fail LANGUAGES CXX)
add_library(compile_fail_case STATIC \"${ARG_SOURCE}\")
target_include_directories(compile_fail_case PRIVATE \"${PROJECT_SOURCE_DIR}/include\")
target_compile_features(compile_fail_case PUBLIC ${STD_CPP})
")

  try_compile(
    COMPILE_RESULT
    "${TEST_BINARY_DIR}"
    "${TEST_SOURCE_DIR}"
    perfect_map_compile_fail
    compile_fail_case
    CMAKE_FLAGS "-DCMAKE_TRY_COMPILE_TARGET_TYPE=LIBRARY"
    OUTPUT_VARIABLE COMPILE_OUTPUT
  )

  if(COMPILE_RESULT)
    message(FATAL_ERROR "${ARG_NAME}: compilation unexpectedly succeeded")
  endif()

  string(FIND "${COMPILE_OUTPUT}" "${ARG_EXPECTED_TEXT}" EXPECTED_POS)
  if(EXPECTED_POS EQUAL -1)
    message(FATAL_ERROR "${ARG_NAME}: expected text not found in compiler output\n${COMPILE_OUTPUT}")
  endif()
endfunction()
