#pragma once

#include <algorithm>
#include <bit>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <print>
#include <system_error>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>

#include "ball_grid.hpp"
#include "bounded_small_set.hpp"
#include "random_index_set.hpp"

// TODO: find a better place for this
template <typename T>
concept Numeric = std::integral<T> or std::floating_point<T>;

template <std::size_t NRows, std::size_t NCols, typename RNGType>
class Simulator
{
  private:
    using UpdateSet = BoundedSmallSet<std::size_t, 10>;

  public:
    Simulator(BallGrid<NRows, NCols> &&ball_grid, RNGType &&rng)
        // TODO: should i std::move() or std::forward() rng here ?
        : m_ball_grid{ std::move(ball_grid) }, m_collision_set{},
          m_ball_to_collision_partners{}, m_rng{ std::move(rng) }
    {
        // TODO: initiailzie the collision set by iterating over the balls and
        // checking if each ball has a potential collision with each of its
        // neighbors; if it does, then add it to the collision set. and add the
        // collision partner to the set of partners for that ball
        update_balls(std::views::iota(0UZ, m_ball_grid.size()));
    }

    auto
    run(std::uint64_t max_iters)
    {
        for ([[maybe_unused]] const auto _ : std::views::iota(0UZ, max_iters))
        {
            // TODO: if the collision set is empty, then break; we must not
            // have any collisions left to resolve
            if (m_collision_set.is_empty())
            {
                break;
            }

            // TODO: randomly select a ball that has at least 1 potential
            // collision remaining. use the m_collision_set to select such a
            // ball. then randomly select one of its neighbors that it can
            // collide with; there must be at least one such neighbor, or it
            // would not have been in the collision set
            const auto ball1_id = m_collision_set.random_element(m_rng);
            const auto &ball2_candidates
                = m_ball_to_collision_partners[ball1_id];
            assert((ball2_candidates.size() != 0) and "uhoh !!!");
            // assert((not ball2_candidates.empty()) and "uhoh !!!");
            auto ball2_id = std::size_t{};
            std::ranges::sample(ball2_candidates, &ball2_id, 1, m_rng);

            // TODO: perform the collision by updating the pair's velocities
            // according to the laws of elastic collisions
            assert(ball1_id != ball2_id and "these should not be the same");
            const auto displacement
                = normalize(m_ball_grid.displacement(ball1_id, ball2_id));

            auto &ball1 = m_ball_grid[ball1_id];
            auto &ball2 = m_ball_grid[ball2_id];

            const auto component1
                = displacement * dot(displacement, ball1.velocity);
            const auto component2
                = displacement * dot(displacement, ball2.velocity);

            ball1.velocity += component2 - component1;
            ball2.velocity += component1 - component2;

            // TODO: for each ball in the pair AND for each ball in the
            // neighbor sets of each ball in the pair, check if it can
            // potentially collide with one of its neighbors; iterate over the
            // neighbors and check collision condition for each neighbor. if
            // so, add it to collision set. if not, remove it from collision
            // set
            //
            // gather all balls to update into a single container to iterate
            // over
            const auto neighbors1 = m_ball_grid.get_neighbors(ball1_id);
            const auto neighbors2 = m_ball_grid.get_neighbors(ball2_id);
            // auto balls_to_update = std::unordered_set<std::size_t>{};
            auto balls_to_update = UpdateSet{};

            balls_to_update.insert_range(neighbors1);
            balls_to_update.insert_range(neighbors2);
            balls_to_update.insert_range(
                std::initializer_list{ ball1_id, ball2_id });

            assert(balls_to_update.size() <= 10
                   and "there should be at most 10 balls in a single update "
                       "step");
            update_balls(balls_to_update);

            // TODO: increment num collisions counter
            m_num_collisions++;
        }
    }

    auto
    print_stats() const -> void
    {
        std::println("completed ?: {}", m_collision_set.is_empty());
        std::println("number of collisions: {}", m_num_collisions);
        // TODO: print collisions per sec
        // TODO: print size of collision set every few iterations
    }

    auto
    get_number_of_collisions() const -> std::size_t
    {
        return m_num_collisions;
    }

    auto
    to_file(const std::filesystem::path &filepath) const -> void
    {
        // TODO: add the number of collisions to the file. youd have to update
        // the python parser to extract this new piece of data
        // NOTE: i explicitly require the given filename to NOT exist so i dont
        // overwrite existing data
        auto file = std::ofstream{ filepath,
                                   std::ios::binary | std::ios::noreplace };
        if (not file.is_open())
        {
            throw std::filesystem::filesystem_error{
                "could not open file", filepath,
                std::error_code{ errno, std::generic_category() }
            };
        }

        // first 2 u64 values are the dimensions of the grid
        constexpr auto nrows = to_big_endian(std::uint64_t{ NRows });
        file.write(reinterpret_cast<const char *>(&nrows),
                   sizeof(decltype(nrows)));

        constexpr auto ncols = to_big_endian(std::uint64_t{ NCols });
        file.write(reinterpret_cast<const char *>(&ncols),
                   sizeof(decltype(ncols)));

        // then, each pair of velocity coordinates for each ball in order of id
        // from 0 to size()-1
        assert(
            sizeof(double) == 8
            and "the custom binary format expects doubles are 64-bit values");
        for (auto big_endian_ball_velocity :
             std::views::iota(0UZ, m_ball_grid.size())
                 | std::views::transform(
                     [this](const auto ball_id)
                     { return m_ball_grid[ball_id].velocity; }))
        {
            const auto [x, y] = big_endian_ball_velocity;
            file.write(reinterpret_cast<const char *>(&x),
                       sizeof(decltype(x)));
            file.write(reinterpret_cast<const char *>(&y),
                       sizeof(decltype(y)));
        }
    }

  private:
    // maintains the state of all balls in the system. the positions of the
    // balls in this grid is significant and determines its neighbors, even tho
    // the physical position is not stored explicitly
    BallGrid<NRows, NCols> m_ball_grid;

    // every ball id in this set has at least one collision remaining with at
    // least one of its neighbors
    RandomIndexSet<NRows * NCols> m_collision_set;

    // associates with each ball id I a set of ball ids S such that I can
    // collide with all balls in S
    // TODO: i think the collision set can be even smaller than a neighbor set,
    // but id have to verify that
    std::unordered_map<std::size_t, NeighborSet> m_ball_to_collision_partners;
    // std::unordered_map<std::size_t, std::unordered_set<std::size_t>>
    // m_ball_to_collision_partners;

    RNGType m_rng;
    std::uint64_t m_num_collisions = 0;

    // the requires clause just says the range is a range of std::size_t
    auto
    update_balls(std::ranges::input_range auto &&ball_ids) -> void
        requires std::convertible_to<
            std::ranges::range_value_t<decltype(ball_ids)>,
            std::size_t>
    {
        for (const auto ball_id : ball_ids)
        {
            // we start by removing ball_id since it may not have any
            // collisions rn. if it does, we will discover that and
            // re-insert it below
            m_collision_set.erase(ball_id);
            // also remove all collision partners since ball_id may not
            // have any. we will re-discover them below
            m_ball_to_collision_partners[ball_id].clear();

            const auto neighbor_ids = m_ball_grid.get_neighbors(ball_id);
            for (const auto neighbor_id : neighbor_ids)
            {
                if (m_ball_grid.is_collision_pair(ball_id, neighbor_id))
                {
                    m_collision_set.insert(ball_id);
                    m_ball_to_collision_partners[ball_id].insert(neighbor_id);
                }
            }
        }
    }

    // TODO: this code is untested for non-big endian systems
    [[nodiscard]]
    constexpr static auto
    to_big_endian(const Numeric auto value) -> decltype(value)
    {
        if constexpr (std::endian::native == std::endian::big)
        {
            return value;
        }

        // TODO: im suspicious of reversing the byte order of a floating point
        // value... make sure this is correct and not ub somehow
        if constexpr (std::is_floating_point_v<decltype(value)>)
        {
            auto bytes
                = std::bit_cast<std::array<std::uint8_t, sizeof(value)>>(
                    value);
            std::ranges::reverse(bytes);
            return std::bit_cast<decltype(value)>(bytes.data());
        }
        else
        {
            // integers can just use byteswap
            return std::byteswap(value);
        }
    }
};
