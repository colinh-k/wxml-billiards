#include <catch2/catch_test_macros.hpp>

#include "random_index_set.hpp"

#include <algorithm>
#include <initializer_list>
#include <random>
#include <ranges>
#include <stdexcept>

SCENARIO("RandomIndexSet is initialized empty")
{
    GIVEN("an empty set")
    {
        constexpr auto size = 3UZ;
        auto set = RandomIndexSet<size>{};

        THEN("initializes empty with size 0")
        {
            REQUIRE(set.is_empty());
            REQUIRE(set.size() == 0);
        }

        THEN("contains no valid ids")
        {
            for (auto id : std::views::iota(0UZ, size))
            {
                REQUIRE(not set.contains(id));
            }
        }

        WHEN("contains() is called with invalid id ")
        {
            THEN("throws an error")
            {
                // also see REQUIRE_THROWS_WITH to check the exception string
                REQUIRE_THROWS_AS(set.contains(size + 1),
                                  std::invalid_argument);
                REQUIRE_THROWS_AS(set.contains(size + 2),
                                  std::invalid_argument);
            }
        }
    }
}

SCENARIO("RandomIndexSet insert")
{
    GIVEN("an empty set")
    {
        constexpr auto size = 4UZ;
        auto set = RandomIndexSet<size>{};

        WHEN("insert() is called with one valid id")
        {
            constexpr auto valid_id = 0UZ;
            set.insert(valid_id);

            THEN("set contains that id")
            {
                REQUIRE(set.contains(valid_id));
            }

            THEN("set has size 1 and is non empty")
            {
                REQUIRE(set.size() == 1);
                REQUIRE(not set.is_empty());
            }
        }

        WHEN("insert() is called successively with multiple valid ids")
        {
            constexpr auto valid_ids = std::array{ 1UZ, 3UZ, 0UZ };
            for (auto valid_id : valid_ids)
            {
                set.insert(valid_id);
            }

            THEN("set contains those ids")
            {
                for (auto valid_id : valid_ids)
                {
                    REQUIRE(set.contains(valid_id));
                }
            }

            THEN("set does not contain non-inserted valid ids")
            {
                REQUIRE(not set.contains(2));
            }

            THEN("set has correct size and is non empty")
            {
                REQUIRE(set.size() == valid_ids.size());
                REQUIRE(not set.is_empty());
            }
        }

        WHEN("insert() is called successively with duplicate valid ids")
        {
            constexpr auto valid_id = 1UZ;
            set.insert(valid_id);
            set.insert(valid_id);
            set.insert(valid_id);
            set.insert(valid_id);

            THEN("set contains that id")
            {
                REQUIRE(set.contains(valid_id));
            }

            THEN("set does not contain non-inserted ids")
            {
                for (auto non_inserted_id : { 3UZ, 2UZ, 0UZ })
                {
                    REQUIRE(not set.contains(non_inserted_id));
                }
            }

            THEN("set has size 1 and is non empty")
            {
                REQUIRE(set.size() == 1);
                REQUIRE(not set.is_empty());
            }
        }

        WHEN("insert() is called with invalid id ")
        {
            THEN("throws an error")
            {
                REQUIRE_THROWS_AS(set.insert(size + 1), std::invalid_argument);
                REQUIRE_THROWS_AS(set.insert(size + 2), std::invalid_argument);
            }
        }
    }

    GIVEN("a full set")
    {
        constexpr auto size = 5UZ;
        auto set = RandomIndexSet<size>{};

        for (auto id : std::views::iota(0UZ, size))
        {
            set.insert(id);
        }

        WHEN("insert() is called with a valid id")
        {
            set.insert(3);

            THEN("set still contains all ids")
            {
                for (auto id : std::views::iota(0UZ, size))
                {
                    REQUIRE(set.contains(id));
                }
            }

            THEN("size is still full")
            {
                REQUIRE(set.size() == size);
                REQUIRE(not set.is_empty());
            }
        }
    }
}

SCENARIO("RandomIndexSet erase and insert")
{
    GIVEN("an empty set")
    {
        constexpr auto size = 5UZ;
        auto set = RandomIndexSet<size>{};

        WHEN("erase() is called multiple times with one valid id")
        {
            set.erase(3UZ);
            set.erase(3UZ);
            set.erase(3UZ);
            set.erase(3UZ);

            THEN("set still contains no valid ids")
            {
                for (auto valid_id : std::views::iota(0UZ, size))
                {
                    REQUIRE(not set.contains(valid_id));
                }
            }

            THEN("set remains empty with size 0")
            {
                REQUIRE(set.size() == 0);
                REQUIRE(set.is_empty());
            }
        }

        WHEN("insert() and erase() called on same valid id")
        {
            set.insert(3UZ);
            set.erase(3UZ);

            THEN("set still contains no valid ids")
            {
                for (auto valid_id : std::views::iota(0UZ, size))
                {
                    REQUIRE(not set.contains(valid_id));
                }
            }

            THEN("set remains empty with size 0")
            {
                REQUIRE(set.size() == 0);
                REQUIRE(set.is_empty());
            }
        }

        WHEN("insert() all then erase() 2 valid ids")
        {
            constexpr auto valid_ids = std::array{ 1UZ, 3UZ, 0UZ, 2UZ, 4UZ };
            constexpr auto erased_ids = std::array{ 4UZ, 1UZ };
            constexpr auto non_erased_ids = std::array{ 3UZ, 0UZ, 2UZ };

            for (auto valid_id : valid_ids)
            {
                set.insert(valid_id);
            }
            for (auto erased_id : erased_ids)
            {
                set.erase(erased_id);
            }

            THEN("set contains the non-erased ids")
            {
                for (auto non_erased_id : non_erased_ids)
                {
                    REQUIRE(set.contains(non_erased_id));
                }
            }

            THEN("set does not contain erased ids")
            {
                for (auto erased_id : erased_ids)
                {
                    REQUIRE(not set.contains(erased_id));
                }
            }

            THEN("set has correct size and is non empty")
            {
                REQUIRE(set.size() == non_erased_ids.size());
                REQUIRE(not set.is_empty());
            }
        }

        WHEN("insert() 2 then erase() 1 multiple times")
        {
            set.insert(3);
            set.insert(0);
            set.erase(3);
            set.erase(3);
            set.erase(3);
            set.erase(3);

            THEN("set contains inserted id")
            {
                REQUIRE(set.contains(0));
            }

            THEN("set does not contain erased id")
            {
                REQUIRE(not set.contains(3));
            }

            THEN("set has expected size and is nonempty")
            {
                REQUIRE(set.size() == 1);
                REQUIRE(not set.is_empty());
            }
        }

        WHEN(
            "insert() then erase() are called successively with same valid id")
        {
            constexpr auto valid_id = 2UZ;
            set.insert(valid_id);
            set.erase(valid_id);
            set.insert(valid_id);
            set.erase(valid_id);
            set.insert(valid_id);
            set.insert(valid_id);
            set.erase(valid_id);
            set.erase(valid_id);

            THEN("set still contains no valid ids")
            {
                for (auto id : std::views::iota(0UZ, size))
                {
                    REQUIRE(not set.contains(id));
                }
            }

            THEN("set remains empty with size 0")
            {
                REQUIRE(set.size() == 0);
                REQUIRE(set.is_empty());
            }
        }

        WHEN("erase() is called with invalid id ")
        {
            THEN("throws an error")
            {
                REQUIRE_THROWS_AS(set.erase(size + 1), std::invalid_argument);
                REQUIRE_THROWS_AS(set.erase(size + 2), std::invalid_argument);
            }
        }
    }
}

SCENARIO("RandomIndexSet random_element")
{
    // number of times the random function is called and conditions are checked
    constexpr auto random_iterations = 10'000;

    GIVEN("an rng engine")
    {
        constexpr auto seed = 329;
        auto rng = std::mt19937{ seed };

        GIVEN("an empty set")
        {
            constexpr auto size = 5UZ;
            auto set = RandomIndexSet<size>{};

            THEN("random_element() throws")
            {
                REQUIRE_THROWS_AS(set.random_element(rng), std::logic_error);
            }
        }

        GIVEN("a set that was inserted into and erased from until empty")
        {
            constexpr auto size = 5UZ;
            auto set = RandomIndexSet<size>{};

            for (auto id : std::views::iota(0UZ, size))
            {
                set.insert(id);
            }
            for (auto id : std::views::iota(0UZ, size))
            {
                set.erase(id);
            }

            THEN("random_element() throws")
            {
                REQUIRE_THROWS_AS(set.random_element(rng), std::logic_error);
                REQUIRE_THROWS_AS(set.random_element(rng), std::logic_error);
                REQUIRE_THROWS_AS(set.random_element(rng), std::logic_error);
            }
        }

        GIVEN("a set with 1 element")
        {
            constexpr auto size = 6UZ;
            auto set = RandomIndexSet<size>{};
            constexpr auto id = 2UZ;
            set.insert(id);

            THEN("random_element() returns that element")
            {
                for (auto _ : std::views::iota(0, random_iterations))
                {
                    REQUIRE(set.random_element(rng) == id);
                }
            }
        }

        GIVEN("a half full set")
        {
            constexpr auto size = 6UZ;
            auto set = RandomIndexSet<size>{};
            constexpr auto ids = std::array{ 2UZ, 1UZ, 5UZ, 0UZ };
            for (auto id : ids)
            {
                set.insert(id);
            }

            THEN("random_element() returns element in the set")
            {
                for (auto _ : std::views::iota(0, random_iterations))
                {
                    const auto actual = set.random_element(rng);
                    REQUIRE(set.contains(actual));
                    REQUIRE(std::ranges::contains(ids, actual));
                }
            }
        }

        GIVEN("a set that was inserted into and erased from")
        {
            constexpr auto size = 10UZ;
            auto set = RandomIndexSet<size>{};

            constexpr auto inserted = std::array{ 2UZ, 1UZ, 5UZ, 0UZ, 7UZ };
            constexpr auto inserted_and_erased = std::array{ 9UZ, 3UZ, 6UZ };
            for (auto id : inserted_and_erased)
            {
                set.insert(id);
            }
            for (auto id : inserted)
            {
                set.insert(id);
            }
            for (auto id : inserted_and_erased)
            {
                set.erase(id);
            }

            THEN("random_element() returns element in the set")
            {
                for (auto _ : std::views::iota(0, random_iterations))
                {
                    const auto actual = set.random_element(rng);
                    REQUIRE(set.contains(actual));
                    REQUIRE(std::ranges::contains(inserted, actual));
                }
            }
        }
    }
}
