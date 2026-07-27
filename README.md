# RayTracing

A small OpenGL/GLUT-based ray tracing project written in C++. Displays a Cornell Box by default

## Overview

This repository builds a simple ray tracer that reads a scene description from `scene.sdf`, renders the scene line-by-line, and displays the result in an OpenGL window.

## Features

- C++17 ray tracing implementation
- Uses OpenGL and GLUT/FreeGLUT for rendering
- Supports scene input from `scene.sdf`
- Writes output to `Resultat.ppm`

## Requirements

- CMake 3.14 or newer
- A C++17-compatible compiler
- OpenGL development libraries
- GLUT or FreeGLUT

## Build Instructions

From the project root:

```sh
mkdir -p build
cd build
cmake ..
cmake --build .
```

On Windows, if you are using Visual Studio, open the generated solution file in `build` and build the `Raytracer` target.

## Run

From the `build` folder:

```sh
./Raytracer
```

The application expects `scene.sdf` to be available in the working directory. If the scene file is not found, it prints `No Scene Found`.

## Files

- `AppMain.cpp` - application entry point, GLUT callbacks, and main loop
- `Raytracer.cpp` / `Raytracer.h` - ray tracing core logic
- `Scene.cpp` / `Scene.h` - scene loading and object management
- `Reader.cpp` / `Reader.h` - scene file parsing
- `Resultat.ppm` - example output image file
- `CMakeLists.txt` - project build configuration

## Notes

- If GLUT is not available on the system, CMake will fetch FreeGLUT automatically.
- The renderer draws incrementally using `glutIdleFunc` while the ray tracer computes the image.

## License

No license is defined in this repository. Add a license file if you want to share or reuse the code.
