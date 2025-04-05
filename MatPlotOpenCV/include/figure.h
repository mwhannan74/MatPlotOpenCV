#pragma once

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include "color.h"
#include "plot_command.h"
#include "axes.h"

namespace mpocv
{

    class Figure
    {
    public:
        Figure(int w = 640, int h = 480)
            : width_(w), height_(h),
            canvas_(h, w, CV_8UC3, cv::Scalar(255, 255, 255))
        {}

        /* ------------------------------------------------------------------
         *  Public API – adds retained drawing commands
         * ----------------------------------------------------------------*/
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
            dirty_ = true;
        }

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

        void text(double x, double y, const std::string& msg,
            Color c = Color::Black())
        {
            PlotCommand cmd;
            cmd.type = CmdType::Text;
            cmd.color = c;
            cmd.txt.x = x;
            cmd.txt.y = y;
            cmd.txt.text = msg;
            cmds_.push_back(std::move(cmd));
            dirty_ = true;
        }

        /* Axis / style setters */
        void set_xlim(double lo, double hi) { axes_.xmin = lo; axes_.xmax = hi; axes_.autoscale = false; dirty_ = true; }
        void set_ylim(double lo, double hi) { axes_.ymin = lo; axes_.ymax = hi; axes_.autoscale = false; dirty_ = true; }
        void autoscale(bool on = true) { axes_.autoscale = on; dirty_ = true; }
        void equal_scale(bool on = true) { axes_.equal_scale = on; dirty_ = true; }
        void grid(bool on = true) { axes_.grid = on; dirty_ = true; }

        void title(const std::string& t) { title_ = t; dirty_ = true; }
        void xlabel(const std::string& t) { xlabel_ = t; dirty_ = true; }
        void ylabel(const std::string& t) { ylabel_ = t; dirty_ = true; }

        /* ------------------------------------------------------------------ */
        void render()
        {
            if (!dirty_) return;

            // 1) Autoscale pass ------------------------------------------------
            if (axes_.autoscale)
            {
                axes_.reset();
                for (const auto& cmd : cmds_)
                {
                    const std::vector<double>* xs = nullptr;
                    const std::vector<double>* ys = nullptr;
                    if (cmd.type == CmdType::Line) { xs = &cmd.line.x; ys = &cmd.line.y; }
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
                { // no data fallback
                    axes_.xmin = 0; axes_.xmax = 1; axes_.ymin = 0; axes_.ymax = 1;
                }
            }

            // Equal scale adjustment
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

            // 2) Clear canvas --------------------------------------------------
            canvas_.setTo(cv::Scalar(255, 255, 255));

            // 3) Draw grid and axes -------------------------------------------
            draw_grid();
            draw_axes();

            // 4) Iterate commands ---------------------------------------------
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
                            cvcol, static_cast<int>(cmd.line.thickness), cv::LINE_AA);
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
                    cv::putText(canvas_, cmd.txt.text,
                        data_to_pixel(cmd.txt.x, cmd.txt.y),
                        cv::FONT_HERSHEY_SIMPLEX,
                        cmd.txt.font_scale,
                        cvcol, 1, cv::LINE_AA);
                    break;
                }
                }
            }

            // 5) Labels --------------------------------------------------------
            const int margin = 50;
            if (!title_.empty())
            {
                cv::putText(canvas_, title_, cv::Point(margin, margin / 2),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            }
            if (!xlabel_.empty())
            {
                cv::putText(canvas_, xlabel_, cv::Point(width_ / 2 - 40, height_ - 10),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            }
            if (!ylabel_.empty())
            {
                cv::putText(canvas_, ylabel_, cv::Point(10, height_ / 2),
                    cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            }

            dirty_ = false;
        }

        void show(const std::string& window_name = "Figure")
        {
            if (dirty_) render();
            cv::imshow(window_name, canvas_);
            cv::waitKey(1);
        }

        void save(const std::string& filename)
        {
            if (dirty_) render();
            cv::imwrite(filename, canvas_);
        }

    private:
        // margins for plot region
        const int margin_left = 60;
        const int margin_right = 20;
        const int margin_top = 40;
        const int margin_bottom = 60;

        int width_, height_;
        cv::Mat canvas_;
        std::vector<PlotCommand> cmds_;
        Axes axes_;
        std::string title_;
        std::string xlabel_;
        std::string ylabel_;
        bool dirty_{ true };

        /* ---------------- helper functions ---------------- */
        cv::Point2i data_to_pixel(double x, double y) const
        {
            double x_frac = (x - axes_.xmin) / (axes_.xmax - axes_.xmin + 1e-12);
            double y_frac = (y - axes_.ymin) / (axes_.ymax - axes_.ymin + 1e-12);
            int plot_w = width_ - margin_left - margin_right;
            int plot_h = height_ - margin_top - margin_bottom;
            int px = margin_left + static_cast<int>(x_frac * plot_w + 0.5);
            int py = height_ - margin_bottom - static_cast<int>(y_frac * plot_h + 0.5);
            return { px, py };
        }

        void draw_axes()
        {
            // axis lines
            cv::line(canvas_, { margin_left, margin_top }, { margin_left, height_ - margin_bottom }, cv::Scalar(0, 0, 0), 1);
            cv::line(canvas_, { margin_left, height_ - margin_bottom }, { width_ - margin_right, height_ - margin_bottom }, cv::Scalar(0, 0, 0), 1);
        }

        void draw_grid()
        {
            if (!axes_.grid) return;
            const int num_ticks = 10;
            double x_step = (axes_.xmax - axes_.xmin) / num_ticks;
            double y_step = (axes_.ymax - axes_.ymin) / num_ticks;
            for (int i = 1; i < num_ticks; ++i)
            {
                // vertical lines
                double xv = axes_.xmin + i * x_step;
                cv::Point2i p1 = data_to_pixel(xv, axes_.ymin);
                cv::Point2i p2 = data_to_pixel(xv, axes_.ymax);
                cv::line(canvas_, p1, p2, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);
                // horizontal lines
                double yv = axes_.ymin + i * y_step;
                p1 = data_to_pixel(axes_.xmin, yv);
                p2 = data_to_pixel(axes_.xmax, yv);
                cv::line(canvas_, p1, p2, cv::Scalar(220, 220, 220), 1, cv::LINE_AA);
            }
        }
    };

} // namespace mpocv
