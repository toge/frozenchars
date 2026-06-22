cmake_minimum_required(VERSION 3.25)
project(frozenchars LANGUAGES CXX)

set(FROZENCHARS_VERSION "0.1.0")

option(BUILD_TEST "Enable tests" ON)

## add warning
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g3")
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fconstexpr-steps=10000000")
endif()

if (MSVC)
    if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
        string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
    endif()
endif()

## optimization
if (CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native")
endif()
if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ox")
endif()

## other flag
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

if(BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
