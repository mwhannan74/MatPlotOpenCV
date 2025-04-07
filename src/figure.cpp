// =============================================================================
//  MatPlotOpenCV - Minimal 2D plotting library using OpenCV
//  Copyright (c) 2025 Michael Hannan
//
//  This file is part of MatPlotOpenCV and is licensed under the BSD 3-Clause
//  License. See the LICENSE file in the project root for full terms.
// =============================================================================

#include "figure.h"

namespace mpocv
{

    /* --------------------------------------------------------------------------
     *  Public?facing API
     * ------------------------------------------------------------------------*/

     // ---------------------------------------------------------------------------
     // Construction & basic settings
     // ---------------------------------------------------------------------------
    Figure::Figure(int w, int h)
        : width_(w), height_(h),
        canvas_(h, w, CV_8UC3, cv::Scalar(255, 255, 255))
    {}

    void Figure::set_xlim(double lo, double hi)
    {
        axes_.xmin = lo; axes_.xmax = hi; axes_.autoscale = false; dirty_ = true;
    }
    void Figure::set_ylim(double lo, double hi)
    {
        axes_.ymin = lo; axes_.ymax = hi; axes_.autoscale = false; dirty_ = true;
    }
    void Figure::axis_tight()
    {
        axes_.pad_frac = 0.0; dirty_ = true;
    }
    void Figure::axis_pad(double frac)
    {
        axes_.pad_frac = std::max(0.0, frac); dirty_ = true;
    }
    void Figure::autoscale(bool on) { axes_.autoscale = on; dirty_ = true; }
    void Figure::equal_scale(bool on) { axes_.equal_scale = on; dirty_ = true; }
    void Figure::grid(bool on) { axes_.grid = on; dirty_ = true; }
    void Figure::title(const std::string& t) { title_ = t; dirty_ = true; }
    void Figure::xlabel(const std::string& t) { xlabel_ = t; dirty_ = true; }
    void Figure::ylabel(const std::string& t) { ylabel_ = t; ylabel_cache_valid_ = false; dirty_ = true; }
    void Figure::legend(bool on, const std::string& loc)
    {
        legend_on_ = on; legend_loc_ = loc; dirty_ = true;
    }

    // ---------------------------------------------------------------------------
    // Plot primitives (thin wrappers that delegate to helpers)
    // ---------------------------------------------------------------------------
    void Figure::plot(const std::vector<double>& x, const std::vector<double>& y,
        Color c, float thickness, const std::string& label)
    {
        add_line_command(x, y, c, thickness, label);
    }
    void Figure::plot(std::vector<double>&& x, std::vector<double>&& y,
        Color c, float thickness, const std::string& label)
    {
        add_line_command(std::move(x), std::move(y), c, thickness, label);
    }

    void Figure::scatter(const std::vector<double>& x, const std::vector<double>& y,
        Color c, float marker_size, const std::string& label)
    {
        add_scatter_command(x, y, c, marker_size, label);
    }
    void Figure::scatter(std::vector<double>&& x, std::vector<double>&& y,
        Color c, float marker_size, const std::string& label)
    {
        add_scatter_command(std::move(x), std::move(y), c, marker_size, label);
    }

    void Figure::text(double x, double y, const std::string& msg, Color c,
        double font_scale, int thickness,
        TextData::HAlign ha, TextData::VAlign va,
        const std::string& label)
    {
        PlotCommand cmd;
        cmd.type = CmdType::Text;
        cmd.color = c;
        cmd.label = label;
        cmd.txt = { x, y, msg, font_scale, thickness, ha, va };
        cmds_.push_back(std::move(cmd));
        dirty_ = true;
    }

    void Figure::circle(double cx, double cy, double radius,
        const ShapeStyle& style, const std::string& label)
    {
        PlotCommand cmd;
        cmd.type = CmdType::Circle;
        cmd.circle = { cx, cy, radius, style };
        cmd.label = label;
        data_bounds_.expand(cx - radius, cy - radius);
        data_bounds_.expand(cx + radius, cy + radius);
        cmds_.push_back(std::move(cmd));
        dirty_ = true;
    }

    void Figure::rect_xywh(double x, double y, double w, double h,
        const ShapeStyle& style, const std::string& label)
    {
        PlotCommand cmd;
        cmd.type = CmdType::RectXYWH;
        cmd.rect = { x, y, x + w, y + h, style };
        cmd.label = label;
        data_bounds_.expand(x, y);
        data_bounds_.expand(x + w, y + h);
        cmds_.push_back(std::move(cmd));
        dirty_ = true;
    }

    void Figure::rect_ltrb(double x0, double y0, double x1, double y1,
        const ShapeStyle& style, const std::string& label)
    {
        PlotCommand cmd;
        cmd.type = CmdType::RectLTRB;
        cmd.rect = { x0, y0, x1, y1, style };
        cmd.label = label;
        data_bounds_.expand(x0, y0);
        data_bounds_.expand(x1, y1);
        cmds_.push_back(std::move(cmd));
        dirty_ = true;
    }

    void Figure::rotated_rect(double cx, double cy, double w, double h, double angle_deg,
        const ShapeStyle& style, const std::string& label)
    {
        PlotCommand cmd;
        cmd.type = CmdType::RotatedRect;
        cmd.rot_rect = { cx, cy, w, h, angle_deg, style };
        cmd.label = label;

        const double r = 0.5 * std::sqrt(w * w + h * h);
        data_bounds_.expand(cx - r, cy - r);
        data_bounds_.expand(cx + r, cy + r);
        cmds_.push_back(std::move(cmd));
        dirty_ = true;
    }

    void Figure::polygon(const std::vector<double>& x, const std::vector<double>& y,
        const ShapeStyle& style, const std::string& label)
    {
        if (x.size() != y.size() || x.empty()) return;

        PlotCommand cmd;
        cmd.type = CmdType::Polygon;
        cmd.polygon = { x, y, style };
        cmd.label = label;

        for (size_t i = 0; i < x.size(); ++i)
            data_bounds_.expand(x[i], y[i]);

        cmds_.push_back(std::move(cmd));
        dirty_ = true;
    }

    void Figure::ellipse(double cx, double cy, double w, double h, double angle_deg,
        const ShapeStyle& style, const std::string& label)
    {
        PlotCommand cmd;
        cmd.type = CmdType::Ellipse;
        cmd.ellipse = { cx, cy, w, h, angle_deg, style };
        cmd.label = label;
        data_bounds_.expand(cx - 0.5 * w, cy - 0.5 * h);
        data_bounds_.expand(cx + 0.5 * w, cy + 0.5 * h);
        cmds_.push_back(std::move(cmd));
        dirty_ = true;
    }

    // ---------------------------------------------------------------------------
    // Rendering & I/O
    // ---------------------------------------------------------------------------
    void Figure::render()
    {
        if (!dirty_) return;

        /* 1) autoscale --------------------------------------------------------- */
        if (axes_.autoscale)
        {
            if (data_bounds_.valid())
            {
                axes_.xmin = data_bounds_.xmin; axes_.xmax = data_bounds_.xmax;
                axes_.ymin = data_bounds_.ymin; axes_.ymax = data_bounds_.ymax;
            }
            else { axes_.xmin = 0; axes_.xmax = 1; axes_.ymin = 0; axes_.ymax = 1; }
        }

        /* 1b) optional padding -------------------------------------------------- */
        if (axes_.pad_frac > 0.0)
        {
            const double dx = (axes_.xmax - axes_.xmin) * axes_.pad_frac;
            const double dy = (axes_.ymax - axes_.ymin) * axes_.pad_frac;
            axes_.xmin -= dx; axes_.xmax += dx;
            axes_.ymin -= dy; axes_.ymax += dy;
        }
        fix_ranges(axes_);

        /* 2) equal?scale -------------------------------------------------------- */
        if (axes_.equal_scale)
        {
            const double xrange = axes_.xmax - axes_.xmin;
            const double yrange = axes_.ymax - axes_.ymin;
            const double span = std::max(xrange, yrange);
            const double xmid = 0.5 * (axes_.xmin + axes_.xmax);
            const double ymid = 0.5 * (axes_.ymin + axes_.ymax);
            axes_.xmin = xmid - span / 2; axes_.xmax = xmid + span / 2;
            axes_.ymin = ymid - span / 2; axes_.ymax = ymid + span / 2;
        }
        fix_ranges(axes_);

        /* 3) ticks ------------------------------------------------------------- */
        const TickInfo xt = make_ticks(axes_.xmin, axes_.xmax);
        const TickInfo yt = make_ticks(axes_.ymin, axes_.ymax);

        /* 4) clear canvas ------------------------------------------------------ */
        canvas_.setTo(cv::Scalar(255, 255, 255));

        /* 5) grid & axes ------------------------------------------------------- */
        draw_grid(xt, yt);
        draw_axes(xt, yt);

        /* 6) retained commands ------------------------------------------------- */
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
                        data_to_pixel(X[i], Y[i]), cvcol,
                        static_cast<int>(cmd.line.thickness), cv::LINE_AA);
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
                cv::putText(canvas_, cmd.txt.text, p, cv::FONT_HERSHEY_SIMPLEX,
                    cmd.txt.font_scale, cvcol, cmd.txt.thickness, cv::LINE_AA);
                break;
            }
            /* --- shape commands (Circle / Rect / RotRect / Poly / Ellipse) --- */
            case CmdType::Circle:
            {
                const auto& d = cmd.circle;
                const cv::Point center = data_to_pixel(d.cx, d.cy);
                const int radius_px = static_cast<int>(d.radius * plot_width() / (axes_.xmax - axes_.xmin));

                if (d.style.fill_alpha > 0.0f && d.style.fill_alpha < 1.0f)
                {
                    cv::Mat tmp = canvas_.clone();
                    cv::circle(tmp, center, radius_px, cv_color(d.style.fill_color), cv::FILLED, cv::LINE_AA);
                    blend_shape(tmp, d.style.fill_alpha);
                }
                else if (d.style.fill_alpha >= 1.0f)
                {
                    cv::circle(canvas_, center, radius_px, cv_color(d.style.fill_color), cv::FILLED, cv::LINE_AA);
                }
                if (d.style.thickness > 0.0f)
                {
                    cv::circle(canvas_, center, radius_px, cv_color(d.style.line_color),
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
                const cv::Rect  r(p0, p1);

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
                    cv::Size2f(static_cast<float>(d.width * plot_width() / (axes_.xmax - axes_.xmin)),
                        static_cast<float>(d.height * plot_height() / (axes_.ymax - axes_.ymin))),
                    static_cast<float>(-d.angle_deg));
                cv::Point2f verts[4]; r.points(verts);
                std::vector<cv::Point> pts(4);
                for (int i = 0; i < 4; ++i) pts[i] = verts[i];

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
                    cv::fillPoly(tmp, std::vector<std::vector<cv::Point>>{ pts }, cv_color(d.style.fill_color), cv::LINE_AA);
                    blend_shape(tmp, d.style.fill_alpha);
                }
                else if (d.style.fill_alpha >= 1.0f)
                {
                    cv::fillPoly(canvas_, std::vector<std::vector<cv::Point>>{ pts }, cv_color(d.style.fill_color), cv::LINE_AA);
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
                const cv::Size axes(static_cast<int>(0.5 * d.width * plot_width() / (axes_.xmax - axes_.xmin)),
                    static_cast<int>(0.5 * d.height * plot_height() / (axes_.ymax - axes_.ymin)));

                if (d.style.fill_alpha > 0.0f && d.style.fill_alpha < 1.0f)
                {
                    cv::Mat tmp = canvas_.clone();
                    cv::ellipse(tmp, center, axes, -d.angle_deg, 0, 360, cv_color(d.style.fill_color), cv::FILLED, cv::LINE_AA);
                    blend_shape(tmp, d.style.fill_alpha);
                }
                else if (d.style.fill_alpha >= 1.0f)
                {
                    cv::ellipse(canvas_, center, axes, -d.angle_deg, 0, 360, cv_color(d.style.fill_color), cv::FILLED, cv::LINE_AA);
                }
                if (d.style.thickness > 0.0f)
                {
                    cv::ellipse(canvas_, center, axes, -d.angle_deg, 0, 360, cv_color(d.style.line_color),
                        static_cast<int>(d.style.thickness), cv::LINE_AA);
                }
                break;
            }
            }
        }

        /* 7) legend ----------------------------------------------------------- */
        if (legend_on_)
        {
            std::vector<const PlotCommand*> items;
            for (const auto& c : cmds_) if (!c.label.empty()) items.push_back(&c);
            if (!items.empty())
            {
                int maxTextW = 0, textH = 0, bl = 0;
                for (auto* pc : items)
                {
                    auto sz = cv::getTextSize(pc->label, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &bl);
                    maxTextW = std::max(maxTextW, sz.width);
                    textH = std::max(textH, sz.height + bl);
                }
                const int sw = 20;
                const int lh = textH + 6;
                const int boxW = sw + 8 + maxTextW + 10;
                const int boxH = lh * static_cast<int>(items.size()) + 10;

                cv::Point anchor = legend_anchor(boxW, boxH);

                cv::rectangle(canvas_, anchor, { anchor.x + boxW, anchor.y + boxH }, cv::Scalar(255, 255, 255), cv::FILLED, cv::LINE_AA);
                cv::rectangle(canvas_, anchor, { anchor.x + boxW, anchor.y + boxH }, cv::Scalar(0, 0, 0), 1);

                for (size_t i = 0; i < items.size(); ++i)
                {
                    int y = anchor.y + 5 + static_cast<int>(i) * lh + lh / 2;
                    const PlotCommand* pc = items[i];
                    cv::Scalar col(pc->color.b, pc->color.g, pc->color.r);
                    switch (pc->type)
                    {
                    case CmdType::Line:
                        cv::line(canvas_, { anchor.x + 5, y }, { anchor.x + 5 + sw, y }, col, 2, cv::LINE_AA);
                        break;
                    case CmdType::Scatter:
                    case CmdType::Circle:
                        cv::circle(canvas_, { anchor.x + 5 + sw / 2, y }, 4, col, cv::FILLED, cv::LINE_AA);
                        break;
                    default:
                        cv::rectangle(canvas_, { anchor.x + 5, y - 4 }, { anchor.x + 5 + sw, y + 4 }, col, cv::FILLED, cv::LINE_AA);
                    }
                    cv::putText(canvas_, pc->label, { anchor.x + 5 + sw + 8, y + 4 }, cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
                }
            }
        }

        /* 8) title & labels --------------------------------------------------- */
        if (!title_.empty())
        {
            cv::putText(canvas_, title_, { kMargin, kMargin / 2 }, cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
        }
        if (!xlabel_.empty())
        {
            cv::putText(canvas_, xlabel_, { width_ / 2 - 40, height_ - 10 }, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
        }
        draw_ylabel();

        dirty_ = false;
    }

    void Figure::show(const std::string& window_name)
    {
        if (dirty_) render();
        cv::imshow(window_name, canvas_);
        cv::waitKey(1);
    }

    void Figure::save(const std::string& filename)
    {
        if (dirty_) render();
        cv::imwrite(filename, canvas_);
    }




    /* --------------------------------------------------------------------------
     *  Private helpers
     * ------------------------------------------------------------------------*/
    cv::Point2i Figure::data_to_pixel(double x, double y) const
    {
        const double xf = (x - axes_.xmin) / (axes_.xmax - axes_.xmin);
        const double yf = (y - axes_.ymin) / (axes_.ymax - axes_.ymin);
        int px = kMarginLeft + static_cast<int>(xf * plot_width() + 0.5);
        int py = height_ - kMarginBottom - static_cast<int>(yf * plot_height() + 0.5);
        return { px, py };
    }

    cv::Scalar Figure::cv_color(const Color& c, float alpha)
    {
        return cv::Scalar(c.b, c.g, c.r, static_cast<uchar>(alpha * 255));
    }

    void Figure::blend_shape(const cv::Mat& shape, float alpha)
    {
        cv::addWeighted(shape, alpha, canvas_, 1.0 - alpha, 0.0, canvas_);
    }

    /* --------------------------------------------------------------------------
     *  Tick helpers
     * ------------------------------------------------------------------------*/
    double Figure::nice_num(double range, bool round)
    {
        if (range <= 0) range = 1.0;  /* numerical robustness */
        const double expv = std::floor(std::log10(range));
        const double f = range / std::pow(10.0, expv);   /* 1-10 */

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

    TickInfo Figure::make_ticks(double lo, double hi, int target)
    {
        const double range = nice_num(hi - lo, false);
        const double step = nice_num(range / (target - 1), true);
        const double graph_lo = std::floor(lo / step) * step;
        const double graph_hi = std::ceil(hi / step) * step;

        TickInfo out;
        for (double v = graph_lo; v <= graph_hi + 0.5 * step; v += step)
        {
            if (v < lo - 1e-12 || v > hi + 1e-12) continue;
            out.locs.push_back(v);
            std::ostringstream ss;
            ss << std::fixed << std::setprecision((step < 1) ? 1 : 0) << v;
            out.labels.push_back(ss.str());
        }
        return out;
    }

    void Figure::draw_axes(const TickInfo& xt, const TickInfo& yt)
    {
        const cv::Scalar black(0, 0, 0);
        const int font = cv::FONT_HERSHEY_SIMPLEX;

        cv::line(canvas_, { kMarginLeft, height_ - kMarginBottom }, { width_ - kMarginRight, height_ - kMarginBottom }, black, 1);
        for (size_t i = 0; i < xt.locs.size(); ++i)
        {
            cv::Point2i p = data_to_pixel(xt.locs[i], axes_.ymin);
            cv::line(canvas_, { p.x, p.y }, { p.x, p.y + kTickLen }, black, 1);
            cv::putText(canvas_, xt.labels[i], { p.x - 10, p.y + 18 }, font, 0.4, black, 1, cv::LINE_AA);
        }

        cv::line(canvas_, { kMarginLeft, kMarginTop }, { kMarginLeft, height_ - kMarginBottom }, black, 1);
        for (size_t i = 0; i < yt.locs.size(); ++i)
        {
            cv::Point2i p = data_to_pixel(axes_.xmin, yt.locs[i]);
            cv::line(canvas_, { p.x - kTickLen, p.y }, { p.x, p.y }, black, 1);
            cv::putText(canvas_, yt.labels[i], { p.x - 30, p.y + 4 }, font, 0.4, black, 1, cv::LINE_AA);
        }
    }

    void Figure::draw_grid(const TickInfo& xt, const TickInfo& yt)
    {
        if (!axes_.grid) return;
        const cv::Scalar light(220, 220, 220);
        for (double xv : xt.locs)
        {
            if (xv < axes_.xmin || xv > axes_.xmax) continue;
            cv::line(canvas_, data_to_pixel(xv, axes_.ymin), data_to_pixel(xv, axes_.ymax), light, 1, cv::LINE_AA);
        }
        for (double yv : yt.locs)
        {
            if (yv < axes_.ymin || yv > axes_.ymax) continue;
            cv::line(canvas_, data_to_pixel(axes_.xmin, yv), data_to_pixel(axes_.xmax, yv), light, 1, cv::LINE_AA);
        }
    }

    void Figure::draw_ylabel()
    {
        if (ylabel_.empty()) return;
        if (!ylabel_cache_valid_)
        {
            int baseline = 0;
            auto sz = cv::getTextSize(ylabel_, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseline);
            cv::Mat txt(sz.height + baseline, sz.width, CV_8UC3, cv::Scalar(255, 255, 255));
            cv::putText(txt, ylabel_, { 0, sz.height }, cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
            cv::rotate(txt, ylabel_cache_, cv::ROTATE_90_COUNTERCLOCKWISE);
            ylabel_cache_valid_ = true;
        }
        const int x = kMarginLeft - 55;
        const int y = kMarginTop + (plot_height() - ylabel_cache_.rows) / 2;
        if (x >= 0 && y >= 0 && x + ylabel_cache_.cols <= canvas_.cols && y + ylabel_cache_.rows <= canvas_.rows)
        {
            ylabel_cache_.copyTo(canvas_(cv::Rect(x, y, ylabel_cache_.cols, ylabel_cache_.rows)));
        }
    }

    cv::Point2i Figure::anchored_text_pos(const TextData& td) const
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

    void Figure::ensure_nonzero_span(double& lo, double& hi)
    {
        if (lo == hi)
        {
            const double eps = (std::abs(lo) > 1e-12) ? std::abs(lo) * 1e-3 : 1e-3;
            lo -= eps; hi += eps;
        }
    }
    void Figure::fix_ranges(Axes& a)
    {
        ensure_nonzero_span(a.xmin, a.xmax);
        ensure_nonzero_span(a.ymin, a.ymax);
    }

    void Figure::expand_bounds(const std::vector<double>& xs, const std::vector<double>& ys)
    {
        for (size_t i = 0; i < xs.size(); ++i) data_bounds_.expand(xs[i], ys[i]);
    }

    cv::Point Figure::legend_anchor(int boxW, int boxH) const
    {
        const int left = kMarginLeft;
        const int right = width_ - kMarginRight - boxW;
        const int top = kMarginTop;
        const int bottom = height_ - kMarginBottom - boxH;
        const int hmid = left + (plot_width() - boxW) / 2;
        const int vmid = top + (plot_height() - boxH) / 2;

        if (legend_loc_ == "northWest")      return { left,  top };
        else if (legend_loc_ == "north")      return { hmid,  top };
        else if (legend_loc_ == "northEast")  return { right, top };
        else if (legend_loc_ == "west")       return { left,  vmid };
        else if (legend_loc_ == "center")     return { hmid,  vmid };
        else if (legend_loc_ == "east")       return { right, vmid };
        else if (legend_loc_ == "southWest")  return { left,  bottom };
        else if (legend_loc_ == "south")      return { hmid,  bottom };
        else /* southEast or unknown */        return { right, bottom };
    }

} // namespace mpocv
