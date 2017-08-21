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
     * Tag
     ***********************************************************************/

    typedef storage::Pool<
        typename utility::tl::MakeList<
            Node<char>, char
        >::type,
#       ifndef _DEBUG
            storage::FFAllocator
#       else
            storage::FAllocator
#       endif
        //std::allocator
        //cv::allocator
    > Pool;

    static inline const char * to_string(Tag tag)
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
     * declaration Tree
     ***********************************************************************/

    template<typename CharType>
    class Tree
    {
    public:
        typedef Node<CharType> Node;

    public:
        Tree();

    public:
        inline void clear();
        inline bool empty() const;
        inline Node const & root() const;
        inline Node & root();
        inline Pool & pool();

    private:
        Node root_;
        Pool pool_;
    };

    /***********************************************************************
     * implementation Tree
     ***********************************************************************/

    template<typename CharType>
    inline Tree<CharType>::Tree()
        : root_()
        , pool_()
    {
        root_.construct(pool_);
    }

    template<typename CharType>
    inline bool Tree<CharType>::empty() const
    {
        return (root_.type() == NIL);
    }

    template<typename CharType>
    inline void Tree<CharType>::clear()
    {
        root_. destruct(pool_);
        root_.construct(pool_);
    }

    template<typename CharType>
    inline Node<CharType> const & Tree<CharType>::root() const
    {
        return root_;
    }

    template<typename CharType>
    inline Node<CharType> & Tree<CharType>::root()
    {
        return root_;
    }

    template<typename CharType>
    inline Pool & Tree<CharType>::pool()
    {
        return pool_;
    }
}

CV_FS_PRIVATE_END
