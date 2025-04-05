#pragma once

 /* OpenCV ------------------------------------------------------------------*/
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

/* C++ std -----------------------------------------------------------------*/
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

/* Project headers ---------------------------------------------------------*/
#include "color.h"
#include "plot_command.h"
#include "axes.h"

namespace mpocv
{
    /* --------------------------------------------------------------------------
     * Forward-declared helper needs the struct FIRST
     * ------------------------------------------------------------------------*/
    struct TickInfo
    {
        std::vector<double> locs;   ///< tick locations in data space
        std::vector<std::string> labels; ///< formatted strings
    };

    /* Internal struct for tick lists ----------------------------------- */
    //struct TickInfo { std::vector<double> locs; std::vector<std::string> labels; };

    /**
     * @class Figure
     * @brief A retained-command plotting canvas.
     *
     * The Figure class stores a list of drawing commands (lines, scatter points,
     * text) and converts them to pixels only when #render(), #show() or #save()
     * is called.  This keeps the user-facing API similar to matplotlib / Matlab
     * while avoiding unnecessary redraws.
     * 
     * A Figure owns:
     *   * a pixel buffer (cv::Mat)
     *   * an Axes object describing the data coordinate system
     *   * a vector of PlotCommand structs
     *
     * Thread safety: concurrent access to a single Figure must be guarded by
     * the caller.  Separate Figure instances can be used from different threads
     * without locking.
     */
    class Figure
    {
    public:
        /**
         * @brief Construct a new Figure.
         * @param w  Canvas width  in pixels.
         * @param h  Canvas height in pixels.
         */
        Figure(int w = 640, int h = 480)
            : width_(w), height_(h),
            canvas_(h, w, CV_8UC3, cv::Scalar(255, 255, 255))
        {}

        /* ------------------------------------------------------------------
         * Public API – adds retained drawing commands
         * ----------------------------------------------------------------*/

         /**
          * @brief Draw a connected poly-line.
          * @param x         Vector of x data values.
          * @param y         Vector of y data values (same length as @p x).
          * @param c         Line color (default blue).
          * @param thickness Line thickness in pixels.
          */
        void plot(const std::vector<double>& x,
            const std::vector<double>& y,
            Color c = Color::Blue(),
            float thickness = 1.f)
        {
            PlotCommand cmd;
            cmd.type = CmdType::Line;
            cmd.color = c;
            cmd.line.x = x;
            cmd.line.y = y;
            cmd.line.thickness = thickness;
            cmds_.push_back(std::move(cmd));
            dirty_ = true;      // mark canvas as out-of-date
        }

        /**
         * @brief Draw unconnected markers.
         * @param x            Vector of x positions.
         * @param y            Vector of y positions.
         * @param c            Marker color (default red).
         * @param marker_size  Radius of the filled circle in pixels.
         */
        void scatter(const std::vector<double>& x,
            const std::vector<double>& y,
            Color c = Color::Red(),
            float marker_size = 4.f)
        {
            PlotCommand cmd;
            cmd.type = CmdType::Scatter;
            cmd.color = c;
            cmd.scatter.x = x;
            cmd.scatter.y = y;
            cmd.scatter.marker_size = marker_size;
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }

        /**
         * @brief Place a text annotation at data coordinates.
         *
         * Alignment follows matplotlib semantics:
         *   HAlign::Left   – anchor at left edge of text box
         *   HAlign::Center – anchor at text box center
         *   HAlign::Right  – anchor at right edge
         *
         *   VAlign::Baseline – y is the text baseline (default)
         *   VAlign::Center   – vertical center
         *   VAlign::Top      – top edge
         *   VAlign::Bottom   – bottom edge (descenders)
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

        /* Axis / style setters ------------------------------------------------*/
        void set_xlim(double lo, double hi) { axes_.xmin = lo; axes_.xmax = hi; axes_.autoscale = false; dirty_ = true; }
        void set_ylim(double lo, double hi) { axes_.ymin = lo; axes_.ymax = hi; axes_.autoscale = false; dirty_ = true; }
        void autoscale(bool on = true) { axes_.autoscale = on; dirty_ = true; }
        void equal_scale(bool on = true) { axes_.equal_scale = on; dirty_ = true; }
        void grid(bool on = true) { axes_.grid = on; dirty_ = true; }

        void title(const std::string& t) { title_ = t; dirty_ = true; }
        void xlabel(const std::string& t) { xlabel_ = t; dirty_ = true; }
        void ylabel(const std::string& t) { ylabel_ = t; dirty_ = true; }

        /**
         * @brief Convert retained commands to pixels.
         *
         * The function is idempotent: if nothing changed since the last call
         * (#dirty_ == false) it returns immediately.
         */
        void render()
        {
            if (!dirty_) return;

            /* 1) Autoscale ---------------------------------------------------- */
            if (axes_.autoscale)
            {
                axes_.reset();
                for (const auto& cmd : cmds_)
                {
                    /* Select the correct x/y vectors for this command */
                    const std::vector<double>* xs = nullptr;
                    const std::vector<double>* ys = nullptr;
                    if (cmd.type == CmdType::Line) { xs = &cmd.line.x;     ys = &cmd.line.y; }
                    else if (cmd.type == CmdType::Scatter) { xs = &cmd.scatter.x; ys = &cmd.scatter.y; }
                    if (xs)
                    {
                        for (size_t i = 0; i < xs->size(); ++i)
                        {
                            axes_.xmin = std::min(axes_.xmin, (*xs)[i]);
                            axes_.xmax = std::max(axes_.xmax, (*xs)[i]);
                            axes_.ymin = std::min(axes_.ymin, (*ys)[i]);
                            axes_.ymax = std::max(axes_.ymax, (*ys)[i]);
                        }
                    }
                }
                if (!std::isfinite(axes_.xmin))
                {
                    /* Fallback when no data is present */
                    axes_.xmin = 0; axes_.xmax = 1; axes_.ymin = 0; axes_.ymax = 1;
                }
            }

            /* Guarantee non-zero spans before further math */
            fix_ranges(axes_);

            /* 2) Equal-scale adjustment -------------------------------------- */
            if (axes_.equal_scale)
            {
                double xrange = axes_.xmax - axes_.xmin;
                double yrange = axes_.ymax - axes_.ymin;
                double span = std::max(xrange, yrange);
                double xmid = 0.5 * (axes_.xmin + axes_.xmax);
                double ymid = 0.5 * (axes_.ymin + axes_.ymax);
                axes_.xmin = xmid - span / 2;
                axes_.xmax = xmid + span / 2;
                axes_.ymin = ymid - span / 2;
                axes_.ymax = ymid + span / 2;
            }

            /* Re-check spans (equal_scale could have collapsed them) */
            fix_ranges(axes_);

            /* 3) Clear canvas ------------------------------------------------- */
            canvas_.setTo(cv::Scalar(255, 255, 255));

            /* 4) Draw grid & axes -------------------------------------------- */
            draw_grid();
            draw_axes();

            /* 5) Draw retained commands -------------------------------------- */
            for (const auto& cmd : cmds_)
            {
                cv::Scalar cvcol(cmd.color.b, cmd.color.g, cmd.color.r);

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
                    /* Compute anchor point based on alignment ----------------- */
                    int bl = 0;
                    auto sz = cv::getTextSize(cmd.txt.text, cv::FONT_HERSHEY_SIMPLEX,
                        cmd.txt.font_scale, cmd.txt.thickness, &bl);
                    cv::Point2i p = data_to_pixel(cmd.txt.x, cmd.txt.y);

                    if (cmd.txt.halign == TextData::HAlign::Center)  p.x -= sz.width / 2;
                    else if (cmd.txt.halign == TextData::HAlign::Right) p.x -= sz.width;

                    if (cmd.txt.valign == TextData::VAlign::Center)     p.y += sz.height / 2;
                    else if (cmd.txt.valign == TextData::VAlign::Top)   p.y += sz.height;
                    else if (cmd.txt.valign == TextData::VAlign::Bottom) p.y -= bl;

                    cv::putText(canvas_, cmd.txt.text, p,
                        cv::FONT_HERSHEY_SIMPLEX,
                        cmd.txt.font_scale, cvcol,
                        cmd.txt.thickness, cv::LINE_AA);
                    break;
                }
                }
            }

            /* 6) Figure title and axis labels ------------------------------- */
            const int margin = 50;
            if (!title_.empty())
            {
                cv::putText(canvas_, title_, { margin, margin / 2 },
                    cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            }
            if (!xlabel_.empty())
            {
                cv::putText(canvas_, xlabel_, { width_ / 2 - 40, height_ - 10 },
                    cv::FONT_HERSHEY_SIMPLEX, 0.5,
                    cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            }
            draw_ylabel();   /* rotated y-label */

            dirty_ = false;  /* canvas is now up-to-date */
        }

        /**
         * @brief Render (if needed) and display in an OpenCV window.
         * @param window_name  Name passed to cv::imshow.
         */
        void show(const std::string& window_name = "Figure")
        {
            if (dirty_) render();
            cv::imshow(window_name, canvas_);
            cv::waitKey(1);   /* small delay so the window appears */
        }

        /**
         * @brief Render (if needed) and write the canvas to disk.
         * @param filename  Output file path (format inferred from extension).
         */
        void save(const std::string& filename)
        {
            if (dirty_) render();
            cv::imwrite(filename, canvas_);
        }

    private:
        /* ------------------------------------------------------------------
         * Constant margins around the plotting region (pixels)
         * ----------------------------------------------------------------*/
        const int margin_left = 60;
        const int margin_right = 20;
        const int margin_top = 40;
        const int margin_bottom = 60;

        /* Canvas and retained state -----------------------------------------*/
        int width_, height_;
        cv::Mat canvas_;
        std::vector<PlotCommand> cmds_;
        Axes axes_;
        std::string title_, xlabel_, ylabel_;
        bool dirty_{ true };

        /* -------------------------------------------------------- helpers ---*/
        int  plot_width() const { return width_ - margin_left - margin_right; }
        int  plot_height() const { return height_ - margin_top - margin_bottom; }

        /**
         * @brief Convert data coordinates to pixel coordinates.
         * @param x  Data x.
         * @param y  Data y.
         * @return   cv::Point in image space (origin top-left).
         */
        cv::Point2i data_to_pixel(double x, double y) const
        {
            double x_frac = (x - axes_.xmin) / (axes_.xmax - axes_.xmin + 1e-12);
            double y_frac = (y - axes_.ymin) / (axes_.ymax - axes_.ymin + 1e-12);

            int px = margin_left + static_cast<int>(x_frac * plot_width() + 0.5);
            int py = height_ - margin_bottom - static_cast<int>(y_frac * plot_height() + 0.5);
            return { px, py };
        }

        /* ---------- axis, grid, tick helpers ------------------------------ */
        static double nice_num(double range, bool round);
        static TickInfo make_ticks(double lo, double hi, int target = 6);
        void           draw_axes();
        void           draw_grid();

        /* ---------- rotated y-label helper -------------------------------- */
        void draw_ylabel();

        /* ---------- safety helpers ---------------------------------------- */
        static void ensure_nonzero_span(double& lo, double& hi);
        static void fix_ranges(Axes& a);
    };

    /* ----------------------------------------------------------------------
     * Inline helper implementations
     * --------------------------------------------------------------------*/

     /* Choose a "nice" number for tick spacing */
    inline double Figure::nice_num(double range, bool round)
    {
        const double expv = std::floor(std::log10(range));
        const double f = range / std::pow(10.0, expv);   /* 1-10 */
        double nf;
        if (round)
            nf = (f < 1.5) ? 1 : (f < 3) ? 2 : (f < 7) ? 5 : 10;
        else
            nf = (f <= 1) ? 1 : (f <= 2) ? 2 : (f <= 5) ? 5 : 10;
        return nf * std::pow(10.0, expv);
    }

    /* Build tick positions + formatted labels */
    inline TickInfo Figure::make_ticks(double lo, double hi, int target)
    {
        const double range = nice_num(hi - lo, false);
        const double step = nice_num(range / (target - 1), true);
        const double graph_lo = std::floor(lo / step) * step;
        const double graph_hi = std::ceil(hi / step) * step;

        TickInfo out;
        for (double v = graph_lo; v <= graph_hi + 0.5 * step; v += step)
        {
            out.locs.push_back(v);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision((step < 1) ? 1 : 0) << v;
            out.labels.push_back(ss.str());
        }
        return out;
    }

    /* Draw axis lines, ticks and numeric labels */
    inline void Figure::draw_axes()
    {
        const auto xticks = make_ticks(axes_.xmin, axes_.xmax);
        const auto yticks = make_ticks(axes_.ymin, axes_.ymax);

        const cv::Scalar black(0, 0, 0);
        const int tick_len = 5;
        const int font = cv::FONT_HERSHEY_SIMPLEX;

        /* X axis */
        cv::line(canvas_, { margin_left, height_ - margin_bottom },
            { width_ - margin_right, height_ - margin_bottom },
            black, 1);
        for (size_t i = 0; i < xticks.locs.size(); ++i)
        {
            cv::Point2i p = data_to_pixel(xticks.locs[i], axes_.ymin);
            cv::line(canvas_, { p.x, p.y }, { p.x, p.y + tick_len }, black, 1);
            cv::putText(canvas_, xticks.labels[i],
                { p.x - 10, p.y + 18 }, font, 0.4, black, 1, cv::LINE_AA);
        }

        /* Y axis */
        cv::line(canvas_, { margin_left, margin_top },
            { margin_left, height_ - margin_bottom },
            black, 1);
        for (size_t i = 0; i < yticks.locs.size(); ++i)
        {
            cv::Point2i p = data_to_pixel(axes_.xmin, yticks.locs[i]);
            cv::line(canvas_, { p.x - tick_len, p.y }, { p.x, p.y }, black, 1);
            cv::putText(canvas_, yticks.labels[i],
                { p.x - 45, p.y + 4 }, font, 0.4, black, 1, cv::LINE_AA);
        }
    }

    /* Draw grid lines matching the tick positions */
    inline void Figure::draw_grid()
    {
        if (!axes_.grid) return;

        const auto xticks = make_ticks(axes_.xmin, axes_.xmax);
        const auto yticks = make_ticks(axes_.ymin, axes_.ymax);

        for (double xv : xticks.locs)
        {
            cv::Point2i p1 = data_to_pixel(xv, axes_.ymin);
            cv::Point2i p2 = data_to_pixel(xv, axes_.ymax);
            cv::line(canvas_, p1, p2, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);
        }
        for (double yv : yticks.locs)
        {
            cv::Point2i p1 = data_to_pixel(axes_.xmin, yv);
            cv::Point2i p2 = data_to_pixel(axes_.xmax, yv);
            cv::line(canvas_, p1, p2, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);
        }
    }

    /* Rotate and blit y-axis label */
    inline void Figure::draw_ylabel()
    {
        if (ylabel_.empty()) return;

        int baseline = 0;
        auto sz = cv::getTextSize(ylabel_, cv::FONT_HERSHEY_SIMPLEX,
            0.5, 1, &baseline);

        cv::Mat txt(sz.height + baseline, sz.width, CV_8UC3,
            cv::Scalar(255, 255, 255));
        cv::putText(txt, ylabel_, { 0, sz.height },
            cv::FONT_HERSHEY_SIMPLEX, 0.5,
            cv::Scalar(0, 0, 0), 1, cv::LINE_AA);

        cv::Mat rot;
        cv::rotate(txt, rot, cv::ROTATE_90_COUNTERCLOCKWISE);

        /* Top-left corner for copy */
        const int x = 10;
        const int y = margin_top + (plot_height() - rot.rows) / 2;
        if (x >= 0 && y >= 0 &&
            x + rot.cols <= canvas_.cols &&
            y + rot.rows <= canvas_.rows)
        {
            rot.copyTo(canvas_(cv::Rect(x, y, rot.cols, rot.rows)));
        }
    }

    /* ----------------------------------------------------------------------
     * Safety helpers
     * --------------------------------------------------------------------*/
    inline void Figure::ensure_nonzero_span(double& lo, double& hi)
    {
        if (lo == hi)
        {
            const double eps = (std::abs(lo) > 1e-12)
                ? std::abs(lo) * 1e-3 : 1e-3;
            lo -= eps; hi += eps;
        }
    }
    inline void Figure::fix_ranges(Axes& a)
    {
        ensure_nonzero_span(a.xmin, a.xmax);
        ensure_nonzero_span(a.ymin, a.ymax);
    }

} // namespace mpocv
