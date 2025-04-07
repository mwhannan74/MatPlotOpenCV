#pragma once

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "color.h"
#include "plot_command.h"   // already defines CmdType
#include "axes.h"

namespace mpocv
{
    /* --------------------------------------------------------------------------
     * Forward-declarations
     * ------------------------------------------------------------------------*/
    struct TickInfo
    {
        std::vector<double>        locs;   ///< Tick locations in data space.
        std::vector<std::string>   labels; ///< Formatted tick label strings.
    };
        
    struct Bounds
    {
        double xmin = std::numeric_limits<double>::infinity(); ///< Minimum x value.
        double xmax = -std::numeric_limits<double>::infinity(); ///< Maximum x value.
        double ymin = std::numeric_limits<double>::infinity(); ///< Minimum y value.
        double ymax = -std::numeric_limits<double>::infinity(); ///< Maximum y value.

        void expand(double x, double y)
        {
            xmin = std::min(xmin, x);   xmax = std::max(xmax, x);
            ymin = std::min(ymin, y);   ymax = std::max(ymax, y);
        }
        bool valid() const { return std::isfinite(xmin); }
    };



    /**
     * @class Figure
     * @brief A retained-command plotting canvas.
     *
     * The Figure class stores a list of drawing commands (lines, scatter points,
     * text) and converts them to pixels only when render(), show() or save()
     * is called. This keeps the user-facing API similar to matplotlib / Matlab
     * while avoiding unnecessary redraws.
     *
     * A Figure owns:
     *   - a pixel buffer (cv::Mat)
     *   - an Axes object describing the data coordinate system
     *   - a vector of PlotCommand structs
     *
     * Thread safety: concurrent access to a single Figure must be guarded by
     * the caller. Separate Figure instances can be used from different threads
     * without locking.
     */
    class Figure
    {
    public:
        /**
         * @brief Construct a new Figure object.
         *
         * Creates a new Figure with a white canvas of the specified width and height.
         *
         * @param w Canvas width in pixels. Defaults to 640.
         * @param h Canvas height in pixels. Defaults to 480.
         */
        Figure(int w = 640, int h = 480);


        // ========================================================================
        // Primary methods for drawing lines, shapes, and scatter plots.
        // ========================================================================

        /**
          * @brief Draw a connected poly-line (lvalue overload).
          *
          * Draws a connected poly-line using the provided x and y coordinates.
          * The points are connected sequentially.
          *
          * @param x Vector of x data values.
          * @param y Vector of y data values (must be the same length as @p x).
          * @param c Line color. Defaults to blue.
          * @param thickness Line thickness in pixels.
          */
        void plot(const std::vector<double>& x,
            const std::vector<double>& y,
            Color c = Color::Blue(),
            float thickness = 1.f,
            const std::string& label = "");

        /**
         * @brief Draw a connected poly-line (rvalue overload – avoids copy).
         *
         * Draws a connected poly-line by moving the provided x and y coordinates.
         * This overload avoids copying the input vectors.
         *
         * @param x Vector of x data values.
         * @param y Vector of y data values (must be the same length as @p x).
         * @param c Line color. Defaults to blue.
         * @param thickness Line thickness in pixels.
         */
        void plot(std::vector<double>&& x,
            std::vector<double>&& y,
            Color c = Color::Blue(),
            float thickness = 1.f,
            const std::string& label = "");

        /**
         * @brief Draw unconnected markers (lvalue overload).
         *
         * Draws unconnected markers at the specified data points.
         *
         * @param x Vector of x data values.
         * @param y Vector of y data values (must be the same length as @p x).
         * @param c Marker color. Defaults to red.
         * @param marker_size Size of the markers.
         */
        void scatter(const std::vector<double>& x,
            const std::vector<double>& y,
            Color c = Color::Red(),
            float marker_size = 4.f,
            const std::string& label = "");

        /**
         * @brief Draw unconnected markers (rvalue overload – avoids copy).
         *
         * Draws unconnected markers at the specified data points by moving the input vectors.
         *
         * @param x Vector of x data values.
         * @param y Vector of y data values (must be the same length as @p x).
         * @param c Marker color. Defaults to red.
         * @param marker_size Size of the markers.
         */
        void scatter(std::vector<double>&& x,
            std::vector<double>&& y,
            Color c = Color::Red(),
            float marker_size = 4.f,
            const std::string& label = "");

        /**
         * @brief Place a text annotation at data coordinates.
         *
         * Places a text annotation on the canvas at the given data coordinates with
         * specified alignment. Alignment follows matplotlib semantics:
         *   - HAlign::Left: Anchor at left edge of text box.
         *   - HAlign::Center: Anchor at text box center.
         *   - HAlign::Right: Anchor at right edge.
         *
         *   - VAlign::Baseline: y is the text baseline (default).
         *   - VAlign::Center: Vertical center.
         *   - VAlign::Top: Top edge.
         *   - VAlign::Bottom: Bottom edge (descenders).
         *
         * @param x Data x coordinate.
         * @param y Data y coordinate.
         * @param msg Text message to display.
         * @param c Text color. Defaults to black.
         * @param font_scale Scale factor for the text font.
         * @param thickness Thickness of the text.
         * @param ha Horizontal alignment.
         * @param va Vertical alignment.
         */
        void text(double x, double y, const std::string& msg,
            Color c = Color::Black(),
            double font_scale = 0.4,
            int thickness = 1,
            TextData::HAlign ha = TextData::HAlign::Left,
            TextData::VAlign va = TextData::VAlign::Baseline,
            const std::string& label = "");

        /**
         * @brief Draw a filled or stroked circle centered at data coordinates.
         *
         * Adds a circle drawing command to the figure.
         *
         * @param cx X-coordinate of the circle center.
         * @param cy Y-coordinate of the circle center.
         * @param radius Radius of the circle.
         * @param style Line and fill styling for the circle.
         */
        void circle(double cx, double cy, double radius,
            const ShapeStyle& style,
            const std::string& label = "");

        /**
         * @brief Draw a rectangle using lower-left corner, width, and height.
         *
         * Adds a rectangle drawing command using MATLAB-style [x y w h] semantics.
         *
         * @param x X-coordinate of the lower-left corner.
         * @param y Y-coordinate of the lower-left corner.
         * @param w Rectangle width.
         * @param h Rectangle height.
         * @param style Line and fill styling for the rectangle.
         */
        void rect_xywh(double x, double y, double w, double h,
            const ShapeStyle& style,
            const std::string& label = "");

        /**
         * @brief Draw a rectangle using top-left and bottom-right corners.
         *
         * Adds a rectangle drawing command using OpenCV-style (x0, y0, x1, y1) bounds.
         *
         * @param x0 X-coordinate of the top-left or first corner.
         * @param y0 Y-coordinate of the top-left or first corner.
         * @param x1 X-coordinate of the bottom-right or second corner.
         * @param y1 Y-coordinate of the bottom-right or second corner.
         * @param style Line and fill styling for the rectangle.
         */
        void rect_ltrb(double x0, double y0, double x1, double y1,
            const ShapeStyle& style,
            const std::string& label = "");

        /**
         * @brief Draw a rotated rectangle using center point, size, and rotation angle.
         *
         * Adds a rotated rectangle using OpenCV's RotatedRect-style specification.
         *
         * @param cx X-coordinate of the rectangle center.
         * @param cy Y-coordinate of the rectangle center.
         * @param w Width of the rectangle (horizontal side length).
         * @param h Height of the rectangle (vertical side length).
         * @param angle_deg Rotation angle in degrees (counter-clockwise).
         * @param style Line and fill styling for the rectangle.
         */
        void rotated_rect(double cx, double cy, double w, double h, double angle_deg,
            const ShapeStyle& style,
            const std::string& label = "");

        /**
         * @brief Draw a filled or stroked polygon from a sequence of data points.
         *
         * Adds a polygon defined by x[i], y[i] coordinates. OpenCV-style closed polygon.
         *
         * @param x Vector of x-coordinates.
         * @param y Vector of y-coordinates (must match size of @p x).
         * @param style Line and fill styling for the polygon.
         */
        void polygon(const std::vector<double>& x, const std::vector<double>& y,
            const ShapeStyle& style,
            const std::string& label = "");

        /**
         * @brief Draw a filled or stroked ellipse.
         *
         * Adds an ellipse centered at (cx, cy) with given width, height, and rotation.
         *
         * @param cx X-coordinate of the ellipse center.
         * @param cy Y-coordinate of the ellipse center.
         * @param w Width of the full ellipse (major axis).
         * @param h Height of the full ellipse (minor axis).
         * @param angle_deg Rotation angle in degrees (counter-clockwise).
         * @param style Line and fill styling for the ellipse.
         */
        void ellipse(double cx, double cy, double w, double h, double angle_deg,
            const ShapeStyle& style,
            const std::string& label = "");


        // ========================================================================
        // Helper methods for axes, labels, and grid.
        // ========================================================================

        /**
         * @brief Set the x-axis limits.
         *
         * Sets the lower and upper limits for the x-axis and disables autoscaling.
         *
         * @param lo Lower bound for the x-axis.
         * @param hi Upper bound for the x-axis.
         */
        void set_xlim(double lo, double hi);

        /**
         * @brief Set the y-axis limits.
         *
         * Sets the lower and upper limits for the y-axis and disables autoscaling.
         *
         * @param lo Lower bound for the y-axis.
         * @param hi Upper bound for the y-axis.
         */
        void set_ylim(double lo, double hi);

        /**
         * @brief Automatically scale axes tightly around the plotted data.
         *
         * Sets the padding fraction to 0.0, meaning the axis limits will closely
         * match the data bounds. No extra margin is added around the data.
         *
         * @note Calling this will invalidate the current render and force a re-draw
         *       on the next call to render(), show(), or save().
         */
        void axis_tight();

        /**
         * @brief Set a fractional padding for the auto-scaled axes.
         *
         * When autoscale is enabled and the figure is rendered, the axis limits
         * will be expanded on each side by the given fraction of the data range.
         * For example, a fraction of 0.05 means 5% extra space on each boundary.
         *
         * @param frac Padding fraction in [0.0, 1.0]. A value of 0.0 means no extra
         *             padding (tight), while 0.1 would add 10% padding, etc.
         *
         * @note Values below 0.0 are clamped to 0.0. Any change here takes effect
         *       on the next render() or show().
         */
        void axis_pad(double frac);

        /**
         * @brief Enable or disable autoscaling.
         *
         * When enabled, the axes limits will automatically adjust to the data bounds.
         *
         * @param on Flag to enable autoscaling (true) or disable it (false).
         */
        void autoscale(bool on = true);

        /**
         * @brief Enable or disable equal scaling.
         *
         * Adjusts the plot so that one unit in the x-axis is the same length as one unit in the y-axis.
         *
         * @param on Flag to enable equal scaling (true) or disable it (false).
         */
        void equal_scale(bool on = true);

        /**
         * @brief Enable or disable grid lines.
         *
         * Turns the grid lines on or off.
         *
         * @param on Flag to enable grid lines (true) or disable them (false).
         */
        void grid(bool on = true);

        /**
         * @brief Set the figure title.
         *
         * Sets the title text displayed on the figure.
         *
         * @param t Title text.
         */
        void title(const std::string& t);

        /**
         * @brief Set the x-axis label.
         *
         * Sets the label for the x-axis.
         *
         * @param t Label text.
         */
        void xlabel(const std::string& t);

        /**
         * @brief Set the y-axis label.
         *
         * Sets the label for the y-axis and invalidates any cached rotated label.
         *
         * @param t Label text.
         */
        void ylabel(const std::string& t);

        /**
         * @brief Enable / disable the legend box.
         *
         * @param on  True to draw the legend, false to hide it.
         * @param loc Legend location keyword: "northEast", "northWest",
         *            "southEast", "southWest", "north", "south",
         *            "east", "west", or "center".
         */
        void legend(bool on = true, const std::string& loc = "northEast");


        // ========================================================================
        // Core functions for rendering, showing, and saving the figure.
        // ========================================================================

        /**
         * @brief Convert retained commands to pixel representation.
         *
         * Renders the plot by converting retained drawing commands to pixels.
         * If the figure is already rendered (i.e., not dirty), the function returns immediately.
         */
        void render();

        /**
         * @brief Render (if needed) and display the figure in an OpenCV window.
         *
         * If the canvas is marked as dirty, render() is called before displaying.
         *
         * @param window_name Name of the OpenCV window. Defaults to "Figure".
         */
        void show(const std::string& window_name = "Figure");

        /**
         * @brief Render (if needed) and save the figure to disk.
         *
         * Renders the canvas if needed and writes the image to disk.
         * The image format is inferred from the file extension.
         *
         * @param filename Output file path.
         */
        void save(const std::string& filename);


    private:

        // Constant margins around the plotting region (pixels)
        static constexpr int kMarginLeft = 60;  ///< Left margin in pixels.
        static constexpr int kMarginRight = 20; ///< Right margin in pixels.
        static constexpr int kMarginTop = 40;   ///< Top margin in pixels.
        static constexpr int kMarginBottom = 60;///< Bottom margin in pixels.
        static constexpr int kTickLen = 5;      ///< Length of tick marks in pixels.
        static constexpr int kMargin = 50;      ///< Margin for the title.

        // Canvas and retained state
        int                       width_, height_;  ///< Canvas dimensions in pixels.
        cv::Mat                   canvas_;          ///< OpenCV image matrix representing the canvas.
        std::vector<PlotCommand>  cmds_;            ///< Retained plot commands.
        Axes                      axes_;            ///< Axes representing the data coordinate system.
        std::string               title_, xlabel_, ylabel_; ///< Title and axis labels.
        bool                      dirty_{ true };   ///< Flag indicating if the canvas needs re-rendering.
        
        // Legend
        std::string legend_loc_{ "northEast" };
        bool        legend_on_{ false };

        // Cached data bounds for fast autoscale
        Bounds                    data_bounds_;     ///< Cached bounds of the plotted data.

        // Cached rotated y‑label
        cv::Mat                   ylabel_cache_;    ///< Cached rotated image for the y-axis label.
        bool                      ylabel_cache_valid_{ false }; ///< Flag indicating if the y-label cache is valid.

        /// @brief Returns the width of the plot area.
        int  plot_width()  const { return width_ - kMarginLeft - kMarginRight; }

        /// @brief Returns the height of the plot area.
        int  plot_height() const { return height_ - kMarginTop - kMarginBottom; }

        /**
         * @brief Convert data coordinates to pixel coordinates.
         *
         * Converts a point in data space to pixel coordinates in the image.
         *
         * @param x Data x value.
         * @param y Data y value.
         * @return cv::Point representing the pixel coordinates.
         */
        cv::Point2i data_to_pixel(double x, double y) const;

        /**
         * @brief Convert a custom Color to OpenCV cv::Scalar with optional alpha.
         *
         * Converts an mpocv::Color to a cv::Scalar in BGR(A) format. The alpha
         * channel is returned as a scaled 0–255 uchar if specified, but is only
         * meaningful when drawing into a 4-channel (CV_8UC4) canvas.
         *
         * @param c The Color to convert (RGB format).
         * @param alpha Alpha multiplier in range [0.0, 1.0]. Default is 1.0 (opaque).
         * @return cv::Scalar The OpenCV-compatible color (B, G, R, A).
         */
        cv::Scalar cv_color(const Color& c, float alpha = 1.0f);

        /**
         * @brief Blend a shape drawn into a temporary buffer onto the main canvas using alpha.
         *
         * @param shape Image containing the shape (same size as canvas_).
         * @param alpha Blend factor in range [0.0, 1.0].
         */
        void blend_shape(const cv::Mat& shape, float alpha);


        /* ---------- axis, grid, tick helpers ------------------------------ */
        /// @brief Computes a "nice" number for tick spacing.
        static double nice_num(double range, bool round);

        /// @brief Generates tick positions and formatted labels.
        static TickInfo make_ticks(double lo, double hi, int target = 6);

        /// @brief Draws the axis lines, ticks, and numeric labels.
        void draw_axes(const TickInfo& xt, const TickInfo& yt);

        /// @brief Draws grid lines corresponding to tick positions.
        void draw_grid(const TickInfo& xt, const TickInfo& yt);

        /// @brief Expands the cached data bounds using the given vectors.
        void expand_bounds(const std::vector<double>& xs, const std::vector<double>& ys);

        /* ---------- rotated y-label helper -------------------------------- */
        /// @brief Draws and caches the rotated y-axis label.
        void draw_ylabel();

        /* ---------- text alignment helper --------------------------------- */
        /// @brief Computes the anchored text position based on alignment.
        cv::Point2i anchored_text_pos(const TextData& td) const;

        /* ---------- safety helpers ---------------------------------------- */
        /// @brief Ensures that the span between lo and hi is non-zero.
        static void ensure_nonzero_span(double& lo, double& hi);
        
        /// @brief Fixes the ranges of the axes to ensure non-zero spans.
        static void fix_ranges(Axes& a);

        /**
         * @brief Return the upper‑left pixel of the legend box for a given location.
         *
         * @param boxW  Legend box width  (pixels)
         * @param boxH  Legend box height (pixels)
         * @return cv::Point   anchor (top‑left) inside the plot area
         */
        cv::Point legend_anchor(int boxW, int boxH) const;

        /* ---------- internal helpers to add commands ---------------------- */
        /**
         * @brief Adds a line drawing command.
         *
         * Internal helper to add a connected poly-line command.
         *
         * @tparam VX Type of x coordinate container.
         * @tparam VY Type of y coordinate container.
         * @param x Vector (or movable container) of x data values.
         * @param y Vector (or movable container) of y data values.
         * @param c Color for the line.
         * @param thickness Thickness of the line.
         */
        template<typename VX, typename VY>
        void add_line_command(VX&& x, VY&& y, Color c, float thickness, const std::string& label = "")
        {
            PlotCommand cmd;
            cmd.type = CmdType::Line;
            cmd.color = c;
            cmd.label = label;
            cmd.line.x = std::forward<VX>(x);
            cmd.line.y = std::forward<VY>(y);
            cmd.line.thickness = thickness;
            expand_bounds(cmd.line.x, cmd.line.y);
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }

        /**
         * @brief Adds a scatter drawing command.
         *
         * Internal helper to add a scatter (marker) command.
         *
         * @tparam VX Type of x coordinate container.
         * @tparam VY Type of y coordinate container.
         * @param x Vector (or movable container) of x data values.
         * @param y Vector (or movable container) of y data values.
         * @param c Color for the markers.
         * @param marker_size Size of the markers.
         */
        template<typename VX, typename VY>
        void add_scatter_command(VX&& x, VY&& y, Color c, float marker_size, const std::string& label)
        {
            PlotCommand cmd;
            cmd.type = CmdType::Scatter;
            cmd.color = c;
            cmd.label = label;
            cmd.scatter.x = std::forward<VX>(x);
            cmd.scatter.y = std::forward<VY>(y);
            cmd.scatter.marker_size = marker_size;
            expand_bounds(cmd.scatter.x, cmd.scatter.y);
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }

    }; // class Figure 

} // namespace mpocv
