cmake_minimum_required(VERSION 3.16)
project(specs_optical_compensation VERSION 1.0.0 LANGUAGES CXX)

add_library(optical_compensation
    src/optical_compensation.cpp
    src/perceptual_render_bridge.cpp)
target_include_directories(optical_compensation PUBLIC include)
target_compile_features(optical_compensation PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(optical_compensation PRIVATE /W4)
else()
  target_compile_options(optical_compensation PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(optical_compensation_test tests/test_bridge.cpp)
  target_link_libraries(optical_compensation_test PRIVATE optical_compensation)
  add_test(NAME optical_compensation_test COMMAND optical_compensation_test)
endif()
