#pragma once

#include <array>
#include <cassert>
#include <cmath>

constexpr auto zero_delta = 1e-5;

using Vec2 = std::array<double, 2>;

// dot product
auto
dot(const Vec2 &vec1, const Vec2 &vec2) -> double
{
    return (vec1[0] * vec2[0]) + (vec1[1] * vec2[1]);
}

// performs vec1 - vec2 element-wise
auto
operator-(const Vec2 &vec1, const Vec2 &vec2) -> Vec2
{
    const auto &[x1, y1] = vec1;
    const auto &[x2, y2] = vec2;

    return { x1 - x2, y1 - y2 };
}

auto
operator+=(Vec2 &vec1, const Vec2 &vec2) -> void
{
    auto &[x1, y1] = vec1;
    const auto &[x2, y2] = vec2;
    x1 += x2;
    y1 += y2;
}

// multiply by scalar
auto
operator*(const Vec2 &vec, double scalar) -> Vec2
{
    const auto &[x, y] = vec;
    return { x * scalar, y * scalar };
}

auto
normalize(const Vec2 &vec) -> Vec2
{
    const auto norm = std::sqrt(dot(vec, vec));
    assert(std::fabs(norm) > zero_delta
           and "attempting to normalize a vector that is close to zero");

    return { vec[0] / norm, vec[1] / norm };
}
