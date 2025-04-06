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
        Figure(int w = 640, int h = 480)
            : width_(w), height_(h),
            canvas_(h, w, CV_8UC3, cv::Scalar(255, 255, 255))
        {}

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
            float thickness = 1.f)
        {
            add_line_command(x, y, c, thickness);
        }

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
            float thickness = 1.f)
        {
            add_line_command(std::move(x), std::move(y), c, thickness);
        }

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
            float marker_size = 4.f)
        {
            add_scatter_command(x, y, c, marker_size);
        }

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
            float marker_size = 4.f)
        {
            add_scatter_command(std::move(x), std::move(y), c, marker_size);
        }

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
            TextData::VAlign va = TextData::VAlign::Baseline)
        {
            PlotCommand cmd;
            cmd.type = CmdType::Text;
            cmd.color = c;
            cmd.txt = { x, y, msg, font_scale, thickness, ha, va };
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }

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
        void circle(double cx, double cy, double radius, const ShapeStyle& style)
        {
            PlotCommand cmd;
            cmd.type = CmdType::Circle;
            cmd.circle = { cx, cy, radius, style };
            data_bounds_.expand(cx - radius, cy - radius);
            data_bounds_.expand(cx + radius, cy + radius);
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }

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
        void rect_xywh(double x, double y, double w, double h, const ShapeStyle& style)
        {
            PlotCommand cmd;
            cmd.type = CmdType::RectXYWH;
            cmd.rect = { x, y, x + w, y + h, style };
            data_bounds_.expand(x, y);
            data_bounds_.expand(x + w, y + h);
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }

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
        void rect_ltrb(double x0, double y0, double x1, double y1, const ShapeStyle& style)
        {
            PlotCommand cmd;
            cmd.type = CmdType::RectLTRB;
            cmd.rect = { x0, y0, x1, y1, style };
            data_bounds_.expand(x0, y0);
            data_bounds_.expand(x1, y1);
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }

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
        void rotated_rect(double cx, double cy, double w, double h, double angle_deg, const ShapeStyle& style)
        {
            PlotCommand cmd;
            cmd.type = CmdType::RotatedRect;
            cmd.rot_rect = { cx, cy, w, h, angle_deg, style };

            // Expand bounds using a conservative bounding circle
            const double r = 0.5 * std::sqrt(w * w + h * h);
            data_bounds_.expand(cx - r, cy - r);
            data_bounds_.expand(cx + r, cy + r);
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }

        /**
         * @brief Draw a filled or stroked polygon from a sequence of data points.
         *
         * Adds a polygon defined by x[i], y[i] coordinates. OpenCV-style closed polygon.
         *
         * @param x Vector of x-coordinates.
         * @param y Vector of y-coordinates (must match size of @p x).
         * @param style Line and fill styling for the polygon.
         */
        void polygon(const std::vector<double>& x, const std::vector<double>& y, const ShapeStyle& style)
        {
            if (x.size() != y.size() || x.empty()) return;

            PlotCommand cmd;
            cmd.type = CmdType::Polygon;
            cmd.polygon = { x, y, style };
            for (size_t i = 0; i < x.size(); ++i)
                data_bounds_.expand(x[i], y[i]);

            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }

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
        void ellipse(double cx, double cy, double w, double h, double angle_deg, const ShapeStyle& style)
        {
            PlotCommand cmd;
            cmd.type = CmdType::Ellipse;
            cmd.ellipse = { cx, cy, w, h, angle_deg, style };
            data_bounds_.expand(cx - 0.5 * w, cy - 0.5 * h);
            data_bounds_.expand(cx + 0.5 * w, cy + 0.5 * h);
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }


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
        void set_xlim(double lo, double hi)
        {
            axes_.xmin = lo; axes_.xmax = hi; axes_.autoscale = false; dirty_ = true;
        }

        /**
         * @brief Set the y-axis limits.
         *
         * Sets the lower and upper limits for the y-axis and disables autoscaling.
         *
         * @param lo Lower bound for the y-axis.
         * @param hi Upper bound for the y-axis.
         */
        void set_ylim(double lo, double hi)
        {
            axes_.ymin = lo; axes_.ymax = hi; axes_.autoscale = false; dirty_ = true;
        }

        /**
         * @brief Automatically scale axes tightly around the plotted data.
         *
         * Sets the padding fraction to 0.0, meaning the axis limits will closely
         * match the data bounds. No extra margin is added around the data.
         *
         * @note Calling this will invalidate the current render and force a re-draw
         *       on the next call to render(), show(), or save().
         */
        void axis_tight()
        {
            axes_.pad_frac = 0.0;
            dirty_ = true;
        }

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
        void axis_pad(double frac)
        {
            axes_.pad_frac = std::max(0.0, frac);
            dirty_ = true;
        }

        /**
         * @brief Enable or disable autoscaling.
         *
         * When enabled, the axes limits will automatically adjust to the data bounds.
         *
         * @param on Flag to enable autoscaling (true) or disable it (false).
         */
        void autoscale(bool on = true) { axes_.autoscale = on; dirty_ = true; }

        /**
         * @brief Enable or disable equal scaling.
         *
         * Adjusts the plot so that one unit in the x-axis is the same length as one unit in the y-axis.
         *
         * @param on Flag to enable equal scaling (true) or disable it (false).
         */
        void equal_scale(bool on = true) { axes_.equal_scale = on; dirty_ = true; }

        /**
         * @brief Enable or disable grid lines.
         *
         * Turns the grid lines on or off.
         *
         * @param on Flag to enable grid lines (true) or disable them (false).
         */
        void grid(bool on = true) { axes_.grid = on; dirty_ = true; }

        /**
         * @brief Set the figure title.
         *
         * Sets the title text displayed on the figure.
         *
         * @param t Title text.
         */
        void title(const std::string& t) { title_ = t; dirty_ = true; }

        /**
         * @brief Set the x-axis label.
         *
         * Sets the label for the x-axis.
         *
         * @param t Label text.
         */
        void xlabel(const std::string& t) { xlabel_ = t; dirty_ = true; }

        /**
         * @brief Set the y-axis label.
         *
         * Sets the label for the y-axis and invalidates any cached rotated label.
         *
         * @param t Label text.
         */
        void ylabel(const std::string& t)
        {
            ylabel_ = t; ylabel_cache_valid_ = false; dirty_ = true;
        }

        // ========================================================================
        // Core functions for rendering, showing, and saving the figure.
        // ========================================================================

        /**
         * @brief Convert retained commands to pixel representation.
         *
         * Renders the plot by converting retained drawing commands to pixels.
         * If the figure is already rendered (i.e., not dirty), the function returns immediately.
         */
        void render()
        {
            if (!dirty_) return;

            // 1) Autoscale fast‑path
            if (axes_.autoscale)
            {
                if (data_bounds_.valid())
                {
                    axes_.xmin = data_bounds_.xmin; axes_.xmax = data_bounds_.xmax;
                    axes_.ymin = data_bounds_.ymin; axes_.ymax = data_bounds_.ymax;
                }
                else
                {
                    /* Fallback when no data is present */
                    axes_.xmin = 0; axes_.xmax = 1; axes_.ymin = 0; axes_.ymax = 1;
                }
            }

            // 1b) Optional padding
            if (axes_.pad_frac > 0.0)
            {
                const double dx = (axes_.xmax - axes_.xmin) * axes_.pad_frac;
                const double dy = (axes_.ymax - axes_.ymin) * axes_.pad_frac;
                axes_.xmin -= dx;  axes_.xmax += dx;
                axes_.ymin -= dy;  axes_.ymax += dy;
            }

            // Guarantee non-zero spans before further math
            fix_ranges(axes_);

            // 2) Equal-scale adjustment
            if (axes_.equal_scale)
            {
                const double xrange = axes_.xmax - axes_.xmin;
                const double yrange = axes_.ymax - axes_.ymin;
                const double span = std::max(xrange, yrange);
                const double xmid = 0.5 * (axes_.xmin + axes_.xmax);
                const double ymid = 0.5 * (axes_.ymin + axes_.ymax);
                axes_.xmin = xmid - span / 2;
                axes_.xmax = xmid + span / 2;
                axes_.ymin = ymid - span / 2;
                axes_.ymax = ymid + span / 2;
            }
            fix_ranges(axes_);  /* re‑check spans */

            // 3) Tick generation (single pass)
            const TickInfo xticks = make_ticks(axes_.xmin, axes_.xmax);
            const TickInfo yticks = make_ticks(axes_.ymin, axes_.ymax);

            // 4) Clear canvas
            canvas_.setTo(cv::Scalar(255, 255, 255));

            // 5) Draw grid & axes
            draw_grid(xticks, yticks);
            draw_axes(xticks, yticks);

            // 6) Draw retained commands
            for (const auto& cmd : cmds_)
            {
                const cv::Scalar cvcol(cmd.color.b, cmd.color.g, cmd.color.r);

                switch (cmd.type)
                {
                    case CmdType::Line:
                    {
                        const auto& X = cmd.line.x;
                        const auto& Y = cmd.line.y;
                        for (size_t i = 1; i < X.size(); ++i)
                        {
                            cv::line(canvas_, data_to_pixel(X[i - 1], Y[i - 1]),
                                data_to_pixel(X[i], Y[i]),
                                cvcol,
                                static_cast<int>(cmd.line.thickness),
                                cv::LINE_AA);
                        }
                        break;
                    }
                    case CmdType::Scatter:
                    {
                        const auto& X = cmd.scatter.x;
                        const auto& Y = cmd.scatter.y;
                        for (size_t i = 0; i < X.size(); ++i)
                        {
                            cv::circle(canvas_, data_to_pixel(X[i], Y[i]),
                                static_cast<int>(cmd.scatter.marker_size),
                                cvcol, cv::FILLED, cv::LINE_AA);
                        }
                        break;
                    }
                    case CmdType::Text:
                    {
                        const cv::Point2i p = anchored_text_pos(cmd.txt);
                        cv::putText(canvas_, cmd.txt.text, p,
                            cv::FONT_HERSHEY_SIMPLEX,
                            cmd.txt.font_scale, cvcol,
                            cmd.txt.thickness, cv::LINE_AA);
                        break;
                    }
                    case CmdType::Circle:
                    {
                        const auto& d = cmd.circle;
                        const cv::Point center = data_to_pixel(d.cx, d.cy);
                        const int radius_px = static_cast<int>(d.radius * plot_width() / (axes_.xmax - axes_.xmin));

                        if (d.style.fill_alpha > 0.0f && d.style.fill_alpha < 1.0f)
                        {
                            cv::Mat tmp = canvas_.clone();
                            cv::circle(tmp, center, radius_px,
                                cv_color(d.style.fill_color), cv::FILLED, cv::LINE_AA);
                            blend_shape(tmp, d.style.fill_alpha);
                        }
                        else if (d.style.fill_alpha >= 1.0f)
                        {
                            cv::circle(canvas_, center, radius_px,
                                cv_color(d.style.fill_color), cv::FILLED, cv::LINE_AA);
                        }

                        if (d.style.thickness > 0.0f)
                        {
                            cv::circle(canvas_, center, radius_px,
                                cv_color(d.style.line_color),
                                static_cast<int>(d.style.thickness), cv::LINE_AA);
                        }
                        break;
                    }
                    case CmdType::RectXYWH:
                    case CmdType::RectLTRB:
                    {
                        const auto& d = cmd.rect;
                        const cv::Point p0 = data_to_pixel(d.x0, d.y0);
                        const cv::Point p1 = data_to_pixel(d.x1, d.y1);
                        const cv::Rect r(p0, p1);

                        if (d.style.fill_alpha > 0.0f && d.style.fill_alpha < 1.0f)
                        {
                            cv::Mat tmp = canvas_.clone();
                            cv::rectangle(tmp, r, cv_color(d.style.fill_color), cv::FILLED, cv::LINE_AA);
                            blend_shape(tmp, d.style.fill_alpha);
                        }
                        else if (d.style.fill_alpha >= 1.0f)
                        {
                            cv::rectangle(canvas_, r, cv_color(d.style.fill_color), cv::FILLED, cv::LINE_AA);
                        }

                        if (d.style.thickness > 0.0f)
                        {
                            cv::rectangle(canvas_, r, cv_color(d.style.line_color),
                                static_cast<int>(d.style.thickness), cv::LINE_AA);
                        }
                        break;
                    }
                    case CmdType::RotatedRect:
                    {
                        const auto& d = cmd.rot_rect;
                        cv::RotatedRect r(data_to_pixel(d.cx, d.cy),
                            cv::Size2f(
                                static_cast<float>(d.width * plot_width() / (axes_.xmax - axes_.xmin)),
                                static_cast<float>(d.height * plot_height() / (axes_.ymax - axes_.ymin))),
                            static_cast<float>(-d.angle_deg));

                        cv::Point2f verts[4];
                        r.points(verts);

                        std::vector<cv::Point> pts(4);
                        for (int i = 0; i < 4; ++i)
                            pts[i] = verts[i];

                        if (d.style.fill_alpha > 0.0f && d.style.fill_alpha < 1.0f)
                        {
                            cv::Mat tmp = canvas_.clone();
                            cv::fillConvexPoly(tmp, pts, cv_color(d.style.fill_color), cv::LINE_AA);
                            blend_shape(tmp, d.style.fill_alpha);
                        }
                        else if (d.style.fill_alpha >= 1.0f)
                        {
                            cv::fillConvexPoly(canvas_, pts, cv_color(d.style.fill_color), cv::LINE_AA);
                        }

                        if (d.style.thickness > 0.0f)
                        {
                            cv::polylines(canvas_, pts, true, cv_color(d.style.line_color),
                                static_cast<int>(d.style.thickness), cv::LINE_AA);
                        }
                        break;
                    }
                    case CmdType::Polygon:
                    {
                        const auto& d = cmd.polygon;
                        std::vector<cv::Point> pts;
                        for (size_t i = 0; i < d.x.size(); ++i)
                            pts.push_back(data_to_pixel(d.x[i], d.y[i]));

                        if (d.style.fill_alpha > 0.0f && d.style.fill_alpha < 1.0f)
                        {
                            cv::Mat tmp = canvas_.clone();
                            cv::fillPoly(tmp, std::vector<std::vector<cv::Point>>{ pts },
                                cv_color(d.style.fill_color), cv::LINE_AA);
                            blend_shape(tmp, d.style.fill_alpha);
                        }
                        else if (d.style.fill_alpha >= 1.0f)
                        {
                            cv::fillPoly(canvas_, std::vector<std::vector<cv::Point>>{ pts },
                                cv_color(d.style.fill_color), cv::LINE_AA);
                        }

                        if (d.style.thickness > 0.0f)
                        {
                            cv::polylines(canvas_, pts, true, cv_color(d.style.line_color),
                                static_cast<int>(d.style.thickness), cv::LINE_AA);
                        }
                        break;
                    }
                    case CmdType::Ellipse:
                    {
                        const auto& d = cmd.ellipse;
                        const cv::Point center = data_to_pixel(d.cx, d.cy);
                        const cv::Size axes(
                            static_cast<int>(0.5 * d.width * plot_width() / (axes_.xmax - axes_.xmin)),
                            static_cast<int>(0.5 * d.height * plot_height() / (axes_.ymax - axes_.ymin)));

                        if (d.style.fill_alpha > 0.0f && d.style.fill_alpha < 1.0f)
                        {
                            cv::Mat tmp = canvas_.clone();
                            cv::ellipse(tmp, center, axes, -d.angle_deg, 0, 360,
                                cv_color(d.style.fill_color), cv::FILLED, cv::LINE_AA);
                            blend_shape(tmp, d.style.fill_alpha);
                        }
                        else if (d.style.fill_alpha >= 1.0f)
                        {
                            cv::ellipse(canvas_, center, axes, -d.angle_deg, 0, 360,
                                cv_color(d.style.fill_color), cv::FILLED, cv::LINE_AA);
                        }

                        if (d.style.thickness > 0.0f)
                        {
                            cv::ellipse(canvas_, center, axes, -d.angle_deg, 0, 360,
                                cv_color(d.style.line_color),
                                static_cast<int>(d.style.thickness), cv::LINE_AA);
                        }
                        break;
                    }
                } //switch
            }

            // 7) Figure title and axis labels
            if (!title_.empty())
            {
                cv::putText(canvas_, title_, { kMargin, kMargin / 2 },
                    cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            }
            if (!xlabel_.empty())
            {
                cv::putText(canvas_, xlabel_, { width_ / 2 - 40, height_ - 10 },
                    cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            }
            draw_ylabel();   // rotated y-label

            dirty_ = false;  // canvas is now up-to-date
        }

        /**
         * @brief Render (if needed) and display the figure in an OpenCV window.
         *
         * If the canvas is marked as dirty, render() is called before displaying.
         *
         * @param window_name Name of the OpenCV window. Defaults to "Figure".
         */
        void show(const std::string& window_name = "Figure")
        {
            if (dirty_) render();
            cv::imshow(window_name, canvas_);
            cv::waitKey(1);   /* small delay so the window appears */
        }

        /**
         * @brief Render (if needed) and save the figure to disk.
         *
         * Renders the canvas if needed and writes the image to disk.
         * The image format is inferred from the file extension.
         *
         * @param filename Output file path.
         */
        void save(const std::string& filename)
        {
            if (dirty_) render();
            cv::imwrite(filename, canvas_);
        }




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
        cv::Point2i data_to_pixel(double x, double y) const
        {
            const double x_frac = (x - axes_.xmin) / (axes_.xmax - axes_.xmin);
            const double y_frac = (y - axes_.ymin) / (axes_.ymax - axes_.ymin);

            const int px = kMarginLeft + static_cast<int>(x_frac * plot_width() + 0.5);
            const int py = height_ - kMarginBottom - static_cast<int>(y_frac * plot_height() + 0.5);
            return { px, py };
        }

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
        cv::Scalar cv_color(const Color& c, float alpha = 1.0f)
        {
            return cv::Scalar(c.b, c.g, c.r, static_cast<uchar>(alpha * 255));
        }

        /**
         * @brief Blend a shape drawn into a temporary buffer onto the main canvas using alpha.
         *
         * @param shape Image containing the shape (same size as canvas_).
         * @param alpha Blend factor in range [0.0, 1.0].
         */
        void blend_shape(const cv::Mat& shape, float alpha)
        {
            cv::addWeighted(shape, alpha, canvas_, 1.0 - alpha, 0.0, canvas_);
        }


        /* ---------- axis, grid, tick helpers ------------------------------ */
        /// @brief Computes a "nice" number for tick spacing.
        static double  nice_num(double range, bool round);
        /// @brief Generates tick positions and formatted labels.
        static TickInfo make_ticks(double lo, double hi, int target = 6);
        /// @brief Draws the axis lines, ticks, and numeric labels.
        void           draw_axes(const TickInfo& xt, const TickInfo& yt);
        /// @brief Draws grid lines corresponding to tick positions.
        void           draw_grid(const TickInfo& xt, const TickInfo& yt);

        /* ---------- rotated y-label helper -------------------------------- */
        /// @brief Draws and caches the rotated y-axis label.
        void draw_ylabel();

        /* ---------- text alignment helper --------------------------------- */
        /// @brief Computes the anchored text position based on alignment.
        cv::Point2i anchored_text_pos(const TextData& td) const
        {
            int bl = 0;
            const auto sz = cv::getTextSize(td.text, cv::FONT_HERSHEY_SIMPLEX,
                td.font_scale, td.thickness, &bl);
            cv::Point2i p = data_to_pixel(td.x, td.y);

            if (td.halign == TextData::HAlign::Center)       p.x -= sz.width / 2;
            else if (td.halign == TextData::HAlign::Right)   p.x -= sz.width;

            if (td.valign == TextData::VAlign::Center)       p.y += sz.height / 2;
            else if (td.valign == TextData::VAlign::Top)     p.y += sz.height;
            else if (td.valign == TextData::VAlign::Bottom)  p.y -= bl;

            return p;
        }

        /* ---------- safety helpers ---------------------------------------- */
        /// @brief Ensures that the span between lo and hi is non-zero.
        static void ensure_nonzero_span(double& lo, double& hi);
        /// @brief Fixes the ranges of the axes to ensure non-zero spans.
        static void fix_ranges(Axes& a);

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
        void add_line_command(VX&& x, VY&& y, Color c, float thickness)
        {
            PlotCommand cmd;
            cmd.type = CmdType::Line;
            cmd.color = c;
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
        void add_scatter_command(VX&& x, VY&& y, Color c, float marker_size)
        {
            PlotCommand cmd;
            cmd.type = CmdType::Scatter;
            cmd.color = c;
            cmd.scatter.x = std::forward<VX>(x);
            cmd.scatter.y = std::forward<VY>(y);
            cmd.scatter.marker_size = marker_size;
            expand_bounds(cmd.scatter.x, cmd.scatter.y);
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }
        /// @brief Expands the cached data bounds using the given vectors.
        void expand_bounds(const std::vector<double>& xs, const std::vector<double>& ys)
        {
            for (size_t i = 0; i < xs.size(); ++i)
                data_bounds_.expand(xs[i], ys[i]);
        }
    };

    /* ----------------------------------------------------------------------
     * Inline helper implementations
     * --------------------------------------------------------------------*/

     /// @brief Choose a "nice" number for tick spacing.
    inline double Figure::nice_num(double range, bool round)
    {
        if (range <= 0) range = 1.0;  /* numerical robustness */
        const double expv = std::floor(std::log10(range));
        const double f = range / std::pow(10.0, expv);   /* 1‑10 */

        double nf = 1;
        if (round)
        {
            if (f < 1.5)      nf = 1;
            else if (f < 3)   nf = 2;
            else if (f < 7)   nf = 5;
            else              nf = 10;
        }
        else
        {
            if (f <= 1)       nf = 1;
            else if (f <= 2)  nf = 2;
            else if (f <= 5)  nf = 5;
            else              nf = 10;
        }
        return nf * std::pow(10.0, expv);
    }

    /// @brief Build tick positions and formatted labels.
    inline TickInfo Figure::make_ticks(double lo, double hi, int target)
    {
        const double range = nice_num(hi - lo, false);
        const double step = nice_num(range / (target - 1), true);
        const double graph_lo = std::floor(lo / step) * step;
        const double graph_hi = std::ceil(hi / step) * step;

        TickInfo out;
        for (double v = graph_lo; v <= graph_hi + 0.5 * step; v += step)
        {
            if (v < lo - 1e-12 || v > hi + 1e-12)        // <‑‑ clip here
                continue;

            out.locs.push_back(v);

            std::ostringstream ss;
            ss << std::fixed << std::setprecision((step < 1) ? 1 : 0) << v;
            out.labels.push_back(ss.str());
        }
        return out;
    }

    /// @brief Draw axis lines, ticks and numeric labels.
    inline void Figure::draw_axes(const TickInfo& xticks, const TickInfo& yticks)
    {
        const cv::Scalar black(0, 0, 0);
        const int font = cv::FONT_HERSHEY_SIMPLEX;

        /* X‑axis ---------------------------------------------------------------- */
        cv::line(canvas_, { kMarginLeft, height_ - kMarginBottom },
            { width_ - kMarginRight, height_ - kMarginBottom },
            black, 1);

        for (size_t i = 0; i < xticks.locs.size(); ++i)
        {
            cv::Point2i p = data_to_pixel(xticks.locs[i], axes_.ymin);
            cv::line(canvas_, { p.x, p.y },
                { p.x, p.y + kTickLen }, black, 1);

            cv::putText(canvas_, xticks.labels[i],
                { p.x - 10, p.y + 18 },
                font, 0.4, black, 1, cv::LINE_AA);
        }

        /* Y‑axis ---------------------------------------------------------------- */
        cv::line(canvas_, { kMarginLeft, kMarginTop },
            { kMarginLeft, height_ - kMarginBottom },
            black, 1);

        for (size_t i = 0; i < yticks.locs.size(); ++i)
        {
            cv::Point2i p = data_to_pixel(axes_.xmin, yticks.locs[i]);
            cv::line(canvas_, { p.x - kTickLen, p.y },
                { p.x, p.y }, black, 1);

            constexpr int labelOffset = 30;          // pixels left of tick
            cv::putText(canvas_, yticks.labels[i],
                { p.x - labelOffset, p.y + 4 },
                font, 0.4, black, 1, cv::LINE_AA);
        }
    }


    /// @brief Draw grid lines matching the tick positions.
    inline void Figure::draw_grid(const TickInfo& xticks, const TickInfo& yticks)
    {
        if (!axes_.grid) return;

        const cv::Scalar light(220, 220, 220);   // light grey

        /* vertical grid lines -------------------------------------------------- */
        for (double xv : xticks.locs)
        {
            if (xv < axes_.xmin || xv > axes_.xmax) continue;   // safety

            const cv::Point2i p1 = data_to_pixel(xv, axes_.ymin);
            const cv::Point2i p2 = data_to_pixel(xv, axes_.ymax);
            cv::line(canvas_, p1, p2, light, 1, cv::LINE_AA);
        }

        /* horizontal grid lines ------------------------------------------------ */
        for (double yv : yticks.locs)
        {
            if (yv < axes_.ymin || yv > axes_.ymax) continue;   // safety

            const cv::Point2i p1 = data_to_pixel(axes_.xmin, yv);
            const cv::Point2i p2 = data_to_pixel(axes_.xmax, yv);
            cv::line(canvas_, p1, p2, light, 1, cv::LINE_AA);
        }
    }


    /// @brief Rotate and blit y-axis label (cached).
    inline void Figure::draw_ylabel()
    {
        if (ylabel_.empty()) return;

        if (!ylabel_cache_valid_)
        {
            int baseline = 0;
            const auto sz = cv::getTextSize(ylabel_, cv::FONT_HERSHEY_SIMPLEX,
                0.5, 1, &baseline);

            cv::Mat txt(sz.height + baseline, sz.width, CV_8UC3,
                cv::Scalar(255, 255, 255));
            cv::putText(txt, ylabel_, { 0, sz.height },
                cv::FONT_HERSHEY_SIMPLEX, 0.5,
                cv::Scalar(0, 0, 0), 1, cv::LINE_AA);

            cv::rotate(txt, ylabel_cache_, cv::ROTATE_90_COUNTERCLOCKWISE);
            ylabel_cache_valid_ = true;
        }

        /* Top-left corner for copy */
        const int label_offset = 55;  // or tweak as needed
        const int x = kMarginLeft - label_offset;
        
        const int y = kMarginTop + (plot_height() - ylabel_cache_.rows) / 2;        

        if (x >= 0 && y >= 0 &&
            x + ylabel_cache_.cols <= canvas_.cols &&
            y + ylabel_cache_.rows <= canvas_.rows)
        {
            ylabel_cache_.copyTo(canvas_(cv::Rect(x, y,
                ylabel_cache_.cols,
                ylabel_cache_.rows)));
        }
    }

    /// @brief Ensures that the span between lo and hi is non-zero.
    inline void Figure::ensure_nonzero_span(double& lo, double& hi)
    {
        if (lo == hi)
        {
            const double eps = (std::abs(lo) > 1e-12)
                ? std::abs(lo) * 1e-3 : 1e-3;
            lo -= eps; hi += eps;
        }
    }
    /// @brief Fixes the axes ranges to ensure non-zero spans.
    inline void Figure::fix_ranges(Axes& a)
    {
        ensure_nonzero_span(a.xmin, a.xmax);
        ensure_nonzero_span(a.ymin, a.ymax);
    }

} // namespace mpocv
