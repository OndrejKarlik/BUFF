#pragma once
#include "Lib/Pixel.h"
#include "Lib/Vector.h"

#define IMGUI_DEFINE_MATH_OPERATORS

#define IM_VEC2_CLASS_EXTRA                                                                                  \
    operator Buff::Vector2() const {                                                                         \
        return {x, y};                                                                                       \
    }                                                                                                        \
    constexpr ImVec2(const Buff::Vector2& in)                                                                \
        : x(in.x)                                                                                            \
        , y(in.y) {}                                                                                         \
    constexpr ImVec2(const Buff::Pixel& in)                                                                  \
        : x(float(in.x))                                                                                     \
        , y(float(in.y)) {}                                                                                  \
    ImVec2& operator=(const Buff::Vector2& in) {                                                             \
        x = in.x;                                                                                            \
        y = in.y;                                                                                            \
        return *this;                                                                                        \
    }                                                                                                        \
    Buff::Vector2 operator+(const Buff::Vector2& in) const {                                                 \
        return {x + in.x, y + in.y};                                                                         \
    }                                                                                                        \
    Buff::Vector2 operator-(const Buff::Vector2& in) const {                                                 \
        return {x - in.x, y - in.y};                                                                         \
    }
