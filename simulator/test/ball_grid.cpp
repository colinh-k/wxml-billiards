#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "ball_grid.hpp"
#include "vec2.hpp"

#include <algorithm>
#include <initializer_list>
#include <numbers>
#include <ranges>
#include <stdexcept>
#include <unordered_set>

// used for floating-point comparisons
constexpr auto epsilon = 1e-7;

SCENARIO("BallGrid operator[]")
{
    constexpr auto stationary_ball = Ball{ { 0.0, 0.0 } };

    GIVEN("an empty grid")
    {
        constexpr auto nrows = 3UZ;
        constexpr auto ncols = 3UZ;
        auto grid = BallGrid<nrows, ncols>{};

        THEN("size is nrows by ncols")
        {
            REQUIRE(grid.size() == nrows * ncols);
        }

        THEN("returns stationary ball for flat index argument")
        {
            for (auto index : std::views::iota(0UZ, grid.size()))
            {
                REQUIRE(grid[index] == stationary_ball);
            }
        }

        THEN("returns stationary ball for row/col index arguments")
        {
            for (auto row : std::views::iota(0UZ, nrows))
            {
                for (auto col : std::views::iota(0UZ, ncols))
                {
                    REQUIRE(grid[row, col] == stationary_ball);
                }
            }
        }

        WHEN("a ball's velocity is updated with flat index")
        {
            constexpr auto new_ball = Ball{ { 12.0, 34.0 } };
            grid[3] = new_ball;

            THEN("the ball is updated")
            {
                REQUIRE(grid[3] == new_ball);
            }
        }

        WHEN("a ball's velocity is updated with row/col index")
        {
            constexpr auto new_ball = Ball{ { 12.0, 34.0 } };
            grid[1, 0] = new_ball;

            THEN("the ball is updated")
            {
                REQUIRE(grid[1, 0] == new_ball);
            }
        }

        WHEN("every ball is written to with flat index")
        {
            for (auto index : std::views::iota(0UZ, grid.size()))
            {
                const auto new_ball
                    = Ball{ { static_cast<double>(index),
                              static_cast<double>(index + 1) } };
                grid[index] = new_ball;
            }

            THEN("we can read back the new value")
            {
                for (auto index : std::views::iota(0UZ, grid.size()))
                {
                    const auto expected_ball
                        = Ball{ { static_cast<double>(index),
                                  static_cast<double>(index + 1) } };
                    REQUIRE(grid[index] == expected_ball);
                }
            }
        }

        WHEN("every ball is written to with row/col index")
        {
            for (auto row : std::views::iota(0UZ, nrows))
            {
                for (auto col : std::views::iota(0UZ, ncols))
                {
                    const auto new_ball = Ball{ { static_cast<double>(row),
                                                  static_cast<double>(col) } };
                    grid[row, col] = new_ball;
                }
            }

            THEN("we can read back the new value")
            {
                for (auto row : std::views::iota(0UZ, nrows))
                {
                    for (auto col : std::views::iota(0UZ, ncols))
                    {
                        const auto expected_ball
                            = Ball{ { static_cast<double>(row),
                                      static_cast<double>(col) } };
                        REQUIRE(grid[row, col] == expected_ball);
                    }
                }
            }
        }
    }
}

// i dont feel like implementing operator== for BoundedSmallSet in general, so
// we can compare the expected sets using this fn
constexpr auto
operator==(const NeighborSet &set, const auto &values)
{
    return set.size() == values.size()
           and std::ranges::all_of(values, [&](const auto &value)
                                   { return set.contains(value); });
};

SCENARIO("BallGrid get_neighbors")
{
    GIVEN("an empty grid")
    {
        constexpr auto nrows = 4UZ;
        constexpr auto ncols = 5UZ;
        auto grid = BallGrid<nrows, ncols>{};

        THEN("throws for invalid index")
        {
            REQUIRE_THROWS_AS(grid.get_neighbors(grid.size()),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(grid.get_neighbors(grid.size() + 1),
                              std::invalid_argument);
        }

        // NOTE: i worked out the neighbor sets of a 4x5 grid by hand, but only
        // wrote down a few test cases here since its tedious to write them all
        // out, but i prolly should...
        THEN("returns correct neighbors")
        {
            REQUIRE(grid.get_neighbors(0)
                    == std::initializer_list<std::size_t>{ 1, 5 });
            REQUIRE(grid.get_neighbors(1)
                    == std::initializer_list<std::size_t>{ 5, 6, 2, 0 });
            REQUIRE(grid.get_neighbors(2)
                    == std::initializer_list<std::size_t>{ 6, 7, 1, 3 });
            REQUIRE(
                grid.get_neighbors(6)
                == std::initializer_list<std::size_t>{ 11, 12, 5, 7, 1, 2 });
            REQUIRE(grid.get_neighbors(9)
                    == std::initializer_list<std::size_t>{ 14, 8, 4 });
            REQUIRE(
                grid.get_neighbors(12)
                == std::initializer_list<std::size_t>{ 16, 17, 11, 13, 6, 7 });
            REQUIRE(
                grid.get_neighbors(13)
                == std::initializer_list<std::size_t>{ 17, 18, 12, 14, 7, 8 });
            REQUIRE(grid.get_neighbors(15)
                    == std::initializer_list<std::size_t>{ 16, 10, 11 });
        }
    }
}

SCENARIO("BallGrid displacement()")
{
    GIVEN("an empty grid")
    {
        constexpr auto nrows = 4UZ;
        constexpr auto ncols = 5UZ;
        auto grid = BallGrid<nrows, ncols>{};

        THEN("throws for invalid indices")
        {
            REQUIRE_THROWS_AS(grid.displacement(grid.size(), 0),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(grid.displacement(0, grid.size()),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(grid.displacement(grid.size(), grid.size()),
                              std::invalid_argument);
        }

        THEN("returns zero vector if both indices are the same")
        {
            for (auto index : std::views::iota(0UZ, grid.size()))
            {
                const auto [x, y] = grid.displacement(index, index);
                REQUIRE_THAT(x, Catch::Matchers::WithinAbs(0.0, epsilon));
                REQUIRE_THAT(y, Catch::Matchers::WithinAbs(0.0, epsilon));
            }
        }

        THEN("correct for hand-verified values [ASSUMES BALL RADIUS == 1]")
        {
            constexpr auto input_and_expected_values = std::to_array<
                std::pair<std::pair<std::size_t, std::size_t>, Vec2>>(
                { { { 6, 7 }, { -2.0, 0.0 } },
                  { { 15, 10 }, { 1.0, std::numbers::sqrt3 } },
                  { { 14, 19 }, { -1.0, -std::numbers::sqrt3 } } });

            for (auto [input_pair, expected] : input_and_expected_values)
            {
                const auto [expected_x, expected_y] = expected;

                const auto [idx1, idx2] = input_pair;
                const auto [actual_x, actual_y]
                    = grid.displacement(idx1, idx2);

                REQUIRE_THAT(actual_x,
                             Catch::Matchers::WithinAbs(expected_x, epsilon));
                REQUIRE_THAT(actual_y,
                             Catch::Matchers::WithinAbs(expected_y, epsilon));
            }
        }
    }
}

SCENARIO("BallGrid is_neighbor_pair")
{
    GIVEN("an empty grid")
    {
        constexpr auto nrows = 4UZ;
        constexpr auto ncols = 5UZ;
        auto grid = BallGrid<nrows, ncols>{};

        THEN("throws for invalid indices")
        {
            REQUIRE_THROWS_AS(grid.is_neighbor_pair(grid.size(), 0),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(grid.is_neighbor_pair(0, grid.size()),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(grid.is_neighbor_pair(grid.size(), grid.size()),
                              std::invalid_argument);
        }

        THEN("returns false if both indices are the same")
        {
            for (auto index : std::views::iota(0UZ, grid.size()))
            {
                REQUIRE(not grid.is_neighbor_pair(index, index));
            }
        }

        THEN("returns correct value for a few hand-verified pairs")
        {
            constexpr auto input_and_expected_values = std::to_array<
                std::pair<std::pair<std::size_t, std::size_t>, bool>>(
                { { { 0, 1 }, true },
                  { { 6, 2 }, true },
                  { { 9, 13 }, false },
                  { { 19, 17 }, false } });

            for (auto [input_pair, expected_value] : input_and_expected_values)
            {
                const auto [idx1, idx2] = input_pair;
                REQUIRE(grid.is_neighbor_pair(idx1, idx2) == expected_value);
                // should be the same regardless of input order
                REQUIRE(grid.is_neighbor_pair(idx2, idx1) == expected_value);
            }
        }
    }
}

SCENARIO("BallGrid is_collision_pair")
{
    GIVEN("an empty grid")
    {
        constexpr auto nrows = 4UZ;
        constexpr auto ncols = 5UZ;
        auto grid = BallGrid<nrows, ncols>{};

        THEN("throws for invalid indices")
        {
            REQUIRE_THROWS_AS(grid.is_collision_pair(grid.size(), 0),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(grid.is_collision_pair(0, grid.size()),
                              std::invalid_argument);
            REQUIRE_THROWS_AS(grid.is_collision_pair(grid.size(), grid.size()),
                              std::invalid_argument);
        }

        WHEN("a single velocity is assigned")
        {
            grid[6].velocity = { 1.0, 0.0 };

            THEN("returns correct value for neighbors")
            {
                REQUIRE(grid.is_collision_pair(6, 7));
                REQUIRE(grid.is_collision_pair(6, 12));
                REQUIRE(grid.is_collision_pair(6, 2));

                REQUIRE(not grid.is_collision_pair(6, 5));
                REQUIRE(not grid.is_collision_pair(6, 1));
                REQUIRE(not grid.is_collision_pair(6, 11));
            }

            THEN("returns false if both indices are the same")
            {
                for (auto index : std::views::iota(0UZ, grid.size()))
                {
                    REQUIRE(not grid.is_collision_pair(index, index));
                }
            }
        }

        WHEN("neighboring balls have the same velocity vector")
        {
            grid[13].velocity = { -1.0, +1.0 };
            grid[17].velocity = { -1.0, +1.0 };

            THEN("no collision possible")
            {
                REQUIRE(not grid.is_collision_pair(13, 17));
                REQUIRE(not grid.is_collision_pair(17, 13));
            }
        }

        // idrk how to describe this...
        WHEN("neighboring balls have velocity in same direction but "
             "peripheral has bigger magnitude")
        {
            grid[4].velocity = { 2.0, -2.0 };
            grid[8].velocity = { 1.0, -1.0 };

            THEN("no collision possible")
            {
                REQUIRE(not grid.is_collision_pair(4, 8));
                REQUIRE(not grid.is_collision_pair(8, 4));
            }
        }

        WHEN("non neighboring balls have collideable velocities")
        {
            grid[5].velocity = { 1.0, 1.0 };
            grid[12].velocity = { -1.0, -1.0 };

            THEN("no collision possible")
            {
                REQUIRE(not grid.is_collision_pair(5, 12));
                REQUIRE(not grid.is_collision_pair(12, 5));
            }
        }
    }
}
