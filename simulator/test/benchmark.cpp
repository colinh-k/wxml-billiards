#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/benchmark/catch_chronometer.hpp>
#include <catch2/benchmark/catch_constructor.hpp>
#include <catch2/benchmark/catch_optimizer.hpp>
#include <catch2/catch_test_macros.hpp>

#include "ball_grid.hpp"
#include "random_index_set.hpp"
#include "simulator.hpp"

#include <random>
#include <utility>

// TODO: can i just use Catch::Benchmark::deoptimize_value(<value>) and
// Catch::Benchmark::keep_memory() ??
auto
do_not_optimize(auto &&value) -> void
{
    asm volatile("" : "+r,m"(value) : : "memory");
}

auto
clobber() -> void
{
    asm volatile("" : : : "memory");
}

// NOTE: since the object under test has an expensive constructor and we must
// construct a new instance for each measurement iteration for tests which
// mutate the object, we use a single object instance across
// across measurement calls. in order to reset the object, we call clear()
// after calling the function-of-interest. this means most measurements measure
// the function-of-interest AND a call to clear(). but clear() should be cheap,
// so this is an ok compromise
//
// NOTE: i updated the benchmarks to use a new object on every measurment
// iteration by using the std::vector trick provided in the docs; this lets us
// only measure the functions-of-interest, not overhead introduced by resetting
// the object, eg via clear(). also, for all benchmarks which dont modify the
// object, i passed the object using std::as_const() in the lambda capture.
// this requires that each measurement doesnt modify the object shared across
// iterations, which adds further guardrails against bad benchmarks
TEST_CASE("benchmark RandomIndexSet member functions", "[RandomIndexSet]")
{
    constexpr auto seed = 834756UZ;
    constexpr auto size = 40UZ * 40UZ;

    BENCHMARK_ADVANCED("default constructor")(auto meter)
    {
        auto sets
            = std::vector<Catch::Benchmark::storage_for<RandomIndexSet<size>>>(
                static_cast<std::size_t>(meter.runs()));
        meter.measure([&](std::size_t i) { sets[i].construct(); });
    };

    // there are 2 benchmarks that require a new object on each measurement,
    // which is a little more cumbersome to write than benchmarks that dont
    // modify the object
    BENCHMARK_ADVANCED("insert with valid id")(auto meter)
    {
        auto sets = std::vector(static_cast<std::size_t>(meter.runs()),
                                RandomIndexSet<size>{});
        meter.measure(
            [&](std::size_t i)
            {
                sets[i].insert(size - 1);
                // set.clear();
            });
    };

    BENCHMARK_ADVANCED("erase existing id")(auto meter)
    {
        constexpr auto target = 435UZ;
        auto sets = std::vector(static_cast<std::size_t>(meter.runs()),
                                RandomIndexSet<size>{});

        meter.measure(
            [&](std::size_t i)
            {
                // NOTE: its bad we have to call insert() in the measurement
                // but theres no other way to reset the object
                // set.insert(target);
                sets[i].erase(target);
                // set.clear();
            });
    };

    auto set = RandomIndexSet<size>{};
    do_not_optimize(set);

    BENCHMARK_ADVANCED("erase non-existent id")(auto meter)
    {
        constexpr auto target = 435UZ;

        meter.measure([&] { set.erase(target); });
    };

    BENCHMARK_ADVANCED("contains with valid id")(auto meter)
    {
        meter.measure([&, &set = std::as_const(set)]
                      { return set.contains(size - 1); });
    };

    BENCHMARK_ADVANCED("random_element from set with 1 element")(auto meter)
    {
        auto rng = std::mt19937{ seed };

        set.insert(435);

        meter.measure([&, &set = std::as_const(set)]
                      { return set.random_element(rng); });
    };

    BENCHMARK_ADVANCED("random_element from set with 2 elements")(auto meter)
    {
        auto rng = std::mt19937{ seed };

        set.insert(435);
        set.insert(672);

        meter.measure([&, &set = std::as_const(set)]
                      { return set.random_element(rng); });
    };

    BENCHMARK_ADVANCED("random_element from set with all elements")(auto meter)
    {
        auto rng = std::mt19937{ seed };

        for (auto id : std::views::iota(0UZ, size))
        {
            set.insert(id);
        }

        meter.measure([&, &set = std::as_const(set)]
                      { return set.random_element(rng); });
    };
}

// the initial performance profile suggests is_collision_pair,
// is_neighbor_pair, and get_neighbors are the functions in which main spends
// the most time. but actually all of these are called from within
// Simulator::update_balls(), which is where we spend the majority of time in
// main
TEST_CASE("benchmark BallGrid member functions", "[BallGrid]")
{
    // NOTE: be aware that changing grid size might violate assumptions in the
    // code below
    constexpr auto nrows = 40UZ;
    constexpr auto ncols = 40UZ;

    auto grid = BallGrid<nrows, ncols>{};
    do_not_optimize(grid);

    BENCHMARK_ADVANCED("displacement of neighbor balls")(auto meter)
    {
        meter.measure([&, &grid = std::as_const(grid)]
                      { return grid.displacement(7, 13); });
    };

    // TODO: write a unit test case for this situation to make sure the return
    // value is the expected result, especially for a large grid like this
    BENCHMARK_ADVANCED("is_collision_pair of colliding balls")(auto meter)
    {
        // i chose a pair in the middle so they both have a full neighbor set
        constexpr auto index1 = grid.grid_coordinate_to_index(
            GridCoordinate{ nrows / 2, ncols / 2 });
        constexpr auto index2 = grid.grid_coordinate_to_index(
            GridCoordinate{ nrows / 2 + 1, ncols / 2 + 0 });

        grid[index1] = Ball{ Vec2{ 0.0, 1.0 } };
        grid[index2] = Ball{ Vec2{ 0.0, -1.0 } };

        meter.measure([&, &grid = std::as_const(grid)]
                      { return grid.is_collision_pair(index1, index2); });
    };

    BENCHMARK_ADVANCED("is_collision_pair of non-colliding balls")(auto meter)
    {
        constexpr auto index1 = grid.grid_coordinate_to_index(
            GridCoordinate{ nrows / 2, ncols / 2 });
        constexpr auto index2 = grid.grid_coordinate_to_index(
            GridCoordinate{ nrows / 2 - 1, ncols / 2 + 0 });

        grid[index1] = Ball{ Vec2{ 0.0, 1.0 } };
        grid[index2] = Ball{ Vec2{ 0.0, -1.0 } };

        meter.measure([&, &grid = std::as_const(grid)]
                      { return grid.is_collision_pair(index1, index2); });
    };

    BENCHMARK_ADVANCED("is_neighbor_pair of neighbors")(auto meter)
    {
        constexpr auto index1 = grid.grid_coordinate_to_index(
            GridCoordinate{ nrows / 2, ncols / 2 });
        constexpr auto index2 = grid.grid_coordinate_to_index(
            GridCoordinate{ nrows / 2 - 1, ncols / 2 + 0 });

        meter.measure([&, &grid = std::as_const(grid)]
                      { return grid.is_neighbor_pair(index1, index2); });
    };

    BENCHMARK_ADVANCED("is_neighbor_pair of non-neighbors")(auto meter)
    {
        constexpr auto index1 = grid.grid_coordinate_to_index(
            GridCoordinate{ nrows / 2, ncols / 2 });
        constexpr auto index2 = grid.grid_coordinate_to_index(
            GridCoordinate{ nrows / 2 - 2, ncols / 2 + 0 });

        meter.measure([&, &grid = std::as_const(grid)]
                      { return grid.is_neighbor_pair(index1, index2); });
    };

    BENCHMARK_ADVANCED("get_neighbors of ball with all 6 neighbors")(
        auto meter)
    {
        meter.measure([&, &grid = std::as_const(grid)]
                      { return grid.get_neighbors(17); });
    };
}

TEST_CASE("benchmark Simulator::run()", "[Simulator]")
{
    constexpr auto seed = 834756UZ;
    auto rng = std::mt19937{ seed };

    constexpr auto nrows = 20UZ;
    constexpr auto ncols = 20UZ;
    auto grid = BallGrid<nrows, ncols>{};
    // give the grid some velocity, just like we would for a halfplane
    // simulation setup. without this, run() completes almost immediately bc
    // there is nothing to simulate
    grid[nrows / 2, 0] = Ball{ Vec2{ 10.0, 0.0 } };

    // TODO: remember that run() mutates the sim state, so we need to construct
    // a fresh sim for each measurment. use a vector of size meter.runs() or
    // whatever the docs say so that each run gets its own sim to mutate
    // independently
    BENCHMARK_ADVANCED("run")(auto meter)
    {
        constexpr auto max_iterations = 1'000'000UZ;
        // we need to explicitly make a copy of grid/rng and then pass the
        // newly copied temporary to the ctor of simulator as an
        // rvalue-reference so that each simulator gets its own/separate copy
        // of the data and rng
        auto simulators
            = std::vector(static_cast<std::size_t>(meter.runs()),
                          Simulator{ BallGrid{ grid }, std::mt19937{ rng } });

        meter.measure(
            [&](std::size_t i)
            {
                // Catch::Benchmark::deoptimize_value(simulators[i]);
                simulators[i].run(max_iterations);
                // Catch::Benchmark::deoptimize_value(simulators[i]);
                // Catch::Benchmark::keep_memory();
            });
    };
}
