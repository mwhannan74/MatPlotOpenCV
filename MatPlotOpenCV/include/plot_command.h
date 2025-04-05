#pragma once
#include <string>
#include <vector>
#include "color.h"

namespace mpocv
{

    enum class CmdType { Line, Scatter, Text };

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

    struct PlotCommand
    {
        CmdType type{ CmdType::Line };
        Color   color{ Color::Blue() };

        LineData    line;
        ScatterData scatter;
        TextData    txt;
    };

} // namespace mpocv
