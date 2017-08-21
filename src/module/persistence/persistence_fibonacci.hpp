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
    template<typename IntType> struct IntMax {};

    template<> struct IntMax<uint8_t>
    { static const uint8_t  value = uint8_t (~uint8_t ()); };
    template<> struct IntMax<uint16_t>
    { static const uint16_t value = uint16_t(~uint16_t()); };
    template<> struct IntMax<uint32_t>
    { static const uint32_t value = uint32_t(~uint32_t()); };
    template<> struct IntMax<uint64_t>
    { static const uint64_t value = uint64_t(~uint64_t()); };

    template<> struct IntMax<int8_t>
    { static const int8_t  value = (uint8_t (int8_t (-1)) >> 1); };
    template<> struct IntMax<int16_t>
    { static const int16_t value = (uint16_t(int16_t(-1)) >> 1); };
    template<> struct IntMax<int32_t>
    { static const int32_t value = (uint32_t(int32_t(-1)) >> 1); };
    template<> struct IntMax<int64_t>
    { static const int64_t value = (uint64_t(int64_t(-1)) >> 1); };

    /* fibonacci sequence generator */

    template<typename IntType, size_t x> struct FibonacciGenerator
    {
    private:
        static const IntType max
            = IntMax<IntType>::value;
        static const IntType lhs
            = FibonacciGenerator<IntType, x - 1>::value;
        static const IntType rhs
            = FibonacciGenerator<IntType, x - 2>::value;

    public:
        static const IntType value
            = max - lhs > rhs
            ? lhs + rhs
            : max
            ;
    };

    template<typename IntType> struct FibonacciGenerator<IntType, 0>
    {
        static const IntType value = IntType(0);
    };
    template<typename IntType> struct FibonacciGenerator<IntType, 1>
    {
        static const IntType value = IntType(1);
    };
    template<> struct FibonacciGenerator<uint64_t, 47>
    {
        static const uint64_t value = 2971215073ULL;
    };
    template<> struct FibonacciGenerator<uint64_t, 48>
    {
        static const uint64_t value = 4807526976ULL;
    };

    /* find the specified number */

    template
    <
        size_t x,
        template<size_t> class Validator,
        bool = Validator<x>::value
    >
    struct Finder
    {
        static const size_t value
            = Finder<x + 1, Validator, Validator<x + 1>::value>::value;
    };

    template<size_t x, template<size_t> class Validator>
    struct Finder<x, Validator, true>
    {
        static const size_t value = x;
    };

    /* index of the last fibonacci number in {UintType} */

    template<typename UintType>
    struct FibonacciMax
    {
    private:
        template<size_t x> struct cmp_t
        {
            static const bool value
                = IntMax<UintType>::value
                - FibonacciGenerator<UintType, x + 1>::value
                < FibonacciGenerator<UintType, x    >::value
                ;
        };

    public:
        static const size_t value = Finder<0, cmp_t>::value + 2;
    };

    /* index of the left fibonacci number of y */

    template<typename UintType, UintType y>
    struct FibonacciLeft
    {
    private:
        template<size_t x> struct cmp_t
        {
            static const bool value
                =  FibonacciGenerator<UintType, x>::value > y
                || FibonacciMax<UintType>::value == x;
        };

    public:
        static const size_t value = Finder<1, cmp_t>::value - 1;
    };

    /* index of the right fibonacci number of y */

    template<typename UintType, UintType y>
    struct FibonacciRight
    {
    private:
        template<size_t x> struct cmp_t
        {
            static const bool value
                = FibonacciGenerator<UintType, x>::value >= y;
        };

    public:
        static const size_t value = Finder<2, cmp_t>::value;
    };

    /* find the last fibonacci number limited by UintType */

    template<typename UintType> struct Fibonacci
    {
    public:
        static const size_t size = FibonacciMax<UintType>::value;
        static const UintType data[size];

        template<size_t i> struct f
        { static const UintType y = FibonacciGenerator<UintType, i>::value; };
    };
}

template<typename IndexType, typename ValueType>
struct RuntimeFibonacci
{
    typedef internal::Fibonacci<ValueType> Array;

    static IndexType left(ValueType y)
    {
        typedef ValueType const * const_iterator;

        const_iterator beg = Array::data;
        const_iterator end = beg + Array::size;
        const_iterator cur = beg + 1;

        while (cur != end && *cur <= y)
            ++cur;

        --cur;
        return static_cast<IndexType>(cur - beg);
    }

    static IndexType right(ValueType y)
    {
        /* mostly, y is a small number. */

        typedef ValueType const * const_iterator;

        const_iterator beg = Array::data;
        const_iterator end = beg + Array::size;
        const_iterator cur = beg + 2;

        while (cur != end && *cur < y)
            ++cur;

        return static_cast<IndexType>(cur - beg);
    }

    static ValueType at(IndexType x)
    {
        if (x >= Array::size)
            return internal::IntMax<ValueType>::value;
        else
            return Array::data[x];
    }
};

template<typename IndexType, typename ValueType>
struct CompiletimeFibonacci
{
    typedef internal::Fibonacci<ValueType> Array;

    template<ValueType y>
    struct left
    {
        static const IndexType value
            = internal::FibonacciLeft<ValueType, y>::value
            ;
    };

    template<ValueType y>
    struct right
    {
        static const IndexType value
            = internal::FibonacciRight<ValueType, y>::value
            ;
    };

    template<IndexType x>
    struct at
    {
        static const ValueType value
            = internal::FibonacciGenerator<ValueType, x>::value
            ;
    };
};

CV_FS_PRIVATE_END
