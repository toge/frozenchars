cmake_minimum_required(VERSION 3.25)

set(FROZENCHARS_VERSION "0.8.2")

project(frozenchars VERSION ${FROZENCHARS_VERSION} LANGUAGES CXX)

option(BUILD_TEST "Enable tests" ON)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    # 警告オプションは GCC / Clang 系 / MSVC の 3 種に集約（AppleClang は Clang に含める）
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wextra>
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-pedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(frozenchars INTERFACE)
add_library(frozenchars::frozenchars ALIAS frozenchars)
target_compile_features(frozenchars INTERFACE cxx_std_23)
target_compile_options(frozenchars INTERFACE
    $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-ops-limit=2147483647>
    $<$<CXX_COMPILER_ID:GNU>:-fconstexpr-loop-limit=100000000>
    $<$<CXX_COMPILER_ID:Clang,AppleClang>:-fconstexpr-steps=100000000>
    $<$<CXX_COMPILER_ID:MSVC>:/constexpr:steps10000000>
)

target_include_directories(frozenchars
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

find_package(glaze CONFIG QUIET)
if(glaze_FOUND)
    add_library(frozenchars_glaze INTERFACE)
    add_library(frozenchars::glaze ALIAS frozenchars_glaze)
    set_target_properties(frozenchars_glaze PROPERTIES EXPORT_NAME glaze)
    target_link_libraries(frozenchars_glaze INTERFACE frozenchars glaze::glaze)
endif()

install(TARGETS frozenchars
    EXPORT frozencharsTargets
)
if(TARGET frozenchars_glaze)
    install(TARGETS frozenchars_glaze
        EXPORT frozencharsGlazeTargets
    )
endif()
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
if(TARGET frozenchars_glaze)
    install(EXPORT frozencharsGlazeTargets
        FILE frozencharsGlazeTargets.cmake
        NAMESPACE frozenchars::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/frozenchars"
    )
endif()

if(BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
