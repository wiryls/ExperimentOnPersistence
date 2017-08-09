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
    template<bool Expr, class Type_T = void>
    struct enable_if_t {};
    template<class Type_T>
    struct enable_if_t<true, Type_T> { typedef Type_T type; };

    /* is_same */
    template<typename Lhs_T, typename Rhs_T> struct is_same_t
    { static const bool value = false; };
    template<typename Lhs_T>                 struct is_same_t<Lhs_T, Lhs_T>
    { static const bool value = true;  };

    /* if */
    template<bool Expr, typename Then_T, typename Else_T>
    struct if_t;
    template<typename Then_T, typename Else_T>
    struct if_t< true, Then_T, Else_T> { typedef Then_T type; };
    template<typename Then_T, typename Else_T>
    struct if_t<false, Then_T, Else_T> { typedef Else_T type; };

    /* assert */
    template<bool expr> struct assert_t;
    template<>          struct assert_t<true> { typedef void type; };

    /* remove reference and const */
    template<typename T> struct remove_cv             { typedef T type; };
    template<typename T> struct remove_cv<T &>        { typedef T type; };
    template<typename T> struct remove_cv<T const>    { typedef T type; };
    template<typename T> struct remove_cv<T const &>  { typedef T type; };
}

/****************************************************************************
 * utility - typelist
 ***************************************************************************/

namespace utility { namespace tl
{
    /* end of any list */

    struct end {};

    /* list */

    template<typename Type, typename Next> struct iter;

    template<typename Type, typename NType, typename NNext>
    struct iter<Type, iter<NType, NNext> >
    {
        typedef Type               type;
        typedef iter<NType, NNext> next;
    };

    template<typename Type>
    struct iter<Type, end>
    {
        typedef Type type;
        typedef end  next;
    };

    /* search Elem from Iter, using Equ for comparing */

    template<
        typename Iter,
        typename Elem,
        template<typename Lhs, typename Elem> class Equ = is_same_t
    > struct find;

    template<
        typename BeginType,
        typename BeginNext,
        typename Elem,
        template<typename, typename> class Equ
    >
    struct find<iter<BeginType, BeginNext>, Elem, Equ>
    {
    private:
        template<typename Iter, bool>
        struct impl;

        template<typename Type>
        struct impl<iter<Type, end>, false>
        {
            typedef end type;
        };

        template<typename Type, typename Next>
        struct impl<iter<Type, Next>, true>
        {
            typedef iter<Type, Next> type;
        };

        template<typename LastType, typename Type, typename Next>
        struct impl<iter<LastType, iter<Type, Next> >, false>
        {
            typedef typename impl<
                iter<Type, Next>, Equ<Type, Elem>::value
            >::type type;
        };

    public:
        typedef typename impl<
            iter<BeginType, BeginNext>, Equ<BeginType, Elem>::value
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
    struct find_by_id<iter<BeginType, BeginNext>, IdType, id, Equ>
    {
    private:
        template<typename Iter, bool>
        struct impl;

        template<typename Type>
        struct impl<iter<Type, end>, false>
        {
            typedef end type;
        };

        template<typename Type, typename Next>
        struct impl<iter<Type, Next>, true>
        {
            typedef iter<Type, Next> type;
        };

        template<typename LastType, typename Type, typename Next>
        struct impl<iter<LastType, iter<Type, Next> >, false>
        {
            typedef typename impl<
                iter<Type, Next>, Equ<Type, id>::value
            >::type type;
        };

    public:
        typedef typename impl<
            iter<BeginType, BeginNext>, Equ<BeginType, id>::value
        >::type type;
    };

    /* whether list contain elem? */

    template<
        typename Iter,
        typename Elem,
        template<typename Lhs, typename Rhs> class Equ = is_same_t
    > struct contain;

    template<
        typename Type,
        typename Next,
        typename Elem,
        template<typename Lhs, typename Rhs> class Equ
    >
    struct contain<iter<Type, Next>, Elem, Equ>
    {
    private:
        typedef typename find<iter<Type, Next>, Elem, Equ>::type result;
    public:
        static const bool value = !is_same_t<result, end>::value;
    };

    /* intercept the list if find `end` */

    template<typename Iter> struct cut;

    template<
        typename BeginType,
        typename BeginNext
    > struct cut< iter<BeginType, BeginNext> >
    {
    private:
        template<typename Iter>
        struct impl;


        template<typename Type, typename Next>
        struct impl< iter<Type, Next> >
        {
            typedef iter<Type, typename impl<Next>::type > type;
        };

        template<typename Next>
        struct impl< iter<end, Next> >
        {
            typedef end type;
        };

        template<typename Type>
        struct impl< iter<Type, end> >
        {
            typedef iter<Type, end> type;
        };

    public:
        typedef typename impl< iter<BeginType, BeginNext> >::type type;
    };

    /* make a list */

    template<
        class T00,
        class T01 = end, class T02 = end, class T03 = end, class T04 = end,
        class T05 = end, class T06 = end, class T07 = end, class T08 = end,
        class T09 = end, class T10 = end, class T11 = end, class T12 = end>
    struct make_list
    {
        typedef typename cut<
            iter<T00,
            iter<T01, iter<T02, iter<T03, iter<T04, iter<T05, iter<T06,
            iter<T07, iter<T08, iter<T09, iter<T10, iter<T11, iter<T12,
            end > > > > > > > > > > > > > >::type

            type;
    };
} }

/****************************************************************************
 * utility - type
 ***************************************************************************/

namespace utility
{
    /* isprimitive_t */
    template<typename T> struct isprimitive_t
    {
    private:
        typedef typename
            tl::make_list<
               bool,
             int8_t,  int16_t,  int32_t,  int64_t,
            uint8_t, uint16_t, uint32_t, uint64_t,
              float,   double
            >::type
        list;
    public:
        static const bool value = tl::contain<list, T>::value;
    };

    /* issigned_t */
    template<typename T> struct issigned_t
    {
    private:
        typedef typename tl::make_list<
            int8_t, int16_t, int32_t, int64_t
        >::type list;
    public:
        static const bool value = tl::contain<list, T>::value;
    };

    /* isunsigned_t */
    template<typename T> struct isunsigned_t
    {
    private:
        typedef typename tl::make_list<
            uint8_t, uint16_t, uint32_t, uint64_t
        >::type list;
    public:
        static const bool value = tl::contain<list, T>::value;
    };

    /* isinteger_t */
    template<typename T> struct isinteger_t
    {
        static const bool value
            =    issigned_t<T>::value
            || isunsigned_t<T>::value
            ;
    };

    /* isreal_t */
    template<typename T> struct isreal_t
    {
    private:
        typedef typename tl::make_list<
            float, double
        >::type list;
    public:
        static const bool value = tl::contain<list, T>::value;
    };

    /* is_type_defined */
    template<typename Type> struct iscomplete_t
    {
    private:
        template<typename, typename = void>
        struct impl
        {
            static const bool value = false;
        };
        template<typename T>
        struct impl<T, typename enable_if_t<(sizeof(T) > 0)>::type>
        {
            static const bool value = true;
        };

    public:
        static const bool value = impl<Type>::value;
    };
}

/****************************************************************************
 * function
 ***************************************************************************/

namespace utility
{
    template<typename L, typename R> inline
    typename enable_if_t<
        isprimitive_t<typename remove_cv<L>::type>::value &&
        is_same_t<
            typename remove_cv<L>::type,
            typename remove_cv<R>::type
        >::value
        , typename remove_cv<L>::type
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
