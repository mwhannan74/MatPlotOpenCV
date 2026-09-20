// =============================================================================
//  MatPlotOpenCV - Minimal 2D plotting library using OpenCV
//  Copyright (c) 2025 Michael Hannan
//
//  This file is part of MatPlotOpenCV and is licensed under the BSD 3-Clause
//  License. See the LICENSE file in the project root for full terms.
// =============================================================================

#include <cmath>
#define _USE_MATH_DEFINES
#include <vector>

#include "figure.h"

// This demo introduces the main MatPlotOpenCV workflow through three figures:
//
//   1. Plot sampled data and annotate a point of interest.
//   2. Visualize a 2-D path with equal axis scaling.
//   3. Draw and style the supported geometric shapes.
//
// Calls such as plot(), scatter(), and circle() add drawing instructions to a
// Figure. The instructions are rendered only when render(), show(), or save()
// is called, so the application remains in control of when rendering occurs.
int main()
{
    using namespace mpocv;

    // -------------------------------------------------------------------------
    // Figure 1: Plot sampled signals
    // -------------------------------------------------------------------------

    // MatPlotOpenCV accepts data in std::vector<double> containers. This sample
    // generates two signals, but the vectors could just as easily contain data
    // produced by a simulation or a running algorithm.
    std::vector<double> xs, ys1, ys2;
    const int sample_count = 200;
    for (int i = 0; i < sample_count; ++i)
    {
        const double t = i * 0.05;
        xs.push_back(t);
        ys1.push_back(std::sin(t));
        ys2.push_back(0.5 * std::sin(t + 0.5));
    }

    // Construct a figure by specifying its image dimensions in pixels. Plot
    // calls retain the supplied data and styling until the figure is rendered.
    // Call form: Figure(width_pixels, height_pixels)
    Figure fig1(800, 600);

    // Call form: plot(x_values, y_values, color, line_thickness_pixels, legend_label)
    fig1.plot(xs, ys1, Color::Blue(), 2.0f, "sin(t)");
    fig1.plot(xs, ys2, Color::Cyan(), 2.0f, "0.5*sin(t+0.5)");

    // Scatter points and text annotations use the same data-coordinate system
    // as the plotted lines.
    // Call form: scatter(x_values, y_values, color, marker_radius_pixels, legend_label)
    fig1.scatter({ M_PI / 2 }, { 1.0 }, Color::Red(), 6.0f);

    // Call form: text(x, y, message, color, font_scale, text_thickness, horizontal_alignment, vertical_alignment, legend_label)
    // Arguments after color are optional, so this call uses their defaults.
    fig1.text(M_PI / 2, 1.05, "peak", Color::Black());

    // Configure the axes and labels before rendering. axis_tight() removes the
    // default autoscale padding, while legend() displays labeled commands.
    // Call form: grid(enabled)
    fig1.grid(true);

    // Call form: axis_tight()
    fig1.axis_tight();

    // Call form: legend(enabled, location)
    fig1.legend(true, "northEast");

    // Call form: title(text)
    fig1.title("Two sine waves");

    // Call form: xlabel(text)
    fig1.xlabel("x-axis");

    // Call form: ylabel(text)
    fig1.ylabel("y-axis");

    // show() renders the retained commands and displays the resulting image.
    // save() writes that image to disk; it does not require a display window.
    // Call form: show(window_name)
    fig1.show("Demo Figure 1");

    // Call form: save(filename)
    fig1.save("demo1_sine_circle.png");

    // -------------------------------------------------------------------------
    // Figure 2: Visualize a 2-D path
    // -------------------------------------------------------------------------

    std::vector<double> path_x = { 0, 1, 2, 3, 4, 5, 6 };
    std::vector<double> path_y = { 0, 0.5, 1.5, 1.0, 0.5, 0.0, -0.5 };

    Figure fig2(600, 600);

    // Combine a line, endpoint markers, and annotations to describe the path.
    fig2.plot(path_x, path_y, Color::Blue(), 2.0f);
    fig2.scatter({ path_x.front() }, { path_y.front() }, Color::Green(), 6.0f);
    fig2.scatter({ path_x.back() }, { path_y.back() }, Color::Red(), 6.0f);
    fig2.text(path_x.front(), path_y.front() + 0.1, "Start", Color::Black());
    fig2.text(path_x.back(), path_y.back() - 0.1, "End", Color::Black());

    // Equal scaling makes one x-unit occupy the same number of pixels as one
    // y-unit. This is important when displaying physical paths and geometry.
    // Call form: equal_scale(enabled)
    fig2.equal_scale(true);
    fig2.grid(true);
    fig2.title("2D Object Path");
    fig2.xlabel("X Position");
    fig2.ylabel("Y Position");
    fig2.show("Demo Figure 2");
    fig2.save("demo2_path.png");

    // -------------------------------------------------------------------------
    // Figure 3: Draw styled shapes
    // -------------------------------------------------------------------------

    Figure fig3(800, 600);

    // ShapeStyle controls the outline and fill independently. A fill alpha of
    // zero is transparent, while one is fully opaque.
    ShapeStyle circle_style;
    circle_style.line_color = Color::Black();
    circle_style.thickness = 2.0f;
    circle_style.fill_color = Color::Red();
    circle_style.fill_alpha = 0.5f;

    // Call form: circle(center_x, center_y, radius, style, legend_label)
    fig3.circle(2, 1, 0.5, circle_style);

    ShapeStyle rect_xywh_style;
    rect_xywh_style.line_color = Color::Blue();
    rect_xywh_style.thickness = 2.0f;
    rect_xywh_style.fill_color = Color::Cyan();
    rect_xywh_style.fill_alpha = 0.6f;

    // Call form: rect_xywh(lower_left_x, lower_left_y, width, height, style, legend_label)
    fig3.rect_xywh(2, 0.5, 1.0, 1.5, rect_xywh_style);

    ShapeStyle rect_ltrb_style;
    rect_ltrb_style.line_color = Color::Green();
    rect_ltrb_style.thickness = 2.0f;
    rect_ltrb_style.fill_color = Color::Yellow();
    rect_ltrb_style.fill_alpha = 0.4f;

    // Call form: rect_ltrb(x0, y0, x1, y1, style, legend_label)
    fig3.rect_ltrb(4.0, 0.5, 5.0, 2.0, rect_ltrb_style);

    ShapeStyle rot_rect_style;
    rot_rect_style.line_color = Color::Magenta();
    rot_rect_style.thickness = 2.0f;
    rot_rect_style.fill_color = Color::Green();
    rot_rect_style.fill_alpha = 0.4f;

    // Call form: rotated_rect(center_x, center_y, width, height, angle_degrees, style, legend_label)
    fig3.rotated_rect(6.5, 1.25, 1.2, 0.8, 30.0, rot_rect_style);

    ShapeStyle poly_style;
    poly_style.line_color = Color::Black();
    poly_style.thickness = 1.5f;
    poly_style.fill_color = Color::Magenta();
    poly_style.fill_alpha = 0.5f;

    // Call form: polygon(x_vertices, y_vertices, style, legend_label)
    fig3.polygon({ 1.5, 2.0, 2.5, 2.0 }, { 3.0, 3.5, 3.0, 2.5 }, poly_style);

    ShapeStyle ellipse_style;
    ellipse_style.line_color = Color::Blue();
    ellipse_style.thickness = 2.0f;
    ellipse_style.fill_color = Color::Red();
    ellipse_style.fill_alpha = 0.3f;

    // Call form: ellipse(center_x, center_y, width, height, angle_degrees, style, legend_label)
    fig3.ellipse(4.5, 2.0, 2.0, 1.0, 45.0, ellipse_style);

    // Equal scaling preserves the intended geometry of circles and rotated
    // shapes in this example.
    fig3.equal_scale(true);
    fig3.grid(true);
    fig3.title("Shape Rendering Test");
    fig3.xlabel("X");
    fig3.ylabel("Y");
    fig3.show("Demo Figure 3");
    fig3.save("demo3_shapes.png");

    // show() performs a short event update so application loops are not blocked.
    // The demo waits here only to keep all three windows open for inspection.
    cv::waitKey(0);
    return 0;
}
