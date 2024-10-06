cmake_minimum_required(VERSION 3.25)

include(scripts/cmake/bundled-vcpkg.cmake)
enable_vcpkg(VERSION 2024.09.30 PATH "${CMAKE_CURRENT_SOURCE_DIR}/.vcpkg")

project(pgs)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp")
add_library(libpgs INTERFACE)
target_sources(libpgs INTERFACE ${sources})
target_include_directories(libpgs INTERFACE "src/pgs")
target_compile_features(libpgs INTERFACE cxx_std_23)
set_target_properties(libpgs PROPERTIES
	MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
)

include(scripts/cmake/warnings.cmake)
target_compile_warnings(libpgs INTERFACE
    GNU all extra pedantic error
    MSVC 4 X
)

add_executable(test test.cpp)
target_link_libraries(test PRIVATE libpgs)

find_package(Boost REQUIRED COMPONENTS gil)
find_package(ZLIB REQUIRED)
target_link_libraries(Boost::gil INTERFACE PNG::PNG)

find_package(PNG REQUIRED)
target_link_libraries(Boost::gil INTERFACE ZLIB::ZLIB)

target_link_libraries(test PRIVATE Boost::gil)

install(TARGETS libpgs)


# Tests ================================================================================================================
add_executable(pgs_tests)
file(GLOB_RECURSE pgs_tests_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
target_sources(pgs_tests PRIVATE ${pgs_tests_SOURCES})
target_include_directories(pgs_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_definitions(pgs_tests PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS)

set(DOCTEST_NO_INSTALL OFF CACHE INTERNAL "")

find_package(doctest CONFIG REQUIRED)
target_link_libraries(pgs_tests PRIVATE libpgs doctest::doctest)
