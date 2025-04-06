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
#include <utility>
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
        std::vector<double>        locs;   ///< tick locations in data space
        std::vector<std::string>   labels; ///< formatted strings
    };

    /* --------------------------------------------------------------------------
     * Helper for cached data bounds (used by the autoscale fast?path)
     * ------------------------------------------------------------------------*/
    struct Bounds
    {
        double xmin = std::numeric_limits<double>::infinity();
        double xmax = -std::numeric_limits<double>::infinity();
        double ymin = std::numeric_limits<double>::infinity();
        double ymax = -std::numeric_limits<double>::infinity();

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
          * @brief Draw a connected poly-line (lvalue overload).
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
        void set_xlim(double lo, double hi)
        {
            axes_.xmin = lo; axes_.xmax = hi; axes_.autoscale = false; dirty_ = true;
        }

        void set_ylim(double lo, double hi)
        {
            axes_.ymin = lo; axes_.ymax = hi; axes_.autoscale = false; dirty_ = true;
        }

        void autoscale(bool on = true) { axes_.autoscale = on; dirty_ = true; }
        void equal_scale(bool on = true) { axes_.equal_scale = on; dirty_ = true; }
        void grid(bool on = true) { axes_.grid = on; dirty_ = true; }

        void title(const std::string& t) { title_ = t; dirty_ = true; }
        void xlabel(const std::string& t) { xlabel_ = t; dirty_ = true; }
        void ylabel(const std::string& t)
        {
            ylabel_ = t; ylabel_cache_valid_ = false; dirty_ = true;
        }

        /**
         * @brief Convert retained commands to pixels.
         *
         * The function is idempotent: if nothing changed since the last call
         * (#dirty_ == false) it returns immediately.
         */
        void render()
        {
            if (!dirty_) return;

            /* 1) Autoscale fast?path ----------------------------------------- */
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

            /* Guarantee non-zero spans before further math ------------------- */
            fix_ranges(axes_);

            /* 2) Equal-scale adjustment -------------------------------------- */
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
            fix_ranges(axes_);  /* re?check spans */

            /* 3) Tick generation (single pass) ------------------------------- */
            const TickInfo xticks = make_ticks(axes_.xmin, axes_.xmax);
            const TickInfo yticks = make_ticks(axes_.ymin, axes_.ymax);

            /* 4) Clear canvas ------------------------------------------------- */
            canvas_.setTo(cv::Scalar(255, 255, 255));

            /* 5) Draw grid & axes -------------------------------------------- */
            draw_grid(xticks, yticks);
            draw_axes(xticks, yticks);

            /* 6) Draw retained commands -------------------------------------- */
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
                }
            }

            /* 7) Figure title and axis labels -------------------------------- */
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
        static constexpr int kMarginLeft = 60;
        static constexpr int kMarginRight = 20;
        static constexpr int kMarginTop = 40;
        static constexpr int kMarginBottom = 60;
        static constexpr int kTickLen = 5;
        static constexpr int kMargin = 50;   // title margin

        /* Canvas and retained state -----------------------------------------*/
        int                       width_, height_;
        cv::Mat                   canvas_;
        std::vector<PlotCommand>  cmds_;
        Axes                      axes_;
        std::string               title_, xlabel_, ylabel_;
        bool                      dirty_{ true };

        /* Cached data bounds for fast autoscale -----------------------------*/
        Bounds                    data_bounds_;

        /* Cached rotated y?label -------------------------------------------*/
        cv::Mat                   ylabel_cache_;
        bool                      ylabel_cache_valid_{ false };

        /* -------------------------------------------------------- helpers ---*/
        int  plot_width()  const { return width_ - kMarginLeft - kMarginRight; }
        int  plot_height() const { return height_ - kMarginTop - kMarginBottom; }

        /**
         * @brief Convert data coordinates to pixel coordinates.
         */
        cv::Point2i data_to_pixel(double x, double y) const
        {
            const double x_frac = (x - axes_.xmin) / (axes_.xmax - axes_.xmin);
            const double y_frac = (y - axes_.ymin) / (axes_.ymax - axes_.ymin);

            const int px = kMarginLeft + static_cast<int>(x_frac * plot_width() + 0.5);
            const int py = height_ - kMarginBottom - static_cast<int>(y_frac * plot_height() + 0.5);
            return { px, py };
        }

        /* ---------- axis, grid, tick helpers ------------------------------ */
        static double  nice_num(double range, bool round);
        static TickInfo make_ticks(double lo, double hi, int target = 6);
        void           draw_axes(const TickInfo& xt, const TickInfo& yt);
        void           draw_grid(const TickInfo& xt, const TickInfo& yt);

        /* ---------- rotated y-label helper -------------------------------- */
        void draw_ylabel();

        /* ---------- text alignment helper --------------------------------- */
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
        static void ensure_nonzero_span(double& lo, double& hi);
        static void fix_ranges(Axes& a);

        /* ---------- internal helpers to add commands ---------------------- */
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
        void expand_bounds(const std::vector<double>& xs, const std::vector<double>& ys)
        {
            for (size_t i = 0; i < xs.size(); ++i)
                data_bounds_.expand(xs[i], ys[i]);
        }
    };

    /* ----------------------------------------------------------------------
     * Inline helper implementations
     * --------------------------------------------------------------------*/

     /* Choose a "nice" number for tick spacing */
    inline double Figure::nice_num(double range, bool round)
    {
        if (range <= 0) range = 1.0;  /* numerical robustness */
        const double expv = std::floor(std::log10(range));
        const double f = range / std::pow(10.0, expv);   /* 1?10 */

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
    inline void Figure::draw_axes(const TickInfo& xticks, const TickInfo& yticks)
    {
        const cv::Scalar black(0, 0, 0);
        const int font = cv::FONT_HERSHEY_SIMPLEX;

        /* X axis ----------------------------------------------------------- */
        cv::line(canvas_, { kMarginLeft, height_ - kMarginBottom },
            { width_ - kMarginRight, height_ - kMarginBottom },
            black, 1);
        for (size_t i = 0; i < xticks.locs.size(); ++i)
        {
            cv::Point2i p = data_to_pixel(xticks.locs[i], axes_.ymin);
            cv::line(canvas_, { p.x, p.y }, { p.x, p.y + kTickLen }, black, 1);
            cv::putText(canvas_, xticks.labels[i],
                { p.x - 10, p.y + 18 }, font, 0.4, black, 1, cv::LINE_AA);
        }

        /* Y axis ----------------------------------------------------------- */
        cv::line(canvas_, { kMarginLeft, kMarginTop },
            { kMarginLeft, height_ - kMarginBottom },
            black, 1);
        for (size_t i = 0; i < yticks.locs.size(); ++i)
        {
            cv::Point2i p = data_to_pixel(axes_.xmin, yticks.locs[i]);
            cv::line(canvas_, { p.x - kTickLen, p.y }, { p.x, p.y }, black, 1);
            cv::putText(canvas_, yticks.labels[i],
                { p.x - 45, p.y + 4 }, font, 0.4, black, 1, cv::LINE_AA);
        }
    }

    /* Draw grid lines matching the tick positions */
    inline void Figure::draw_grid(const TickInfo& xticks, const TickInfo& yticks)
    {
        if (!axes_.grid) return;

        for (double xv : xticks.locs)
        {
            const cv::Point2i p1 = data_to_pixel(xv, axes_.ymin);
            const cv::Point2i p2 = data_to_pixel(xv, axes_.ymax);
            cv::line(canvas_, p1, p2, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);
        }
        for (double yv : yticks.locs)
        {
            const cv::Point2i p1 = data_to_pixel(axes_.xmin, yv);
            const cv::Point2i p2 = data_to_pixel(axes_.xmax, yv);
            cv::line(canvas_, p1, p2, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);
        }
    }

    /* Rotate and blit y-axis label (cached) */
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
        const int x = 10;
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
