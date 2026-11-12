#pragma once

#include <mdspan>
#include <numbers>
#include <ranges>
#include <stdexcept>
#include <unordered_set>

#include "bounded_small_set.hpp"
#include "vec2.hpp"

// balls can have at most 6 neighbors
using NeighborSet = BoundedSmallSet<std::size_t, 6>;

struct Ball
{
    Vec2 velocity;

    auto operator==(const Ball &) const -> bool = default;
};

struct GridCoordinate
{
    std::size_t row;
    std::size_t col;
};

// represents a 2d grid of packed balls in the plane. the number of balls in
// each row and column of the grid is defined by the template arguments
//
// TODO: im starting to realize this class doesnt rlly operate on the balls; it
// just computes useful values based on indices, like neighbor sets. is it
// necessary to store the balls in this class then ? logically it kinda makes
// sense so this class can control access to the balls and answer queries about
// neighbors and collisions
template <std::size_t NRows, std::size_t NCols> class BallGrid
{
  public:
    auto &&
    operator[](this auto &&self, std::size_t index)
    {
        return self.m_balls.at(index);
    }

    auto &&
    operator[](this auto &&self, std::size_t row, std::size_t col)
    {
        // TODO: i dont think mdspan::operator[] performs bounds checking, so
        // do that here
        return GridView{ self.m_balls.data() }[row, col];
    }

    [[nodiscard]]
    constexpr static auto
    size() -> std::size_t
    {
        return nballs;
    }

    // computes displacement vector from ball at index2 to ball at index1
    //
    // TODO: positions can be computed at compile-time, so displacement should
    // also be compile-time known. i dont think its necessary to pre compute
    // all displacements between every pair of balls tho... its probably a
    // better trade-off to do that at runtime
    auto
    displacement(std::size_t index1, std::size_t index2) const -> Vec2
    {
        if (index1 >= size() or index2 >= size())
        {
            throw std::invalid_argument{
                "at least one requested index is too large for this grid"
            };
        }

        const auto &position1 = ball_id_to_position[index1];
        const auto &position2 = ball_id_to_position[index2];
        return position1 - position2;
    }

    // a ball in halfplane can have up to 6 neighbors
    // TODO: we should prolly cache this computation in a table so we dont have
    // to iterate every time we need to look up the neighbors of a ball
    auto
    get_neighbors(std::size_t index) const -> NeighborSet
    {
        if (index >= size())
        {
            throw std::invalid_argument{
                "requested index is too large for this grid"
            };
        }

        return ball_id_to_neighbor_set[index];
    }

    auto
    is_neighbor_pair(std::size_t index1, std::size_t index2) const -> bool
    {
        if (index1 >= size() or index2 >= size())
        {
            throw std::invalid_argument{
                "at least one index was too large for this grid"
            };
        }

        const auto &neighbors_of_1 = get_neighbors(index1);
        return neighbors_of_1.contains(index2);
    }

    [[nodiscard]]
    auto
    is_collision_pair(std::size_t index1, std::size_t index2) const -> bool
    {
        if (index1 >= size() or index2 >= size())
        {
            throw std::invalid_argument{
                "at least one index was too large for this grid"
            };
        }

        // a ball cannot collide with itself. we normalize the displacement
        // vector below, so we need to make sure it is nonzero
        if (index1 == index2)
        {
            return false;
        }

        const auto &ball1 = operator[](index1);
        const auto &ball2 = operator[](index2);

        // TODO: we should check displacement is nonzero before normalizing
        const auto displacement
            = normalize(this->displacement(index1, index2));

        const auto component_in_direction
            = dot(displacement, ball1.velocity - ball2.velocity);

        const auto are_facing
            = (std::fabs(component_in_direction) > zero_delta)
              and (component_in_direction < 0);

        return is_neighbor_pair(index1, index2) and are_facing;
    }

    // TODO: add tests for these since i just moved them to the public
    // interface
    //
    // returns (row, col) coordinate pair
    constexpr auto
    index_to_grid_coordinate(std::size_t index) const -> GridCoordinate
    {
        return { index / NCols, index % NCols };
    }

    static constexpr auto
    grid_coordinate_to_index(GridCoordinate coordindate) -> std::size_t
    {
        return coordindate.row * NCols + coordindate.col;
    }

  private:
    // NOTE: we can specify extents of mdspan at compile-time
    using GridView
        = std::mdspan<Ball, std::extents<std::size_t, NRows, NCols>>;

    static constexpr auto ball_radius = 1.0;
    static constexpr auto nballs = NRows * NCols;

    std::array<Ball, nballs> m_balls;

    // converts a 2d index coordinate pair into m_balls, ie referring to a
    // particular ball, to an x-y position in 2d space. assumes balls are
    // arranged with the center of ball index 0 at the origin, and others are
    // packed tightly in the plane according to the number of rows and cols
    //
    // to understand what is happening here, it would be good to draw a
    // picture of a collection of packed balls in the plane
    consteval static auto
    index_to_position(std::size_t row, std::size_t col) -> Vec2
    {
        constexpr auto dx = ball_radius * 2.0;
        // a^2 + b^2 = c^2 -> b^2 = c^2 - a^2 -> b = sqrt(c^2 - a^2)
        static_assert(ball_radius == 1.0,
                      "computing dy below assumes ball radius equal to 1");
        // NOTE: this expression should be used if ball_radius != 1, but
        // std::sqrt() is not constexpr, so we couldnt compute the ball
        // positions as a lookup table with this formula:
        //
        // const auto dy = std::sqrt(((2 * ball_radius) * (2 * ball_radius))
        // - (ball_radius * ball_radius));
        constexpr auto dy = std::numbers::sqrt3;

        // for odd rows, the row starts shifted toward positive-x direction by
        // 1 ball radius
        const auto x = (row % 2 == 0)
                           ? static_cast<double>(col) * dx
                           : (static_cast<double>(col) * dx) + ball_radius;
        const auto y = static_cast<double>(row) * dy;

        return { x, y };
    }

    // a ball in halfplane can have up to 6 neighbors
    // NOTE: this function initializes a lookup table that associates each ball
    // with its set of neighbors, which is known at compile time
    consteval static auto
    compute_neighbors() -> std::array<NeighborSet, nballs>
    {
        // the offsets used to compute neighbor sets depends on if this ball is
        // in an even or odd row. draw it by hand to see this is true. each
        // neighbor lies at this ball's (row,col) coordinate plus each of
        // the (row,col) pairs in this offsets list

        auto result = std::array<NeighborSet, nballs>{};

        constexpr auto even_offsets = std::array<std::pair<int, int>, 6>{
            { { 0, 1 }, { 0, -1 }, { 1, 0 }, { 1, -1 }, { -1, 0 }, { -1, -1 } }
        };
        constexpr auto odd_offsets = std::array<std::pair<int, int>, 6>{
            { { 0, 1 }, { 0, -1 }, { 1, 1 }, { 1, 0 }, { -1, 0 }, { -1, 1 } }
        };

        for (const auto row : std::views::iota(0UZ, NRows))
        {
            for (const auto col : std::views::iota(0UZ, NCols))
            {
                const auto &offsets
                    = (row % 2 == 0) ? even_offsets : odd_offsets;

                const auto index
                    = grid_coordinate_to_index({ .row = row, .col = col });
                for (const auto [dr, dc] : offsets)
                {
                    const auto neighbor_row
                        = static_cast<std::ptrdiff_t>(row) + dr;
                    const auto neighbor_col
                        = static_cast<std::ptrdiff_t>(col) + dc;

                    if (0 <= neighbor_row
                        and neighbor_row < static_cast<std::ptrdiff_t>(NRows)
                        and 0 <= neighbor_col
                        and neighbor_col < static_cast<std::ptrdiff_t>(NCols))
                    {
                        const auto neighbor_index = grid_coordinate_to_index(
                            { static_cast<std::size_t>(neighbor_row),
                              static_cast<std::size_t>(neighbor_col) });
                        result[index].insert(neighbor_index);
                    }
                }
            }
        }

        return result;
    }

    consteval static auto
    compute_positions() -> std::array<Vec2, nballs>
    {
        auto result = std::array<Vec2, nballs>{};

        for (const auto row : std::views::iota(0UZ, NRows))
        {
            for (const auto col : std::views::iota(0UZ, NCols))
            {
                const auto index
                    = grid_coordinate_to_index({ .row = row, .col = col });
                result[index] = index_to_position(row, col);
            }
        }

        return result;
    }

    // a lookup table associating each ball with its set of neighbors, known at
    // compile time
    static constexpr std::array<NeighborSet, nballs> ball_id_to_neighbor_set
        = compute_neighbors();

    // lookup table associating each ball with its position expressed as x-y
    // coordinates
    static constexpr std::array<Vec2, nballs> ball_id_to_position
        = compute_positions();
};
