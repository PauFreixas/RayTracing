# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/pau18/Documents/Coding/RayTracing/build/_deps/freeglut-src")
  file(MAKE_DIRECTORY "C:/Users/pau18/Documents/Coding/RayTracing/build/_deps/freeglut-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/pau18/Documents/Coding/RayTracing/build/_deps/freeglut-build"
  "C:/Users/pau18/Documents/Coding/RayTracing/build/_deps/freeglut-subbuild/freeglut-populate-prefix"
  "C:/Users/pau18/Documents/Coding/RayTracing/build/_deps/freeglut-subbuild/freeglut-populate-prefix/tmp"
  "C:/Users/pau18/Documents/Coding/RayTracing/build/_deps/freeglut-subbuild/freeglut-populate-prefix/src/freeglut-populate-stamp"
  "C:/Users/pau18/Documents/Coding/RayTracing/build/_deps/freeglut-subbuild/freeglut-populate-prefix/src"
  "C:/Users/pau18/Documents/Coding/RayTracing/build/_deps/freeglut-subbuild/freeglut-populate-prefix/src/freeglut-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/pau18/Documents/Coding/RayTracing/build/_deps/freeglut-subbuild/freeglut-populate-prefix/src/freeglut-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/pau18/Documents/Coding/RayTracing/build/_deps/freeglut-subbuild/freeglut-populate-prefix/src/freeglut-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
