# MatPlotOpenCV

[![License: BSD-3-Clause](https://img.shields.io/badge/License-BSD--3--Clause-blue.svg)](https://opensource.org/licenses/BSD-3-Clause)

MatPlotOpenCV combines a familiar MATLAB-style plotting interface with explicit,
application-controlled OpenCV rendering. It makes live 2-D debugging
visualizations easy to add without bringing a separate graphics engine or
background rendering system into the application.

MatPlotOpenCV is a small C++17 plotting library built on OpenCV 4. It supports
lines, scatter plots, text, basic shapes, axes, grids, labels, and legends.
Figures can be displayed in an OpenCV window or saved as images.

The project is developed and tested on Windows. Its CMake build also supports
Linux through standard OpenCV package discovery, but Linux has not yet been
tested as part of this release-readiness work.

## Why MatPlotOpenCV?

MatPlotOpenCV was created to solve a practical problem in autonomous robotics
software development: understanding what an algorithm is doing while it is
running. Logging data for later analysis is useful, but it does not provide live
feedback and requires additional logging, storage, and post-processing
infrastructure. Many general-purpose plotting libraries bring their own
rendering engines, event loops, or background threads, which can interfere with
timing-sensitive simulation and debugging code.

MatPlotOpenCV takes a deliberately simpler approach. Plotting calls build a
retained list of drawing instructions; they do not immediately render the
figure. Rendering occurs only when the application explicitly calls `render()`,
`show()`, or `save()`. The library does not create its own background rendering
thread, so the application controls when plotting work occurs and can place it
at an appropriate point in its processing loop.

The public API is intentionally MATLAB-inspired. Creating a figure, plotting
data, adding labels or shapes, and displaying or saving the result requires
only a few direct, readable function calls. Users do not need to construct a
scene graph, manage a rendering context, configure callbacks, or learn a
complicated C++ graphics framework. The goal is an interface that is quick to
learn and convenient to use inside existing robotics and autonomy code.

OpenCV provides the rendering backend. Instead of introducing OpenGL or another
graphics framework, MatPlotOpenCV draws an ordinary 2-D image using OpenCV image
processing operations. A figure can be displayed through OpenCV HighGUI or
written directly to an image file without opening a window. Saved images can
also be incorporated into an application's existing recording or
network-streaming workflow.

The library is primarily intended for live visualization during simulation,
algorithm development, and remote debugging of robotics and autonomy software.
It may also be used in deployed systems when appropriate, but it is not
designed as a hard real-time visualization system. Displaying a window requires
a graphical session, and OpenCV HighGUI may impose UI-thread requirements
depending on the selected backend. OpenCV builds with Qt support may provide
additional window controls such as zooming, although those controls are
provided by OpenCV rather than MatPlotOpenCV.

MatPlotOpenCV is intentionally limited to lightweight 2-D plotting and a
focused set of plotting primitives. It is not intended to replace MATLAB,
Matplotlib, or a full scientific-visualization system. Its purpose is to provide
fast, predictable, application-controlled visualization with minimal
dependencies and minimal interference with the code being observed.

## Example plots

### Two sine waves

<p align="center">
  <img
    src="images/time_plot.jpg"
    alt="Two sine waves plotted with MatPlotOpenCV"
    width="700">
</p>

### 2-D path

<p align="center">
  <img
    src="images/2d_plot.jpg"
    alt="Two-dimensional object path plotted with MatPlotOpenCV"
    width="700">
</p>

### Shapes

<p align="center">
  <img
    src="images/shapes.jpg"
    alt="Shape rendering examples plotted with MatPlotOpenCV"
    width="700">
</p>

## Quick start

```cpp
#include "figure.h"

using namespace mpocv;

Figure figure(640, 480);
figure.plot(x, y, Color::Blue(), 2.0f, "signal");
figure.scatter(px, py, Color::Red(), 5.0f, "events");
figure.text(3.14, 1.0, "Peak");
figure.grid(true);
figure.legend();
figure.title("Demo");
figure.xlabel("Time [s]");
figure.ylabel("Amplitude");
figure.save("demo.png");
figure.show();
```

## Requirements

- CMake 3.16 or newer
- A C++17 compiler
- OpenCV 4.5 or newer with `core`, `highgui`, `imgproc`, and `imgcodecs`
- Doxygen when building the optional API documentation target

## Build on Windows

The default Windows OpenCV location is configured in
`cmake\local_paths.cmake`. It currently expects:

```text
C:\opencv\build\x64\vc16\lib
```

From PowerShell in the repository root:

```powershell
cmake -S . -B build
cmake --build build --config Release
```

Run the demo with:

```powershell
.\build\Release\matplotopencv_demo.exe
```

If OpenCV is installed elsewhere, override the cached path without editing the
project files:

```powershell
cmake -S . -B build -DMATPLOTOPENCV_OPENCV_DIR="C:/path/to/opencv/cmake"
```

The value must name the directory containing `OpenCVConfig.cmake`. Forward
slashes are intentional in CMake values; Windows path separators are used for
PowerShell executable paths.

## Build on Linux

Install OpenCV and make its CMake package discoverable, then run:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/matplotopencv_demo
```

The Windows-only local path file is not loaded on Linux. If OpenCV is installed
in a nonstandard location, provide its package directory explicitly:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DOpenCV_DIR=/path/to/opencv/cmake
```

## Build options

- `MATPLOTOPENCV_BUILD_DEMO=ON|OFF`
- `MATPLOTOPENCV_BUILD_DOCS=ON|OFF`

The demo defaults to `ON` when building MatPlotOpenCV directly and `OFF` when
it is included by another CMake project. Documentation defaults to `OFF` in
both cases so Doxygen is not required for a normal build.

To enable and generate API documentation:

```powershell
cmake -S . -B build -DMATPLOTOPENCV_BUILD_DOCS=ON
cmake --build build --config Release --target doc
```

## Testing

The test source in `tests/figure_tests.cpp` builds as the
`matplotopencv_tests` executable when MatPlotOpenCV is configured as the
top-level project. It provides regression coverage for input validation,
OpenCV logging behavior, equal axis scaling, rotated-ellipse bounds, and the
stability of manually configured axis limits. Some tests create temporary PNG
files, inspect their rendered pixels, and remove the files before exiting.

After a Release build, run the tests on Windows with:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

On Linux, run:

```bash
ctest --test-dir build --output-on-failure
```

A successful run reports that all registered tests passed. The
`--output-on-failure` option displays diagnostic output if a test fails.

## Repository layout

```text
MatPlotOpenCV/
|-- CMakeLists.txt
|-- cmake/
|-- demo/
|-- docs/
|-- images/
|-- include/
|-- src/
`-- tests/
```

The demo provides additional manual visual validation.

## Limitations

- OpenCV Hershey fonts do not support rich text or LaTeX.
- Vector output, subplots, interactive zoom, and interactive pan are not
  implemented by this library.
- Concurrent access to one `Figure` is not safe. HighGUI may also require calls
  from the application's UI thread, depending on the OpenCV backend.
- Coordinate vectors passed to `plot()` and `scatter()` must have matching
  lengths; mismatched vectors throw `std::invalid_argument`. `polygon()`
  currently ignores empty or mismatched coordinate vectors.
- Circles, rotated rectangles, and rotated ellipses represent their documented
  data-space geometry accurately when equal scaling is enabled. With unequal
  x- and y-axis scales, their screen-space rendering is not an exact affine
  transform of the data-space shape.
- Legend swatches for labeled shapes currently use the default command color
  instead of the shape's configured line or fill color.
- Tick labels use one decimal place for intervals below one, so very small
  ranges can display repeated labels.
- Line thickness, marker size, text scale, text thickness, and `ShapeStyle`
  numeric values are not fully validated before being passed to OpenCV. Use
  positive, finite sizes and scales, and keep fill alpha in the range `[0, 1]`.

## License

MatPlotOpenCV is licensed under the [BSD 3-Clause License](LICENSE).
