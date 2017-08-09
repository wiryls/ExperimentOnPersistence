/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once

#include "persistence_private.hpp"

CV_FS_PRIVATE_BEGIN

namespace internal
{
    /* max value of integer type.
     * PS. numeric_limits<>::max() are not constant expressions before C++0x.
     */
    template<typename integer_t> struct intmax_t {};

    template<> struct intmax_t<uint8_t>
    { static const uint8_t  value = uint8_t (~uint8_t ()); };
    template<> struct intmax_t<uint16_t>
    { static const uint16_t value = uint16_t(~uint16_t()); };
    template<> struct intmax_t<uint32_t>
    { static const uint32_t value = uint32_t(~uint32_t()); };
    template<> struct intmax_t<uint64_t>
    { static const uint64_t value = uint64_t(~uint64_t()); };

    template<> struct intmax_t<int8_t>
    { static const int8_t  value = (uint8_t (int8_t (-1)) >> 1); };
    template<> struct intmax_t<int16_t>
    { static const int16_t value = (uint16_t(int16_t(-1)) >> 1); };
    template<> struct intmax_t<int32_t>
    { static const int32_t value = (uint32_t(int32_t(-1)) >> 1); };
    template<> struct intmax_t<int64_t>
    { static const int64_t value = (uint64_t(int64_t(-1)) >> 1); };

    /* fibonacci sequence generator */

    template<typename integer_t, size_t x> struct fibonacci_generator_t
    {
    private:
        static const integer_t max
            = intmax_t<integer_t>::value;
        static const integer_t lhs
            = fibonacci_generator_t<integer_t, x - 1>::value;
        static const integer_t rhs
            = fibonacci_generator_t<integer_t, x - 2>::value;

    public:
        static const integer_t value
            = max - lhs > rhs
            ? lhs + rhs
            : max
            ;
    };

    template<typename integer_t> struct fibonacci_generator_t<integer_t, 0>
    {
        static const integer_t value = integer_t(0);
    };
    template<typename integer_t> struct fibonacci_generator_t<integer_t, 1>
    {
        static const integer_t value = integer_t(1);
    };
    template<> struct fibonacci_generator_t<uint64_t, 47>
    {
        static const uint64_t value = 2971215073ULL;
    };
    template<> struct fibonacci_generator_t<uint64_t, 48>
    {
        static const uint64_t value = 4807526976ULL;
    };

    /* find the specified number */

    template
    <
        size_t x,
        template<size_t> class validator_t,
        bool = validator_t<x>::value
    >
    struct finder_t
    {
        static const size_t value
            = finder_t<x + 1, validator_t, validator_t<x + 1>::value>::value;
    };

    template<size_t x, template<size_t> class validator_t>
    struct finder_t<x, validator_t, true>
    {
        static const size_t value = x;
    };

    /* index of the last fibonacci number in {uint_t} */

    template<typename uint_t>
    struct fibonacci_max_t
    {
    private:
        template<size_t x> struct cmp_t
        {
            static const bool value
                = intmax_t<uint_t>::value
                - fibonacci_generator_t<uint_t, x + 1>::value
                < fibonacci_generator_t<uint_t, x    >::value
                ;
        };

    public:
        static const size_t value = finder_t<0, cmp_t>::value + 2;
    };

    /* index of the left fibonacci number of y */

    template<typename uint_t, uint_t y>
    struct fibonacci_left_t
    {
    private:
        template<size_t x> struct cmp_t
        {
            static const bool value
                =  fibonacci_generator_t<uint_t, x>::value > y
                || fibonacci_max_t<uint_t>::value == x;
        };

    public:
        static const size_t value = finder_t<1, cmp_t>::value - 1;
    };

    /* index of the right fibonacci number of y */

    template<typename uint_t, uint_t y>
    struct fibonacci_right_t
    {
    private:
        template<size_t x> struct cmp_t
        {
            static const bool value
                = fibonacci_generator_t<uint_t, x>::value >= y;
        };

    public:
        static const size_t value = finder_t<2, cmp_t>::value;
    };

    /* find the last fibonacci number limited by uint_t */

    template<typename uint_t> struct fibonacci_t
    {
    public:
        static const size_t size = fibonacci_max_t<uint_t>::value;
        static const uint_t data[size];

        template<size_t i> struct f
        { static const uint_t y = fibonacci_generator_t<uint_t, i>::value; };
    };
}

template<typename index_type, typename value_type>
struct runtime_fibonacci_t
{
    typedef internal::fibonacci_t<value_type> array;

    static index_type left(value_type y)
    {
        typedef value_type const * const_iterator;

        const_iterator beg = array::data;
        const_iterator end = beg + array::size;
        const_iterator cur = beg + 1;

        while (cur != end && *cur <= y)
            ++cur;

        --cur;
        return static_cast<index_type>(cur - beg);
    }

    static index_type right(value_type y)
    {
        /* mostly, y is a small number. */

        typedef value_type const * const_iterator;

        const_iterator beg = array::data;
        const_iterator end = beg + array::size;
        const_iterator cur = beg + 2;

        while (cur != end && *cur < y)
            ++cur;

        return static_cast<index_type>(cur - beg);
    }

    static value_type at(index_type x)
    {
        if (x >= array::size)
            return internal::intmax_t<value_type>::value;
        else
            return array::data[x];
    }
};

template<typename index_type, typename value_type>
struct compiletime_fibonacci_t
{
    typedef internal::fibonacci_t<value_type> array;

    template<value_type y>
    struct left
    {
        static const index_type value
            = internal::fibonacci_left_t<value_type, y>::value
            ;
    };

    template<value_type y>
    struct right
    {
        static const index_type value
            = internal::fibonacci_right_t<value_type, y>::value
            ;
    };

    template<index_type x>
    struct at
    {
        static const value_type value
            = internal::fibonacci_generator_t<value_type, x>::value
            ;
    };
};

CV_FS_PRIVATE_END
