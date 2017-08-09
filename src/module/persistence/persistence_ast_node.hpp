/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once

#include "persistence_private.hpp"
#include "persistence_utility.hpp"
#include "persistence_string.hpp"
#include "persistence_fibonacci.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 * exception
 ***************************************************************************/

namespace exception
{
    using chars::soss_t;
    using chars::fmt;

    static inline void node_type_not_match(int now, int ept, POS_TYPE_)
    {
        error(0,
            ( soss_t<>()
                * "node type is `" | fmt<16>(now) | "`, "
                | "but not `" | fmt<16>(ept) | '`'
            ), POS_ARGS_
        );
    }

    static inline void node_type_out_of_range(int now, POS_TYPE_)
    {
        error(0,
            ( soss_t<>()
                * "node type `" | fmt<16>(now) | "` is out of range "
            ), POS_ARGS_
        );
    }
}

/***************************************************************************
 * Declaration
 ***************************************************************************/

namespace ast
{
    /************************************************************************
     * node type
     ***********************************************************************/

    //! Enum for node type.
    /*! A node is something like a variant. It can be following types. */
    enum tag_t
    {
        NIL = 0, //!< empty    [scalar][default]

        I64,     //!< int64    [scalar]
        DBL,     //!< double   [scalar]
        STR,     //!< string   [container]
        SEQ,     //!< sequence [container]
        MAP      //!< map      [container]
    };

    /************************************************************************
     * node traits
     ***********************************************************************/

    /** @brief Bind some types or methods to TAG. They make generic
    programming easier.
    */
    template<typename node_type, tag_t TAG> struct traits;
}

namespace ast
{
    /** @brief `node_t` is the node type of an abstract syntax tree,
    something like variant. It can be null, int64_t, double, string,
    sequence, map.

    Some features:
     - always 16 bytes in x86 and x64,
     - trivial and standard layout (plain old data),
     - wide char string support,
     - short string optimization,
     - growth factor of containers == 1.618,
     - memory pool needed.
    */
    template<typename char_t>
    union node_t
    {
    public:

        /** @brief `pair_t` is used in container map.

        Note: It is safe to cast `pair_t[n]` to `node_t[2*n]`. So we can use
        allocator<node_t> for `pair_t`.
        */
        typedef node_t   pair_t[2];
        
        /** @brief Define size_type. Member `.size` is at most an uint32_t
        in order to keep `node_t` always 16 bytes.
        */
        typedef uint32_t size_type;

        /** @brief About fibonacci sequence.

        Capacity of a container is stored as an index of fibonacci sequence.
        So we need look up fibonacci sequence at runtime.
        */
        typedef runtime_fibonacci_t<uint8_t, size_type> cap;

        /** @brief Make traits `friend`.
        */
        template<typename, tag_t> friend struct traits;

    public:

        /** @brief Construct this node.

        `node_t` has no constructors. Use this method to construct node.
        Note: there may be memory leak if node has been constructed.

        @param pool A collection of allocators. See class `pool_t`.
        */
        template<typename pool_t> inline
        void construct(pool_t & pool);

        /** @overload
        @param tag  Type of node to be constructed. See enum `tag_t`.
        @param pool A collection of allocators. See class `pool_t`.
        */
        template<typename pool_t> inline
        void construct(tag_t tag, pool_t & pool);

        /** @brief Destruct this node. Released memory if used.
        @param pool A collection of allocators. See class `pool_t`.
        */
        template<typename pool_t> inline
        void destruct(pool_t & pool);

        /** @brief `*this = rhs`.
        @param rhs  node to copy.
        @param pool A collection of allocators. See class `pool_t`.
        */
        template<typename pool_t> inline
        void copy(node_t const & rhs, pool_t & pool);

        /** @brief `*this = move(rhs)`.
        @param rhs  node to move.
        @param pool A collection of allocators. See class `pool_t`.
        */
        template<typename pool_t> inline
        void move(node_t & rhs, pool_t & pool);

        /** @brief Swap `*this` with rhs.
        @param rhs node to swap.
        */
        inline void swap(node_t & rhs);

        /** @brief Compare `*this` with rhs.
        @param rhs node to compare.
        @return true if *this equal to rhs, false if not.
        */
        inline bool equal(node_t const & rhs) const;

        /** @brief Get the type of this node.
        @return tag of this node. See enum `tag_t`
        */
        inline tag_t type() const;

        /** @overload
        @param pool A collection of allocators. See class `pool_t`.
        */
        template<tag_t TAG, typename pool_t> inline
        void
        construct(pool_t & pool);

        /** @brief Set value of node.

        Destruct, re-construct and assign.
        [Need to specify the TAG]
        @param val  `int64_t` or `double` value.
        @param pool A collection of allocators. See class `pool_t`.
        */
        template<tag_t TAG, typename pool_t> inline
        void
        set
        (typename traits<node_t, TAG>::const_reference val, pool_t & pool);

        /** @overload
        @param beg  Beginning of elements to copy.
        @param end  End of elements to copy. Range: [beg, end)
        @param pool A collection of allocators. See class `pool_t`.
        */
        template<tag_t TAG, typename pool_t> inline
        void
        set
        (
            typename traits<node_t, TAG>::container::const_iterator beg,
            typename traits<node_t, TAG>::container::const_iterator end,
            pool_t & pool
        );

        /** @brief Get `int64_t` or `double` value of node.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Const reference of value.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::const_reference
        val() const;

        /** @overload
        @return Reference of value.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::reference
        val();

        /** @brief Get the size of a built-in container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Size (uint32_t).
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::size_type
        size() const;

        /** @brief Get the capacity of a built-in container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Capacity (uint32_t).
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::size_type
        capacity() const;

        /** @brief Get the rawdata of a built-in container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return A pointer to rawdata.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::const_pointer
        raw() const;

        /** @brief Add a copy of an element to the end of built-in container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param val  An element to copy.
        @param pool A collection of allocators. See class `pool_t`.
        */
        template<tag_t TAG, typename pool_t> inline
        typename traits<node_t, TAG>::container::void_type
        push_back
        (
            typename traits<node_t, TAG>::container::const_reference val,
            pool_t & pool
        );

        /** @brief Move an element to the end of built-in container.

        After `move_back`, the element will be cleared.
        Note: Do not use `node.move_back<SEQ>(node, pool)`!
              Node will set NIL without being destructed.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param val  An element to move.
        @param pool A collection of allocators. See class `pool_t`.
        */
        template<tag_t TAG, typename pool_t> inline
        typename traits<node_t, TAG>::container::void_type
        move_back
        (
            typename traits<node_t, TAG>::container::reference val,
            pool_t & pool
        );

        /** @brief Delete an element of built-in container. Do nothing
        if the element does not belong to container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param iter An element to delete.
        @param pool A collection of allocators. See class `pool_t`.
        @return the following element of deleted element.
        */
        template<tag_t TAG, typename pool_t> inline
        typename traits<node_t, TAG>::container::iterator
        erase
        (
            typename traits<node_t, TAG>::container::const_iterator iter,
            pool_t & pool
        );

        /** @overload
        @param begin Beginning of elements to delete.
        @param end   End of elements to delete. Range: [begin, end)
        @param pool  A collection of allocators. See class `pool_t`.
        @return the following element of deleted elements.
        */
        template<tag_t TAG, typename pool_t> inline
        typename traits<node_t, TAG>::container::iterator
        erase
        (
            typename traits<node_t, TAG>::container::const_iterator begin,
            typename traits<node_t, TAG>::container::const_iterator end,
            pool_t & pool
        );

        /** @brief Delete the last element of built-in container.
        Do nothing if empty.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param pool  A collection of allocators. See class `pool_t`.
        */
        template<tag_t TAG, typename pool_t> inline
        typename traits<node_t, TAG>::container::void_type
        pop_back(pool_t & pool);

        /** @brief Delete all element of built-in container.
        
        Note: Won't release memory. Use `destruct` and `construct` if needed.
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param pool  A collection of allocators. See class `pool_t`.
        */
        template<tag_t TAG, typename pool_t> inline
        typename traits<node_t, TAG>::container::void_type
        clear(pool_t & pool);

        /** @brief Get first element of built-in container.
        
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Const iterator of the first element.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::const_iterator
        begin() const;

        /** @overload
        @return Iterator of the first element.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::iterator
        begin();

        /** @brief Get the next one of the last element of built-in
        container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Const iterator.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::const_iterator
        end() const;

        /** @overload
        @return Iterator.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::iterator
        end();

        /** @brief Get last element of built-in container.
        
        Note: At present, it is a normal iterator instead of a real reverse
              iterator.
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Const iterator of the last element.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::const_reverse_iterator
        rbegin() const;

        /** @overload
        @return Iterator of the last element.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::reverse_iterator
        rbegin();

        /** @brief Get the previous one of the first element of built-in
        container.
        
        Note: At present, it is a normal iterator instead of a real reverse
              iterator.
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Const iterator of the previous one of the first element.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::const_reverse_iterator
        rend() const;

        /** @overload
        @return Iterator of the previous one of the first element.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::reverse_iterator
        rend();

        /** @brief Search an element by index.
        
        Return end<TAG>() if not found.
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param index Index of an element.
        @return Const iterator of the element.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::const_iterator
        at(typename traits<node_t, TAG>::container::index_type index) const;

        /** @overload
        @param index Index of an element.
        @return Iterator of the element.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::iterator
        at(typename traits<node_t, TAG>::container::index_type index);

        /** @brief Search an element of by key.
        
        Return end<TAG>() if not found.
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param key Key to search. Usually it is the first element of a pair.
        @return Const iterator of the element.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::const_iterator
        find
        (typename traits<node_t, TAG>::container::key_const_reference key)
        const;

        /** @overload
        @param key Key to search. Usually it is the first element of a pair.
        @return Iterator of the element.
        */
        template<tag_t TAG> inline
        typename traits<node_t, TAG>::container::iterator
        find
        (typename traits<node_t, TAG>::container::key_const_reference key);

    private:

        typename traits<node_t, NIL>::layout nil; //!< Memory layout of NIL
        typename traits<node_t, I64>::layout i64; //!< Memory layout of I64
        typename traits<node_t, DBL>::layout dbl; //!< Memory layout of DBL
        typename traits<node_t, STR>::layout str; //!< Memory layout of STR
        typename traits<node_t, SEQ>::layout seq; //!< Memory layout of SEQ
        typename traits<node_t, MAP>::layout map; //!< Memory layout of MAP
    };

}

/***************************************************************************
 * Implementataion
 ***************************************************************************/

namespace ast
{
    template<typename char_t> union node_t;

    /************************************************************************
    * NIL
    ***********************************************************************/

    template<typename char_t>
    struct traits<node_t<char_t>, NIL>
    {
    public:
        static const tag_t TAG = NIL;

    public:
        typedef node_t<char_t> node_t;

    public:
        struct layout
        {
            uint8_t pad[15];
            uint8_t tag;
        };

    public:
        template<typename pool_t>
        static inline
        void
        construct(node_t & node, pool_t & /*pool*/)
        {
            node.nil.tag = TAG;
        }
        template<typename pool_t>
        static inline
        void
        destruct(node_t & /*node*/, pool_t & /*pool*/)
        {
            ;
        }
        template<typename pool_t>
        static inline
        void
        copy(node_t & lhs, node_t const & /*rhs*/, pool_t & pool)
        {
            construct(lhs, pool);
        }
        static inline
        bool
        equal(node_t const & /*lhs*/, node_t const & /*rhs*/)
        {
            return true;
        }
    };

    /************************************************************************
     * I64
     ***********************************************************************/

    template<typename char_t>
    struct traits<node_t<char_t>, I64>
    {
    public:
        static const tag_t TAG = I64;

    public:
        typedef node_t<char_t>     node_t;
        typedef int64_t            value_type;
        typedef value_type       & reference;
        typedef value_type const & const_reference;

    public:
        struct layout
        {
            int64_t val;
            uint8_t pad[7];
            uint8_t tag;
        };

    public:
        template<typename pool_t>
        static inline
        void
        construct(node_t & node, pool_t & /*pool*/)
        {
            node.nil.tag = TAG;
            val(node) = value_type();
        }
        template<typename pool_t>
        static inline
        void
        destruct(node_t & node, pool_t & /*pool*/)
        {
            node.nil.tag = NIL;
        }
        template<typename pool_t>
        static inline
        void
        copy(node_t & lhs, node_t const & rhs, pool_t & /*pool*/)
        {
            val(lhs) = val(rhs);
            lhs.nil.tag = TAG;
        }
        static inline
        bool
        equal(node_t const & lhs, node_t const & rhs)
        {
            return (val(lhs) == val(rhs));
        }
        static inline
        const_reference
        val(node_t const & node)
        {
            return node.i64.val;
        }
        static inline
        reference
        val(node_t & node)
        {
            return const_cast<reference>
                (val(static_cast<node_t const &>(node)));
        }
    };

    /************************************************************************
     * DBL
     ***********************************************************************/

    template<typename char_t>
    struct traits<node_t<char_t>, DBL>
    {
    public:
        static const tag_t TAG = DBL;

    public:
        typedef node_t<char_t>     node_t;
        typedef double             value_type;
        typedef value_type       & reference;
        typedef value_type const & const_reference;

    public:
        struct layout
        {
            double  val;
            uint8_t pad[7];
            uint8_t tag;
        };

    public:
        template<typename pool_t>
        static inline
        void
        construct(node_t & node, pool_t & /*pool*/)
        {
            node.nil.tag = TAG;
            val(node) = value_type();
        }
        template<typename pool_t>
        static inline
        void
        destruct(node_t & node, pool_t & /*pool*/)
        {
            node.nil.tag = NIL;
        }
        template<typename pool_t>
        static inline
        void
        copy(node_t & lhs, node_t const & rhs, pool_t & /*pool*/)
        {
            val(lhs) = val(rhs);
            lhs.nil.tag = TAG;
        }
        static inline
        bool
        equal(node_t const & lhs, node_t const & rhs)
        {
            return (val(lhs) == val(rhs));
        }
        static inline
        const_reference
        val(node_t const & node)
        {
            return node.dbl.val;
        }
        static inline
        reference
        val(node_t & node)
        {
            return const_cast<reference>
                (val(static_cast<node_t const &>(node)));
        }
    };

    /************************************************************************
     * STR
     ***********************************************************************/

    template<typename char_t>
    struct traits<node_t<char_t>, STR>
    {
    public:
        static const tag_t TAG = STR;

    public:
        typedef node_t<char_t>             node_t;
        typedef typename node_t::size_type size_type;

        struct container
        {
            typedef char_t             value_type;
            typedef value_type       *       pointer;
            typedef value_type const * const_pointer;
            typedef value_type       &       reference;
            typedef value_type const & const_reference;
            typedef       pointer                    iterator;
            typedef const_pointer              const_iterator;
            typedef       pointer            reverse_iterator; /* TODO: */
            typedef const_pointer      const_reverse_iterator; /* TODO: */
            typedef size_type          index_type;
            typedef void               void_type;

            template<typename pool_t>
            static inline void copy
            (reference lhs, const_reference rhs, pool_t & /*pool*/)
            {
                lhs = rhs;
            }
            template<typename pool_t>
            static inline void move
            (reference lhs, reference rhs, pool_t & /*pool*/)
            {
                lhs = rhs;
            }
        };

    public:
        union layout
        {
            /* short string */
            union
            {
                char_t raw[14 / sizeof(char_t)];
                struct
                {
                    uint8_t pad[14];
                    uint8_t siz;
                    uint8_t tag;
                } ext;
            } sht;

            /* normal(long) string */
            struct
            {
                union
                {
                    char_t * ptr;
                    uint64_t pad;
                }        raw;
                uint32_t siz;
                uint8_t  exp;
                uint8_t  pad[2];
                uint8_t  tag;
            } lng;
        };

    private:

        static const uint8_t LONG = uint8_t(~uint8_t());

    private:
        static inline
        bool
        is_shortstring(node_t const & node)
        {
            return node.str.sht.ext.siz != LONG;
        }
        static inline
        void
        update(node_t & node, size_type siz)
        {
            if (is_shortstring(node))
                node.str.sht.ext.siz = static_cast<uint8_t>(siz);
            else
                node.str.lng    .siz = siz;
        }

    public:
        template<typename pool_t>
        static inline
        void
        construct(node_t & node, pool_t & /*pool*/)
        {
            node.nil.tag = TAG;
            /* '\0' at end */
            node.str.sht.raw [0] = char_t();
            node.str.sht.ext.siz = 1;
        }
        template<typename pool_t>
        static inline
        void
        destruct(node_t & node, pool_t & pool)
        {
            typedef typename container::pointer pointer;
            if (is_shortstring(node) == false) {
                pointer   mem = raw(node);
                size_type cap = capacity(node) + size_type(1);
                pool.deallocate(mem, cap);
            }
            node.nil.tag = NIL;
        }
        template<typename pool_t>
        static inline
        void
        copy(node_t & lhs, node_t const & rhs, pool_t & pool)
        {
            size_type siz = size(rhs);
            reserve(lhs, siz, pool);
            siz += 1; /* '\0' at end */

            {   /* copy data */
                typedef typename container::value_type value_type;
                size_type mem_siz = siz * sizeof(value_type);
                ::memcpy(raw(lhs), raw(rhs), mem_siz);
            }
            {   /* update */
                update(lhs, siz);
                lhs.nil.tag = TAG;
            }
        }
        static inline
        bool
        equal(node_t const & lhs, node_t const & rhs)
        {
            if (size(lhs) != size(rhs))
                return false;

            typedef typename container::value_type value_type;
            size_type mem_siz = (size(lhs) + 1) * sizeof(value_type);
            return (::memcmp(raw(lhs), raw(rhs), mem_siz) == 0);
        }
        static inline
        size_type
        size(node_t const & node)
        {
            return static_cast<size_type>
                ( is_shortstring(node)
                ? node.str.sht.ext.siz
                : node.str.lng.siz
                )
                -
                size_type(1) /* '\0' NOT included */
                ;
        }
        static inline
        size_type
        capacity(node_t const & node)
        {
            return static_cast<size_type>
                ( is_shortstring(node)
                ? (14 / sizeof(char_t))
                : node_t::cap::at(node.str.lng.exp)
                )
                -
                size_type(1) /* '\0' NOT included */
                ;
        }
        static inline
        typename container::const_pointer
        raw(node_t const & node)
        {
            return static_cast<typename container::const_pointer>
                ( is_shortstring(node)
                ? node.str.sht.raw
                : node.str.lng.raw.ptr
                )
                ;
        }
        static inline
        typename container::pointer
        raw(node_t & node)
        {
            return const_cast<typename container::pointer>
                (raw(static_cast<node_t const &>(node)));
        }
        template<typename pool_t>
        static inline
        void
        reserve(node_t & node, size_type cap, pool_t & pool)
        {
            if (cap <= capacity(node))
                return;

            typedef typename container::value_type value_type;
            typedef typename container::pointer    pointer;

            /* alloc new space */
            uint8_t   exp = node_t::cap::right(cap + 1); /* '\0' at end */
                      cap = node_t::cap::at(exp);
            pointer   mem = pool.template allocate<value_type>(cap);
            pointer   old =  raw(node);
            size_type siz = size(node) + 1;

            /* copy */
            size_type mem_siz = siz * sizeof(value_type);
            ::memcpy(mem, old, mem_siz);

            /* release old space */
            if (is_shortstring(node) == false)
                pool.deallocate(old, capacity(node) + size_type(1));

            /* update */
            node.str.sht.ext.siz = LONG;
            node.str.lng.siz     = siz;
            node.str.lng.exp     = exp;
            node.str.lng.raw.ptr = mem;
        }
        template<typename pool_t>
        static inline
        void
        resize(node_t & node, size_type siz, pool_t & pool)
        {
            typedef typename container::pointer pointer;

            reserve(node, siz, pool);

            pointer beg = raw(node) + size(node);
            pointer end = raw(node) + siz;

            /* construct */
            for (pointer cur = beg; cur <= end; ++cur)
                (*cur) = typename container::value_type();

            /*  destruct */
            for (pointer cur = end; cur <  beg; ++cur)
                (*cur) = typename container::value_type();

            /* update */
            siz += 1; /* '\0' at end */
            update(node, siz);
        }
    };

    /************************************************************************
     * SEQ
     ***********************************************************************/

    template<typename char_t>
    struct traits<node_t<char_t>, SEQ>
    {
    public:
        static const tag_t TAG = SEQ;

    public:
        typedef node_t<char_t>             node_t;
        typedef typename node_t::size_type size_type;

        struct container
        {
            typedef node_t             value_type;
            typedef value_type       *       pointer;
            typedef value_type const * const_pointer;
            typedef value_type       &       reference;
            typedef value_type const & const_reference;
            typedef pointer                          iterator;
            typedef const_pointer              const_iterator;
            typedef pointer                  reverse_iterator; /* TODO: */
            typedef const_pointer      const_reverse_iterator; /* TODO: */
            typedef size_type          index_type;
            typedef void               void_type;

            template<typename pool_t>
            static inline void copy
            (reference lhs, const_reference rhs, pool_t & pool)
            {
                lhs.copy(rhs, pool);
            }
            template<typename pool_t>
            static inline void move
            (reference lhs, reference rhs, pool_t & pool)
            {
                lhs.move(rhs, pool);
            }
        };

    public:

        struct layout
        {
            union
            {
                typename container::pointer ptr;
                uint64_t                    pad;
            }        raw;
            uint32_t siz;
            uint8_t  exp;
            uint8_t  pad[2];
            uint8_t  tag;
        };

    public:
        template<typename pool_t>
        static inline
        void
        construct(node_t & node, pool_t & /*pool*/)
        {
            node.seq.raw.ptr = NULL;
            node.seq.siz = 0;
            node.seq.exp = 0;
            node.nil.tag = TAG;
        }
        template<typename pool_t>
        static inline
        void
        destruct(node_t & node, pool_t & pool)
        {
            typedef typename container::pointer pointer;

            pointer beg = raw(node);
            pointer end = beg + size(node);
            for (pointer cur = beg; cur != end; ++cur)
                (*cur).destruct(pool);

            if (beg != NULL)
                pool.deallocate(beg, capacity(node));

            node.nil.tag = NIL;
        }
        template<typename pool_t>
        static inline
        void
        copy(node_t & lhs, node_t const & rhs, pool_t & pool)
        {
            typedef typename container::      pointer       pointer;
            typedef typename container::const_pointer const_pointer;

            size_type siz = size(rhs);
            destruct (lhs, pool);
            construct(lhs, pool);
            reserve  (lhs, siz, pool);

                  pointer dst = raw(lhs);
            const_pointer src = raw(rhs);
            const_pointer end = src + siz;

            while (src != end) {
                (*dst).construct(pool);
                (*dst).copy((*src), pool);
                ++dst;
                ++src;
            }

            lhs.seq.siz = rhs.seq.siz;
        }
        static inline
        bool
        equal(node_t const & lhs, node_t const & rhs)
        {
            if (size(lhs) != size(rhs))
                return false;

            typedef typename container::const_pointer const_pointer;
            const_pointer ilhs = raw(lhs);
            const_pointer irhs = raw(rhs);
            const_pointer iend = irhs + size(rhs);
            while (irhs != iend) {
                if ((*ilhs).equal(*irhs) == false)
                    return false;
                ++ilhs;
                ++irhs;
            }

            return true;
        }
        static inline
        size_type
        size(node_t const & node)
        {
            return static_cast<size_type>(node.seq.siz);
        }
        static inline
        size_type
        capacity(node_t const & node)
        {
            return static_cast<size_type>(node_t::cap::at(node.seq.exp));
        }
        static inline
        typename container::const_pointer
        raw(node_t const & node)
        {
            return node.seq.raw.ptr;
        }
        static inline
        typename container::pointer
        raw(node_t & node)
        {
            return const_cast<typename container::pointer>
                (raw(static_cast<node_t const &>(node)));
        }
        template<typename pool_t>
        static inline
        void
        reserve(node_t & node, size_type cap, pool_t & pool)
        {
            if (cap <= capacity(node))
                return;

            typedef typename container::value_type value_type;
            typedef typename container::pointer    pointer;

            /* alloc new space */
            uint8_t exp = node_t::cap::right(cap);
                    cap = node_t::cap::at(exp);
            pointer mem = pool.template allocate<value_type>(cap);

            /* move */
            pointer beg = raw(node);
            pointer end = beg + size(node);
            pointer lhs = mem;
            for (pointer rhs = beg; rhs != end; ++rhs) {
                (*lhs).construct(pool);
                (*lhs).move((*rhs), pool);
                ++lhs;
            }

            /* release */
            if (beg != NULL)
                pool.deallocate(beg, capacity(node));

            /* update */
            node.seq.exp     = exp;
            node.seq.raw.ptr = mem;
        }
        template<typename pool_t>
        static inline
        void
        resize(node_t & node, size_type siz, pool_t & pool)
        {
            typedef typename container::pointer pointer;

            reserve(node, siz, pool);

            pointer beg = raw(node) + size(node);
            pointer end = raw(node) + siz;

            for (pointer cur = beg; cur < end; ++cur)
                (*cur).construct(pool);

            for (pointer cur = end; cur < beg; ++cur)
                (*cur). destruct(pool);

            node.seq.siz = siz;
        }
    };

    /************************************************************************
     * MAP
     ***********************************************************************/

    template<typename char_t>
    struct traits<node_t<char_t>, MAP>
    {
    public:
        static const tag_t TAG = MAP;

    public:
        typedef node_t<char_t>             node_t;
        typedef typename node_t::pair_t    pair_t;
        typedef typename node_t::size_type size_type;

        struct container
        {
            typedef pair_t             value_type;
            typedef value_type       * pointer;
            typedef value_type const * const_pointer;
            typedef value_type       & reference;
            typedef value_type const & const_reference;
            typedef       pointer                    iterator;
            typedef const_pointer              const_iterator;
            typedef       pointer            reverse_iterator; /* TODO: */
            typedef const_pointer      const_reverse_iterator; /* TODO: */
            typedef node_t     const & key_const_reference;
            typedef size_type          index_type;
            typedef void               void_type;

            template<typename pool_t>
            static inline void copy
            (reference lhs, const_reference rhs, pool_t & pool)
            {
                lhs[0].copy(rhs[0], pool);
                lhs[1].copy(rhs[1], pool);
            }
            template<typename pool_t>
            static inline void move
            (reference lhs, reference rhs, pool_t & pool)
            {
                lhs[0].move(rhs[0], pool);
                lhs[1].move(rhs[1], pool);
            }
            static inline bool equal
            (const_reference lhs, key_const_reference rhs)
            {
                return lhs[0].equal(rhs);
            }
        };

    public:
        struct layout
        {
            union
            {
                typename container::pointer ptr;
                uint64_t                    pad;
            }        raw;
            uint32_t siz;
            uint8_t  exp;
            uint8_t  pad[2];
            uint8_t  tag;
        };

    public:
        template<typename pool_t>
        static inline
        void
        construct(node_t & node, pool_t & /*pool*/)
        {
            node.map.raw.ptr = NULL;
            node.map.siz = 0;
            node.map.exp = 0;
            node.nil.tag = TAG;
        }
        template<typename pool_t>
        static inline
        void
        destruct(node_t & node, pool_t & pool)
        {
            typedef typename container::pointer pointer;

            pointer beg = raw(node);
            pointer end = beg + size(node);
            for (pointer cur = beg; cur != end; ++cur) {
                (*cur)[0].destruct(pool);
                (*cur)[1].destruct(pool);
            }

            if (beg != NULL) {
                node_t *  mem = reinterpret_cast<node_t *>(beg);
                size_type siz = capacity(node) << 1;
                pool.deallocate(mem, siz);
            }

            node.nil.tag = NIL;
        }
        template<typename pool_t>
        static inline
        void
        copy(node_t & lhs, node_t const & rhs, pool_t & pool)
        {
            typedef typename container::      pointer       pointer;
            typedef typename container::const_pointer const_pointer;

            size_type siz = size(rhs);
            destruct (lhs, pool);
            construct(lhs, pool);
            reserve  (lhs, siz, pool);

                  pointer dst = raw(lhs);
            const_pointer src = raw(rhs);
            const_pointer end = src + siz;
            while (src != end) {
                (*dst)[0].construct(pool);
                (*dst)[1].construct(pool);
                (*dst)[0].copy((*src)[0], pool);
                (*dst)[1].copy((*src)[1], pool);
                ++dst;
                ++src;
            }

            lhs.map.siz = rhs.map.siz;
        }
        static inline
        bool
        equal(node_t const & lhs, node_t const & rhs)
        {
            if (size(lhs) != size(rhs))
                return false;

            typedef typename container::const_pointer const_pointer;
            const_pointer dst = raw(lhs);
            const_pointer src = raw(rhs);
            const_pointer end = src + size(rhs);
            while (src != end) {
                if ((*src)[0].equal((*dst)[0]) == false ||
                    (*src)[1].equal((*dst)[1]) == false )
                    return false;
                ++dst;
                ++src;
            }

            return true;
        }
        static inline
        size_type
        size(node_t const & node)
        {
            return static_cast<size_type>(node.map.siz);
        }
        static inline
        size_type
        capacity(node_t const & node)
        {
            return static_cast<size_type>(node_t::cap::at(node.map.exp));
        }
        static inline
        typename container::const_pointer
        raw(node_t const & node)
        {
            return node.map.raw.ptr;
        }
        static inline
        typename container::pointer
        raw(node_t & node)
        {
            return const_cast<typename container::pointer>
                (raw(static_cast<node_t const &>(node)));
        }
        template<typename pool_t>
        static inline
        void
        reserve(node_t & node, size_type cap, pool_t & pool)
        {
            if (cap <= capacity(node))
                return;

            typedef typename container::pointer pointer;

            /* alloc new space */
            uint8_t exp = node_t::cap::right(cap);
                    cap = node_t::cap::at(exp);
            pointer mem = reinterpret_cast<pointer>(
                pool.template allocate<node_t>(cap << 1)
            );

            /* move */
            pointer beg = raw(node);
            pointer end = beg + size(node);
            pointer lhs = mem;
            for (pointer rhs = beg; rhs != end; ++rhs, ++lhs) {
                (*lhs)[0].construct(pool);
                (*lhs)[1].construct(pool);
                (*lhs)[0].move((*rhs)[0], pool);
                (*lhs)[1].move((*rhs)[1], pool);
            }

            /* release */
            if (beg != NULL) {
                node_t *  old = reinterpret_cast<node_t *>(beg);
                size_type siz = capacity(node) << 1;
                pool.deallocate(old, siz);
            }

            /* update */
            node.map.exp     = exp;
            node.map.raw.ptr = mem;
        }
        template<typename pool_t>
        static inline
        void
        resize(node_t & node, size_type siz, pool_t & pool)
        {
            typedef typename container::pointer pointer;

            reserve(node, siz, pool);

            pointer beg = raw(node) + size(node);
            pointer end = raw(node) + siz;

            for (pointer cur = beg; cur < end; ++cur) {
                (*cur)[0].construct(pool);
                (*cur)[1].construct(pool);
            }

            for (pointer cur = end; cur < beg; ++cur) {
                (*cur)[0]. destruct(pool);
                (*cur)[1]. destruct(pool);
            }

            node.map.siz = siz;
        }
    };

}

namespace ast
{
    /************************************************************************
     * methods
     ***********************************************************************/

    template<typename char_t> template<typename pool_t>
    inline void node_t<char_t>::construct(pool_t & pool)
    {
        traits<node_t, NIL>::construct(*this, pool);
    }

    template<typename char_t> template<typename pool_t>
    inline void node_t<char_t>::
    construct(tag_t tag, pool_t & pool)
    {
        switch (tag)
        {
        case NIL: { construct<NIL>(pool); break; }
        case I64: { construct<I64>(pool); break; }
        case DBL: { construct<DBL>(pool); break; }
        case STR: { construct<STR>(pool); break; }
        case SEQ: { construct<SEQ>(pool); break; }
        case MAP: { construct<MAP>(pool); break; }
        default:  { exception::node_type_out_of_range(tag, POS_); break; }
        }
    }

    template<typename char_t> template<typename pool_t>
    inline void node_t<char_t>::
    destruct(pool_t & pool)
    {
        /* mark this as NIL(no need to destruct)
         * to prevent potential infinite recursion.
         */
        uint8_t tag = nil.tag;
        nil.    tag = NIL;

        switch (tag)
        {
        case NIL: { break; }
        case I64: { traits<node_t, I64>::destruct(*this, pool); break; }
        case DBL: { traits<node_t, DBL>::destruct(*this, pool); break; }
        case STR: { traits<node_t, STR>::destruct(*this, pool); break; }
        case SEQ: { traits<node_t, SEQ>::destruct(*this, pool); break; }
        case MAP: { traits<node_t, MAP>::destruct(*this, pool); break; }
        default:  { exception::node_type_out_of_range(tag, POS_); break; }
        }
    }

    template<typename char_t> template<typename pool_t>
    inline void node_t<char_t>::
    copy(node_t const & rhs, pool_t & pool)
    {
        node_t & lhs = *this;
        if (&lhs == &rhs)
            return;

        if (lhs.type() == NIL)
        {
            tag_t tag = rhs.type();
            lhs.construct(tag, pool);

            /* copy data */
            switch (tag)
            {
            case NIL: { break; }
            case I64: { traits<node_t, I64>::copy(lhs, rhs, pool); break; }
            case DBL: { traits<node_t, DBL>::copy(lhs, rhs, pool); break; }
            case STR: { traits<node_t, STR>::copy(lhs, rhs, pool); break; }
            case SEQ: { traits<node_t, SEQ>::copy(lhs, rhs, pool); break; }
            case MAP: { traits<node_t, MAP>::copy(lhs, rhs, pool); break; }
            default:  { exception::node_type_out_of_range(tag,POS_); break; }
            }
        }
        else
        {
            /* there may be parent-child relationship between lhs and rhs,
             * using a tmp node to avoid rhs destructed before copy starting.
             */

            /* tmp = rhs */
            node_t tmp;
            tmp.construct(pool);
            tmp.copy(rhs, pool);

            /* lhs = move(tmp) */
            lhs. destruct(pool);
            lhs.construct(pool);
            lhs.move(tmp, pool);
        }
    }

    template<typename char_t> template<typename pool_t>
    inline void node_t<char_t>::
    move(node_t & rhs, pool_t & pool)
    {
        node_t & lhs = *this;
        if (&lhs == &rhs)
            return;

        if (lhs.type() == NIL) {
            ::memcpy(&lhs, &rhs, sizeof(node_t));
            ::memset(&rhs,    0, sizeof(node_t));
        } else {
            /* there may be parent-child relationship between lhs and rhs,
             * using a tmp node to avoid rhs destructed before copy starting.
             */

            /* tmp = move(rhs), separated form this */
            node_t tmp;
            tmp.construct(pool);
            tmp.move(rhs, pool);

            /* lhs = move(tmp) */
            lhs. destruct(pool);
            lhs.construct(pool);
            lhs.move(tmp, pool);
        }
    }

    template<typename char_t>
    inline void node_t<char_t>::
    swap(node_t & rhs)
    {
        node_t & lhs = *this;
        node_t   tmp;

        ::memcpy(&tmp, &rhs, sizeof(node_t));
        ::memcpy(&rhs, &lhs, sizeof(node_t));
        ::memcpy(&lhs, &tmp, sizeof(node_t));
        ::memset(&tmp,    0, sizeof(node_t));
    }

    template<typename char_t>
    inline bool node_t<char_t>::
    equal(node_t const & rhs) const
    {
        node_t const & lhs = *this;

        if (&lhs == &rhs)
            return true;
        if (lhs.type() != rhs.type())
            return false;

        tag_t tag = lhs.type();
        switch (tag)
        {
        case NIL: { return traits<node_t, NIL>::equal(lhs, rhs); break; }
        case I64: { return traits<node_t, I64>::equal(lhs, rhs); break; }
        case DBL: { return traits<node_t, DBL>::equal(lhs, rhs); break; }
        case STR: { return traits<node_t, STR>::equal(lhs, rhs); break; }
        case SEQ: { return traits<node_t, SEQ>::equal(lhs, rhs); break; }
        case MAP: { return traits<node_t, MAP>::equal(lhs, rhs); break; }
        default:  { exception::node_type_out_of_range(tag,POS_); break; }
        }

        return false;
    }

    template<typename char_t>
    inline tag_t node_t<char_t>::
    type() const
    {
        return static_cast<tag_t>(nil.tag);
    }

    template<typename char_t> template<tag_t TAG, typename pool_t>
    inline void node_t<char_t>::
    construct(pool_t & pool)
    {
        traits<node_t, TAG>::construct(*this, pool);
    }

    template<typename char_t> template<tag_t TAG, typename pool_t>
    inline void node_t<char_t>::
    set(typename traits<node_t, TAG>::const_reference value, pool_t & pool)
    {
        destruct      (pool);
        construct<TAG>(pool);
        traits<node_t, TAG>::val(*this) = value;
    }

    template<typename char_t> template<tag_t TAG, typename pool_t>
    inline void node_t<char_t>::
    set
    (
        typename traits<node_t, TAG>::container::const_iterator ibeg,
        typename traits<node_t, TAG>::container::const_iterator iend,
        pool_t & pool
    )
    {
        destruct      (pool);
        construct<TAG>(pool);

        typedef traits<node_t, TAG>                traits;
        typedef typename traits::container         container;
        typedef typename container::      iterator       iterator;
        typedef typename container::const_iterator const_iterator;

        size_type len = static_cast<size_type>(iend - ibeg);
        traits::resize(*this, len, pool);

              iterator ilhs = begin<TAG>();
        const_iterator irhs = ibeg;
        while (irhs != iend) {
            container::copy((*ilhs), (*irhs), pool);
            ++ilhs;
            ++irhs;
        }
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::const_reference
    node_t<char_t>::
    val() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);
        return traits<node_t, TAG>::val(*this);
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::reference
    node_t<char_t>::
    val()
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);
        return traits<node_t, TAG>::val(*this);
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::size_type
    node_t<char_t>::
    size() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);
        return traits<node_t, TAG>::size(*this);
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::size_type
    node_t<char_t>::
    capacity() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);
        return traits<node_t, TAG>::capacity(*this);
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    const_pointer node_t<char_t>::
    raw() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);
        return traits<node_t, TAG>::raw(*this);
    }

    template<typename char_t> template<tag_t TAG, typename pool_t>
    inline typename traits<node_t<char_t>, TAG>::container::
    void_type node_t<char_t>::
    push_back
    (
        typename traits<node_t, TAG>::container::const_reference value,
        pool_t & pool
    )
    {
        typedef traits<node_t, TAG>                   traits;
        typedef typename traits::size_type            size_type;
        typedef typename traits::container::reference reference;

        if (type() == NIL)
            traits::construct(*this, pool);
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        size_type siz = traits::size(*this);
        traits::resize(*this, siz + 1, pool);

        reference bak = traits::raw(*this)[siz];
        traits::container::copy(bak, value, pool);
    }

    template<typename char_t> template<tag_t TAG, typename pool_t>
    inline typename traits<node_t<char_t>, TAG>::container::
    void_type node_t<char_t>::
    move_back
    (
        typename traits<node_t, TAG>::container::reference value,
        pool_t & pool
    )
    {
        typedef traits<node_t, TAG>                   traits;
        typedef typename traits::size_type            size_type;
        typedef typename traits::container::reference reference;

        if (type() == NIL)
            traits::construct(*this, pool);
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        size_type siz = traits::size(*this);
        traits::resize(*this, siz + 1, pool);

        reference bak = traits::raw(*this)[siz];
        traits::container::move(bak, value, pool);
    }

    template<typename char_t> template<tag_t TAG, typename pool_t>
    inline typename traits<node_t<char_t>, TAG>::container::
    iterator node_t<char_t>::
    erase
    (
        typename traits<node_t, TAG>::container::const_iterator citer,
        pool_t & pool
    )
    {
        typedef traits<node_t, TAG>          traits;
        typedef typename traits::container   container;
        typedef typename container::iterator iterator;

        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        iterator ibeg = begin<TAG>();
        iterator iend = end  <TAG>();
        iterator iter = ibeg + (citer - ibeg);
        iterator ipos = iter;
        if (iter < ibeg || iter >= iend)
            return iend;

        while (++iter != iend)
            traits::container::move((*(iter - 1)), (*iter), pool);

        traits::resize(*this, traits::size(*this) - 1, pool);
        return ipos;
    }

    template<typename char_t> template<tag_t TAG, typename pool_t>
    inline typename traits<node_t<char_t>, TAG>::container::
    iterator node_t<char_t>::
    erase
    (
        typename traits<node_t, TAG>::container::const_iterator cifst,
        typename traits<node_t, TAG>::container::const_iterator cilst,
        pool_t & pool
    )
    {
        typedef traits<node_t, TAG>          traits;
        typedef typename traits::container   container;
        typedef typename container::iterator iterator;

        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        iterator ibeg = begin<TAG>();
        iterator iend = end  <TAG>();

        if (cifst >= cilst)
            return iend;

        iterator ifst = ibeg + (cifst - ibeg);
        iterator ilst = ibeg + (cilst - ibeg);

        if (ifst < ibeg)
            ifst = ibeg;
        if (ilst > iend)
            ilst = iend;

        iterator ipos = ifst;
        size_type siz
            = traits::size(*this)
            - static_cast<size_type>(ilst - ifst)
            ;

        while (ilst != iend) {
            traits::container::move((*ifst), (*ilst), pool);
            ++ifst;
            ++ilst;
        }

        traits::resize(*this, siz, pool);
        return ipos;
    }

    template<typename char_t> template<tag_t TAG, typename pool_t>
    inline typename traits<node_t<char_t>, TAG>::container::
    void_type node_t<char_t>::
    pop_back(pool_t & pool)
    {
        typedef traits<node_t, TAG>        traits;
        typedef typename traits::size_type size_type;

        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        size_type siz = traits::size(*this);
        if (siz > 0)
            traits::resize(*this, siz - 1, pool);
    }

    template<typename char_t> template<tag_t TAG, typename pool_t>
    inline typename traits<node_t<char_t>, TAG>::container::
    void_type node_t<char_t>::
    clear(pool_t & pool)
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        traits<node_t, TAG>::resize(*this, 0, pool);
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    const_iterator node_t<char_t>::
    begin() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        return traits<node_t, TAG>::raw(*this);
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::iterator
    node_t<char_t>::
    begin()
    {
        return const_cast<typename traits<node_t, TAG>::container::iterator>
            (static_cast<node_t const &>(*this).begin<TAG>());
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    const_iterator node_t<char_t>::
    end() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        return traits<node_t, TAG>::raw (*this)
            +  traits<node_t, TAG>::size(*this)
            ;
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::iterator
    node_t<char_t>::
    end()
    {
        return const_cast<typename traits<node_t, TAG>::container::iterator>
            (static_cast<node_t const &>(*this).end<TAG>());
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    const_reverse_iterator node_t<char_t>::
    rbegin() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        return traits<node_t, TAG>::raw (*this)
            +  traits<node_t, TAG>::size(*this)
            -  1
            ;
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    reverse_iterator node_t<char_t>::
    rbegin()
    {
        return const_cast
            <typename traits<node_t, TAG>::container::reverse_iterator>
            (static_cast<node_t const &>(*this).rbegin<TAG>());
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    const_reverse_iterator node_t<char_t>::
    rend() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        return traits<node_t, TAG>::raw(*this)
            -  1
            ;
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    reverse_iterator node_t<char_t>::
    rend()
    {
        return const_cast
            <typename traits<node_t, TAG>::container::reverse_iterator>
            (static_cast<node_t const &>(*this).rend<TAG>());
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    const_iterator node_t<char_t>::
    at(typename traits<node_t, TAG>::container::index_type idx) const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        if (idx <  traits<node_t, TAG>::size(*this))
            return traits<node_t, TAG>::raw (*this) + idx;

        return traits<node_t, TAG>::raw (*this)
            +  traits<node_t, TAG>::size(*this)
            ;
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    iterator node_t<char_t>::
    at(typename traits<node_t, TAG>::container::index_type idx)
    {
        return const_cast<typename traits<node_t, TAG>::container::iterator>
            (static_cast<node_t const &>(*this).at<TAG>(idx));
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    const_iterator node_t<char_t>::
    find(typename traits<node_t, TAG>::container::key_const_reference key)
    const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        typedef typename traits<node_t, TAG>::container container;
        typedef typename container::const_iterator      const_iterator;

        const_iterator ibeg = begin<TAG>();
        const_iterator iend = end  <TAG>();
        for (const_iterator iter = ibeg; iter < iend; ++iter)
            if (container::equal((*iter), key))
                return iter;

        return iend;
    }

    template<typename char_t> template<tag_t TAG>
    inline typename traits<node_t<char_t>, TAG>::container::
    iterator node_t<char_t>::
    find(typename traits<node_t, TAG>::container::key_const_reference key)
    {
        return const_cast<typename traits<node_t, TAG>::container::iterator>
            (static_cast<node_t const &>(*this).find<TAG>(key));
    }
}

CV_FS_PRIVATE_END
