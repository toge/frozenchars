# compile-fail ケースを1件検証するスクリプト。
#
# add_compile_fail_test() が ctest のテストとして登録し、`ctest` から
# `cmake -D... -P run_compile_fail.cmake` として実行される。
# 検証が実行時になることで、失敗が名前付きテストとして ctest に報告される。
# （`-P` スクリプトから try_compile は呼べないため、コンパイラを直接起動する）
#
# 必須変数: CASE_NAME / CASE_SOURCE / CASE_INCLUDE_DIR / CASE_CXX_COMPILER /
#           CASE_FLAGS / CASE_OUT_FLAG / CASE_INC_FLAG / CASE_COMPILE_FLAG
# 任意変数: CASE_EXPECTED_TEXT / CASE_EXPECTED_TEXTS / CASE_WORK_DIR

cmake_minimum_required(VERSION 3.25)

foreach(_required CASE_NAME CASE_SOURCE CASE_INCLUDE_DIR CASE_CXX_COMPILER CASE_FLAGS CASE_OUT_FLAG CASE_INC_FLAG CASE_COMPILE_FLAG)
  if(NOT DEFINED ${_required})
    message(FATAL_ERROR "${_required} must be set")
  endif()
endforeach()

if(NOT CASE_WORK_DIR)
  set(CASE_WORK_DIR "${CMAKE_CURRENT_BINARY_DIR}")
endif()

set(_obj "${CASE_WORK_DIR}/${CASE_NAME}.o")
file(MAKE_DIRECTORY "${CASE_WORK_DIR}")

string(REPLACE " " ";" _flags "${CASE_FLAGS}")

execute_process(
  COMMAND "${CASE_CXX_COMPILER}" ${_flags} "${CASE_INC_FLAG}${CASE_INCLUDE_DIR}" "${CASE_COMPILE_FLAG}" "${CASE_SOURCE}" "${CASE_OUT_FLAG}${_obj}"
  RESULT_VARIABLE COMPILE_RESULT
  OUTPUT_VARIABLE COMPILE_OUTPUT
  ERROR_VARIABLE COMPILE_OUTPUT
)

file(REMOVE "${_obj}")

if(COMPILE_RESULT EQUAL 0)
  message(FATAL_ERROR "${CASE_NAME}: compilation unexpectedly succeeded")
endif()

if(CASE_EXPECTED_TEXT)
  string(FIND "${COMPILE_OUTPUT}" "${CASE_EXPECTED_TEXT}" EXPECTED_POS)
else()
  set(EXPECTED_POS -1)
endif()

if(EXPECTED_POS EQUAL -1)
  # MSVC など他コンパイラ向けの代替期待テキストも確認する
  foreach(text ${CASE_EXPECTED_TEXTS})
    string(FIND "${COMPILE_OUTPUT}" "${text}" ALT_POS)
    if(ALT_POS GREATER -1)
      set(EXPECTED_POS 0)
      break()
    endif()
  endforeach()
  if(EXPECTED_POS EQUAL -1)
    message(FATAL_ERROR "${CASE_NAME}: expected text not found in compiler output\n${COMPILE_OUTPUT}")
  endif()
endif()