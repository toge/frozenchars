cmake_minimum_required(VERSION 3.25)

set(FROZENCHARS_VERSION "0.5.0")

project(frozenchars VERSION ${FROZENCHARS_VERSION} LANGUAGES CXX)

option(FROZENCHARS_BUILD_TEST "Enable tests" ON)
option(ENABLE_WASI_MINIMAL "Enable WASI minimal (no exceptions) mode - wasi-sdk wasm32-wasip1 assumed" OFF)

if(ENABLE_WASI_MINIMAL)
  add_compile_definitions(FROZENCHARS_WASI_MINIMAL=1)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    $<$<CXX_COMPILER_ID:GNU>:-Wall>
    $<$<CXX_COMPILER_ID:GNU>:-Wextra>
    $<$<CXX_COMPILER_ID:GNU>:-pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-Wall>
    $<$<CXX_COMPILER_ID:Clang>:-Wextra>
    $<$<CXX_COMPILER_ID:Clang>:-pedantic>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wall>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wextra>
    $<$<CXX_COMPILER_ID:AppleClang>:-pedantic>
    $<$<CXX_COMPILER_ID:Clang>:-fconstexpr-steps=100000000>
    $<$<CXX_COMPILER_ID:AppleClang>:-fconstexpr-steps=100000000>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
    $<$<CXX_COMPILER_ID:MSVC>:/constexpr:steps10000000>
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(frozenchars INTERFACE)
add_library(frozenchars::frozenchars ALIAS frozenchars)
target_compile_features(frozenchars INTERFACE cxx_std_23)

find_package(glaze CONFIG QUIET)
if(glaze_FOUND)
    target_link_libraries(frozenchars INTERFACE glaze::glaze)
endif()

target_include_directories(frozenchars
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

install(TARGETS frozenchars
    EXPORT frozencharsTargets
)
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/include/frozenchars.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/include/frozenchars
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/frozencharsConfigVersion.cmake"
    VERSION ${FROZENCHARS_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/frozencharsConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/frozencharsConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/frozenchars"
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/frozencharsConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/frozencharsConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/frozenchars"
)

install(EXPORT frozencharsTargets
    FILE frozencharsTargets.cmake
    NAMESPACE frozenchars::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/frozenchars"
)

if(FROZENCHARS_BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
