#include <cmath>
#define _USE_MATH_DEFINES
#include <vector>
#include "figure.h"

int main()
{
    using namespace mpocv;

    // ------------------------ Two Sine Waves ------------------------

    std::vector<double> xs, ys1, ys2;
    const int N = 200;
    for (int i = 0; i < N; ++i)
    {
        double t = i * 0.05;
        xs.push_back(t);
        ys1.push_back(std::sin(t));
        ys2.push_back(0.5 * std::sin(t + 0.5));
    }

    Figure fig1(800, 600);
    fig1.plot(xs, ys1, Color::Blue(), 2.0f, "sin(t)");
    fig1.plot(xs, ys2, Color::Cyan(), 2.0f, "0.5*sin(t+0.5)");
    fig1.scatter({ M_PI / 2 }, { 1.0 }, Color::Red(), 6.0f);
    fig1.text(M_PI / 2, 1.05, "peak", Color::Black());
    fig1.grid(true);
    fig1.axis_tight();
    fig1.legend(true, "northEast");
    fig1.title("Two sine waves");
    fig1.xlabel("x-axis");
    fig1.ylabel("y-axis");
    fig1.show("Demo Figure 1");
    fig1.save("demo1_sine_circle.png");

    // ------------------------ 2D Object Path ------------------------

    std::vector<double> path_x = { 0, 1, 2, 3, 4, 5, 6 };
    std::vector<double> path_y = { 0, 0.5, 1.5, 1.0, 0.5, 0.0, -0.5 };

    Figure fig2(600, 600);
    fig2.plot(path_x, path_y, Color::Blue(), 2.0f);
    fig2.scatter({ path_x.front() }, { path_y.front() }, Color::Green(), 6.0f);
    fig2.scatter({ path_x.back() }, { path_y.back() }, Color::Red(), 6.0f);
    fig2.text(path_x.front(), path_y.front() + 0.1, "Start", Color::Black());
    fig2.text(path_x.back(), path_y.back() - 0.1, "End", Color::Black());
    fig2.equal_scale(true);
    fig2.grid(true);
    fig2.title("2D Object Path");
    fig2.xlabel("X Position");
    fig2.ylabel("Y Position");
    fig2.show("Demo Figure 2");
    fig2.save("demo2_path.png");

    // ------------------------ Shape Drawing Test ------------------------

    Figure fig3(800, 600);

    // Circle
    ShapeStyle circle_style;
    circle_style.line_color = Color::Black();
    circle_style.thickness = 2.0f;
    circle_style.fill_color = Color::Red();
    circle_style.fill_alpha = 0.5f;
    fig3.circle(2, 1, 0.5, circle_style);

    // Rectangle [x y w h]
    ShapeStyle rect_xywh_style;
    rect_xywh_style.line_color = Color::Blue();
    rect_xywh_style.thickness = 2.0f;
    rect_xywh_style.fill_color = Color::Cyan();
    rect_xywh_style.fill_alpha = 0.6f;
    fig3.rect_xywh(2, 0.5, 1.0, 1.5, rect_xywh_style);

    // Rectangle by corners
    ShapeStyle rect_ltrb_style;
    rect_ltrb_style.line_color = Color::Green();
    rect_ltrb_style.thickness = 2.0f;
    rect_ltrb_style.fill_color = Color::Yellow();
    rect_ltrb_style.fill_alpha = 0.4f;
    fig3.rect_ltrb(4.0, 0.5, 5.0, 2.0, rect_ltrb_style);

    // Rotated rectangle
    ShapeStyle rot_rect_style;
    rot_rect_style.line_color = Color::Magenta();
    rot_rect_style.thickness = 2.0f;
    rot_rect_style.fill_color = Color::Green();
    rot_rect_style.fill_alpha = 0.4f;
    fig3.rotated_rect(6.5, 1.25, 1.2, 0.8, 30.0, rot_rect_style);

    // Polygon
    ShapeStyle poly_style;
    poly_style.line_color = Color::Black();
    poly_style.thickness = 1.5f;
    poly_style.fill_color = Color::Magenta();
    poly_style.fill_alpha = 0.5f;
    fig3.polygon({ 1.5, 2.0, 2.5, 2.0 }, { 3.0, 3.5, 3.0, 2.5 }, poly_style);

    // Ellipse
    ShapeStyle ellipse_style;
    ellipse_style.line_color = Color::Blue();
    ellipse_style.thickness = 2.0f;
    ellipse_style.fill_color = Color::Red();
    ellipse_style.fill_alpha = 0.3f;
    fig3.ellipse(4.5, 2.0, 2.0, 1.0, 45.0, ellipse_style);

    fig3.equal_scale(true);
    fig3.grid(true);
    fig3.title("Shape Rendering Test");
    fig3.xlabel("X");
    fig3.ylabel("Y");
    fig3.show("Demo Figure 3");
    fig3.save("demo3_shapes.png");

    // ------------------------
    cv::waitKey(0);
    return 0;
}
