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
    using chars::Soss;
    using chars::fmt;

    static inline void node_type_not_match(int now, int ept, POS_TYPE_)
    {
        error(0,
            ( Soss<>()
                * "node type is `" | fmt<16>(now) | "`, "
                | "but not `" | fmt<16>(ept) | '`'
            ), POS_ARGS_
        );
    }

    static inline void node_type_out_of_range(int now, POS_TYPE_)
    {
        error(0,
            ( Soss<>()
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
    enum Tag
    {
        NIL = 0, //!< empty    [scalar][default]

        I64,     //!< int64    [scalar]
        DBL,     //!< double   [scalar]
        STR,     //!< string   [container]
        SEQ,     //!< sequence [container]
        MAP      //!< map      [container]
    };

    /************************************************************************
     * node Traits
     ***********************************************************************/

    /** @brief Bind some types or methods to TAG. They make generic
    programming easier.
    */
    template<typename NodeType, Tag TAG> struct Traits;
}

namespace ast
{
    /** @brief `Node` is the node type of an abstract syntax tree,
    something like variant. It can be null, int64_t, double, string,
    sequence, map.

    Some features:
     - always 16 bytes in x86 and x64,
     - trivial and standard layout (plain old data),
     - wide char string support,
     - small string optimization,
     - growth factor of containers == 1.618,
     - memory pool needed.
    */
    template<typename CharType>
    union Node
    {
    public:

        /** @brief `Pair` is used in container map.

        Note: It is safe to cast `Pair[n]` to `Node[2*n]`. So we can use
        allocator<Node> for `Pair`.
        */
        typedef Node   Pair[2];

        /** @brief Define size_type. Member `.size` is at most an uint32_t
        in order to keep `Node` always 16 bytes.
        */
        typedef uint32_t size_type;

        /** @brief About fibonacci sequence.

        Capacity of a container is stored as an index of fibonacci sequence.
        So we need look up fibonacci sequence at runtime.
        */
        typedef RuntimeFibonacci<uint8_t, size_type> Cap;

        /** @brief Make Traits `friend`.
        */
        template<typename, Tag> friend struct Traits;

    public:

        /** @brief Construct this node.

        `Node` has no constructors. Use this method to construct node.
        Note: there may be memory leak if node has been constructed.

        @param pool A collection of allocators. See class `Pool`.
        */
        template<typename PoolType> inline
        void construct(PoolType & pool);

        /** @overload
        @param tag  Type of node to be constructed. See enum `Tag`.
        @param pool A collection of allocators. See class `Pool`.
        */
        template<typename PoolType> inline
        void construct(Tag tag, PoolType & pool);

        /** @brief Destruct this node. Released memory if used.
        @param pool A collection of allocators. See class `Pool`.
        */
        template<typename PoolType> inline
        void destruct(PoolType & pool);

        /** @brief `*this = rhs`.
        @param rhs  node to copy.
        @param pool A collection of allocators. See class `Pool`.
        */
        template<typename PoolType> inline
        void copy(Node const & rhs, PoolType & pool);

        /** @brief `*this = move(rhs)`.
        @param rhs  node to move.
        @param pool A collection of allocators. See class `Pool`.
        */
        template<typename PoolType> inline
        void move(Node & rhs, PoolType & pool);

        /** @brief Swap `*this` with rhs.
        @param rhs node to swap.
        */
        inline void swap(Node & rhs);

        /** @brief Compare `*this` with rhs.
        @param rhs node to compare.
        @return true if *this equal to rhs, false if not.
        */
        inline bool equal(Node const & rhs) const;

        /** @brief Get the type of this node.
        @return tag of this node. See enum `Tag`
        */
        inline Tag type() const;

        /** @overload
        @param pool A collection of allocators. See class `Pool`.
        */
        template<Tag TAG, typename PoolType> inline
        void
        construct(PoolType & pool);

        /** @brief Set value of node.

        Destruct, re-construct and assign.
        [Need to specify the TAG]
        @param val  `int64_t` or `double` value.
        @param pool A collection of allocators. See class `Pool`.
        */
        template<Tag TAG, typename PoolType> inline
        void
        set
        (typename Traits<Node, TAG>::const_reference val, PoolType & pool);

        /** @overload
        @param beg  Beginning of elements to copy.
        @param end  End of elements to copy. Range: [beg, end)
        @param pool A collection of allocators. See class `Pool`.
        */
        template<Tag TAG, typename PoolType> inline
        void
        set
        (
            typename Traits<Node, TAG>::Container::const_iterator beg,
            typename Traits<Node, TAG>::Container::const_iterator end,
            PoolType & pool
        );

        /** @brief Get `int64_t` or `double` value of node.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Const reference of value.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::const_reference
        val() const;

        /** @overload
        @return Reference of value.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::reference
        val();

        /** @brief Get the size of a built-in container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Size (uint32_t).
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::size_type
        size() const;

        /** @brief Get the capacity of a built-in container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Capacity (uint32_t).
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::size_type
        capacity() const;

        /** @brief Get the rawdata of a built-in container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return A pointer to rawdata.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::const_pointer
        raw() const;

        /** @brief Add a copy of an element to the end of built-in container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param val  An element to copy.
        @param pool A collection of allocators. See class `Pool`.
        */
        template<Tag TAG, typename PoolType> inline
        typename Traits<Node, TAG>::Container::void_type
        push_back
        (
            typename Traits<Node, TAG>::Container::const_reference val,
            PoolType & pool
        );

        /** @brief Move an element to the end of built-in container.

        After `move_back`, the element will be cleared.
        Note: Do not use `node.move_back<SEQ>(node, pool)`!
              Node will set NIL without being destructed.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param val  An element to move.
        @param pool A collection of allocators. See class `Pool`.
        */
        template<Tag TAG, typename PoolType> inline
        typename Traits<Node, TAG>::Container::void_type
        move_back
        (
            typename Traits<Node, TAG>::Container::reference val,
            PoolType & pool
        );

        /** @brief Delete an element of built-in container. Do nothing
        if the element does not belong to container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param iter An element to delete.
        @param pool A collection of allocators. See class `Pool`.
        @return the following element of deleted element.
        */
        template<Tag TAG, typename PoolType> inline
        typename Traits<Node, TAG>::Container::iterator
        erase
        (
            typename Traits<Node, TAG>::Container::const_iterator iter,
            PoolType & pool
        );

        /** @overload
        @param begin Beginning of elements to delete.
        @param end   End of elements to delete. Range: [begin, end)
        @param pool  A collection of allocators. See class `Pool`.
        @return the following element of deleted elements.
        */
        template<Tag TAG, typename PoolType> inline
        typename Traits<Node, TAG>::Container::iterator
        erase
        (
            typename Traits<Node, TAG>::Container::const_iterator begin,
            typename Traits<Node, TAG>::Container::const_iterator end,
            PoolType & pool
        );

        /** @brief Delete the last element of built-in container.
        Do nothing if empty.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param pool  A collection of allocators. See class `Pool`.
        */
        template<Tag TAG, typename PoolType> inline
        typename Traits<Node, TAG>::Container::void_type
        pop_back(PoolType & pool);

        /** @brief Delete all element of built-in container.

        Note: Won't release memory. Use `destruct` and `construct` if needed.
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param pool  A collection of allocators. See class `Pool`.
        */
        template<Tag TAG, typename PoolType> inline
        typename Traits<Node, TAG>::Container::void_type
        clear(PoolType & pool);

        /** @brief Get first element of built-in container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Const iterator of the first element.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::const_iterator
        begin() const;

        /** @overload
        @return Iterator of the first element.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::iterator
        begin();

        /** @brief Get the next one of the last element of built-in
        container.

        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Const iterator.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::const_iterator
        end() const;

        /** @overload
        @return Iterator.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::iterator
        end();

        /** @brief Get last element of built-in container.

        Note: At present, it is a normal iterator instead of a real reverse
              iterator.
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Const iterator of the last element.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::const_reverse_iterator
        rbegin() const;

        /** @overload
        @return Iterator of the last element.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::reverse_iterator
        rbegin();

        /** @brief Get the previous one of the first element of built-in
        container.

        Note: At present, it is a normal iterator instead of a real reverse
              iterator.
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @return Const iterator of the previous one of the first element.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::const_reverse_iterator
        rend() const;

        /** @overload
        @return Iterator of the previous one of the first element.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::reverse_iterator
        rend();

        /** @brief Search an element by index.

        Return end<TAG>() if not found.
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param index Index of an element.
        @return Const iterator of the element.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::const_iterator
        at(typename Traits<Node, TAG>::Container::index_type index) const;

        /** @overload
        @param index Index of an element.
        @return Iterator of the element.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::iterator
        at(typename Traits<Node, TAG>::Container::index_type index);

        /** @brief Search an element of by key.

        Return end<TAG>() if not found.
        [Need to specify the TAG]
        [May throw an exception if TAG does not match]
        @param key Key to search. Usually it is the first element of a pair.
        @return Const iterator of the element.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::const_iterator
        find
        (typename Traits<Node, TAG>::Container::key_const_reference key)
        const;

        /** @overload
        @param key Key to search. Usually it is the first element of a pair.
        @return Iterator of the element.
        */
        template<Tag TAG> inline
        typename Traits<Node, TAG>::Container::iterator
        find
        (typename Traits<Node, TAG>::Container::key_const_reference key);

    private:

        typename Traits<Node, NIL>::Layout nil; //!< Memory layout of NIL
        typename Traits<Node, I64>::Layout i64; //!< Memory layout of I64
        typename Traits<Node, DBL>::Layout dbl; //!< Memory layout of DBL
        typename Traits<Node, STR>::Layout str; //!< Memory layout of STR
        typename Traits<Node, SEQ>::Layout seq; //!< Memory layout of SEQ
        typename Traits<Node, MAP>::Layout map; //!< Memory layout of MAP
    };

}

/***************************************************************************
 * Implementataion
 ***************************************************************************/

namespace ast
{
    template<typename CharType> union Node;

    /************************************************************************
    * NIL
    ***********************************************************************/

    template<typename CharType>
    struct Traits<Node<CharType>, NIL>
    {
    public:
        static const Tag TAG = NIL;

    public:
        typedef Node<CharType> Node;

    public:
        struct Layout
        {
            uint8_t tag;
            uint8_t pad[15];
        };

    public:
        template<typename PoolType>
        static inline
        void
        construct(Node & node, PoolType & /*pool*/)
        {
            node.nil.tag = TAG;
        }
        template<typename PoolType>
        static inline
        void
        destruct(Node & /*node*/, PoolType & /*pool*/)
        {
            ;
        }
        template<typename PoolType>
        static inline
        void
        copy(Node & lhs, Node const & /*rhs*/, PoolType & pool)
        {
            construct(lhs, pool);
        }
        static inline
        bool
        equal(Node const & /*lhs*/, Node const & /*rhs*/)
        {
            return true;
        }
    };

    /************************************************************************
     * I64
     ***********************************************************************/

    template<typename CharType>
    struct Traits<Node<CharType>, I64>
    {
    public:
        static const Tag TAG = I64;

    public:
        typedef Node<CharType>     Node;
        typedef int64_t            value_type;
        typedef value_type       & reference;
        typedef value_type const & const_reference;

    public:
        struct Layout
        {
            uint8_t tag;
            uint8_t pad[7];
            int64_t val;
        };

    public:
        template<typename PoolType>
        static inline
        void
        construct(Node & node, PoolType & /*pool*/)
        {
            node.i64.tag = TAG;
            val(node) = value_type();
        }
        template<typename PoolType>
        static inline
        void
        destruct(Node & node, PoolType & /*pool*/)
        {
            node.i64.tag = NIL;
        }
        template<typename PoolType>
        static inline
        void
        copy(Node & lhs, Node const & rhs, PoolType & /*pool*/)
        {
            val(lhs) = val(rhs);
            lhs.i64.tag = TAG;
        }
        static inline
        bool
        equal(Node const & lhs, Node const & rhs)
        {
            return (val(lhs) == val(rhs));
        }
        static inline
        const_reference
        val(Node const & node)
        {
            return node.i64.val;
        }
        static inline
        reference
        val(Node & node)
        {
            return const_cast<reference>
                (val(static_cast<Node const &>(node)));
        }
    };

    /************************************************************************
     * DBL
     ***********************************************************************/

    template<typename CharType>
    struct Traits<Node<CharType>, DBL>
    {
    public:
        static const Tag TAG = DBL;

    public:
        typedef Node<CharType>     Node;
        typedef double             value_type;
        typedef value_type       & reference;
        typedef value_type const & const_reference;

    public:
        struct Layout
        {
            uint8_t tag;
            uint8_t pad[7];
            double  val;
        };

    public:
        template<typename PoolType>
        static inline
        void
        construct(Node & node, PoolType & /*pool*/)
        {
            node.dbl.tag = TAG;
            val(node) = value_type();
        }
        template<typename PoolType>
        static inline
        void
        destruct(Node & node, PoolType & /*pool*/)
        {
            node.dbl.tag = NIL;
        }
        template<typename PoolType>
        static inline
        void
        copy(Node & lhs, Node const & rhs, PoolType & /*pool*/)
        {
            val(lhs) = val(rhs);
            lhs.dbl.tag = TAG;
        }
        static inline
        bool
        equal(Node const & lhs, Node const & rhs)
        {
            return (val(lhs) == val(rhs));
        }
        static inline
        const_reference
        val(Node const & node)
        {
            return node.dbl.val;
        }
        static inline
        reference
        val(Node & node)
        {
            return const_cast<reference>
                (val(static_cast<Node const &>(node)));
        }
    };

    /************************************************************************
     * STR
     ***********************************************************************/

    template<typename CharType>
    struct Traits<Node<CharType>, STR>
    {
    public:
        static const Tag TAG = STR;

    public:
        typedef Node<CharType>           Node;
        typedef typename Node::size_type size_type;

        struct Container
        {
            typedef CharType           value_type;
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

            template<typename PoolType>
            static inline void copy
            (reference lhs, const_reference rhs, PoolType & /*pool*/)
            {
                lhs = rhs;
            }
            template<typename PoolType>
            static inline void move
            (reference lhs, reference rhs, PoolType & /*pool*/)
            {
                lhs = rhs;
            }
        };

    public:
        union Layout
        {
            /* tag (in common initial sequence)  */
            uint8_t tag;
            
            /* small string */
            struct
            {
                uint8_t  pad;
                uint8_t  siz;
                CharType raw[14 / sizeof(CharType)];
            } sht;

            /* normal(long) string */
            struct
            {
                uint8_t  pad[3];
                uint8_t  exp;
                uint32_t siz;
                union
                {
                    CharType * ptr;
                    uint64_t   pad;
                }        raw;
            } lng;
        };

    private:
        static const uint8_t LONG = uint8_t(~uint8_t());

    private:
        static inline
        bool
        is_smallstring(Node const & node)
        {
            return node.str.sht.siz != LONG;
        }
        static inline
        void
        update(Node & node, size_type siz)
        {
            if (is_smallstring(node))
                node.str.sht.siz = static_cast<uint8_t>(siz);
            else
                node.str.lng.siz = siz;
        }

    public:
        template<typename PoolType>
        static inline
        void
        construct(Node & node, PoolType & /*pool*/)
        {
            node.str.tag = TAG;
            /* '\0' at end */
            node.str.sht.raw [0] = CharType();
            node.str.sht.siz = 1;
        }
        template<typename PoolType>
        static inline
        void
        destruct(Node & node, PoolType & pool)
        {
            typedef typename Container::pointer pointer;
            if (is_smallstring(node) == false) {
                pointer   mem = raw(node);
                size_type cap = capacity(node) + size_type(1);
                pool.deallocate(mem, cap);
            }
            node.str.tag = NIL;
        }
        template<typename PoolType>
        static inline
        void
        copy(Node & lhs, Node const & rhs, PoolType & pool)
        {
            size_type siz = size(rhs);
            reserve(lhs, siz, pool);
            siz += 1; /* '\0' at end */

            {   /* copy data */
                typedef typename Container::value_type value_type;
                size_type mem_siz = siz * sizeof(value_type);
                ::memcpy(raw(lhs), raw(rhs), mem_siz);
            }
            {   /* update */
                update(lhs, siz);
                lhs.str.tag = TAG;
            }
        }
        static inline
        bool
        equal(Node const & lhs, Node const & rhs)
        {
            if (size(lhs) != size(rhs))
                return false;

            typedef typename Container::value_type value_type;
            size_type mem_siz = (size(lhs) + 1) * sizeof(value_type);
            return (::memcmp(raw(lhs), raw(rhs), mem_siz) == 0);
        }
        static inline
        size_type
        size(Node const & node)
        {
            return static_cast<size_type>
                ( is_smallstring(node)
                ? node.str.sht.siz
                : node.str.lng.siz
                )
                -
                size_type(1) /* '\0' NOT included */
                ;
        }
        static inline
        size_type
        capacity(Node const & node)
        {
            return static_cast<size_type>
                ( is_smallstring(node)
                ? (14 / sizeof(CharType))
                : Node::Cap::at(node.str.lng.exp)
                )
                -
                size_type(1) /* '\0' NOT included */
                ;
        }
        static inline
        typename Container::const_pointer
        raw(Node const & node)
        {
            return static_cast<typename Container::const_pointer>
                ( is_smallstring(node)
                ? node.str.sht.raw
                : node.str.lng.raw.ptr
                )
                ;
        }
        static inline
        typename Container::pointer
        raw(Node & node)
        {
            return const_cast<typename Container::pointer>
                (raw(static_cast<Node const &>(node)));
        }
        template<typename PoolType>
        static inline
        void
        reserve(Node & node, size_type cap, PoolType & pool)
        {
            if (cap <= capacity(node))
                return;

            typedef typename Container::value_type value_type;
            typedef typename Container::pointer    pointer;

            /* alloc new space */
            uint8_t   exp = Node::Cap::right(cap + 1); /* '\0' at end */
                      cap = Node::Cap::at(exp);
            pointer   mem = pool.template allocate<value_type>(cap);
            pointer   old =  raw(node);
            size_type siz = size(node) + 1;

            /* copy */
            size_type mem_siz = siz * sizeof(value_type);
            ::memcpy(mem, old, mem_siz);

            /* release old space */
            if (is_smallstring(node) == false)
                pool.deallocate(old, capacity(node) + size_type(1));

            /* update */
            node.str.sht.siz = LONG;
            node.str.lng.siz     = siz;
            node.str.lng.exp     = exp;
            node.str.lng.raw.ptr = mem;
        }
        template<typename PoolType>
        static inline
        void
        resize(Node & node, size_type siz, PoolType & pool)
        {
            typedef typename Container::pointer pointer;

            reserve(node, siz, pool);

            pointer beg = raw(node) + size(node);
            pointer end = raw(node) + siz;

            /* construct */
            for (pointer cur = beg; cur <= end; ++cur)
                (*cur) = typename Container::value_type();

            /*  destruct */
            for (pointer cur = end; cur <  beg; ++cur)
                (*cur) = typename Container::value_type();

            /* update */
            siz += 1; /* '\0' at end */
            update(node, siz);
        }
    };

    /************************************************************************
     * SEQ
     ***********************************************************************/

    template<typename CharType>
    struct Traits<Node<CharType>, SEQ>
    {
    public:
        static const Tag TAG = SEQ;

    public:
        typedef Node<CharType>           Node;
        typedef typename Node::size_type size_type;

        struct Container
        {
            typedef Node               value_type;
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

            template<typename PoolType>
            static inline void copy
            (reference lhs, const_reference rhs, PoolType & pool)
            {
                lhs.copy(rhs, pool);
            }
            template<typename PoolType>
            static inline void move
            (reference lhs, reference rhs, PoolType & pool)
            {
                lhs.move(rhs, pool);
            }
        };

    public:
        struct Layout
        {
            uint8_t  tag;
            uint8_t  pad[2];
            uint8_t  exp;
            uint32_t siz;
            union
            {
                typename Container::pointer ptr;
                uint64_t                    pad;
            }        raw;
        };

    public:
        template<typename PoolType>
        static inline
        void
        construct(Node & node, PoolType & /*pool*/)
        {
            node.seq.raw.ptr = NULL;
            node.seq.siz = 0;
            node.seq.exp = 0;
            node.seq.tag = TAG;
        }
        template<typename PoolType>
        static inline
        void
        destruct(Node & node, PoolType & pool)
        {
            typedef typename Container::pointer pointer;

            pointer beg = raw(node);
            pointer end = beg + size(node);
            for (pointer cur = beg; cur != end; ++cur)
                (*cur).destruct(pool);

            if (beg != NULL)
                pool.deallocate(beg, capacity(node));

            node.seq.tag = NIL;
        }
        template<typename PoolType>
        static inline
        void
        copy(Node & lhs, Node const & rhs, PoolType & pool)
        {
            typedef typename Container::      pointer       pointer;
            typedef typename Container::const_pointer const_pointer;

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
        equal(Node const & lhs, Node const & rhs)
        {
            if (size(lhs) != size(rhs))
                return false;

            typedef typename Container::const_pointer const_pointer;
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
        size(Node const & node)
        {
            return static_cast<size_type>(node.seq.siz);
        }
        static inline
        size_type
        capacity(Node const & node)
        {
            return static_cast<size_type>(Node::Cap::at(node.seq.exp));
        }
        static inline
        typename Container::const_pointer
        raw(Node const & node)
        {
            return node.seq.raw.ptr;
        }
        static inline
        typename Container::pointer
        raw(Node & node)
        {
            return const_cast<typename Container::pointer>
                (raw(static_cast<Node const &>(node)));
        }
        template<typename PoolType>
        static inline
        void
        reserve(Node & node, size_type cap, PoolType & pool)
        {
            if (cap <= capacity(node))
                return;

            typedef typename Container::value_type value_type;
            typedef typename Container::pointer    pointer;

            /* alloc new space */
            uint8_t exp = Node::Cap::right(cap);
                    cap = Node::Cap::at(exp);
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
        template<typename PoolType>
        static inline
        void
        resize(Node & node, size_type siz, PoolType & pool)
        {
            typedef typename Container::pointer pointer;

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

    template<typename CharType>
    struct Traits<Node<CharType>, MAP>
    {
    public:
        static const Tag TAG = MAP;

    public:
        typedef Node<CharType>           Node;
        typedef typename Node::Pair      Pair;
        typedef typename Node::size_type size_type;

        struct Container
        {
            typedef Pair               value_type;
            typedef value_type       * pointer;
            typedef value_type const * const_pointer;
            typedef value_type       & reference;
            typedef value_type const & const_reference;
            typedef       pointer                    iterator;
            typedef const_pointer              const_iterator;
            typedef       pointer            reverse_iterator; /* TODO: */
            typedef const_pointer      const_reverse_iterator; /* TODO: */
            typedef Node     const &   key_const_reference;
            typedef size_type          index_type;
            typedef void               void_type;

            template<typename PoolType>
            static inline void copy
            (reference lhs, const_reference rhs, PoolType & pool)
            {
                lhs[0].copy(rhs[0], pool);
                lhs[1].copy(rhs[1], pool);
            }
            template<typename PoolType>
            static inline void move
            (reference lhs, reference rhs, PoolType & pool)
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
        struct Layout
        {
            uint8_t  tag;
            uint8_t  pad[2];
            uint8_t  exp;
            uint32_t siz;
            union
            {
                typename Container::pointer ptr;
                uint64_t                    pad;
            }        raw;
        };

    public:
        template<typename PoolType>
        static inline
        void
        construct(Node & node, PoolType & /*pool*/)
        {
            node.map.raw.ptr = NULL;
            node.map.siz = 0;
            node.map.exp = 0;
            node.map.tag = TAG;
        }
        template<typename PoolType>
        static inline
        void
        destruct(Node & node, PoolType & pool)
        {
            typedef typename Container::pointer pointer;

            pointer beg = raw(node);
            pointer end = beg + size(node);
            for (pointer cur = beg; cur != end; ++cur) {
                (*cur)[0].destruct(pool);
                (*cur)[1].destruct(pool);
            }

            if (beg != NULL) {
                Node *  mem = reinterpret_cast<Node *>(beg);
                size_type siz = capacity(node) << 1;
                pool.deallocate(mem, siz);
            }

            node.map.tag = NIL;
        }
        template<typename PoolType>
        static inline
        void
        copy(Node & lhs, Node const & rhs, PoolType & pool)
        {
            typedef typename Container::      pointer       pointer;
            typedef typename Container::const_pointer const_pointer;

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
        equal(Node const & lhs, Node const & rhs)
        {
            if (size(lhs) != size(rhs))
                return false;

            typedef typename Container::const_pointer const_pointer;
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
        size(Node const & node)
        {
            return static_cast<size_type>(node.map.siz);
        }
        static inline
        size_type
        capacity(Node const & node)
        {
            return static_cast<size_type>(Node::Cap::at(node.map.exp));
        }
        static inline
        typename Container::const_pointer
        raw(Node const & node)
        {
            return node.map.raw.ptr;
        }
        static inline
        typename Container::pointer
        raw(Node & node)
        {
            return const_cast<typename Container::pointer>
                (raw(static_cast<Node const &>(node)));
        }
        template<typename PoolType>
        static inline
        void
        reserve(Node & node, size_type cap, PoolType & pool)
        {
            if (cap <= capacity(node))
                return;

            typedef typename Container::pointer pointer;

            /* alloc new space */
            uint8_t exp = Node::Cap::right(cap);
                    cap = Node::Cap::at(exp);
            pointer mem = reinterpret_cast<pointer>(
                pool.template allocate<Node>(cap << 1)
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
                Node *  old = reinterpret_cast<Node *>(beg);
                size_type siz = capacity(node) << 1;
                pool.deallocate(old, siz);
            }

            /* update */
            node.map.exp     = exp;
            node.map.raw.ptr = mem;
        }
        template<typename PoolType>
        static inline
        void
        resize(Node & node, size_type siz, PoolType & pool)
        {
            typedef typename Container::pointer pointer;

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

    template<typename CharType> template<typename PoolType>
    inline void Node<CharType>::construct(PoolType & pool)
    {
        Traits<Node, NIL>::construct(*this, pool);
    }

    template<typename CharType> template<typename PoolType>
    inline void Node<CharType>::
    construct(Tag tag, PoolType & pool)
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

    template<typename CharType> template<typename PoolType>
    inline void Node<CharType>::
    destruct(PoolType & pool)
    {
        /* mark this as NIL(no need to destruct)
         * to prevent potential infinite recursion.
         */
        uint8_t tag = nil.tag;
        nil.    tag = NIL;

        switch (tag)
        {
        case NIL: { break; }
        case I64: { Traits<Node, I64>::destruct(*this, pool); break; }
        case DBL: { Traits<Node, DBL>::destruct(*this, pool); break; }
        case STR: { Traits<Node, STR>::destruct(*this, pool); break; }
        case SEQ: { Traits<Node, SEQ>::destruct(*this, pool); break; }
        case MAP: { Traits<Node, MAP>::destruct(*this, pool); break; }
        default:  { exception::node_type_out_of_range(tag, POS_); break; }
        }
    }

    template<typename CharType> template<typename PoolType>
    inline void Node<CharType>::
    copy(Node const & rhs, PoolType & pool)
    {
        Node & lhs = *this;
        if (&lhs == &rhs)
            return;

        if (lhs.type() == NIL)
        {
            Tag tag = rhs.type();
            lhs.construct(tag, pool);

            /* copy data */
            switch (tag)
            {
            case NIL: { break; }
            case I64: { Traits<Node, I64>::copy(lhs, rhs, pool); break; }
            case DBL: { Traits<Node, DBL>::copy(lhs, rhs, pool); break; }
            case STR: { Traits<Node, STR>::copy(lhs, rhs, pool); break; }
            case SEQ: { Traits<Node, SEQ>::copy(lhs, rhs, pool); break; }
            case MAP: { Traits<Node, MAP>::copy(lhs, rhs, pool); break; }
            default:  { exception::node_type_out_of_range(tag,POS_); break; }
            }
        }
        else
        {
            /* there may be parent-child relationship between lhs and rhs,
             * using a tmp node to avoid rhs destructed before copy starting.
             */

            /* tmp = rhs */
            Node tmp;
            tmp.construct(pool);
            tmp.copy(rhs, pool);

            /* lhs = move(tmp) */
            lhs. destruct(pool);
            lhs.construct(pool);
            lhs.move(tmp, pool);
        }
    }

    template<typename CharType> template<typename PoolType>
    inline void Node<CharType>::
    move(Node & rhs, PoolType & pool)
    {
        Node & lhs = *this;
        if (&lhs == &rhs)
            return;

        if (lhs.type() == NIL) {
            ::memcpy(&lhs, &rhs, sizeof(Node));
            ::memset(&rhs,    0, sizeof(Node));
        } else {
            /* there may be parent-child relationship between lhs and rhs,
             * using a tmp node to avoid rhs destructed before copy starting.
             */

            /* tmp = move(rhs), separated form this */
            Node tmp;
            tmp.construct(pool);
            tmp.move(rhs, pool);

            /* lhs = move(tmp) */
            lhs. destruct(pool);
            lhs.construct(pool);
            lhs.move(tmp, pool);
        }
    }

    template<typename CharType>
    inline void Node<CharType>::
    swap(Node & rhs)
    {
        Node & lhs = *this;
        Node   tmp;

        ::memcpy(&tmp, &rhs, sizeof(Node));
        ::memcpy(&rhs, &lhs, sizeof(Node));
        ::memcpy(&lhs, &tmp, sizeof(Node));
        ::memset(&tmp,    0, sizeof(Node));
    }

    template<typename CharType>
    inline bool Node<CharType>::
    equal(Node const & rhs) const
    {
        Node const & lhs = *this;

        if (&lhs == &rhs)
            return true;
        if (lhs.type() != rhs.type())
            return false;

        Tag tag = lhs.type();
        switch (tag)
        {
        case NIL: { return Traits<Node, NIL>::equal(lhs, rhs); break; }
        case I64: { return Traits<Node, I64>::equal(lhs, rhs); break; }
        case DBL: { return Traits<Node, DBL>::equal(lhs, rhs); break; }
        case STR: { return Traits<Node, STR>::equal(lhs, rhs); break; }
        case SEQ: { return Traits<Node, SEQ>::equal(lhs, rhs); break; }
        case MAP: { return Traits<Node, MAP>::equal(lhs, rhs); break; }
        default:  { exception::node_type_out_of_range(tag,POS_); break; }
        }

        return false;
    }

    template<typename CharType>
    inline Tag Node<CharType>::
    type() const
    {
        /* tag is their common initial sequence */
        return static_cast<Tag>(nil.tag);
    }

    template<typename CharType> template<Tag TAG, typename PoolType>
    inline void Node<CharType>::
    construct(PoolType & pool)
    {
        Traits<Node, TAG>::construct(*this, pool);
    }

    template<typename CharType> template<Tag TAG, typename PoolType>
    inline void Node<CharType>::
    set(typename Traits<Node, TAG>::const_reference value, PoolType & pool)
    {
        destruct      (pool);
        construct<TAG>(pool);
        Traits<Node, TAG>::val(*this) = value;
    }

    template<typename CharType> template<Tag TAG, typename PoolType>
    inline void Node<CharType>::
    set
    (
        typename Traits<Node, TAG>::Container::const_iterator ibeg,
        typename Traits<Node, TAG>::Container::const_iterator iend,
        PoolType & pool
    )
    {
        destruct      (pool);
        construct<TAG>(pool);

        typedef Traits<Node, TAG>                Traits;
        typedef typename Traits::Container         Container;
        typedef typename Container::      iterator       iterator;
        typedef typename Container::const_iterator const_iterator;

        size_type len = static_cast<size_type>(iend - ibeg);
        Traits::resize(*this, len, pool);

              iterator ilhs = begin<TAG>();
        const_iterator irhs = ibeg;
        while (irhs != iend) {
            Container::copy((*ilhs), (*irhs), pool);
            ++ilhs;
            ++irhs;
        }
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::const_reference
    Node<CharType>::
    val() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);
        return Traits<Node, TAG>::val(*this);
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::reference
    Node<CharType>::
    val()
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);
        return Traits<Node, TAG>::val(*this);
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::size_type
    Node<CharType>::
    size() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);
        return Traits<Node, TAG>::size(*this);
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::size_type
    Node<CharType>::
    capacity() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);
        return Traits<Node, TAG>::capacity(*this);
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    const_pointer Node<CharType>::
    raw() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);
        return Traits<Node, TAG>::raw(*this);
    }

    template<typename CharType> template<Tag TAG, typename PoolType>
    inline typename Traits<Node<CharType>, TAG>::Container::
    void_type Node<CharType>::
    push_back
    (
        typename Traits<Node, TAG>::Container::const_reference value,
        PoolType & pool
    )
    {
        typedef Traits<Node, TAG>                   Traits;
        typedef typename Traits::size_type            size_type;
        typedef typename Traits::Container::reference reference;

        if (type() == NIL)
            Traits::construct(*this, pool);
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        size_type siz = Traits::size(*this);
        Traits::resize(*this, siz + 1, pool);

        reference bak = Traits::raw(*this)[siz];
        Traits::Container::copy(bak, value, pool);
    }

    template<typename CharType> template<Tag TAG, typename PoolType>
    inline typename Traits<Node<CharType>, TAG>::Container::
    void_type Node<CharType>::
    move_back
    (
        typename Traits<Node, TAG>::Container::reference value,
        PoolType & pool
    )
    {
        typedef Traits<Node, TAG>                   Traits;
        typedef typename Traits::size_type            size_type;
        typedef typename Traits::Container::reference reference;

        if (type() == NIL)
            Traits::construct(*this, pool);
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        size_type siz = Traits::size(*this);
        Traits::resize(*this, siz + 1, pool);

        reference bak = Traits::raw(*this)[siz];
        Traits::Container::move(bak, value, pool);
    }

    template<typename CharType> template<Tag TAG, typename PoolType>
    inline typename Traits<Node<CharType>, TAG>::Container::
    iterator Node<CharType>::
    erase
    (
        typename Traits<Node, TAG>::Container::const_iterator citer,
        PoolType & pool
    )
    {
        typedef Traits<Node, TAG>          Traits;
        typedef typename Traits::Container   Container;
        typedef typename Container::iterator iterator;

        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        iterator ibeg = begin<TAG>();
        iterator iend = end  <TAG>();
        iterator iter = ibeg + (citer - ibeg);
        iterator ipos = iter;
        if (iter < ibeg || iter >= iend)
            return iend;

        while (++iter != iend)
            Traits::Container::move((*(iter - 1)), (*iter), pool);

        Traits::resize(*this, Traits::size(*this) - 1, pool);
        return ipos;
    }

    template<typename CharType> template<Tag TAG, typename PoolType>
    inline typename Traits<Node<CharType>, TAG>::Container::
    iterator Node<CharType>::
    erase
    (
        typename Traits<Node, TAG>::Container::const_iterator cifst,
        typename Traits<Node, TAG>::Container::const_iterator cilst,
        PoolType & pool
    )
    {
        typedef Traits<Node, TAG>          Traits;
        typedef typename Traits::Container   Container;
        typedef typename Container::iterator iterator;

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
            = Traits::size(*this)
            - static_cast<size_type>(ilst - ifst)
            ;

        while (ilst != iend) {
            Traits::Container::move((*ifst), (*ilst), pool);
            ++ifst;
            ++ilst;
        }

        Traits::resize(*this, siz, pool);
        return ipos;
    }

    template<typename CharType> template<Tag TAG, typename PoolType>
    inline typename Traits<Node<CharType>, TAG>::Container::
    void_type Node<CharType>::
    pop_back(PoolType & pool)
    {
        typedef Traits<Node, TAG>        Traits;
        typedef typename Traits::size_type size_type;

        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        size_type siz = Traits::size(*this);
        if (siz > 0)
            Traits::resize(*this, siz - 1, pool);
    }

    template<typename CharType> template<Tag TAG, typename PoolType>
    inline typename Traits<Node<CharType>, TAG>::Container::
    void_type Node<CharType>::
    clear(PoolType & pool)
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        Traits<Node, TAG>::resize(*this, 0, pool);
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    const_iterator Node<CharType>::
    begin() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        return Traits<Node, TAG>::raw(*this);
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::iterator
    Node<CharType>::
    begin()
    {
        return const_cast<typename Traits<Node, TAG>::Container::iterator>
            (static_cast<Node const &>(*this).begin<TAG>());
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    const_iterator Node<CharType>::
    end() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        return Traits<Node, TAG>::raw (*this)
            +  Traits<Node, TAG>::size(*this)
            ;
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::iterator
    Node<CharType>::
    end()
    {
        return const_cast<typename Traits<Node, TAG>::Container::iterator>
            (static_cast<Node const &>(*this).end<TAG>());
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    const_reverse_iterator Node<CharType>::
    rbegin() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        return Traits<Node, TAG>::raw (*this)
            +  Traits<Node, TAG>::size(*this)
            -  1
            ;
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    reverse_iterator Node<CharType>::
    rbegin()
    {
        return const_cast
            <typename Traits<Node, TAG>::Container::reverse_iterator>
            (static_cast<Node const &>(*this).rbegin<TAG>());
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    const_reverse_iterator Node<CharType>::
    rend() const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        return Traits<Node, TAG>::raw(*this)
            -  1
            ;
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    reverse_iterator Node<CharType>::
    rend()
    {
        return const_cast
            <typename Traits<Node, TAG>::Container::reverse_iterator>
            (static_cast<Node const &>(*this).rend<TAG>());
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    const_iterator Node<CharType>::
    at(typename Traits<Node, TAG>::Container::index_type idx) const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        if (idx <  Traits<Node, TAG>::size(*this))
            return Traits<Node, TAG>::raw (*this) + idx;

        return Traits<Node, TAG>::raw (*this)
            +  Traits<Node, TAG>::size(*this)
            ;
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    iterator Node<CharType>::
    at(typename Traits<Node, TAG>::Container::index_type idx)
    {
        return const_cast<typename Traits<Node, TAG>::Container::iterator>
            (static_cast<Node const &>(*this).at<TAG>(idx));
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    const_iterator Node<CharType>::
    find(typename Traits<Node, TAG>::Container::key_const_reference key)
    const
    {
        if (type() != TAG)
            exception::node_type_not_match(type(), TAG, POS_);

        typedef typename Traits<Node, TAG>::Container Container;
        typedef typename Container::const_iterator      const_iterator;

        const_iterator ibeg = begin<TAG>();
        const_iterator iend = end  <TAG>();
        for (const_iterator iter = ibeg; iter < iend; ++iter)
            if (Container::equal((*iter), key))
                return iter;

        return iend;
    }

    template<typename CharType> template<Tag TAG>
    inline typename Traits<Node<CharType>, TAG>::Container::
    iterator Node<CharType>::
    find(typename Traits<Node, TAG>::Container::key_const_reference key)
    {
        return const_cast<typename Traits<Node, TAG>::Container::iterator>
            (static_cast<Node const &>(*this).find<TAG>(key));
    }
}

CV_FS_PRIVATE_END
