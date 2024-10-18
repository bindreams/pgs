cmake_minimum_required(VERSION 3.25)

include(scripts/cmake/bundled-vcpkg.cmake)
enable_vcpkg(VERSION 2024.09.30 PATH "${CMAKE_CURRENT_SOURCE_DIR}/.vcpkg")

project(pgs)

include(scripts/cmake/warnings.cmake)
function(set_common_executable_properties TARGET)
    target_compile_warnings(${TARGET} PRIVATE
        GNU all extra pedantic error no-error=unknown-attributes
        MSVC 4 X
    )

    set_target_properties(${TARGET} PROPERTIES
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
endfunction()

# Library ==============================================================================================================
add_library(pgs INTERFACE)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS "src/pgs/*.hpp")
target_sources(pgs INTERFACE ${sources})
target_include_directories(pgs INTERFACE "src/pgs")
target_compile_features(pgs INTERFACE cxx_std_23)


install(TARGETS pgs)

# Tests ================================================================================================================
add_executable(pgs_tests)

file(GLOB_RECURSE pgs_tests_SOURCES CONFIGURE_DEPENDS "tests/*.cpp")
target_sources(pgs_tests PRIVATE ${pgs_tests_SOURCES})
target_include_directories(pgs_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/tests")
target_compile_definitions(pgs_tests PRIVATE DOCTEST_CONFIG_USE_STD_HEADERS)
set_common_executable_properties(pgs_tests)

find_package(doctest CONFIG REQUIRED)
target_link_libraries(pgs_tests PRIVATE pgs doctest::doctest)

# Test executable ======================================================================================================
add_executable(pgs-extract "src/scripts/pgs-extract.cpp")

target_link_libraries(pgs-extract PRIVATE pgs)

find_package(Boost REQUIRED COMPONENTS gil)
find_package(ZLIB REQUIRED)
target_link_libraries(Boost::gil INTERFACE PNG::PNG)

find_package(PNG REQUIRED)
target_link_libraries(Boost::gil INTERFACE ZLIB::ZLIB)

target_link_libraries(pgs-extract PRIVATE Boost::gil)

find_package(CLI11 CONFIG REQUIRED)
target_link_libraries(pgs-extract PRIVATE CLI11::CLI11)

set_common_executable_properties(pgs-extract)
install(TARGETS pgs-extract)
