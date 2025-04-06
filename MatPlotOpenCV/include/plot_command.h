#pragma once
#include <string>
#include <vector>
#include "color.h"

namespace mpocv
{

    /**
     * @enum CmdType
     * @brief Type?safe discriminator for PlotCommand variants.
     */
    enum class CmdType
    {
        Line, 
        Scatter, 
        Text,
        Circle, 
        RectLTRB, 
        RectXYWH, 
        RotatedRect,
        Polygon, 
        Ellipse
    };


    struct LineData
    {
        std::vector<double> x;
        std::vector<double> y;
        float thickness{ 1.f };
    };

    struct ScatterData
    {
        std::vector<double> x;
        std::vector<double> y;
        float marker_size{ 4.f };
    };

    struct TextData
    {
        double x{ 0 }, y{ 0 };
        std::string text;
        double font_scale{ 0.4 };
        int thickness{ 1 };
        enum class HAlign { Left, Center, Right } halign{ HAlign::Left };
        enum class VAlign { Baseline, Center, Top, Bottom } valign{ VAlign::Baseline };
    };

    struct ShapeStyle
    {
        Color line_color{ Color::Black() };
        float thickness{ 1.0f };
        Color fill_color{ 255, 255, 255 }; // white by default
        float fill_alpha{ 1.0f };          // NOT SUPORTED YET (0 = transparent, 1 = opaque)
    };

    struct CircleData
    {
        double cx{ 0 }, cy{ 0 }, radius{ 1 };
        ShapeStyle style;
    };

    struct RectData
    {
        double x0, y0, x1, y1;  // x0 < x1, y0 < y1
        ShapeStyle style;
    };

    struct RotatedRectData
    {
        double cx, cy;
        double width, height;
        double angle_deg;
        ShapeStyle style;
    };

    struct PolygonData
    {
        std::vector<double> x;
        std::vector<double> y;
        ShapeStyle style;
    };

    struct EllipseData
    {
        double cx, cy;
        double width, height;
        double angle_deg;
        ShapeStyle style;
    };

    struct PlotCommand
    {
        CmdType type{ CmdType::Line };
        Color   color{ Color::Blue() };

        LineData        line;
        ScatterData     scatter;
        TextData        txt;
        CircleData      circle;
        RectData        rect;
        RotatedRectData rot_rect;
        PolygonData     polygon;
        EllipseData     ellipse;
    };

} // namespace mpocv
