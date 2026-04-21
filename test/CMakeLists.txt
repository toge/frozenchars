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
