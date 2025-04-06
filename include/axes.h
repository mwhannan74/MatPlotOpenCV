#pragma once
#include <limits>

namespace mpocv
{

    /**
     * @struct Axes
     * @brief Holds the current axis settings for a Figure.
     *
     * The struct stores the numeric limits in data space, plus several
     * rendering flags.  It is *not* responsible for computing those limits;
     * the Figure class updates the fields during autoscale / padding /
     * equal-scale processing.
     */
    struct Axes
    {
        /** Minimum x-value visible on the plot (data units). */
        double xmin{ 0.0 };
        /** Maximum x-value visible on the plot (data units). */
        double xmax{ 1.0 };
        /** Minimum y-value visible on the plot (data units). */
        double ymin{ 0.0 };
        /** Maximum y-value visible on the plot (data units). */
        double ymax{ 1.0 };

        /**
         * Fractional padding added to each span when autoscaling.
         *
         * A value of 0.05 means the span is enlarged by 5% on *both* ends.
         * Set to 0.0 for a completely “tight” fit around the data.
         */
        double pad_frac{ 0.05 };

        /** If true, limits are computed from the data; otherwise user-supplied. */
        bool autoscale{ true };
        /** If true, x- and y-units are forced to have the same scale. */
        bool equal_scale{ false };
        /** Draw major grid lines when enabled. */
        bool grid{ false };

        /**
         * @brief Reset the limits so that the next autoscale starts fresh.
         *
         * Sets xmin/ymin to +inf and xmax/ymax to -inf so that any real data
         * point will contract / expand the bounds correctly.
         */
        void reset()
        {
            xmin = std::numeric_limits<double>::infinity();
            ymin = std::numeric_limits<double>::infinity();
            xmax = -std::numeric_limits<double>::infinity();
            ymax = -std::numeric_limits<double>::infinity();
        }
    };

} // namespace mpocv
