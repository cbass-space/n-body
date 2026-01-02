# a-N-other Body Simulator?

## Simulation Info

### Features

### Implementation Details

### Libraries

1. [SDL3 & SDL_gpu](https://libsdl.org/) - Cross platform input, windowing, and rendering
2. [SDL_shadercross](https://github.com/libsdl-org/SDL_shadercross) - Cross platform shader compilation
3. [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross) - Dependency for SDL_shadercross
4. [Dear ImGui](https://github.com/ocornut/imgui) & [dear_bindings](https://github.com/dearimgui/dear_bindings) - Immediate mode user interface
5. [stb_ds.h](https://github.com/nothings/stb/blob/master/stb_ds.h) - Generic dynamic arrays
6. [HandmadeMath.h](https://github.com/HandmadeMath/HandmadeMath) - Simple graphic-focused math library

## Building and running

### Required dependencies
1. [Git](https://git-scm.com/) - Version and package control
2. [CMake](https://cmake.org/) - Cross platform build system
3. [Python 3](https://www.python.org/) - For generating ImGui bindings and compiling shaders
4. [glslc](https://github.com/google/shaderc/#downloads) - For compiling GLSL shaders to SPIR-V for cross-platform shader support with SDL_shadercross

### Steps

1. Clone repository

```bash
git clone <URL_TO_REPO> <PROJECT_PATH>
cd <PROJECT_PATH>
```

2. Setup up Python build environment

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install ply==3.11
```

3. Generate CMake profile, build & run

**For Debugging**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
cd build && ./main

# for IDE support
ln -s build/compile_commands.json . 
```

**For Release**
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build && ./main
```
