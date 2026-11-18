cmake_minimum_required(VERSION 3.25)

foreach(_required SOURCE_DIR BINARY_DIR CONSUMER_SOURCE_DIR CMAKE_GENERATOR CMAKE_CXX_COMPILER)
  if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "${_required} is required")
  endif()
endforeach()

file(REMOVE_RECURSE "${BINARY_DIR}")
file(MAKE_DIRECTORY "${BINARY_DIR}")

function(run_checked _label)
  execute_process(
    COMMAND ${ARGN}
    RESULT_VARIABLE _result
    OUTPUT_VARIABLE _stdout
    ERROR_VARIABLE _stderr
  )
  if(NOT _result EQUAL 0)
    message(FATAL_ERROR "${_label} failed (${_result})\n${_stdout}\n${_stderr}")
  endif()
endfunction()

function(run_build _label _binary)
  set(_command "${CMAKE_COMMAND}" --build "${_binary}")
  if(DEFINED CMAKE_BUILD_TYPE AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "")
    list(APPEND _command --config "${CMAKE_BUILD_TYPE}")
  endif()
  run_checked("${_label}" ${_command})
endfunction()

function(run_install _label _binary)
  set(_command "${CMAKE_COMMAND}" --install "${_binary}")
  if(DEFINED CMAKE_BUILD_TYPE AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "")
    list(APPEND _command --config "${CMAKE_BUILD_TYPE}")
  endif()
  run_checked("${_label}" ${_command})
endfunction()

function(configure_project _label _source _binary)
  set(_command
    "${CMAKE_COMMAND}" -S "${_source}" -B "${_binary}" -G "${CMAKE_GENERATOR}"
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
  )
  if(DEFINED CMAKE_GENERATOR_PLATFORM AND NOT "${CMAKE_GENERATOR_PLATFORM}" STREQUAL "")
    list(APPEND _command "-A" "${CMAKE_GENERATOR_PLATFORM}")
  endif()
  if(DEFINED CMAKE_GENERATOR_TOOLSET AND NOT "${CMAKE_GENERATOR_TOOLSET}" STREQUAL "")
    list(APPEND _command "-T" "${CMAKE_GENERATOR_TOOLSET}")
  endif()
  if(DEFINED CMAKE_BUILD_TYPE AND NOT "${CMAKE_BUILD_TYPE}" STREQUAL "")
    list(APPEND _command "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
  endif()
  list(APPEND _command ${ARGN})
  run_checked("${_label}" ${_command})
endfunction()

function(configure_and_build_consumer _kind _prefix _with_glaze_path)
  set(_consumer_args
    "-DCONSUMER_KIND=${_kind}"
    "-DCMAKE_PREFIX_PATH=${_prefix}"
  )
  if(_with_glaze_path)
    if(NOT DEFINED GLAZE_DIR OR "${GLAZE_DIR}" STREQUAL "")
      message(FATAL_ERROR "GLAZE_DIR is required for the glaze consumer")
    endif()
    list(APPEND _consumer_args "-Dglaze_DIR=${GLAZE_DIR}")
  else()
    list(APPEND _consumer_args "-DCMAKE_DISABLE_FIND_PACKAGE_glaze=TRUE")
  endif()
  configure_project(
    "configure ${_kind} consumer"
    "${CONSUMER_SOURCE_DIR}"
    "${BINARY_DIR}/consumer-${_kind}"
    ${_consumer_args}
  )
  run_build("build ${_kind} consumer" "${BINARY_DIR}/consumer-${_kind}")
endfunction()

set(_no_glaze_build "${BINARY_DIR}/library-no-glaze")
set(_no_glaze_prefix "${BINARY_DIR}/install-no-glaze")
configure_project(
  "configure no-glaze library"
  "${SOURCE_DIR}"
  "${_no_glaze_build}"
  "-DBUILD_TEST=OFF"
  "-DCMAKE_DISABLE_FIND_PACKAGE_glaze=TRUE"
  "-DCMAKE_INSTALL_PREFIX=${_no_glaze_prefix}"
)
run_build("build no-glaze library" "${_no_glaze_build}")
run_install("install no-glaze library" "${_no_glaze_build}")
configure_and_build_consumer(core "${_no_glaze_prefix}" FALSE)

if(GLAZE_AVAILABLE)
  configure_and_build_consumer(missing-glaze "${_no_glaze_prefix}" TRUE)

  set(_glaze_build "${BINARY_DIR}/library-glaze")
  set(_glaze_prefix "${BINARY_DIR}/install-glaze")
  configure_project(
    "configure glaze library"
    "${SOURCE_DIR}"
    "${_glaze_build}"
    "-DBUILD_TEST=OFF"
    "-DCMAKE_INSTALL_PREFIX=${_glaze_prefix}"
    "-Dglaze_DIR=${GLAZE_DIR}"
  )
  run_build("build glaze library" "${_glaze_build}")
  run_install("install glaze library" "${_glaze_build}")

  configure_and_build_consumer(core "${_glaze_prefix}" FALSE)
  configure_and_build_consumer(glaze "${_glaze_prefix}" TRUE)
  configure_and_build_consumer(unknown "${_glaze_prefix}" FALSE)
endif()
