#include <cmath>
#define _USE_MATH_DEFINES
#include <vector>
#include "figure.h"

int main()
{
    using namespace mpocv;

    // ------------------------ Sine Wave + Circle ------------------------

    // Generate a sine wave
    std::vector<double> xs, ys;
    const int N = 200;
    for (int i = 0; i < N; ++i)
    {
        double t = i * 0.05;
        xs.push_back(t);
        ys.push_back(std::sin(t));
    }

    // Generate a circle
    std::vector<double> cx, cy;
    const int M = 100;
    for (int i = 0; i < M; ++i)
    {
        double ang = 2 * M_PI * i / M;
        cx.push_back(std::cos(ang));
        cy.push_back(std::sin(ang));
    }

    Figure fig1(800, 600);
    fig1.plot(xs, ys, Color::Blue(), 2.0f);
    fig1.scatter({ M_PI / 2 }, { 1.0 }, Color::Red(), 6.0f);
    fig1.text(M_PI / 2, 1.05, "peak", Color::Black());
    fig1.plot(cx, cy, Color::Green(), 1.5f);
    fig1.equal_scale(true);
    fig1.grid(true);
    fig1.title("MatPlotOpenCV demo: sine + circle");
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

    // Keep both windows open
    cv::waitKey(0);
    return 0;
}
