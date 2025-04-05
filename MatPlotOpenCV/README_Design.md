# MatPlotOpenCV – Design Document  
*A lightweight, cross?platform 2?D plotting library for C++ built on OpenCV*

---

## 1. Motivation & Goals
MatPlotOpenCV aims to offer a **Matlab?/?matplotlib?style** plotting experience while preserving the key advantages of OpenCV:

* **Thread friendliness** – each thread can own its own `Figure` object.  
* **On?demand rendering** – nothing is drawn until `render()` (or `show()`) is called.  
* **Minimal dependencies** – just C++17, OpenCV?(??4.x) and the standard library.  
* **Cross?platform** – tested on Windows and Linux; no OS?specific APIs.  
* **CMake?first** – easy to integrate into any CMake project.

---

## 2. Scope (v0.1)

| Area | Must?have (v0.1) | Nice?to?have (future) |
|------|------------------|-----------------------|
| **Plot types** | line, scatter, xy?line | histogram, bar, step |
| **Styling** | color (RGB/named), line thickness, marker size | line styles, marker types |
| **Axes** | manual limits, auto?scaling, equal?scale toggle | log scale, secondary axes |
| **Labels** | title, x/y labels, free text | LaTeX?style math text |
| **Grid** | major grid on/off | minor grid |
| **Export** | PNG/JPG via OpenCV | SVG/ PDF |
| **Runtime** | single?thread safe with user mutex, retained draw commands | built?in animation helpers |

--- 

## 3. High?Level Design

### 3.1 Retained (Hybrid) Rendering  
* **User API** calls (`plot()`, `scatter()`, `text()`, …) **create `PlotCommand` objects** and append them to the owning `Figure`’s command list.  
* `Figure::render()` walks the list, draws to an internal `cv::Mat` canvas, then clears *only* the image buffer (commands remain so the plot can be redrawn or resized).  
* `Figure::show()` calls `render()` if needed, then pushes the canvas to a named OpenCV window (`cv::imshow`) and performs a small `cv::waitKey`.

### 3.2 Class Diagram (simplified)

```
+-----------------+
| Color           |
|  uint8_t r,g,b  |
+-----------------+

+-----------------+          +--------------------+
| PlotCommand     |<>------- | Variant payload    |
|  enum Type      |          |  LineData          |
|  Color color    |          |  ScatterData       |
|  float lw, ms   |          |  TextData          |
+-----------------+          +--------------------+

+-------------------------------+
| Figure                        |
|  cv::Mat canvas               |
|  std::vector<PlotCommand> cmd |
|  Axes axes                    |
|  std::string title,xlabel,y   |
|  bool dirty                   |
+-------------------------------+
        ^ owns
        |
+-------------------------------+
| Axes                          |
|  double xmin,xmax,ymin,ymax   |
|  bool autoscale, equal_scale  |
|  bool grid                    |
+-------------------------------+
```

---

## 4. Public API Sketch

```cpp
Figure fig(640, 480);                 // create figure (width, height)

fig.plot(x, y, Color::Blue, 2.0f);    // line
fig.scatter(x2, y2, Color(255,0,0), 5.f);
fig.text(3.14, 0.0, "Peak");

fig.set_xlim(0, 10);
fig.set_ylim(-1, 1);
fig.autoscale(false);                 // manual limits
fig.equal_scale(true);
fig.grid(true);

fig.title("Demo Plot");
fig.xlabel("Time [s]");
fig.ylabel("Amplitude");

fig.show("Figure 1");                 // render + display
fig.save("figure1.png");              // optional
```

All API calls are **thread?safe** if the user guards concurrent calls to a single `Figure` with a mutex. Separate `Figure` instances need no locking.

---

## 5. Key Modules

| Module | Responsibility |
|--------|----------------|
| **color.hpp / .cpp** | Named colors, RGB helpers |
| **plot_command.hpp** | `enum class CmdType { Line, Scatter, Text }`, payload structs |
| **axes.hpp** | Axis limits, scaling helpers, data?pixel mapping |
| **figure.hpp / .cpp** | Command storage, render pipeline, OpenCV window management |
| **cmake/** | CMakeLists.txt, toolchain?agnostic |

---

## 6. Rendering Pipeline

1. **Pre?pass**  
   *If `axes.autoscale`*: scan all `PlotCommand` data, compute min/max.  
   *If `equal_scale`*: enlarge smaller span to match larger one.

2. **Canvas prep**  
   `cv::Mat canvas(height, width, CV_8UC3, bg_color)`.

3. **Draw grid & axes**  
   Map axes ticks to pixel coordinates, draw with `cv::line`.

4. **Iterate commands**  
   * Line: walk points, call `cv::line`  
   * Scatter: for each point, `cv::circle`  
   * Text: `cv::putText`

5. **Labels & title**  
   `cv::putText` near margins.

6. **Flush**  
   Set `dirty = false`; return canvas reference.

---

## 7. Platform & Build

* **Language standard**: C++17  # MatPlotOpenCV – Design Document  
*A lightweight, cross?platform 2?D plotting library for C++ built on OpenCV*

---

## 1. Motivation & Goals
MatPlotOpenCV aims to offer a **Matlab?/?matplotlib?style** plotting experience while preserving the key advantages of OpenCV:

* **Thread friendliness** – each thread can own its own `Figure` object.  
* **On?demand rendering** – nothing is drawn until `render()` (or `show()`) is called.  
* **Minimal dependencies** – just C++17, OpenCV?(??4.x) and the standard library.  
* **Cross?platform** – tested on Windows and Linux; no OS?specific APIs.  
* **CMake?first** – easy to integrate into any CMake project.

---

## 2. Scope (v0.1)

| Area | Must?have (v0.1) | Nice?to?have (future) |
|------|------------------|-----------------------|
| **Plot types** | line, scatter, xy?line | histogram, bar, step |
| **Styling** | color (RGB/named), line thickness, marker size | line styles, marker types |
| **Axes** | manual limits, auto?scaling, equal?scale toggle | log scale, secondary axes |
| **Labels** | title, x/y labels, free text | LaTeX?style math text |
| **Grid** | major grid on/off | minor grid |
| **Export** | PNG/JPG via OpenCV | SVG/ PDF |
| **Runtime** | single?thread safe with user mutex, retained draw commands | built?in animation helpers |

---

## 3. High?Level Design

### 3.1 Retained (Hybrid) Rendering  
* **User API** calls (`plot()`, `scatter()`, `text()`, …) **create `PlotCommand` objects** and append them to the owning `Figure`’s command list.  
* `Figure::render()` walks the list, draws to an internal `cv::Mat` canvas, then clears *only* the image buffer (commands remain so the plot can be redrawn or resized).  
* `Figure::show()` calls `render()` if needed, then pushes the canvas to a named OpenCV window (`cv::imshow`) and performs a small `cv::waitKey`.

### 3.2 Class Diagram (simplified)

```
+-----------------+
| Color           |
|  uint8_t r,g,b  |
+-----------------+

+-----------------+          +--------------------+
| PlotCommand     |<>------- | Variant payload    |
|  enum Type      |          |  LineData          |
|  Color color    |          |  ScatterData       |
|  float lw, ms   |          |  TextData          |
+-----------------+          +--------------------+

+-------------------------------+
| Figure                        |
|  cv::Mat canvas               |
|  std::vector<PlotCommand> cmd |
|  Axes axes                    |
|  std::string title,xlabel,y   |
|  bool dirty                   |
+-------------------------------+
        ^ owns
        |
+-------------------------------+
| Axes                          |
|  double xmin,xmax,ymin,ymax   |
|  bool autoscale, equal_scale  |
|  bool grid                    |
+-------------------------------+
```

---

## 4. Public API Sketch

```cpp
Figure fig(640, 480);                 // create figure (width, height)

fig.plot(x, y, Color::Blue, 2.0f);    // line
fig.scatter(x2, y2, Color(255,0,0), 5.f);
fig.text(3.14, 0.0, "Peak");

fig.set_xlim(0, 10);
fig.set_ylim(-1, 1);
fig.autoscale(false);                 // manual limits
fig.equal_scale(true);
fig.grid(true);

fig.title("Demo Plot");
fig.xlabel("Time [s]");
fig.ylabel("Amplitude");

fig.show("Figure 1");                 // render + display
fig.save("figure1.png");              // optional
```

All API calls are **thread?safe** if the user guards concurrent calls to a single `Figure` with a mutex. Separate `Figure` instances need no locking.

---

## 5. Key Modules

| Module | Responsibility |
|--------|----------------|
| **color.hpp / .cpp** | Named colors, RGB helpers |
| **plot_command.hpp** | `enum class CmdType { Line, Scatter, Text }`, payload structs |
| **axes.hpp** | Axis limits, scaling helpers, data?pixel mapping |
| **figure.hpp / .cpp** | Command storage, render pipeline, OpenCV window management |
| **cmake/** | CMakeLists.txt, toolchain?agnostic |

---

## 6. Rendering Pipeline

1. **Pre?pass**  
   *If `axes.autoscale`*: scan all `PlotCommand` data, compute min/max.  
   *If `equal_scale`*: enlarge smaller span to match larger one.

2. **Canvas prep**  
   `cv::Mat canvas(height, width, CV_8UC3, bg_color)`.

3. **Draw grid & axes**  
   Map axes ticks to pixel coordinates, draw with `cv::line`.

4. **Iterate commands**  
   * Line: walk points, call `cv::line`  
   * Scatter: for each point, `cv::circle`  
   * Text: `cv::putText`

5. **Labels & title**  
   `cv::putText` near margins.

6. **Flush**  
   Set `dirty = false`; return canvas reference.

---

## 7. Platform & Build

* **Language standard**: C++17  
* **Compiler support**: MSVC???19.3x, GCC???9, Clang???10  
* **External deps**: OpenCV???4.5 (core + highgui + imgproc)  
* **Build system**: CMake???3.14

### Top?Level `CMakeLists.txt` (excerpt)

```cmake
cmake_minimum_required(VERSION 3.14)
project(MatPlotOpenCV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV 4.5 REQUIRED COMPONENTS core highgui imgproc)

add_library(matplotopencv
    src/figure.cpp
    src/plot_command.cpp
    src/axes.cpp
)
target_include_directories(matplotopencv PUBLIC include)
target_link_libraries(matplotopencv PUBLIC opencv_core opencv_highgui opencv_imgproc)
```

---

## 8. Open Issues & Future Work

* **Text alignment & font scaling** – OpenCV’s `Hershey` fonts are limited.  
* **Performance for large scatter sets** – consider `cv::polylines` batching.  
* **Event handling** – mouse zoom/pan requires a callback layer.  
* **Subplots / layout manager** – extend `Figure` to hold multiple `Axes` regions.  
* **Vector export** – add optional SVG backend using `nanovg` or custom writer.  
* **Automated tests** – image hash comparisons in CI for regression.

---

## 9. Milestone Plan

| Milestone | Deliverables |
|-----------|--------------|
| **0.1** | Basic `Figure`, `plot`, `scatter`, manual axes, render/show |
| **0.2** | Autoscale, grid, equal?scale, save() |
| **0.3** | Text annotations, title/labels |
| **0.4** | CMake install targets, Windows build CI |
| **0.5** | Performance tuning, documentation site, first tag release |

---

### Conclusion
MatPlotOpenCV will provide a pragmatic, zero?friction way to embed quick diagnostic plots in C++ applications without the heavy GUI baggage of full?blown plotting libraries. The retained?command design keeps the API intuitive while giving users explicit control over rendering and threading — a perfect fit for real?time robotics, embedded vision, and data?logging pipelines.
* **Compiler support**: MSVC???19.3x, GCC???9, Clang???10  
* **External deps**: OpenCV???4.5 (core + highgui + imgproc)  
* **Build system**: CMake???3.14

### Top?Level `CMakeLists.txt` (excerpt)

```cmake
cmake_minimum_required(VERSION 3.14)
project(MatPlotOpenCV LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV 4.5 REQUIRED COMPONENTS core highgui imgproc)

add_library(matplotopencv
    src/figure.cpp
    src/plot_command.cpp
    src/axes.cpp
)
target_include_directories(matplotopencv PUBLIC include)
target_link_libraries(matplotopencv PUBLIC opencv_core opencv_highgui opencv_imgproc)
```

---

## 8. Open Issues & Future Work

* **Text alignment & font scaling** – OpenCV’s `Hershey` fonts are limited.  
* **Performance for large scatter sets** – consider `cv::polylines` batching.  
* **Event handling** – mouse zoom/pan requires a callback layer.  
* **Subplots / layout manager** – extend `Figure` to hold multiple `Axes` regions.  
* **Vector export** – add optional SVG backend using `nanovg` or custom writer.  
* **Automated tests** – image hash comparisons in CI for regression.

---

## 9. Milestone Plan

| Milestone | Deliverables |
|-----------|--------------|
| **0.1** | Basic `Figure`, `plot`, `scatter`, manual axes, render/show |
| **0.2** | Autoscale, grid, equal?scale, save() |
| **0.3** | Text annotations, title/labels |
| **0.4** | CMake install targets, Windows build CI |
| **0.5** | Performance tuning, documentation site, first tag release |

---

### Conclusion
MatPlotOpenCV will provide a pragmatic, zero?friction way to embed quick diagnostic plots in C++ applications without the heavy GUI baggage of full?blown plotting libraries. The retained?command design keeps the API intuitive while giving users explicit control over rendering and threading — a perfect fit for real?time robotics, embedded vision, and data?logging pipelines.