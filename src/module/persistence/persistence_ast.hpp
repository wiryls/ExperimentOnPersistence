/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once

#include "persistence_private.hpp"
#include "persistence_ast_node.hpp"
#include "persistence_pool.hpp"

CV_FS_PRIVATE_BEGIN

namespace ast
{
    /***********************************************************************
     * tag_t
     ***********************************************************************/

    typedef storage::pool_t<
        typename utility::tl::make_list<
            node_t<char>, char
        >::type,
#ifndef _DEBUG
        storage::fx2allocator_t
#else
        storage::x2allocator_t
#endif
        //std::allocator
        //cv::allocator
    > pool_t;

    static inline const char * to_string(tag_t tag)
    {
        switch (tag)
        {
        case NIL: return "null";
        case I64: return "int64";
        case DBL: return "double";
        case STR: return "string";
        case SEQ: return "sequence";
        case MAP: return "map";
        default:  return "error type";
        }
    }

    /***********************************************************************
     * declaration tree_t
     ***********************************************************************/

    template<typename char_t>
    class tree_t
    {
    public:
        typedef node_t<char_t> node_t;

    public:
        tree_t();

    public:
        inline void clear();
        inline bool empty() const;
        inline node_t const & root() const;
        inline node_t & root();
        inline pool_t & pool();

    private:
        node_t root_;
        pool_t pool_;
    };

    /***********************************************************************
     * implementation tree_t
     ***********************************************************************/

    template<typename char_t>
    inline tree_t<char_t>::tree_t()
        : root_()
        , pool_()
    {
        root_.construct(pool_);
    }

    template<typename char_t>
    inline bool tree_t<char_t>::empty() const
    {
        return (root_.type() == NIL);
    }

    template<typename char_t>
    inline void tree_t<char_t>::clear()
    {
        root_. destruct(pool_);
        root_.construct(pool_);
    }

    template<typename char_t>
    inline node_t<char_t> const & tree_t<char_t>::root() const
    {
        return root_;
    }

    template<typename char_t>
    inline node_t<char_t> & tree_t<char_t>::root()
    {
        return root_;
    }

    template<typename char_t>
    inline pool_t & tree_t<char_t>::pool()
    {
        return pool_;
    }
}

CV_FS_PRIVATE_END
