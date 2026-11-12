#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <ranges>
#include <stdexcept>

#include "bounded_small_set.hpp"

// verifies that we can insert/use the given empty set as expected
auto
verify_empty_set_behavior(auto &set, const auto capacity)
{

    WHEN("1 element is inserted")
    {
        constexpr auto target = 3UZ;
        set.insert(target);

        THEN("size is 1")
        {
            REQUIRE(set.size() == 1);
        }

        THEN("set contains that element")
        {
            REQUIRE(set.contains(target));
        }
    }

    WHEN("1 element is inserted twice")
    {
        constexpr auto target = 3UZ;
        set.insert(target);
        set.insert(target);

        THEN("size is 1")
        {
            REQUIRE(set.size() == 1);
        }

        THEN("set contains that element")
        {
            REQUIRE(set.contains(target));
        }
    }

    WHEN("1 element is inserted capacity+1-number of times")
    {
        constexpr auto target = 3UZ;
        for (auto _ : std::views::iota(0UZ, capacity + 1))
        {
            set.insert(target);
        }

        THEN("size is 1")
        {
            REQUIRE(set.size() == 1);
        }

        THEN("set contains that element")
        {
            REQUIRE(set.contains(target));
        }
    }

    WHEN("2 elements are inserted")
    {
        constexpr auto target1 = 3UZ;
        constexpr auto target2 = 100UZ;
        set.insert(target1);
        set.insert(target2);

        THEN("size is 2")
        {
            REQUIRE(set.size() == 2);
        }

        THEN("set contains elements")
        {
            REQUIRE(set.contains(target1));
            REQUIRE(set.contains(target2));
        }
    }

    WHEN("2 elements are inserted multiple times")
    {
        constexpr auto target1 = 3UZ;
        constexpr auto target2 = 100UZ;
        set.insert(target1);
        set.insert(target2);
        set.insert(target1);
        set.insert(target1);
        set.insert(target2);
        set.insert(target2);
        set.insert(target2);
        set.insert(target1);

        THEN("size is 2")
        {
            REQUIRE(set.size() == 2);
        }

        THEN("set contains elements")
        {
            REQUIRE(set.contains(target1));
            REQUIRE(set.contains(target2));
        }
    }

    WHEN("capacity-number of elements are inserted once")
    {
        auto elements = std::views::iota(0UZ, capacity);
        for (auto element : elements)
        {
            set.insert(element);
        }

        THEN("size is capacity")
        {
            REQUIRE(set.size() == capacity);
        }

        THEN("set contains all elements")
        {
            for (auto element : elements)
            {
                REQUIRE(set.contains(element));
            }
        }
    }

    WHEN("capacity-number of elements are inserted multiple times each")
    {
        auto elements = std::views::iota(0UZ, capacity);
        for (auto element : elements)
        {
            set.insert(element);
            set.insert(element);
            set.insert(element);
            set.insert(element);
        }

        THEN("size is capacity")
        {
            REQUIRE(set.size() == capacity);
        }

        THEN("set contains all elements")
        {
            for (auto element : elements)
            {
                REQUIRE(set.contains(element));
            }
        }

        THEN("inserting a new element throws")
        {
            REQUIRE_THROWS_AS(set.insert(capacity + 1), std::runtime_error);
        }
    }
}

SCENARIO("basic insert() usage for BoundedSmallSet", "[BoundedSmallSet]")
{
    GIVEN("an empty set")
    {
        constexpr auto capacity = 10UZ;
        auto set = BoundedSmallSet<std::size_t, capacity>{};

        verify_empty_set_behavior(set, capacity);
    }
}

SCENARIO("basic contains() usage for BoundedSmallSet", "[BoundedSmallSet]")
{
    GIVEN("an empty set")
    {
        constexpr auto capacity = 10UZ;
        auto set = BoundedSmallSet<std::size_t, capacity>{};

        THEN("returns false for any value we pass")
        {
            for (auto element : std::views::iota(0UZ, 100'000UZ))
            {
                REQUIRE(not set.contains(element));
            }
        }
    }
}

SCENARIO("clearing and reusing a full BoundedSmallSet", "[BoundedSmallSet]")
{
    GIVEN("a full set that was cleared")
    {
        constexpr auto capacity = 10UZ;
        auto set = BoundedSmallSet<std::size_t, capacity>{};

        const auto elements = std::views::iota(0UZ, capacity);
        for (auto element : elements)
        {
            set.insert(element);
        }
        set.clear();

        THEN("size is 0")
        {
            REQUIRE(set.size() == 0);
        }

        THEN("does not contain elements we inserted anymore")
        {
            for (auto element : elements)
            {
                REQUIRE(not set.contains(element));
            }
        }

        // the set should act like an empty set again
        verify_empty_set_behavior(set, capacity);
    }
}

SCENARIO("BoundedSmallSet acts as a range", "[BoundedSmallSet]")
{
    GIVEN("an empty set")
    {
        constexpr auto capacity = 10UZ;
        auto set = BoundedSmallSet<std::size_t, capacity>{};

        THEN("models expected concepts")
        {
            static_assert(std::ranges::forward_range<decltype(set)>);
            static_assert(std::ranges::forward_range<const decltype(set)>);
            static_assert(std::ranges::sized_range<decltype(set)>);
            static_assert(std::ranges::sized_range<const decltype(set)>);
        }

        THEN("begin/end compare equal")
        {
            REQUIRE(set.begin() == set.end());
        }
    }

    GIVEN("a set with a few elements")
    {
        constexpr auto capacity = 10UZ;
        auto set = BoundedSmallSet<std::size_t, capacity>{};

        // we specifically dont include 0 since 0 is often the default value
        // for the set's internal buffer, so that would lead to a false
        // positive for our test below
        const auto elements = std::views::iota(1UZ, capacity / 2);
        for (auto element : elements)
        {
            set.insert(element);
        }

        THEN("begin/end compare not equal")
        {
            REQUIRE(set.begin() != set.end());
        }

        THEN("std::distance over iterator range equals size")
        {
            REQUIRE(std::ranges::distance(set) == elements.size());
            REQUIRE(std::cmp_equal(std::distance(set.begin(), set.end()),
                                   set.size()));
            REQUIRE(std::cmp_equal(std::ranges::distance(set), set.size()));
        }

        THEN("iteration over set yields only the values we inserted")
        {
            for (auto value : set)
            {
                const auto found = std::ranges::find(elements, value);
                REQUIRE(found != elements.end());
                REQUIRE(*found == value);
            }
        }

        THEN("set works with std::ranges::find")
        {
            for (auto element : elements)
            {
                REQUIRE(*std::ranges::find(set, element) == element);
            }
        }
    }
}
