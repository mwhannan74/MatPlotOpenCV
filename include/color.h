#pragma once
#include <cstdint>

namespace mpocv
{

    struct Color
    {
        uint8_t r{ 0 }, g{ 0 }, b{ 0 };

        constexpr Color() = default;
        constexpr Color(uint8_t red, uint8_t green, uint8_t blue)
            : r(red), g(green), b(blue)
        {}

        /* named helpers */
        static constexpr Color Black() { return { 0, 0, 0 }; }
        static constexpr Color White() { return { 255, 255, 255 }; }
        static constexpr Color Red() { return { 255, 0, 0 }; }
        static constexpr Color Green() { return { 0, 255, 0 }; }
        static constexpr Color Blue() { return { 0, 0, 255 }; }
        static constexpr Color Cyan() { return { 0, 255, 255 }; }
        static constexpr Color Magenta() { return { 255, 0, 255 }; }
        static constexpr Color Yellow() { return { 255, 255, 0 }; }
    };

} // namespace mpocv