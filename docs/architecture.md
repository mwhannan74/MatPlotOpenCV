# MatPlotOpenCV Repository Architecture

MatPlotOpenCV is organized around a small, explicit target graph:

- `mpocv`: the core compiled plotting library
- `MatPlotOpenCV::mpocv`: namespaced alias for downstream consumption
- `matplotopencv_demo`: optional demo executable that exercises the public API

## Layout

```text
MatPlotOpenCV/
|-- CMakeLists.txt
|-- cmake/
|   `-- local_paths.cmake
|-- include/
|-- src/
|-- demo/
|-- docs/
|-- images/
`-- tests/   # reserved for future automated tests
```

## Build Rules

- Public headers live in `include/`.
- Compiled implementation lives in `src/`.
- The repository uses a single top-level `CMakeLists.txt`.
- The demo source lives in `demo/` and is defined from the root build file, linking against `MatPlotOpenCV::mpocv`.
- Machine-local OpenCV configuration is isolated in `cmake/local_paths.cmake` through the cached `MATPLOTOPENCV_OPENCV_DIR` variable.
- Optional build surface is controlled with:
  - `MATPLOTOPENCV_BUILD_DEMO`
  - `MATPLOTOPENCV_BUILD_DOCS`

## Dependency Model

OpenCV is a required dependency of the core library because rendering is implemented directly on `cv::Mat` and displayed with OpenCV windowing primitives. There is no separate optional backend target in the current codebase, so the repository keeps a single core library instead of introducing artificial layering.
