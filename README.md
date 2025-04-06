# MatPlotOpenCV

**MatPlotOpenCV** is a lightweight C++ plotting library built on OpenCV. It provides a simple, MATLAB/matplotlib-inspired interface for creating 2D plots such as lines, scatter markers, and annotated text. It supports axis control, autoscaling, equal scaling, grid display, and basic labeling.

This library is designed for personal or internal use, with minimal dependencies and a focus on clarity, speed, and simplicity.

---

## Core Functionality

### Plot: `plot(x, y, color, thickness)`

Draws a connected line through the points `(x[i], y[i])`.

```cpp
fig.plot(x, y);  // Default: blue, 1px thick
fig.plot(x, y, Color::Green(), 2.0f);
```

Supports both lvalue and rvalue inputs.

---

### Scatter: `scatter(x, y, color, size)`

Draws unconnected filled circles at each point `(x[i], y[i])`.

```cpp
fig.scatter(x, y, Color::Red(), 5.0f);
```

---

### Text Annotation: `text(x, y, msg, color, scale, thickness, halign, valign)`

Places a label at the data point `(x, y)`.

```cpp
fig.text(3.14, 1.0, "Peak", Color::Black(), 0.4, 1);
```

Supports optional alignment:
```cpp
fig.text(0, 0, "Origin", Color::Black(), 0.4, 1,
         TextData::HAlign::Center, TextData::VAlign::Top);
```

---

### Axis Control

```cpp
fig.set_xlim(xmin, xmax);     // Manually set x range
fig.set_ylim(ymin, ymax);     // Manually set y range
fig.autoscale(true);          // Let axes expand to fit data (default: true)
fig.equal_scale(true);        // Equal units in x and y axes
```

---

### Grid Display

```cpp
fig.grid(true);  // Show major grid lines
```

---

### Labels and Title

```cpp
fig.title("Plot Title");
fig.xlabel("X Axis Label");
fig.ylabel("Y Axis Label");
```

---

### Rendering and Output

```cpp
fig.render();                 // Converts commands to pixels (called automatically)
fig.show("Window Name");      // Opens OpenCV window
fig.save("filename.png");     // Saves rendered plot to disk
```

---

## Demo 1: Sine Wave with Marker and Circle

```cpp
#include <cmath>
#include "figure.h"

int main() {
    using namespace mpocv;

    std::vector<double> xs, ys;
    for (int i = 0; i < 200; ++i) {
        double t = i * 0.05;
        xs.push_back(t);
        ys.push_back(std::sin(t));
    }

    std::vector<double> cx, cy;
    for (int i = 0; i < 100; ++i) {
        double ang = 2 * M_PI * i / 100;
        cx.push_back(std::cos(ang));
        cy.push_back(std::sin(ang));
    }

    Figure fig(800, 600);
    fig.plot(xs, ys, Color::Blue(), 2.0f);
    fig.scatter({ M_PI / 2 }, { 1.0 }, Color::Red(), 6.0f);
    fig.text(M_PI / 2, 1.05, "Peak");

    fig.plot(cx, cy, Color::Green(), 1.5f);
    fig.equal_scale(true);

    fig.grid(true);
    fig.title("Sine Wave + Circle");
    fig.xlabel("Time (s)");
    fig.ylabel("Amplitude");

    fig.show("Demo");
    fig.save("demo.png");

    cv::waitKey(0);
}
```

---

## Demo 2: 2D Object Path (X vs Y)

```cpp
#include "figure.h"

int main() {
    using namespace mpocv;

    std::vector<double> x_path = {0, 1, 2, 3, 4, 5, 6};
    std::vector<double> y_path = {0, 0.5, 1.5, 1.0, 0.5, 0.0, -0.5};

    Figure fig(600, 600);
    fig.plot(x_path, y_path, Color::Blue(), 2.0f);
    fig.scatter({ x_path.front() }, { y_path.front() }, Color::Green(), 6.0f);
    fig.scatter({ x_path.back() }, { y_path.back() }, Color::Red(), 6.0f);

    fig.text(x_path.front(), y_path.front() + 0.1, "Start", Color::Black());
    fig.text(x_path.back(), y_path.back() - 0.1, "End", Color::Black());

    fig.equal_scale(true);
    fig.grid(true);
    fig.title("2D Object Path");
    fig.xlabel("X Position");
    fig.ylabel("Y Position");

    fig.show("Path");
    fig.save("path_plot.png");

    cv::waitKey(0);
}
```

---

## Notes

- All `Figure` instances are self-contained. There is no shared global state.
- Plot commands are retained until cleared or the `Figure` is destroyed.
- Coordinate transforms (data-to-pixel) are handled internally, with support for resizing in the future.
