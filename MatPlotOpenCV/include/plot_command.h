#pragma once
#include <string>
#include <vector>
#include "color.h"

namespace mpocv
{

    /**
     * @enum CmdType
     * @brief Type-safe discriminator for PlotCommand variants.
     */
    enum class CmdType
    {
        Line,         ///< Polyline drawing
        Scatter,      ///< Unconnected point markers
        Text,         ///< Text annotation
        Circle,       ///< Filled or outlined circle
        RectLTRB,     ///< Rectangle via (x0, y0) to (x1, y1)
        RectXYWH,     ///< Rectangle via [x, y, width, height]
        RotatedRect,  ///< Rotated rectangle
        Polygon,      ///< Arbitrary polygon
        Ellipse       ///< Filled or outlined ellipse
    };

    /**
     * @struct LineData
     * @brief Data for a connected line plot (polyline).
     */
    struct LineData
    {
        std::vector<double> x;   ///< X-coordinates
        std::vector<double> y;   ///< Y-coordinates
        float thickness{ 1.f };  ///< Line thickness in pixels
    };

    /**
     * @struct ScatterData
     * @brief Data for unconnected circular markers.
     */
    struct ScatterData
    {
        std::vector<double> x;      ///< X-coordinates
        std::vector<double> y;      ///< Y-coordinates
        float marker_size{ 4.f };   ///< Marker radius in pixels
    };

    /**
     * @struct TextData
     * @brief Data for a single text annotation.
     */
    struct TextData
    {
        double x{ 0 }, y{ 0 };                  ///< Anchor point in data coordinates
        std::string text;                      ///< Text string to display
        double font_scale{ 0.4 };              ///< Font scale (OpenCV scalar)
        int thickness{ 1 };                    ///< Stroke thickness
        enum class HAlign { Left, Center, Right } halign{ HAlign::Left }; ///< Horizontal alignment
        enum class VAlign { Baseline, Center, Top, Bottom } valign{ VAlign::Baseline }; ///< Vertical alignment
    };

    /**
     * @struct ShapeStyle
     * @brief Common styling info for filled shapes.
     */
    struct ShapeStyle
    {
        Color line_color{ Color::Black() };  ///< Stroke color
        float thickness{ 1.0f };             ///< Stroke thickness in pixels
        Color fill_color{ 255, 255, 255 };   ///< Fill color (RGB)
        float fill_alpha{ 1.0f };            ///< Fill alpha (0 = transparent, 1 = opaque)
    };

    /**
     * @struct CircleData
     * @brief Circle drawing command.
     */
    struct CircleData
    {
        double cx{ 0 }, cy{ 0 };  ///< Center in data coordinates
        double radius{ 1 };       ///< Radius in data units
        ShapeStyle style;         ///< Fill and stroke settings
    };

    /**
     * @struct RectData
     * @brief Rectangle using absolute corners.
     */
    struct RectData
    {
        double x0, y0, x1, y1;  ///< Two opposite corners (data units)
        ShapeStyle style;       ///< Fill and stroke settings
    };

    /**
     * @struct RotatedRectData
     * @brief Rotated rectangle.
     */
    struct RotatedRectData
    {
        double cx, cy;           ///< Center of rectangle
        double width, height;    ///< Side lengths (unrotated)
        double angle_deg;        ///< Counter-clockwise rotation angle
        ShapeStyle style;        ///< Fill and stroke settings
    };

    /**
     * @struct PolygonData
     * @brief Arbitrary filled or stroked polygon.
     */
    struct PolygonData
    {
        std::vector<double> x;   ///< X-coordinates of polygon vertices
        std::vector<double> y;   ///< Y-coordinates of polygon vertices
        ShapeStyle style;        ///< Fill and stroke settings
    };

    /**
     * @struct EllipseData
     * @brief Ellipse centered at (cx, cy).
     */
    struct EllipseData
    {
        double cx, cy;           ///< Center
        double width, height;    ///< Full width and height (diameters)
        double angle_deg;        ///< Rotation angle in degrees
        ShapeStyle style;        ///< Fill and stroke settings
    };

    /**
     * @struct PlotCommand
     * @brief Union-like container for a single drawing command.
     *
     * The actual command is specified by `type`, and the corresponding
     * field (line, scatter, etc.) is used based on that.
     */
    struct PlotCommand
    {
        CmdType type{ CmdType::Line };  ///< Active drawing type
        Color   color{ Color::Blue() }; ///< Optional fallback / stroke color
        std::string label;              ///< For legend

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
