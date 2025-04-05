#pragma once
#include <limits>

namespace mpocv
{

    struct Axes
    {
        double xmin{ 0.0 };
        double xmax{ 1.0 };
        double ymin{ 0.0 };
        double ymax{ 1.0 };

        bool autoscale{ true };
        bool equal_scale{ false };
        bool grid{ false };

        void reset()
        {
            xmin = std::numeric_limits<double>::infinity();
            ymin = std::numeric_limits<double>::infinity();
            xmax = -std::numeric_limits<double>::infinity();
            ymax = -std::numeric_limits<double>::infinity();
        }
    };

} // namespace mpocv
