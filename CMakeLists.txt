cmake_minimum_required(VERSION 3.16)
project(specs_optical_compensation VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

add_library(optical_compensation
    src/optical_compensation.cpp
    src/perceptual_render_bridge.cpp)
add_library(SPECS::optical_compensation ALIAS optical_compensation)
target_include_directories(optical_compensation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(optical_compensation PUBLIC cxx_std_17)
set_target_properties(optical_compensation PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
if(MSVC)
  target_compile_options(optical_compensation PRIVATE /W4)
else()
  target_compile_options(optical_compensation PRIVATE -Wall -Wextra -Wpedantic)
endif()

configure_package_config_file(
    cmake/SPECSOpticalCompensationConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/SPECSOpticalCompensationConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SPECSOpticalCompensation")
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/SPECSOpticalCompensationConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(TARGETS optical_compensation
    EXPORT SPECSOpticalCompensationTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY include/ DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(EXPORT SPECSOpticalCompensationTargets
    FILE SPECSOpticalCompensationTargets.cmake
    NAMESPACE SPECS::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SPECSOpticalCompensation")
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/SPECSOpticalCompensationConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/SPECSOpticalCompensationConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/SPECSOpticalCompensation")

include(CTest)
if(BUILD_TESTING)
  add_executable(optical_compensation_test tests/test_bridge.cpp)
  target_link_libraries(optical_compensation_test PRIVATE optical_compensation)
  add_test(NAME optical_compensation_test COMMAND optical_compensation_test)
endif()
