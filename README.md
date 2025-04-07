# MatPlotOpenCV

MatPlotOpenCV is a lightweight, header‑only plotting helper that offers a MATLAB / matplotlib‑style API for basic 2‑D graphics—lines, scatter, text, and simple shapes—without pulling in a full GUI or OpenGL backend. Because it relies solely on OpenCV image routines, it’s thread‑safe: you can create and render figures anywhere in your code, and drawing isn’t executed until you explicitly call `render()`.

The goal isn’t to replace full‑featured tools like matplotlib, but to give you a real‑time, minimal‑dependency option for debugging algorithms or visualising quick diagnostics. Features cover the essentials: line and marker styling, filled shapes with alpha, autoscaling (tight or padded), equal‑scale axes, grid, and legends.

Implementation is pure C++17 with OpenCV 4.x as the “canvas.” Rendering is literally pixel drawing on a `cv::Mat`; you view results with `cv::imshow`. (Zoom/pan requires an OpenCV build that includes Qt support.)

---

## Quick Start

```cpp
#include "figure.h"
using namespace mpocv;

Figure fig(640,480);

fig.plot(x, y, Color::Blue(), 2.0f, "signal");
fig.scatter(px, py, Color::Red(), 5.0f, "events");
fig.text(3.14, 1.0, "Peak");

fig.grid(true);
fig.axis_pad(0.05);     // 5% padding (MATLABlike)
fig.legend();           // show in northeast corner

fig.title("Demo");
fig.xlabel("Time [s]");
fig.ylabel("Amplitude");

fig.show();
fig.save("demo.png");
```

---

## Core API (oneliners)

| Action | Call |
|--------|------|
| Line plot | `plot(x, y, color, thickness, label)` |
| Scatter   | `scatter(x, y, color, size, label)` |
| Text      | `text(x, y, msg, color, scale, thick, halign, valign)` |
| Shapes    | `circle`, `rect_xywh`, `rect_ltrb`, `rotated_rect`, `polygon`, `ellipse` (all accept `ShapeStyle` + `label`) |
| Grid      | `grid(true/false)` |
| Tight / padded axes | `axis_tight()`, `axis_pad(frac)` |
| Equal units | `equal_scale(true)` |
| Manual limits | `set_xlim(lo,hi)`, `set_ylim(lo,hi)` |
| Legend     | `legend(on=true, loc="northEast")` |
| Render / display / save | `render()`, `show("win")`, `save("file.png")` |

---

## Examples

### 1 · Two sine waves + legend

```cpp
Figure f(800,600);
f.plot(xs, ys1, Color::Blue(), 2.0f, "sin(t)");
f.plot(xs, ys2, Color::Cyan(), 2.0f, "0.5·sin(t+0.5)");
f.scatter({M_PI/2},{1.0}, Color::Red(), 6.0f, "peak");
f.text(M_PI/2,1.05,"peak");

f.grid(true);
f.axis_tight();
f.legend();               // default NE
f.show();
```

### 2 · Shape test (equalscale)

```cpp
Figure g(600,600);
ShapeStyle s{ Color::Black(), 2.0f, Color::Red(), 0.4f };
g.circle(0,0,1,s,"circle");
g.rect_xywh(-2,-1,1,2,s,"rect");
g.rotated_rect(2,1,1,0.5,30,s,"rotrect");
g.equal_scale(true);
g.grid(true);
g.legend(true,"southWest");
g.show();
```

---

Here’s an updated version of your `## Build / Integration` section that reflects your refined setup, supporting both **basic integration** and **CMake subproject use**, which is now the recommended approach:

---

## Build / Integration

The easiest and most reliable way to use MatPlotOpenCV is to **clone it directly** and add it to your own CMake project:

```bash
git clone https://github.com/mwhannan74/MatPlotOpenCV.git
```

Then in your own `CMakeLists.txt`:

```cmake
add_subdirectory(MatPlotOpenCV)  # path to the cloned repo

find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE mpocv ${OpenCV_LIBS})
```

This gives you:

- Automatic include paths
- Working compiled `.cpp` source
- No install step needed

---

## Notes & Limits

* OpenCV’s Hershey fonts are basic; for rich text or LaTeX you’ll need a different backend.
* Vector output (SVG/PDF) and subplots are not yet implemented.
* Threadsafe as long as each thread owns its own `Figure`.

