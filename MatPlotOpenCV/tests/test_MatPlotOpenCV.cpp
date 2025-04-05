#include <cmath>
#define _USE_MATH_DEFINES
#include <vector>
#include "figure.h"

int main()
{
    using namespace mpocv;

    // Generate a sine wave ------------------------------------------------
    std::vector<double> xs, ys;
    const int N = 200;
    for (int i = 0; i < N; ++i)
    {
        double t = i * 0.05;      // sample step
        xs.push_back(t);
        ys.push_back(std::sin(t));
    }

    // Generate a circle (equal‑scale test) --------------------------------
    std::vector<double> cx, cy;
    const int M = 100;
    for (int i = 0; i < M; ++i)
    {
        double ang = 2 * M_PI * i / M;
        cx.push_back(std::cos(ang));
        cy.push_back(std::sin(ang));
    }

    Figure fig(800, 600);

    // Plot sine wave as a blue line
    fig.plot(xs, ys, Color::Blue(), 2.0f);

    // Scatter the maxima with red markers
    fig.scatter({ M_PI / 2 }, { 1.0 }, Color::Red(), 6.0f);
    fig.text(M_PI / 2, 1.05, "peak", Color::Black());

    // Overlay the circle with equal scale
    fig.plot(cx, cy, Color::Green(), 1.5f);
    fig.equal_scale(true);

    // Style
    fig.grid(true);
    fig.title("MatPlotOpenCV demo: sine + circle");
    fig.xlabel("x-axis");
    fig.ylabel("y-axis");

    // Show and save
    fig.show("Demo Figure");
    fig.save("demo.png");

    // Keep window open until keypress
    cv::waitKey(0);
    return 0;
}
