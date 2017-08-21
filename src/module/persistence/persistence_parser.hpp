/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once

#include "persistence_private.hpp"
#include "persistence_string.hpp"
#include "persistence_ast.hpp"
#include "persistence_io.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 *  Forward Declaration
 ***************************************************************************/

namespace parser
{
    using io::Stream;
    using ast::Tree;

    typedef chars::Buffer<char, 128, std::allocator> Message;
}

/****************************************************************************
 *  Declaration
 ***************************************************************************/

namespace parser
{
    struct Settings
    {
        Settings()
            : enable_json_comment(true)
            , enable_warning_message(true)
            , treate_warning_as_error(false)
            , warning_maximum(4U)
            , stream_buffer_size(8192U)
            , indent_width(4U)
        {}

        bool   enable_json_comment;
        bool   enable_warning_message;
        bool   treate_warning_as_error;
        size_t warning_maximum;
        size_t stream_buffer_size;
        size_t indent_width;      /* '\t' == n' ' */
    };

    typedef bool (*ParseFuncion) (
        Stream         &,
        Tree<char>     &,
        Message        &,
        Settings const &
    );
}

namespace parser { namespace xml
{
    extern bool parse
    (
        Stream         & stream,
        Tree<char>     & result,
        Message        & message,
        Settings const & settings = Settings()
    );
}}

namespace parser { namespace yaml
{
    extern bool parse
    (
        Stream         & stream,
        Tree<char>     & result,
        Message        & message,
        Settings const & settings = Settings()
    );
}}

namespace parser { namespace json
{
    extern bool parse
    (
        Stream         & stream,
        Tree<char>     & result,
        Message        & message,
        Settings const & settings = Settings()
    );
}}

CV_FS_PRIVATE_END
