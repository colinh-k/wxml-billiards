#pragma once

#include <array>
#include <cassert>
#include <numeric>
#include <random>
#include <ranges>
#include <stdexcept>
#include <unordered_map>

// a fixed-size set-like data structure that additionally supports random
// selection. the set must be told how many objects are in its universe. it
// allows a user to insert and remove objects by id/index in O(1). a user can
// also get a uniformly random id/index of an element in the set in O(1). note
// the set only stores ids/indices, not objects
//
// the ids are in the range [0,Size-1] inclusive
//
// Size is the size of the universe of objects this set tracks
//
// in the class, i use the name 'index' and 'id' interchangeably, so i should
// rlly clarify what i mean...
template <std::size_t Size> class RandomIndexSet
{
  public:
    explicit RandomIndexSet()
    {
        // initialize m_ids with [0,Size-1]
        std::ranges::iota(m_ids, 0);
        // every id I starts at index I in m_ids
        std::ranges::iota(m_id_to_index, 0);

#if 0
        m_id_to_index.reserve(Size);
        for (const auto [index, id] :
             std::views::zip(std::views::iota(0UZ), m_ids))
        {
            m_id_to_index[id] = index;
        }
#endif
    }

    // returns the number of elements currently in the set (not the total
    // number of elements in the universe of this set)
    auto
    size() const -> std::size_t
    {
        return m_num_valid;
    }

    auto
    is_empty() const -> bool
    {
        return size() == 0;
    }

    // removes all elements from this set
    auto
    clear() -> void
    {
        m_num_valid = 0;
    }

    // externally, an object is in this set if it is known to the data
    // structure (hard error if not) and it exists in the 1st section of the
    // internal array (index < m_num_valid)
    [[nodiscard]]
    auto
    contains(const std::size_t id) const -> bool
    {
        // if (not m_id_to_index.contains(id))
        if (id >= m_id_to_index.size())
        {
            throw std::invalid_argument{ "unknown id" };
        }
        return m_id_to_index[id] < m_num_valid;
        // return m_id_to_index.at(id) < m_num_valid;
    }

    // does nothing if object is already in the set
    auto
    insert(const std::size_t id) -> void
    {
        if (id >= m_id_to_index.size())
        {
            throw std::invalid_argument{ "unknown id" };
        }
        // assert(m_num_valid < m_ids.size()
        // and "attempting to insert into a set that is already full");

        if (m_num_valid >= m_ids.size())
        {
            // set is full, so object is must be in the set anyway
            return;
        }

        if (contains(id))
        {
            // id is already in the set
            return;
        }

        // swap object with boundary object at index m_num_valid
        const auto index = m_id_to_index[id];
        swap_objects(index, m_num_valid);

        m_num_valid++;
    }

    // does nothing if object is not in the set
    auto
    erase(const std::size_t id) -> void
    {
        if (id >= m_id_to_index.size())
        {
            throw std::invalid_argument{ "unknown id" };
        }

        // set is empty, so object must not be in the set anyway
        if (m_num_valid == 0)
        {
            return;
        }

        if (not contains(id))
        {
            // id is already not in the set
            return;
        }

        m_num_valid--;

        const auto index = m_id_to_index[id];
        swap_objects(index, m_num_valid);
    }

    // TODO: constrain the auto template type to be an rng engine type
    auto
    random_element(auto &rng) const -> std::size_t
    {
        if (m_num_valid < 1)
        {
            throw std::logic_error{
                "attempting to get a random id from an empty set"
            };
        }

        auto distribution
            = std::uniform_int_distribution{ 0UZ, m_num_valid - 1 };
        return m_ids[distribution(rng)];
    }

  private:
    // invariants:
    // -> m_ids[0:m_num_valid) are in the set
    // -> m_ids[m_num_valid:) are not in the set
    // -> m_id_to_index[i] := index into m_ids of object i
    std::array<std::size_t, Size> m_ids;
    std::array<std::size_t, Size> m_id_to_index;
    // std::unordered_map<std::size_t, std::size_t> m_id_to_index;
    std::size_t m_num_valid = 0;

    // swaps the given objects in the array and updates the map's index
    // bookeeping
    auto
    swap_objects(std::size_t id1, std::size_t id2) -> void
    {
        // TODO: assert indices are in the proper range for array
        assert(m_id_to_index.at(m_ids.at(id1)) == id1);
        assert(m_id_to_index.at(m_ids.at(id2)) == id2);

        std::swap(m_ids.at(id1), m_ids.at(id2));
        std::swap(m_id_to_index.at(m_ids.at(id1)),
                  m_id_to_index.at(m_ids.at(id2)));
    }
};
