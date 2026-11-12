#pragma once

#include <array>
#include <cstddef>
#include <ranges>
#include <stdexcept>
#include <type_traits>

// a set-like data structure but its values are stored in a fixed-size array.
// we check for containment by iterating over the buffer, so this data
// structure only makes sense for small sets. also, T should be a small object,
// like a builtin integral type for this data structure to be fast. also this
// data structure does not support erase() due to the use-cases i have planned
// for it
//
// in addition, this class models std::ranges::forward_range over the elements
// of the set
template <typename T, std::size_t Capacity> class BoundedSmallSet
{

  public:
    constexpr auto
    begin(this auto &&self)
    {
        return self.get_valid_subrange().begin();
    }

    // NOTE: the sentinel value is the index m_num_valid so a range over this
    // object has the expected extents since we only have valid elements up to
    // that index
    constexpr auto
    end(this auto &&self)
    {
        return self.get_valid_subrange().end();
    }

    [[nodiscard]]
    constexpr auto
    size() const -> std::size_t
    {
        return m_num_valid;
    }

    [[nodiscard]]
    constexpr auto
    contains(const T &value) const -> bool
    {
        const auto subrange = get_valid_subrange();
        return std::ranges::find(subrange, value) != subrange.end();
    }

    auto
    clear() -> void
    {
        m_num_valid = 0UZ;
    }

    constexpr auto
    insert(const T &value) -> void
    {
        if (contains(value))
        {
            return;
        }

        if (size() == Capacity)
        {
            throw std::runtime_error{
                "cannot insert element: this bounded set is already full"
            };
        }

        m_elements[m_num_valid] = value;
        m_num_valid++;
    }

    // TODO: write tests for this fn
    auto
    insert_range(const std::ranges::input_range auto &range) -> void
        requires std::
            convertible_to<std::ranges::range_value_t<decltype(range)>, T>
    {
        for (const auto &value : range)
        {
            insert(value);
        }
    }

  private:
    std::array<T, Capacity> m_elements;
    std::size_t m_num_valid = 0;

    // returns a range over the valid elements in the buffer
    constexpr auto
    get_valid_subrange() const
    {
        // return m_elements | std::views::take(m_num_valid);
        return std::ranges::subrange{ m_elements.cbegin(),
                                      m_elements.cbegin() + m_num_valid };
    }
};
