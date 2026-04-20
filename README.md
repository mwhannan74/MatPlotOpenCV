# MatPlotOpenCV

[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD--3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

Minimal 2D plotting library using OpenCV.

MatPlotOpenCV is a lightweight C++17 plotting library that offers a MATLAB / matplotlib-style API for basic 2-D graphics: lines, scatter, text, and simple shapes, without pulling in a full GUI or OpenGL backend. It uses OpenCV 4.x as the rendering surface, so figures are drawn directly into a `cv::Mat` and displayed with `cv::imshow`.

The library is implemented as a compiled target with public headers in `include/` and implementation in `src/`. The repository also includes an optional consumer-style demo under `demo/`.

---

## Example Plots

### Two sine waves
![Sine Waves](images/time_plot.jpg)

### 2D Plot
![2D](images/2d_plot.jpg)

### Shapes
![Shapes](images/shapes.jpg)

---

## Quick Start

```cpp
#include "figure.h"
using namespace mpocv;

Figure fig(640, 480);

fig.plot(x, y, Color::Blue(), 2.0f, "signal");
fig.scatter(px, py, Color::Red(), 5.0f, "events");
fig.text(3.14, 1.0, "Peak");

fig.grid(true);
fig.axis_pad(0.05);
fig.legend();

fig.title("Demo");
fig.xlabel("Time [s]");
fig.ylabel("Amplitude");

fig.show();
fig.save("demo.png");
```

---

## Build

This repository builds:

- the `mpocv` static library
- the `matplotopencv_demo` executable when `MATPLOTOPENCV_BUILD_DEMO=ON`
- the `doc` target when `MATPLOTOPENCV_BUILD_DOCS=ON`

### Prerequisites

- CMake 3.16 or newer
- A C++17 compiler
- OpenCV 4.5 or newer with `core`, `highgui`, and `imgproc`
- Doxygen, only if you keep `MATPLOTOPENCV_BUILD_DOCS=ON`

### Local OpenCV path

The repository uses [`cmake/local_paths.cmake`](./cmake/local_paths.cmake) to define the local OpenCV package location:

```cmake
set(MATPLOTOPENCV_OPENCV_DIR "C:/opencv/build/x64/vc16/lib" CACHE PATH ...)
```

Update that path if your local OpenCV package is elsewhere. The value must point to the directory that contains `OpenCVConfig.cmake`.

### Step-by-step build

From the repository root:

```bash
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Build documentation

When `MATPLOTOPENCV_BUILD_DOCS=ON`, build the docs target with:

```bash
cmake --build build --target doc
```

### Run the demo

```bash
Release\matplotopencv_demo.exe
```

### Build options

- `MATPLOTOPENCV_BUILD_DEMO=ON|OFF`
- `MATPLOTOPENCV_BUILD_DOCS=ON|OFF`

---

## CMake Integration

Clone the repository and consume it through its public target:

```cmake
add_subdirectory(MatPlotOpenCV)

find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE MatPlotOpenCV::mpocv)
```

Public headers are exposed automatically through the library target.

---

## Repository Layout

```text
MatPlotOpenCV/
|-- CMakeLists.txt
|-- cmake/
|-- include/
|-- src/
|-- demo/
|-- docs/
|-- images/
`-- tests/
```

`tests/` is currently reserved for future automated tests. The existing visual/manual validation program now lives in `demo/main_demo.cpp`.

The repository intentionally uses a single top-level `CMakeLists.txt`; `demo/` contains source files only. The `build/` directory is generated locally during configuration and build.

---

## Dependencies

- C++17 compiler
- OpenCV 4.x (`core`, `imgproc`, `highgui`)
- Doxygen, only when `MATPLOTOPENCV_BUILD_DOCS=ON`

---

## Notes & Limits

- OpenCV's Hershey fonts are basic; for rich text or LaTeX you need a different backend.
- Vector output (SVG/PDF) and subplots are not yet implemented.
- Thread-safe as long as each thread owns its own `Figure`.

---

## License

This project is licensed under the [BSD 3-Clause License](./LICENSE).
See the LICENSE file for full details.
