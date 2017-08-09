/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once

#include "persistence_private.hpp"
#include "persistence_string.hpp"

CV_FS_PRIVATE_BEGIN

namespace io
{
    class stream_t;
    typedef chars::buffer_t<char, 1024, std::allocator> buffer_t;
}

namespace io
{
    /************************************************************************
     * mode_t
    ************************************************************************/

    enum mode_t
    {
        READ,
        WRITE,
        APPEND
    };

    /************************************************************************
     * stream_t
    ************************************************************************/

    enum seek_t
    {
        BEG,
        CUR,
        END
    };

    enum stream_type
    {
        FILE,
        STRING
    };

    class stream_t
    {
    public:
        typedef char           char_t;
        typedef char_t       * string_t;
        typedef char_t const * const_string_t;
        typedef int64_t        pos_t;
        typedef uint64_t       size_type;

    public:
        virtual ~stream_t();

        virtual bool     open(const_string_t path, mode_t mode)        = 0;
        virtual bool  is_open() const                                  = 0;
        virtual void    close()                                        = 0;

        virtual void     seek(         pos_t offset, seek_t origin)    = 0;
        virtual pos_t    tell()                                        = 0;

        virtual size_type write(const_string_t buffer, size_type size) = 0;
        virtual size_type read(       string_t buffer, size_type size) = 0;

        virtual buffer_t  dump()                                       = 0;

    public:
        static stream_t * build(stream_type type);
    };
}

CV_FS_PRIVATE_END
