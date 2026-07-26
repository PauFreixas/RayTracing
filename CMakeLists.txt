cmake_minimum_required(VERSION 3.14)

# Modern CMake policy override for fetching external dependencies
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(Raytracer CXX)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCES "*.cpp")
add_executable(Raytracer ${SOURCES})

find_package(OpenGL REQUIRED)

if(APPLE)
    find_library(GLUT_LIBRARY GLUT REQUIRED)
    target_link_libraries(Raytracer PRIVATE ${OpenGL_LIBRARIES} ${GLUT_LIBRARY})
    target_compile_definitions(Raytracer PRIVATE GL_SILENCE_DEPRECATION)
else()
    # Try finding system GLUT first
    find_package(GLUT QUIET)

    # Download FreeGLUT automatically if missing locally
    if(NOT GLUT_FOUND)
        message(STATUS "GLUT missing locally. Fetching FreeGLUT automatically...")
        include(FetchContent)
        FetchContent_Declare(
            freeglut
            GIT_REPOSITORY https://github.com/freeglut/freeglut.git
            GIT_TAG        v3.4.0
        )
        set(FREEGLUT_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(freeglut)
    endif()

    if(TARGET freeglut)
        # Make sure g++ can locate <GL/glut.h> inside the downloaded freeglut sources
        target_include_directories(Raytracer PRIVATE ${freeglut_SOURCE_DIR}/include)
        target_link_libraries(Raytracer PRIVATE freeglut OpenGL::GL OpenGL::GLU)
    else()
        target_link_libraries(Raytracer PRIVATE GLUT::GLUT OpenGL::GL OpenGL::GLU)
    endif()
endif()