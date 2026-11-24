cmake_minimum_required(VERSION 3.14)

# Modern CMake policy override for fetching external dependencies
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(Raytracer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(RAYTRACER_BUILD_TESTS "Build the unit and image regression tests" ON)

include(FetchContent)

# ---------------------------------------------------------------------------
# Renderer core: everything except AppMain.cpp. No OpenGL/GLUT dependency,
# so the tests and headless mode can run on machines without a display.
# ---------------------------------------------------------------------------
file(GLOB CORE_SOURCES "src/*.cpp")
list(REMOVE_ITEM CORE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/AppMain.cpp")

add_library(raytracer_core STATIC ${CORE_SOURCES})
target_include_directories(raytracer_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_features(raytracer_core PUBLIC cxx_std_17)

# ---------------------------------------------------------------------------
# Application: the core plus the GLUT preview window and command-line handling.
# ---------------------------------------------------------------------------
add_executable(Raytracer src/AppMain.cpp)
target_link_libraries(Raytracer PRIVATE raytracer_core)

# Default scene used when no scene file is passed on the command line
target_compile_definitions(Raytracer PRIVATE
    RAYTRACER_DEFAULT_SCENE="${CMAKE_CURRENT_SOURCE_DIR}/assets/scene.sdf")

find_package(OpenGL REQUIRED)

if(APPLE)
    find_library(GLUT_LIBRARY GLUT REQUIRED)
    target_link_libraries(Raytracer PRIVATE OpenGL::GL ${GLUT_LIBRARY})
    target_compile_definitions(Raytracer PRIVATE GL_SILENCE_DEPRECATION)
else()
    # Try finding system GLUT first
    find_package(GLUT QUIET)

    # Download FreeGLUT automatically if missing locally
    if(NOT GLUT_FOUND)
        message(STATUS "GLUT missing locally. Fetching FreeGLUT automatically...")
        FetchContent_Declare(
            freeglut
            GIT_REPOSITORY https://github.com/freeglut/freeglut.git
            GIT_TAG        v3.4.0
        )
        set(FREEGLUT_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(freeglut)
    endif()

    if(TARGET freeglut)
        # Make sure the compiler can locate <GL/glut.h> inside the downloaded freeglut sources
        target_include_directories(Raytracer PRIVATE ${freeglut_SOURCE_DIR}/include)
        target_link_libraries(Raytracer PRIVATE freeglut OpenGL::GL OpenGL::GLU)
        # Copy the fetched FreeGLUT DLL next to the executable so it runs on Windows
        if(WIN32)
            add_custom_command(TARGET Raytracer POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:freeglut> $<TARGET_FILE_DIR:Raytracer>)
        endif()
    else()
        target_link_libraries(Raytracer PRIVATE GLUT::GLUT OpenGL::GL OpenGL::GLU)
    endif()
endif()

# ---------------------------------------------------------------------------
# Tests (doctest). Run them with:  ctest --test-dir build -C Release
# ---------------------------------------------------------------------------
if(RAYTRACER_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        doctest
        GIT_REPOSITORY https://github.com/doctest/doctest.git
        GIT_TAG        v2.4.11
    )
    FetchContent_MakeAvailable(doctest)

    file(GLOB TEST_SOURCES "tests/*.cpp")
    add_executable(raytracer_tests ${TEST_SOURCES})
    target_link_libraries(raytracer_tests PRIVATE raytracer_core doctest::doctest)
    target_compile_definitions(raytracer_tests PRIVATE
        RAYTRACER_ASSETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/assets"
        RAYTRACER_REFERENCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/reference"
        RAYTRACER_TEST_OUTPUT_DIR="${CMAKE_CURRENT_BINARY_DIR}/test-output")

    # Unit tests and image regression tests are reported separately.
    add_test(NAME unit  COMMAND raytracer_tests --test-suite-exclude=image)
    add_test(NAME image COMMAND raytracer_tests --test-suite=image)
endif()
