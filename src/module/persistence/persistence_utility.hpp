/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once
#include "persistence_private.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 * utility - helper
 ***************************************************************************/

namespace utility
{
    /* enable_if */
    template<bool Expr, class TypeType = void>
    struct EnableIf {};
    template<class TypeType>
    struct EnableIf<true, TypeType> { typedef TypeType type; };

    /* is_same */
    template<typename LhsType, typename RhsType> struct IsSame
    { static const bool value = false; };
    template<typename LhsType> struct IsSame<LhsType, LhsType>
    { static const bool value = true;  };

    /* if */
    template<bool Expr, typename ThenType, typename ElseType>
    struct If;
    template<typename ThenType, typename ElseType>
    struct If< true, ThenType, ElseType> { typedef ThenType type; };
    template<typename ThenType, typename ElseType>
    struct If<false, ThenType, ElseType> { typedef ElseType type; };

    /* assert */
    template<bool expr> struct Assert;
    template<>          struct Assert<true> { typedef void type; };

    /* remove reference and const */
    template<typename T> struct RemoveCV             { typedef T type; };
    template<typename T> struct RemoveCV<T &>        { typedef T type; };
    template<typename T> struct RemoveCV<T const>    { typedef T type; };
    template<typename T> struct RemoveCV<T const &>  { typedef T type; };
}

/****************************************************************************
 * utility - typelist
 ***************************************************************************/

namespace utility { namespace tl
{
    /* end of any List */

    struct End {};

    /* List */

    template<typename Type, typename Next> struct Iter;

    template<typename Type, typename NextType, typename NextNext>
    struct Iter<Type, Iter<NextType, NextNext> >
    {
        typedef Type                     type;
        typedef Iter<NextType, NextNext> next;
    };

    template<typename Type>
    struct Iter<Type, End>
    {
        typedef Type type;
        typedef End  next;
    };

    /* search Elem from Iter, using Equ for comparing */

    template<
        typename Iter,
        typename Elem,
        template<typename Lhs, typename Elem> class Equ = IsSame
    > struct Find;

    template<
        typename BeginType,
        typename BeginNext,
        typename Elem,
        template<typename, typename> class Equ
    >
    struct Find<Iter<BeginType, BeginNext>, Elem, Equ>
    {
    private:
        template<typename Iter, bool>
        struct Impl;

        template<typename Type>
        struct Impl<Iter<Type, End>, false>
        {
            typedef End type;
        };

        template<typename Type, typename Next>
        struct Impl<Iter<Type, Next>, true>
        {
            typedef Iter<Type, Next> type;
        };

        template<typename LastType, typename Type, typename Next>
        struct Impl<Iter<LastType, Iter<Type, Next> >, false>
        {
            typedef typename Impl<
                Iter<Type, Next>, Equ<Type, Elem>::value
            >::type type;
        };

    public:
        typedef typename Impl<
            Iter<BeginType, BeginNext>, Equ<BeginType, Elem>::value
        >::type type;
    };

    template<
        typename Iter,
        typename IdType,
        IdType   id,
        template<typename Lhs, IdType> class Equ
    > struct find_by_id;

    template<
        typename BeginType,
        typename BeginNext,
        typename IdType,
        IdType   id,
        template<typename, IdType> class Equ
    >
    struct find_by_id<Iter<BeginType, BeginNext>, IdType, id, Equ>
    {
    private:
        template<typename Iter, bool>
        struct Impl;

        template<typename Type>
        struct Impl<Iter<Type, End>, false>
        {
            typedef End type;
        };

        template<typename Type, typename Next>
        struct Impl<Iter<Type, Next>, true>
        {
            typedef Iter<Type, Next> type;
        };

        template<typename LastType, typename Type, typename Next>
        struct Impl<Iter<LastType, Iter<Type, Next> >, false>
        {
            typedef typename Impl<
                Iter<Type, Next>, Equ<Type, id>::value
            >::type type;
        };

    public:
        typedef typename Impl<
            Iter<BeginType, BeginNext>, Equ<BeginType, id>::value
        >::type type;
    };

    /* whether List contain elem? */

    template<
        typename Iter,
        typename Elem,
        template<typename Lhs, typename Rhs> class Equ = IsSame
    > struct Contain;

    template<
        typename Type,
        typename Next,
        typename Elem,
        template<typename Lhs, typename Rhs> class Equ
    >
    struct Contain<Iter<Type, Next>, Elem, Equ>
    {
    private:
        typedef typename Find<Iter<Type, Next>, Elem, Equ>::type result;
    public:
        static const bool value = !IsSame<result, End>::value;
    };

    /* intercept the List if find `end` */

    template<typename Iter> struct Cut;

    template<
        typename BeginType,
        typename BeginNext
    > struct Cut< Iter<BeginType, BeginNext> >
    {
    private:
        template<typename Iter>
        struct Impl;


        template<typename Type, typename Next>
        struct Impl< Iter<Type, Next> >
        {
            typedef Iter<Type, typename Impl<Next>::type > type;
        };

        template<typename Next>
        struct Impl< Iter<End, Next> >
        {
            typedef End type;
        };

        template<typename Type>
        struct Impl< Iter<Type, End> >
        {
            typedef Iter<Type, End> type;
        };

    public:
        typedef typename Impl< Iter<BeginType, BeginNext> >::type type;
    };

    /* make a List */

    template<
        class T00,
        class T01 = End, class T02 = End, class T03 = End, class T04 = End,
        class T05 = End, class T06 = End, class T07 = End, class T08 = End,
        class T09 = End, class T10 = End, class T11 = End, class T12 = End>
    struct MakeList
    {
        typedef typename Cut<
            Iter<T00,
            Iter<T01, Iter<T02, Iter<T03, Iter<T04, Iter<T05, Iter<T06,
            Iter<T07, Iter<T08, Iter<T09, Iter<T10, Iter<T11, Iter<T12,
            End > > > > > > > > > > > > > >::type

            type;
    };
} }

/****************************************************************************
 * utility - type
 ***************************************************************************/

namespace utility
{
    /* IsPrimitive */
    template<typename T> struct IsPrimitive
    {
    private:
        typedef typename
            tl::MakeList<
               bool,
             int8_t,  int16_t,  int32_t,  int64_t,
            uint8_t, uint16_t, uint32_t, uint64_t,
              float,   double
            >::type
        list;
    public:
        static const bool value = tl::Contain<list, T>::value;
    };

    /* IsSigned */
    template<typename T> struct IsSigned
    {
    private:
        typedef typename tl::MakeList<
            int8_t, int16_t, int32_t, int64_t
        >::type list;
    public:
        static const bool value = tl::Contain<list, T>::value;
    };

    /* IsUnsigned */
    template<typename T> struct IsUnsigned
    {
    private:
        typedef typename tl::MakeList<
            uint8_t, uint16_t, uint32_t, uint64_t
        >::type list;
    public:
        static const bool value = tl::Contain<list, T>::value;
    };

    /* IsInteger */
    template<typename T> struct IsInteger
    {
        static const bool value
            =    IsSigned<T>::value
            || IsUnsigned<T>::value
            ;
    };

    /* IsReal */
    template<typename T> struct IsReal
    {
    private:
        typedef typename tl::MakeList<
            float, double
        >::type list;
    public:
        static const bool value = tl::Contain<list, T>::value;
    };

    /* is_type_defined */
    template<typename Type> struct IsComplete
    {
    private:
        template<typename, typename = void>
        struct Impl
        {
            static const bool value = false;
        };
        template<typename T>
        struct Impl<T, typename EnableIf<(sizeof(T) > 0)>::type>
        {
            static const bool value = true;
        };

    public:
        static const bool value = Impl<Type>::value;
    };
}

/****************************************************************************
 * function
 ***************************************************************************/

namespace utility
{
    template<typename L, typename R> inline
    typename EnableIf<
        IsPrimitive<typename RemoveCV<L>::type>::value &&
        IsSame<
            typename RemoveCV<L>::type,
            typename RemoveCV<R>::type
        >::value
        , typename RemoveCV<L>::type
    >::type
        max(L lhs, R rhs)
    {
        return (lhs < rhs) ? rhs : lhs;
    }

    inline uint8_t max(uint8_t lhs, uint8_t rhs)
    {
        return (lhs < rhs) ? rhs : lhs;
    }
}

CV_FS_PRIVATE_END
