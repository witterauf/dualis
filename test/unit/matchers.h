#pragma once

#include <dualis.h>
#include <catch2/catch_all.hpp>

struct EachElementIs : Catch::Matchers::MatcherGenericBase
{
    EachElementIs(const std::byte value)
        : m_value{value}
    {
    }

    template <typename Range> bool match(Range const& other) const
    {
        for (auto const val : other)
        {
            if (val != m_value)
            {
                return false;
            }
        }
        return true;
    }

    std::string describe() const override
    {
        return "Each element is: 0x" + dualis::to_hex_string(m_value);
    }

private:
    std::byte m_value;
};

template <class ByteRange> struct EqualsByteRangeMatcher : Catch::Matchers::MatcherGenericBase
{
    EqualsByteRangeMatcher(const ByteRange& value, std::size_t count)
        : m_value{value}
        , m_count{count}
    {
    }

    template <typename OtherRange> bool match(OtherRange const& other) const
    {
        if (m_count == 0)
        {
            if (m_value.size() != other.size())
            {
                return false;
            }
            return std::memcmp(m_value.data(), other.data(), m_value.size()) == 0;
        }
        else
        {
            if (other.size() != m_count || m_value.size() < m_count)
            {
                return false;
            }
            return std::memcmp(m_value.data(), other.data(), m_count) == 0;
        }
    }

    auto describe() const -> std::string override
    {
        return "\nequals\n" + Catch::rangeToString(m_value);
    }

private:
    const ByteRange& m_value;
    std::size_t m_count{0};
};

struct NotModified : Catch::Matchers::MatcherGenericBase
{
    template <class Allocator> bool match(const Allocator& allocator) const
    {
        return allocator.stats->copied == 0 && allocator.stats->moved == 0 &&
               allocator.stats->deallocated == 0 && allocator.stats->allocated == 0;
    }

    std::string describe() const override
    {
        return "NotModified";
    }
};

template <class ByteRange>
auto EqualsByteRange(const ByteRange& bytes, const std::size_t count = 0)
    -> EqualsByteRangeMatcher<ByteRange>
{
    return EqualsByteRangeMatcher<ByteRange>{bytes, count};
}

template <class T> struct PointsWithinMatcher : Catch::Matchers::MatcherGenericBase
{
    PointsWithinMatcher(const T& object)
        : m_value{object}
    {
    }

    bool match(const std::byte* pointer) const
    {
        auto const* lower = reinterpret_cast<const std::byte*>(&m_value);
        auto const* upper = reinterpret_cast<const std::byte*>(&m_value + 1);
        return pointer >= lower && pointer < upper;
    }

    auto describe() const -> std::string override
    {
        return "is stored within object";
    }

private:
    const T& m_value;
};

template <class T> auto PointsWithin(const T& value) -> PointsWithinMatcher<T>
{
    return PointsWithinMatcher<T>{value};
}
