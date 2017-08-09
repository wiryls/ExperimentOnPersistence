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
    using io::stream_t;
    using ast::tree_t;

    typedef chars::buffer_t<char, 128, std::allocator> message_t;
}

/****************************************************************************
 *  Declaration
 ***************************************************************************/

namespace parser
{
    struct settings_t
    {
        settings_t()
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

    typedef bool (*parse_funcion_t) (
        stream_t         &,
        tree_t<char>     &,
        message_t        &,
        settings_t const &
    );
}

namespace parser { namespace xml
{
    extern bool parse
    (
        stream_t         & stream,
        tree_t<char>     & result,
        message_t        & message,
        settings_t const & settings = settings_t()
    );
}}

namespace parser { namespace yaml
{
    extern bool parse
    (
        stream_t         & stream,
        tree_t<char>     & result,
        message_t        & message,
        settings_t const & settings = settings_t()
    );
}}

namespace parser { namespace json
{
    extern bool parse
    (
        stream_t         & stream,
        tree_t<char>     & result,
        message_t        & message,
        settings_t const & settings = settings_t()
    );
}}

CV_FS_PRIVATE_END
