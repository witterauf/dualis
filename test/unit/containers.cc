#include "matchers.h"
#include "mock_allocator.h"
#include <dualis.h>
#include <list>
#include <vector>

using namespace dualis;
using namespace dualis::literals;

template <class Allocator, typename Allocator::size_type BufferSize>
auto begin(const dualis::detail::_byte_storage<Allocator, BufferSize>& bytes)
{
    return bytes.data();
}

template <class Allocator, typename Allocator::size_type BufferSize>
auto end(const dualis::detail::_byte_storage<Allocator, BufferSize>& bytes)
{
    return bytes.data() + bytes.size();
}

template <class Allocator, std::size_t EmbeddedSize>
auto make_allocated_byte_storage(allocator_stats* stats, unsigned id = 0)
    -> dualis::detail::_byte_storage<Allocator, EmbeddedSize>
{
    return dualis::detail::_byte_storage<Allocator, EmbeddedSize>{EmbeddedSize + 1,
                                                                  Allocator{stats, id}};
}

template <class Allocator, std::size_t EmbeddedSize>
auto make_embedded_byte_storage(allocator_stats* stats, unsigned id = 0)
    -> dualis::detail::_byte_storage<Allocator, EmbeddedSize>
{
    return dualis::detail::_byte_storage<Allocator, EmbeddedSize>{EmbeddedSize - 1,
                                                                  Allocator{stats, id}};
}

#define CLASS_UNDER_TEST "_byte_storage"

SCENARIO(CLASS_UNDER_TEST ": assignment", "[containers]")
{
    static constexpr std::size_t EmbeddedSize = 16;
    static constexpr std::size_t EmbeddedStorageSize = EmbeddedSize - 1;
    static constexpr std::size_t AllocatedStorageSize = EmbeddedSize + 1;
    static constexpr std::size_t DefaultId = 0;
    static constexpr std::size_t DistinctId = 1;
    using namespace dualis::detail;
    std::array<std::byte, EmbeddedSize> TestData{0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x10_b, 0x11_b,
                                                 0x12_b, 0x13_b, 0x10_b, 0x11_b, 0x12_b, 0x13_b,
                                                 0x10_b, 0x11_b, 0x12_b, 0x13_b};

    allocator_stats stats;
    allocator_stats other_stats;

    GIVEN(CLASS_UNDER_TEST " _bytes_; // constructed")
    {
        WHEN("_bytes_.reassign(count); // discard contents and resize")
        {
            using byte_storage = _byte_storage<always_equal_allocator, EmbeddedSize>;

            AND_WHEN("count <= _bytes_.capacity()")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<always_equal_allocator, EmbeddedSize>(&stats);
                auto* old_data = _bytes_.data();
                auto const old_capacity = _bytes_.capacity();

                auto* data = _bytes_.reassign(_bytes_.capacity() - 1);

                THEN("the data pointer remains unchanged")
                {
                    REQUIRE(_bytes_.data() == old_data);
                }
                THEN("the capacity remains unchanged")
                {
                    REQUIRE(_bytes_.capacity() == old_capacity);
                }
                THEN("the return value equals the data pointer")
                {
                    REQUIRE(data == _bytes_.data());
                }
            }
            AND_WHEN("count > _bytes_.capacity()")
            {
                AND_WHEN("_bytes_.is_allocated()")
                {
                    auto _bytes_ =
                        make_allocated_byte_storage<always_equal_allocator, EmbeddedSize>(&stats);
                    _bytes_.get_allocator().start_tracking();

                    auto const count = _bytes_.capacity() + 1;
                    auto* data = _bytes_.reassign(count);

                    THEN("the old data is deallocated")
                    {
                        REQUIRE(stats.deallocated == 1);
                    }
                    THEN("the new data is allocated")
                    {
                        REQUIRE(stats.allocated == 1);
                    }
                    THEN("the new capacity is at least count")
                    {
                        REQUIRE(_bytes_.capacity() >= count);
                    }
                    THEN("the return value equals the data pointer")
                    {
                        REQUIRE(data == _bytes_.data());
                    }
                }
                AND_WHEN("!_bytes_.is_allocated()")
                {
                    auto _bytes_ =
                        make_embedded_byte_storage<always_equal_allocator, EmbeddedSize>(&stats);
                    _bytes_.get_allocator().start_tracking();

                    auto const count = _bytes_.capacity() + 1;
                    auto* data = _bytes_.reassign(count);

                    THEN("no deallocation takes place")
                    {
                        REQUIRE(stats.deallocated == 0);
                    }
                    THEN("the new data is allocated")
                    {
                        REQUIRE(stats.allocated == 1);
                    }
                    THEN("the new capacity is at least count")
                    {
                        REQUIRE(_bytes_.capacity() >= count);
                    }
                    THEN("the return value equals the data pointer")
                    {
                        REQUIRE(data == _bytes_.data());
                    }
                }
            }
        }

        WHEN("_bytes_.assign(bytes_ptr, count); // assign array of bytes")
        {
            auto _bytes_ = make_embedded_byte_storage<always_equal_allocator, EmbeddedSize>(&stats);

            _bytes_.assign(TestData.data(), _bytes_.size());

            THEN("_bytes_ equals _bytes_ptr")
            {
                REQUIRE_THAT(_bytes_, EqualsByteRange(TestData, _bytes_.size()));
            }
        }

        WHEN("_bytes_.change_allocator(new_alloc, destroy); // assign new allocator")
        {
            THEN("_bytes_ has new_alloc")
            {
                static constexpr bool DoesntMatter = true;
                auto _bytes_ = make_embedded_byte_storage<always_equal_allocator, EmbeddedSize>(
                    &stats, DefaultId);
                decltype(_bytes_)::allocator_type new_alloc{&other_stats, DistinctId};

                new_alloc.start_tracking();
                _bytes_.change_allocator(new_alloc, DoesntMatter);

                REQUIRE(_bytes_.get_allocator().id == DistinctId);

                AND_THEN("it is a copy")
                {
                    REQUIRE(new_alloc.stats->copied == 1);
                }
            }

            AND_WHEN("_bytes_.is_allocated()")
            {
                AND_WHEN("allocator_traits::is_always_equal")
                {
                    auto _bytes_ =
                        make_allocated_byte_storage<always_equal_allocator, EmbeddedSize>(
                            &stats, DefaultId);
                    static_assert(decltype(_bytes_)::allocator_traits::is_always_equal::value);
                    decltype(_bytes_)::allocator_type new_alloc{&other_stats, DistinctId};

                    AND_WHEN("destroy")
                    {
                        _bytes_.change_allocator(new_alloc, true);

                        THEN("its data is deallocated")
                        {
                            REQUIRE(stats.deallocated == 1);
                        }
                    }
                    AND_WHEN("!destroy")
                    {
                        _bytes_.change_allocator(new_alloc, false);

                        THEN("its data is not deallocated")
                        {
                            REQUIRE(stats.deallocated == 0);
                        }
                    }
                }
                AND_WHEN("!allocator_traits::is_always_equal")
                {
                    auto _bytes_ =
                        make_allocated_byte_storage<not_always_equal_allocator, EmbeddedSize>(
                            &stats, DefaultId);
                    static_assert(!decltype(_bytes_)::allocator_traits::is_always_equal::value);

                    AND_WHEN("_bytes_.allocator() != new_alloc")
                    {
                        decltype(_bytes_)::allocator_type new_alloc{&other_stats, DistinctId};
                        REQUIRE(_bytes_.get_allocator() != new_alloc);

                        _bytes_.change_allocator(new_alloc, false);

                        THEN("its data is deallocated")
                        {
                            REQUIRE(stats.deallocated == 1);
                        }
                    }
                    AND_WHEN("_bytes_.allocator() == new_alloc")
                    {
                        decltype(_bytes_)::allocator_type new_alloc{&other_stats, DefaultId};
                        REQUIRE(_bytes_.get_allocator() == new_alloc);

                        _bytes_.change_allocator(new_alloc, false);

                        THEN("its data is not deallocated")
                        {
                            REQUIRE(stats.deallocated == 0);
                        }
                    }
                }
            }
        }

        WHEN("_bytes_.assign(_other_); // copy-assign")
        {
            AND_WHEN("!allocator_traits::propagate_on_container_copy_assignment")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(
                    &other_stats, DistinctId);
                static_assert(!decltype(_bytes_)::allocator_traits::
                                  propagate_on_container_copy_assignment::value);
                std::memcpy(_other_.data(), TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(_other_);

                THEN("_bytes_ == _other_")
                {
                    REQUIRE_THAT(_bytes_, EqualsByteRange(_other_));
                }
                THEN("the allocator of _bytes_ is not modified")
                {
                    REQUIRE_THAT(_bytes_.get_allocator(), NotModified());
                    REQUIRE(_bytes_.get_allocator().id == DefaultId);
                }
                THEN("the allocator of _other_ is not modified")
                {
                    REQUIRE_THAT(_other_.get_allocator(), NotModified());
                    REQUIRE(_other_.get_allocator().id == DistinctId);
                }
            }
            AND_WHEN("allocator_traits::propagate_on_container_copy_assignment")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(
                    &other_stats, DistinctId);
                static_assert(decltype(_bytes_)::allocator_traits::
                                  propagate_on_container_copy_assignment::value);
                std::memcpy(_other_.data(), TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(_other_);

                THEN("_bytes_ == _other_")
                {
                    REQUIRE_THAT(_bytes_, EqualsByteRange(_other_));
                }
                THEN("the allocator of _other_ has been propagated to _bytes_")
                {
                    REQUIRE(_bytes_.get_allocator().id == _other_.get_allocator().id);

                    AND_THEN("by copying")
                    {
                        REQUIRE(other_stats.copied == 1);
                    }
                    AND_THEN("they compare equal")
                    {
                        REQUIRE(_bytes_.get_allocator() == _other_.get_allocator());
                    }
                }
            }
        }

        WHEN("_bytes_.take_contents(_other);")
        {
            auto _bytes_ = make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(&stats);

            AND_WHEN("_other_.is_allocated()")
            {
                auto _other_ =
                    make_allocated_byte_storage<propagate_allocator, EmbeddedSize>(&other_stats);

                auto const* old_data = _other_.data();
                auto const old_capacity = _other_.capacity();
                auto const old_size = _other_.size();
                _bytes_.take_contents(_other_);

                THEN("_bytes_ is allocated")
                {
                    REQUIRE(_bytes_.is_allocated());
                }
                THEN("_bytes_ has _other_'s data")
                {
                    REQUIRE(_bytes_.data() == old_data);

                    AND_THEN("_bytes_ has _other_'s capacity")
                    {
                        REQUIRE(_bytes_.capacity() == old_capacity);
                    }
                    AND_THEN("_bytes_ has _other_'s size")
                    {
                        REQUIRE(_bytes_.capacity() == old_size);
                    }
                }
                THEN("_other_ is not allocated anymore")
                {
                    REQUIRE_FALSE(_other_.is_allocated());
                }
                THEN("_other_ is empty")
                {
                    REQUIRE(_other_.size() == 0);
                }
            }
            AND_WHEN("!_other_.is_allocated()")
            {
                auto _other_ =
                    make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(&other_stats);
                _other_.assign(TestData.data(), _other_.size());

                auto const old_size = _other_.size();
                _bytes_.take_contents(_other_);

                THEN("_bytes_ is not allocated")
                {
                    REQUIRE_FALSE(_bytes_.is_allocated());
                }
                THEN("_bytes_ has the same size _other_ had")
                {
                    REQUIRE(_bytes_.size() == old_size);

                    AND_THEN("_bytes_ has the same contents _other_ had")
                    {
                        REQUIRE_THAT(_bytes_, EqualsByteRange(TestData, _bytes_.size()));
                    }
                }
                THEN("_other_ is empty")
                {
                    REQUIRE(_other_.size() == 0);
                }
            }
        }

        WHEN("_bytes_.assign(std::move(_other_)); // move-assign")
        {
            AND_WHEN("!allocator_traits::propagate_on_container_move_assignment && "
                     "!allocator_traits::is_always_equal && _bytes_.get_allocator() != "
                     "_other_.get_allocator()")
            {
                auto _bytes_ = make_embedded_byte_storage<dont_propagate_not_always_equal_allocator,
                                                          EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<dont_propagate_not_always_equal_allocator,
                                                          EmbeddedSize>(&other_stats, DistinctId);
                _other_.assign(TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                auto const old_size = _other_.size();
                _bytes_.assign(std::move(_other_));

                THEN("_other_ is empty")
                {
                    REQUIRE(_other_.size() == 0);
                }
                THEN("_bytes_ has the same size _other_ had")
                {
                    REQUIRE(_bytes_.size() == old_size);

                    AND_THEN("_bytes_ has the same contents _other_ had")
                    {
                        REQUIRE_THAT(_bytes_, EqualsByteRange(TestData, _bytes_.size()));
                    }
                }
                THEN("the allocator of _bytes_ was not touched")
                {
                    REQUIRE_THAT(_bytes_.get_allocator(), NotModified());
                }
                THEN("the allocator of _other_ was not touched")
                {
                    REQUIRE_THAT(_other_.get_allocator(), NotModified());
                }
            }
            AND_WHEN("allocator_traits::propagate_on_container_move_assignment")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<propagate_allocator, EmbeddedSize>(
                    &other_stats, DistinctId);
                _other_.assign(TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(std::move(_other_));

                THEN("the allocator of _bytes_ was replaced by _other_'s")
                {
                    REQUIRE(_bytes_.get_allocator().id == _other_.get_allocator().id);

                    AND_THEN("by moving")
                    {
                        REQUIRE(other_stats.moved == 1);
                    }
                }
            }
            AND_WHEN("!allocator_traits::propagate_on_container_move_assignment")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(
                    &other_stats);
                _other_.assign(TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(std::move(_other_));

                THEN("the allocator of _bytes_ was not touched")
                {
                    REQUIRE_THAT(_bytes_.get_allocator(), NotModified());
                }
                THEN("the allocator of _other_ was not touched")
                {
                    REQUIRE_THAT(_other_.get_allocator(), NotModified());
                }
            }
            AND_WHEN("!_bytes_.is_allocated()")
            {
                auto _bytes_ =
                    make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(
                    &other_stats);
                _other_.assign(TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(std::move(_other_));

                THEN("no deallocation by _bytes_ takes place")
                {
                    REQUIRE(stats.deallocated == 0);
                }
            }
            AND_WHEN("_bytes_.is_allocated()")
            {
                auto _bytes_ =
                    make_allocated_byte_storage<dont_propagate_allocator, EmbeddedSize>(&stats);
                auto _other_ = make_embedded_byte_storage<dont_propagate_allocator, EmbeddedSize>(
                    &other_stats);
                _other_.assign(TestData.data(), _other_.size());
                _bytes_.get_allocator().start_tracking();
                _other_.get_allocator().start_tracking();

                _bytes_.assign(std::move(_other_));

                THEN("the old data of _bytes_ is deallocated")
                {
                    REQUIRE(stats.deallocated == 1);
                }
            }
        }

        WHEN("_bytes_.swap(_other);")
        {
            auto _bytes_ =
                make_allocated_byte_storage<always_equal_allocator, EmbeddedSize>(&stats);
            auto _other_ =
                make_embedded_byte_storage<always_equal_allocator, EmbeddedSize>(&other_stats);
            _bytes_.assign(TestData.data() + 1, _other_.size());
            _other_.assign(TestData.data(), _other_.size());

            const decltype(_bytes_) _bytes_copy_{_bytes_};
            const decltype(_other_) _other_copy_{_other_};

            _bytes_.swap(_other_);

            THEN("_bytes_ contains the contents _other_ had")
            {
                REQUIRE_THAT(_bytes_, EqualsByteRange(_other_copy_));

                AND_THEN("_bytes_ is allocated if _other_ was")
                {
                    REQUIRE(_bytes_.is_allocated() == _other_copy_.is_allocated());
                }
            }
            THEN("_other_ contains the contents _bytes_ had")
            {
                REQUIRE_THAT(_other_, EqualsByteRange(_bytes_copy_));

                AND_THEN("_other_ is allocated if _bytes_ was")
                {
                    REQUIRE(_other_.is_allocated() == _bytes_copy_.is_allocated());
                }
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": construction and deconstruction", "[containers]")
{
    using namespace dualis::detail;
    static constexpr std::size_t EmbeddedSize = 16;
    static constexpr unsigned DistinctId = 13;
    allocator_stats stats;
    always_equal_allocator allocator{&stats, DistinctId};
    std::array<std::byte, EmbeddedSize + 2> TestData{
        0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x10_b,
        0x11_b, 0x12_b, 0x13_b, 0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x33_b, 0x34_b};

    GIVEN("an allocator")
    {
        WHEN(CLASS_UNDER_TEST "::" CLASS_UNDER_TEST " bytes{allocator};")
        {
            _byte_storage<always_equal_allocator, EmbeddedSize> bytes{allocator};

            THEN("allocator is copied into bytes")
            {
                REQUIRE(bytes.get_allocator().id == allocator.id);
                REQUIRE(stats.copied == 1);
            }
            THEN("bytes is empty")
            {
                REQUIRE(bytes.size() == 0);
            }
            THEN("the data of bytes is not in allocated storage")
            {
                REQUIRE_FALSE(bytes.is_allocated());
            }
            THEN("the capacity of bytes is EmbeddedSize")
            {
                REQUIRE(bytes.capacity() == EmbeddedSize);
            }
        }

        AND_GIVEN("an unsigned count")
        {
            WHEN(CLASS_UNDER_TEST "::" CLASS_UNDER_TEST " bytes{count, allocator};")
            {
                AND_WHEN("count > EmbeddedSize")
                {
                    static constexpr std::size_t count = EmbeddedSize + 1;
                    _byte_storage<always_equal_allocator, EmbeddedSize> bytes{count, allocator};

                    THEN("allocator is copied into bytes")
                    {
                        REQUIRE(bytes.get_allocator().id == allocator.id);
                        REQUIRE(stats.copied == 1);
                    }
                    THEN("allocation took place")
                    {
                        REQUIRE(stats.allocated == 1);
                    }
                    THEN("the size of bytes is count")
                    {
                        REQUIRE(bytes.size() == count);
                    }
                    THEN("the data of bytes is in allocated storage")
                    {
                        REQUIRE(bytes.is_allocated());
                    }
                    THEN("the data pointer is not within the bytes object")
                    {
                        REQUIRE_THAT(bytes.data(), !PointsWithin(bytes));
                    }
                    THEN("the capacity of bytes is at least count")
                    {
                        REQUIRE(bytes.capacity() >= count);
                    }
                }
                AND_WHEN("0 < count <= EmbeddedSize")
                {
                    static constexpr std::size_t count = EmbeddedSize;
                    _byte_storage<always_equal_allocator, EmbeddedSize> bytes{count, allocator};

                    THEN("allocator is copied into bytes")
                    {
                        REQUIRE(bytes.get_allocator().id == allocator.id);
                        REQUIRE(stats.copied == 1);
                    }
                    THEN("no allocation took place")
                    {
                        REQUIRE(stats.allocated == 0);
                    }
                    THEN("the size of bytes is count")
                    {
                        REQUIRE(bytes.size() == count);
                    }
                    THEN("the data of bytes is not in allocated storage")
                    {
                        REQUIRE_FALSE(bytes.is_allocated());
                    }
                    THEN("the data pointer is within the bytes object")
                    {
                        REQUIRE_THAT(bytes.data(), PointsWithin(bytes));
                    }
                    THEN("the capacity of bytes is EmbeddedSize")
                    {
                        REQUIRE(bytes.capacity() == EmbeddedSize);
                    }
                }
            }
        }
    }

    GIVEN(CLASS_UNDER_TEST " other;")
    {
        WHEN(CLASS_UNDER_TEST "::" CLASS_UNDER_TEST " bytes{other};")
        {
            AND_WHEN("always")
            {
                _byte_storage<always_equal_allocator, EmbeddedSize> other{EmbeddedSize, allocator};
                other.assign(TestData.data(), other.size());

                _byte_storage<always_equal_allocator, EmbeddedSize> bytes{other};

                THEN("allocator.select_on_container_copy_construction() is called")
                {
                    REQUIRE(stats.selected == 1);
                }
                THEN("bytes and other compare equal")
                {
                    REQUIRE_THAT(bytes, EqualsByteRange(other));
                }
            }
            AND_WHEN("other.is_allocated()")
            {
                _byte_storage<always_equal_allocator, EmbeddedSize> other{EmbeddedSize + 1,
                                                                          allocator};
                other.assign(TestData.data(), other.size());
                REQUIRE(other.is_allocated());

                other.get_allocator().start_tracking();
                _byte_storage<always_equal_allocator, EmbeddedSize> bytes{other};

                THEN("the data of bytes is also in allocated storage")
                {
                    REQUIRE(bytes.is_allocated());
                }
                THEN("an allocation has taken place")
                {
                    REQUIRE(stats.allocated == 1);
                }
                THEN("the capacity of bytes is at least the capacity of other")
                {
                    REQUIRE(bytes.capacity() >= other.capacity());
                }
            }
        }
        WHEN(CLASS_UNDER_TEST "::" CLASS_UNDER_TEST " bytes{std::move(other)};")
        {
            AND_WHEN("always")
            {
                _byte_storage<always_equal_allocator, EmbeddedSize> other{EmbeddedSize, allocator};
                other.assign(TestData.data(), other.size());

                auto const old_id = other.get_allocator().id;
                auto const old_size = other.size();
                _byte_storage<always_equal_allocator, EmbeddedSize> bytes{std::move(other)};

                THEN("the allocator of other is moved to bytes")
                {
                    REQUIRE(stats.moved == 1);
                    REQUIRE(bytes.get_allocator().id == old_id);
                }
                THEN("bytes has the size other had")
                {
                    REQUIRE(bytes.size() == old_size);

                    AND_THEN("bytes has the contents other had")
                    {
                        REQUIRE_THAT(bytes, EqualsByteRange(TestData, old_size));
                    }
                }
                THEN("other is empty")
                {
                    REQUIRE(other.size() == 0);
                }
            }
            AND_WHEN("other.is_allocated()")
            {
                _byte_storage<always_equal_allocator, EmbeddedSize> other{EmbeddedSize + 1,
                                                                          allocator};
                other.assign(TestData.data(), other.size());
                REQUIRE(other.is_allocated());

                auto const* old_data = other.data();
                _byte_storage<always_equal_allocator, EmbeddedSize> bytes{std::move(other)};

                THEN("bytes has the data pointer other had")
                {
                    REQUIRE(bytes.data() == old_data);
                }
                THEN("other has no allocated data anymore")
                {
                    REQUIRE_FALSE(other.is_allocated());
                }
            }
        }

        AND_GIVEN("an allocator")
        {
            WHEN(CLASS_UNDER_TEST "::" CLASS_UNDER_TEST " bytes{other, allocator};")
            {
                _byte_storage<always_equal_allocator, EmbeddedSize> other{EmbeddedSize, allocator};
                other.assign(TestData.data(), other.size());

                allocator.start_tracking();
                _byte_storage<always_equal_allocator, EmbeddedSize> bytes{other, allocator};

                THEN("bytes has allocator")
                {
                    REQUIRE(bytes.get_allocator().id == allocator.id);

                    AND_THEN("by copying")
                    {
                        REQUIRE(stats.copied == 1);
                    }
                }
            }
            WHEN(CLASS_UNDER_TEST "::" CLASS_UNDER_TEST " bytes{std::move(other), allocator};")
            {
                AND_WHEN("always")
                {
                    _byte_storage<always_equal_allocator, EmbeddedSize> other{EmbeddedSize,
                                                                              allocator};
                    other.assign(TestData.data(), other.size());

                    allocator.start_tracking();
                    _byte_storage<always_equal_allocator, EmbeddedSize> bytes{std::move(other),
                                                                              allocator};

                    THEN("bytes has allocator")
                    {
                        REQUIRE(bytes.get_allocator().id == allocator.id);

                        AND_THEN("by copying")
                        {
                            REQUIRE(stats.copied == 1);
                        }
                    }
                    THEN("other is empty")
                    {
                        REQUIRE(other.size() == 0);
                    }
                    THEN("other has no allocated data")
                    {
                        REQUIRE_FALSE(other.is_allocated());
                    }
                }
                AND_WHEN("other.is_allocated()")
                {
                    AND_WHEN("other.get_allocator() != allocator")
                    {
                        allocator_stats other_stats;
                        not_always_equal_allocator other_allocator{&stats, DistinctId};
                        not_always_equal_allocator allocator{&stats, DistinctId + 1};

                        _byte_storage<not_always_equal_allocator, EmbeddedSize> other{
                            EmbeddedSize + 1, other_allocator};
                        REQUIRE(other.is_allocated());
                        other.assign(TestData.data(), other.size());

                        auto const old_size = other.size();
                        auto const old_capacity = other.capacity();
                        allocator.start_tracking();
                        other_allocator.start_tracking();
                        decltype(other) bytes{std::move(other), allocator};

                        THEN("the new data is allocated")
                        {
                            REQUIRE(allocator.stats->allocated == 1);
                        }
                        THEN("the old data is deallocated")
                        {
                            REQUIRE(other_allocator.stats->deallocated == 1);
                        }
                        THEN("bytes has the size other had")
                        {
                            REQUIRE(bytes.size() == old_size);
                            THEN("the data is copied to bytes")
                            {
                                REQUIRE_THAT(bytes, EqualsByteRange(TestData, bytes.size()));
                            }
                        }
                        THEN("the capacity of bytes is at least the capacity other had")
                        {
                            REQUIRE(bytes.capacity() >= old_capacity);
                        }
                    }
                    AND_WHEN(
                        "!allocator_traits::is_always_equal && other.get_allocator() == allocator")
                    {
                        not_always_equal_allocator allocator{&stats, DistinctId};

                        _byte_storage<not_always_equal_allocator, EmbeddedSize> other{
                            EmbeddedSize + 1, allocator};
                        other.assign(TestData.data(), other.size());
                        REQUIRE(other.is_allocated());

                        auto const* old_data = other.data();
                        auto const old_capacity = other.capacity();
                        allocator.start_tracking();
                        _byte_storage<not_always_equal_allocator, EmbeddedSize> bytes{
                            std::move(other), allocator};

                        THEN("bytes has the data pointer other had")
                        {
                            REQUIRE(bytes.data() == old_data);
                        }
                        THEN("bytes has the capacity other had")
                        {
                            REQUIRE(bytes.capacity() == old_capacity);
                        }
                    }
                    AND_WHEN("allocator_traits::is_always_equal")
                    {
                        _byte_storage<always_equal_allocator, EmbeddedSize> other{EmbeddedSize + 1,
                                                                                  allocator};
                        other.assign(TestData.data(), other.size());
                        REQUIRE(other.is_allocated());

                        auto const* old_data = other.data();
                        auto const old_capacity = other.capacity();
                        allocator.start_tracking();
                        _byte_storage<always_equal_allocator, EmbeddedSize> bytes{std::move(other),
                                                                                  allocator};

                        THEN("bytes has the data pointer other had")
                        {
                            REQUIRE(bytes.data() == old_data);
                        }
                        THEN("bytes has the capacity other had")
                        {
                            REQUIRE(bytes.capacity() == old_capacity);
                        }
                    }
                }
            }
        }
    }

    GIVEN(CLASS_UNDER_TEST " bytes;")
    {
        WHEN(CLASS_UNDER_TEST "::~" CLASS_UNDER_TEST "();")
        {
            AND_WHEN("bytes.is_allocated()")
            {
                {
                    _byte_storage<always_equal_allocator, EmbeddedSize> bytes{EmbeddedSize + 1,
                                                                              allocator};
                    REQUIRE(bytes.is_allocated());
                    // deconstructor is called
                }

                THEN("the data is deallocated")
                {
                    REQUIRE(stats.deallocated == 1);
                }
            }
            AND_WHEN("!bytes.is_allocated()")
            {
                {
                    _byte_storage<always_equal_allocator, EmbeddedSize> bytes{EmbeddedSize,
                                                                              allocator};
                    REQUIRE_FALSE(bytes.is_allocated());
                    // deconstructor is called
                }

                THEN("no deallocation takes places")
                {
                    REQUIRE(stats.deallocated == 0);
                }
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": inserting and erasing elements", "[containers]")
{
    using namespace dualis::detail;
    allocator_stats stats;
    always_equal_allocator allocator{&stats};
    static constexpr std::size_t EmbeddedSize = 16;
    using byte_storage = _byte_storage<always_equal_allocator, EmbeddedSize>;
    std::array<std::byte, EmbeddedSize + 2> TestData{
        0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x10_b,
        0x11_b, 0x12_b, 0x13_b, 0x10_b, 0x11_b, 0x12_b, 0x13_b, 0x33_b, 0x34_b};

    GIVEN(CLASS_UNDER_TEST " bytes{count, allocator}; // count > 2")
    {
        byte_storage bytes{3, allocator};
        bytes.assign(TestData.data(), bytes.size());

        WHEN("bytes.append(append_count, APPENDER);")
        {
            AND_WHEN("bytes.size() + append_count <= bytes.capacity()")
            {
                const std::size_t append_count = EmbeddedSize - bytes.size();

                std::byte* passed_data{nullptr};
                auto const old_size = bytes.size();
                auto const old_data = bytes.data();
                bytes.get_allocator().start_tracking();

                bytes.append(append_count, [&passed_data](std::byte* data) { passed_data = data; });

                THEN("the data pointer of bytes is not modified")
                {
                    REQUIRE(bytes.data() == old_data);
                }
                THEN("bytes is append_count elements longer")
                {
                    REQUIRE(bytes.size() == old_size + append_count);
                }
                THEN("APPENDER is passed a pointer to the end of the data")
                {
                    REQUIRE(passed_data == bytes.data() + old_size);
                }
                THEN("no allocation takes place")
                {
                    REQUIRE(stats.allocated == 0);
                }
                THEN("no deallocation takes place")
                {
                    REQUIRE(stats.deallocated == 0);
                }
            }
            AND_WHEN("bytes.size() + append_count > bytes.capacity()")
            {
                AND_WHEN("!bytes.is_allocated()")
                {
                    const auto append_count = (EmbeddedSize - bytes.size()) + 1;

                    std::byte* passed_data{nullptr};
                    auto const old_size = bytes.size();
                    auto const old_data = bytes.data();
                    bytes.get_allocator().start_tracking();

                    bytes.append(append_count,
                                 [&passed_data](std::byte* data) { passed_data = data; });

                    THEN("new data is allocated")
                    {
                        REQUIRE(stats.allocated == 1);
                    }
                    THEN("the data pointer of bytes changes")
                    {
                        REQUIRE(bytes.data() != old_data);
                    }
                    THEN("the old contents is copied over")
                    {
                        REQUIRE(std::memcmp(bytes.data(), TestData.data(), old_size) == 0);
                    }
                    THEN("the new capacity is at least the new size")
                    {
                        REQUIRE(bytes.capacity() >= bytes.size());
                    }
                    THEN("bytes is append_count elements longer")
                    {
                        REQUIRE(bytes.size() == old_size + append_count);
                    }
                    THEN("APPENDER is passed a pointer to the end of the data")
                    {
                        REQUIRE(passed_data == bytes.data() + old_size);
                    }
                }
            }
        }
    }
}

#undef CLASS_UNDER_TEST

#define CLASS_UNDER_TEST "byte_container"

SCENARIO(CLASS_UNDER_TEST ": construction", "[containers]")
{
    using byte_vector_base = dualis::byte_container<not_always_equal_allocator>;

    WHEN(CLASS_UNDER_TEST " bytes{}; // default-construction")
    {
        byte_vector_base bytes{};

        THEN("bytes is empty")
        {
            REQUIRE(bytes.empty());
        }
        THEN("bytes has a default-constructed allocator")
        {
            REQUIRE(bytes.get_allocator() == byte_vector_base::allocator_type{});
        }
    }
    GIVEN("an allocator")
    {
        byte_vector_base::allocator_type allocator;
        allocator.id = 13;

        WHEN(CLASS_UNDER_TEST " bytes{allocator};")
        {
            byte_vector_base bytes{allocator};

            THEN("bytes is empty")
            {
                REQUIRE(bytes.empty());
            }
            THEN("bytes has the given allocator")
            {
                REQUIRE(bytes.get_allocator() == allocator);
            }
        }

        AND_GIVEN("a count")
        {
            const auto count = 12;

            WHEN(CLASS_UNDER_TEST " bytes{count, allocator};")
            {
                byte_vector_base bytes{count, allocator};

                THEN("bytes is count long")
                {
                    REQUIRE(bytes.size() == count);
                }
                THEN("bytes has the given allocator")
                {
                    REQUIRE(bytes.get_allocator() == allocator);
                }
            }

            AND_GIVEN("a value")
            {
                const std::byte value = 0x33_b;
                WHEN(CLASS_UNDER_TEST " bytes{count, value, allocator};")
                {
                    byte_vector_base bytes{count, value, allocator};

                    THEN("bytes is count long")
                    {
                        REQUIRE(bytes.size() == count);
                    }
                    THEN("all elements of bytes are value")
                    {
                        REQUIRE_THAT(bytes, EachElementIs(value));
                    }
                    THEN("bytes has the given allocator")
                    {
                        REQUIRE(bytes.get_allocator() == allocator);
                    }
                }
            }
        }
        AND_GIVEN("an initializer list ilist")
        {
            auto ilist = {0x13_b, 0x14_b, 0x15_b};

            WHEN(CLASS_UNDER_TEST " bytes{ilist, allocator};")
            {
                byte_vector_base bytes{ilist, allocator};

                THEN("bytes equals ilist")
                {
                    REQUIRE_THAT(bytes, EqualsByteRange(std::vector<std::byte>{ilist}));
                }
                THEN("bytes has the given allocator")
                {
                    REQUIRE(bytes.get_allocator() == allocator);
                }
            }
        }
        AND_GIVEN("a byte_pointer and a count")
        {
            auto ilist = {0x13_b, 0x14_b, 0x15_b};

            WHEN(CLASS_UNDER_TEST " bytes{byte_pointer, count, allocator};")
            {
                byte_vector_base bytes{ilist.begin(), ilist.size(), allocator};

                THEN("bytes equals [byte_pointer, byte_pointer+count)")
                {
                    REQUIRE_THAT(bytes, EqualsByteRange(std::vector<std::byte>{ilist}));
                }
                THEN("bytes has the given allocator")
                {
                    REQUIRE(bytes.get_allocator() == allocator);
                }
            }
        }
        AND_GIVEN("first and last iterators")
        {
            WHEN(CLASS_UNDER_TEST " bytes{first, last, allocator};")
            {
                AND_WHEN("the iterators are contiguous")
                {
                    std::vector<std::byte> contiguous{0x13_b, 0x14_b, 0x15_b};
                    static_assert(std::contiguous_iterator<decltype(contiguous)::const_iterator>);

                    byte_vector_base bytes{contiguous.cbegin(), contiguous.cend(), allocator};

                    THEN("bytes equals [byte_pointer, byte_pointer+count)")
                    {
                        REQUIRE_THAT(bytes, EqualsByteRange(contiguous));
                    }
                    THEN("bytes has the given allocator")
                    {
                        REQUIRE(bytes.get_allocator() == allocator);
                    }
                }
                AND_WHEN("the iterators are at most input iterators")
                {
                    std::list<std::byte> input{0x13_b, 0x14_b, 0x15_b};
                    static_assert(!std::random_access_iterator<decltype(input)::const_iterator>);
                    static_assert(std::input_iterator<decltype(input)::const_iterator>);

                    byte_vector_base bytes{input.cbegin(), input.cend(), allocator};

                    THEN("bytes equals [byte_pointer, byte_pointer+count)")
                    {
                        REQUIRE_THAT(bytes, EqualsByteRange(std::vector<std::byte>(input.cbegin(),
                                                                                   input.cend())));
                    }
                    THEN("bytes has the given allocator")
                    {
                        REQUIRE(bytes.get_allocator() == allocator);
                    }
                }
            }
        }
        AND_GIVEN("a " CLASS_UNDER_TEST " other;")
        {
            byte_vector_base other{0x13_b, 0x14_b, 0x15_b};
            byte_vector_base other_old{0x13_b, 0x14_b, 0x15_b};

            WHEN(CLASS_UNDER_TEST " bytes{other, allocator};")
            {
                byte_vector_base bytes{other, allocator};

                THEN("bytes equals other")
                {
                    REQUIRE_THAT(bytes, EqualsByteRange(other));
                }
                THEN("bytes has the given allocator")
                {
                    REQUIRE(bytes.get_allocator() == allocator);
                }
            }
            WHEN(CLASS_UNDER_TEST " bytes{std::move(other), allocator}")
            {
                byte_vector_base bytes{std::move(other), allocator};

                THEN("bytes equals other before the move")
                {
                    REQUIRE_THAT(bytes, EqualsByteRange(other_old));
                }
                THEN("bytes has the given allocator")
                {
                    REQUIRE(bytes.get_allocator() == allocator);
                }
                THEN("other is empty")
                {
                    REQUIRE(other.empty());
                }
            }
        }
        AND_GIVEN("another readable_bytes container")
        {
            std::vector<std::byte> container{0x13_b, 0x14_b, 0x15_b};

            WHEN(CLASS_UNDER_TEST " bytes{container, allocator};")
            {
                byte_vector_base bytes{container, allocator};

                THEN("bytes equals container")
                {
                    REQUIRE_THAT(bytes, EqualsByteRange(container));
                }
                THEN("bytes has the given allocator")
                {
                    REQUIRE(bytes.get_allocator() == allocator);
                }
            }
        }
    }
    AND_GIVEN("a " CLASS_UNDER_TEST " other;")
    {
        byte_vector_base other{0x13_b, 0x14_b, 0x15_b};
        byte_vector_base other_old{0x13_b, 0x14_b, 0x15_b};

        WHEN(CLASS_UNDER_TEST " bytes{other, allocator};")
        {
            byte_vector_base bytes{other};

            THEN("bytes equals other")
            {
                REQUIRE_THAT(bytes, EqualsByteRange(other));
            }
            THEN("bytes has an allocator comparing equal to other's")
            {
                REQUIRE(bytes.get_allocator() == other.get_allocator());
            }
        }
        WHEN(CLASS_UNDER_TEST " bytes{std::move(other), allocator}")
        {
            byte_vector_base bytes{std::move(other)};

            THEN("bytes equals other before the move")
            {
                REQUIRE_THAT(bytes, EqualsByteRange(other_old));
            }
            THEN("bytes has an allocator comparing equal to other's before the move")
            {
                REQUIRE(bytes.get_allocator() == other_old.get_allocator());
            }
            THEN("other is empty")
            {
                REQUIRE(other.empty());
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": assignment", "[containers]")
{
    using byte_vector_base = dualis::byte_container<not_always_equal_allocator>;

    GIVEN(CLASS_UNDER_TEST " bytes;")
    {
        byte_vector_base bytes;

        AND_GIVEN(CLASS_UNDER_TEST " other; // non-empty")
        {
            byte_vector_base other{0x13_b, 0x14_b, 0x15_b};
            byte_vector_base other_old{0x13_b, 0x14_b, 0x15_b};

            WHEN("bytes = other; // copy-assignment")
            {
                bytes = other;

                THEN("bytes and other compare equal")
                {
                    REQUIRE(bytes == other);
                }
                THEN("the allocators of bytes and other compare equal")
                {
                    REQUIRE(bytes.get_allocator() == other.get_allocator());
                }
            }
            WHEN("bytes = std::move(other); // move-assignment")
            {
                bytes = std::move(other);

                THEN("bytes compares equal to other before the move")
                {
                    REQUIRE(bytes == other_old);
                }
                THEN("the allocators of bytes and other before the move compare equal")
                {
                    REQUIRE(bytes.get_allocator() == other_old.get_allocator());
                }
                THEN("other is empty")
                {
                    REQUIRE(other.empty());
                }
            }
        }
        AND_GIVEN("std::initializer_list<std::byte> ilist;")
        {
            std::initializer_list<std::byte> ilist{0x13_b, 0x24_b, 0x36_b};
            WHEN("bytes = ilist;")
            {
                bytes = ilist;

                THEN("bytes has ilist as contents")
                {
                    REQUIRE_THAT(bytes, EqualsByteRange(std::vector<std::byte>(ilist)));
                }
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": iterators", "[containers]")
{
    using byte_vector_base = dualis::byte_container<not_always_equal_allocator>;

    GIVEN(CLASS_UNDER_TEST " bytes; // non-empty")
    {
        byte_vector_base bytes{0x13_b, 0x24_b, 0x35_b};

        THEN("*bytes.begin() equals the first element of bytes")
        {
            REQUIRE(*bytes.begin() == 0x13_b);
        }
        THEN("bytes.end() == bytes.begin() + bytes.size()")
        {
            REQUIRE(bytes.end() == bytes.begin() + bytes.size());
        }
        THEN("*bytes.rbegin() equals the last element of bytes")
        {
            REQUIRE(*bytes.rbegin() == 0x35_b);
        }
        THEN("bytes.rend() == bytes.rbegin() + bytes.size()")
        {
            REQUIRE(bytes.rend() == bytes.rbegin() + bytes.size());
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": element access", "[containers]")
{
    using byte_vector_base = dualis::byte_container<not_always_equal_allocator>;

    GIVEN(CLASS_UNDER_TEST " bytes; // non-empty")
    {
        const std::byte first = 0x13_b;
        const std::byte last = 0x35_b;
        const std::byte expected = 0xff_b;
        byte_vector_base bytes{first, 0x24_b, last};

        WHEN("value = bytes.front();")
        {
            auto const value = bytes.front();
            THEN("value is the first element of bytes")
            {
                REQUIRE(value == first);
            }
        }
        WHEN("value = bytes.back();")
        {
            auto const value = bytes.back();
            THEN("value is the last element of bytes")
            {
                REQUIRE(value == last);
            }
        }

        AND_GIVEN("an offset")
        {
            auto const offset = 1;

            WHEN("value = bytes.at(offset);")
            {
                auto const value = bytes.at(offset);
                THEN("value is the element at that offset")
                {
                    REQUIRE(value == bytes.data()[offset]);
                }
            }
            WHEN("value = bytes[offset];")
            {
                auto const value = bytes[offset];
                THEN("value is the element at that offset")
                {
                    REQUIRE(value == bytes.data()[offset]);
                }
            }
        }

        AND_GIVEN("a byte value")
        {
            auto const value = expected;

            WHEN("bytes.front() = value;")
            {
                bytes.front() = value;
                THEN("the first element of bytes becomes value")
                {
                    REQUIRE(bytes.data()[0] == value);
                }
            }
            WHEN("bytes.back() = value;")
            {
                bytes.back() = value;
                THEN("the first element of bytes becomes value")
                {
                    REQUIRE(bytes.data()[bytes.size() - 1] == value);
                }
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": comparisons", "[containers]")
{
    using byte_vector_base = dualis::byte_container<not_always_equal_allocator>;

    GIVEN(CLASS_UNDER_TEST " a, b;")
    {
        WHEN("result = a <=> b;")
        {
            AND_WHEN("a < b")
            {
                byte_vector_base a{0x13_b, 0x14_b};
                byte_vector_base b{0x14_b, 0x14_b};

                auto const result = a <=> b;

                THEN("result is 'less'")
                {
                    REQUIRE(result == std::strong_ordering::less);
                }
            }
            AND_WHEN("a == b")
            {
                byte_vector_base a{0x13_b, 0x14_b};
                byte_vector_base b{a};

                auto const result = a <=> b;

                THEN("result is 'equal'")
                {
                    REQUIRE(result == std::strong_ordering::equal);
                }
            }
            AND_WHEN("a > b")
            {
                byte_vector_base a{0x14_b, 0x14_b};
                byte_vector_base b{0x13_b, 0x14_b};

                auto const result = a <=> b;

                THEN("result is 'greater'")
                {
                    REQUIRE(result == std::strong_ordering::greater);
                }
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": conversions", "[containers]")
{
    using byte_vector_base = dualis::byte_container<not_always_equal_allocator>;

    GIVEN(CLASS_UNDER_TEST " bytes;")
    {
        byte_vector_base bytes{0x13_b, 0x14_b, 0x15_b};

        WHEN("span = byte_span(bytes); // const")
        {
            auto const span = dualis::byte_span(bytes);

            THEN("the span covers the data of bytes")
            {
                REQUIRE(span.data() == bytes.data());
                REQUIRE(span.size() == bytes.size());
            }
        }
        WHEN("span = writable_byte_span(bytes);")
        {
            auto const span = dualis::writable_byte_span(bytes);

            THEN("the span covers the data of bytes")
            {
                REQUIRE(span.data() == bytes.data());
                REQUIRE(span.size() == bytes.size());
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": appending", "[containers]")
{
    using byte_vector_base = dualis::byte_container<not_always_equal_allocator>;

    GIVEN(CLASS_UNDER_TEST " bytes;")
    {
        auto const expected_values = {0x13_b, 0x14_b, 0x15_b};
        byte_vector_base bytes(expected_values);
        auto const old_size = bytes.size();

        WHEN("bytes.push_back(value);")
        {
            const std::byte value = 0xff_b;
            bytes.push_back(value);
            THEN("bytes is 1 element larger")
            {
                REQUIRE(bytes.size() == old_size + 1);
            }
            THEN("value is the last element of bytes")
            {
                REQUIRE(bytes.back() == value);
            }
        }

        AND_GIVEN("a count and value")
        {
            const std::size_t count = 5;
            const std::byte value = 0xff_b;

            WHEN("bytes.append(count, value);")
            {
                bytes.append(count, value);

                THEN("bytes is count larger")
                {
                    REQUIRE(bytes.size() == old_size + count);
                }
                THEN("the last count elements are all value")
                {
                    auto const span = dualis::byte_span(bytes).subspan(old_size);
                    REQUIRE_THAT(span, EachElementIs(value));
                }
            }
        }

        AND_GIVEN("a byte pointer and a count")
        {
            auto const ilist = {0x80_b, 0x81_b};
            auto const* byte_pointer = ilist.begin();
            auto const count = ilist.size();

            WHEN("bytes.append(byte_pointer, count);")
            {
                bytes.append(byte_pointer, count);

                THEN("bytes is count larger")
                {
                    REQUIRE(bytes.size() == old_size + count);
                }
                THEN("the last count elements equal [byte_pointer, byte_pointer+count)")
                {
                    auto const span = dualis::byte_span(bytes).subspan(old_size);
                    REQUIRE_THAT(span, EqualsByteRange(std::vector<std::byte>(ilist)));
                }
                THEN("the first elements remain unchanged")
                {
                    REQUIRE_THAT(bytes, StartsWithBytes(expected_values));
                }
            }
        }

        AND_GIVEN("uint16_t value;")
        {
            const uint16_t value = 0x1234;

            WHEN("bytes.append_packed<uint16_le>(value);")
            {
                bytes.append_packed<uint16_le>(value);

                THEN("bytes is 2 longer")
                {
                    REQUIRE(bytes.size() == old_size + uint16_le::size());
                }
                THEN("the last two bytes are the little-endian representation of value")
                {
                    auto const span = dualis::byte_span(bytes).subspan(old_size);
                    REQUIRE_THAT(span, EqualsByteRange(std::vector<std::byte>{0x34_b, 0x12_b}));
                }
                THEN("the first elements remain unchanged")
                {
                    REQUIRE_THAT(bytes, StartsWithBytes(expected_values));
                }
            }
        }
        AND_GIVEN("uint16_t value1, value2;")
        {
            const uint16_t value1 = 0x1234;
            const uint16_t value2 = 0x5678;

            WHEN("bytes.append_packed<uint16_le, uint16_le>(value1, value2);")
            {
                bytes.append_packed<uint16_le, uint16_le>(value1, value2);

                THEN("bytes is 2*2 longer")
                {
                    REQUIRE(bytes.size() == old_size + uint16_le::size() * 2);
                }
                THEN("the last two bytes are the little-endian representation of value2")
                {
                    auto const span = dualis::byte_span(bytes).subspan(old_size + 2);
                    REQUIRE_THAT(span, EqualsByteRange(std::vector<std::byte>{0x78_b, 0x56_b}));
                }
                THEN("the first two new bytes are the little-endian representation of value1")
                {
                    auto const span = dualis::byte_span(bytes).subspan(old_size, 2);
                    REQUIRE_THAT(span, EqualsByteRange(std::vector<std::byte>{0x34_b, 0x12_b}));
                }
                THEN("the first elements remain unchanged")
                {
                    REQUIRE_THAT(bytes, StartsWithBytes(expected_values));
                }
            }
        }
        AND_GIVEN("std::ranges::range values; // 2 * uint16_t")
        {
            const uint16_t values[] = {0x1234, 0x5678};

            WHEN("bytes.append_packed_range<uint16_le>(values);")
            {
                bytes.append_packed_range<uint16_le>(values);

                THEN("bytes is 2*2 longer")
                {
                    REQUIRE(bytes.size() == old_size + uint16_le::size() * 2);
                }
                THEN("the last two bytes are the little-endian representation of values[1]")
                {
                    auto const span = dualis::byte_span(bytes).subspan(old_size + 2);
                    REQUIRE_THAT(span, EqualsByteRange(std::vector<std::byte>{0x78_b, 0x56_b}));
                }
                THEN("the first two new bytes are the little-endian representation of values[0]")
                {
                    auto const span = dualis::byte_span(bytes).subspan(old_size, 2);
                    REQUIRE_THAT(span, EqualsByteRange(std::vector<std::byte>{0x34_b, 0x12_b}));
                }
                THEN("the first elements remain unchanged")
                {
                    REQUIRE_THAT(bytes, StartsWithBytes(expected_values));
                }
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": insertion", "[containers]")
{
    using byte_vector_base = dualis::byte_container<not_always_equal_allocator>;

    GIVEN(CLASS_UNDER_TEST " bytes;")
    {
        byte_vector_base bytes{0x13_b, 0x14_b, 0x15_b};
        auto const old_size = bytes.size();

        AND_GIVEN("an offset")
        {
            const std::size_t offset = 1;

            AND_GIVEN("a count and a value")
            {
                const std::size_t count = 3;
                const std::byte value = 0xff_b;

                WHEN("bytes.insert(offset, count, value);")
                {
                    bytes.insert(offset, count, value);

                    THEN("bytes is count elements larger")
                    {
                        REQUIRE(bytes.size() == old_size + count);
                    }
                    THEN("the inserted elements are all value")
                    {
                        auto const span = dualis::byte_span(bytes).subspan(offset, count);
                        REQUIRE_THAT(span, EachElementIs(value));
                    }
                }
            }
        }
    }
}

SCENARIO(CLASS_UNDER_TEST ": erasing", "[containers]")
{
    using byte_vector_base = dualis::byte_container<not_always_equal_allocator>;

    GIVEN(CLASS_UNDER_TEST " bytes;")
    {
        auto const original_elements = {0x13_b, 0x14_b, 0x15_b};
        byte_vector_base bytes(original_elements);
        auto const old_size = bytes.size();

        WHEN("bytes.pop_back();")
        {
            bytes.pop_back();

            THEN("bytes is 1 element smaller")
            {
                REQUIRE(bytes.size() == old_size - 1);
            }
            THEN("bytes is equal to the first elements before erasing")
            {
                REQUIRE_THAT(original_elements, StartsWithBytes(bytes));
            }
        }

        AND_GIVEN("an offset and a count")
        {
            const std::size_t offset = 1;
            WHEN("bytes.erase(offset, count);")
            {
                AND_WHEN("count != npos")
                {
                    const std::size_t count = 1;
                    bytes.erase(offset, count);

                    THEN("bytes is count elements smaller")
                    {
                        REQUIRE(bytes.size() == old_size - count);
                    }
                    THEN("the first offset elements are unchanged")
                    {
                        auto const first_elements = std::vector<std::byte>(
                            original_elements.begin(), original_elements.begin() + offset);
                        REQUIRE_THAT(bytes, StartsWithBytes(first_elements));
                    }
                    THEN("the last (bytes.size() - offset - count) bytes are copied to offset")
                    {
                        REQUIRE_THAT(bytes,
                                     EndsWithBytes(original_elements.begin() + offset + count,
                                                   original_elements.end()));
                    }
                }
                AND_WHEN("count == npos")
                {
                    auto const count = byte_vector_base::npos;
                    bytes.erase(offset, count);

                    THEN("the size of bytes is offset")
                    {
                        REQUIRE(bytes.size() == offset);
                    }
                    THEN("bytes is equal to the first offset elements before erasing")
                    {
                        REQUIRE_THAT(original_elements, StartsWithBytes(bytes));
                    }
                }
            }
        }
    }
}
