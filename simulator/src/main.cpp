#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <print>
#include <random>
#include <ratio>
#include <utility>

#include "ball_grid.hpp"
#include "simulator.hpp"
#include "vec2.hpp"

// invokes the given function and returns the time it took to execute. returns
// runtime in milliseconds
auto
measure_runtime(auto &&function) -> std::chrono::duration<double, std::milli>
{
    using clock_type = std::chrono::high_resolution_clock;

    auto start = clock_type::now();
    std::forward<decltype(function)>(function)();
    auto end = clock_type::now();
    return end - start;
};

// parses commandline args, performs error checking, and returns the filepath
// where the results will be written
auto
get_filepath(int argc, char **argv, std::size_t nrows, std::size_t ncols)
    -> std::filesystem::path
{
    if (argc < 2)
    {
        std::println(stderr, "INFO: usage: {} <path/to/directory>", argv[0]);
        std::exit(1);
    }

    // directory in which the results will be stored
    auto directory_path = std::filesystem::path{ argv[1] };
    if (not std::filesystem::exists(directory_path)
        or not std::filesystem::is_directory(directory_path))
    {
        std::println(
            stderr,
            "INFO: provided path '{}' does not exist or is not a directory",
            directory_path.string());
        std::println(stderr, "INFO: usage: {} <path/to/directory>", argv[0]);
        exit(1);
    }

    const auto filename = std::format("{}x{}.dat", nrows, ncols);
    const auto filepath = directory_path / filename;
    // NOTE: this does not check we can actually open this file, which will
    // happen later. if we cant open it later, then we will fail to save the
    // results, which is unfortunate, but at least this check helps in the
    // common-case
    if (std::filesystem::exists(filepath))
    {
        std::println(stderr,
                     "INFO: cannot save to file '{}'; it already exists",
                     filepath.string());
        std::println(stderr, "INFO: usage: {} <path/to/directory>", argv[0]);
        std::exit(1);
    }

    return filepath;
}

auto
main(int argc, char **argv) -> int
{
#if 0
#ifndef NROWS
#error "number of rows is required"
#else
    constexpr auto nrows = static_cast<std::size_t>(NROWS);
#endif

#ifndef NCOLS
#error "number of columns is required"
#else
    constexpr auto ncols = static_cast<std::size_t>(NCOLS);
#endif
#endif

    constexpr auto nrows = 40UZ;
    constexpr auto ncols = 40UZ;

    const auto filepath = get_filepath(argc, argv, nrows, ncols);

    constexpr auto max_iterations = 100'000'000'000;

    auto balls = BallGrid<nrows, ncols>{};
    // TODO: save the initial impulse to the file as well, or at least write it
    // down somewhere ? maybe in the filename...
    balls[nrows / 2, 0].velocity = { 20.0, 0.0 };

    static constexpr auto rng_seed = 110;
    // clang-format off
    // NOLINTBEGIN(cert-msc51-cpp, cert-msc32-c, bugprone-random-generator-seed)
    // clang-format on
    auto rng = std::mt19937{ rng_seed };
    // clang-format off
    // NOLINTEND(cert-msc51-cpp, cert-msc32-c, bugprone-random-generator-seed)
    // clang-format on

    auto sim = Simulator{ std::move(balls), std::move(rng) };

    const auto runtime = measure_runtime([&sim] { sim.run(max_iterations); });

    const auto collision_per_second
        = static_cast<double>(sim.get_number_of_collisions())
          / std::chrono::duration<double>{ runtime }.count();

    std::println("{:-^70}", " simulation summary ");
    // left-aligned labels, right-aligned values
    std::println("{:25}{:>45}", "runtime:", runtime);
    std::println("{:25}{:>45d}",
                 "number of collisions:", sim.get_number_of_collisions());
    std::println("{:25}{:>45.2f}",
                 "collisions per second:", collision_per_second);
    std::println("{:25}{:>45}", "saving to file:", filepath.string());
    std::println("{:-^70}", "");

    sim.to_file(filepath);

    return 0;
}
