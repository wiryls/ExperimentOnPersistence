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
    class Stream;
    typedef chars::Buffer<char, 1024, std::allocator> Buffer;
}

namespace io
{
    /************************************************************************
     * Mode
    ************************************************************************/

    enum Mode
    {
        READ,
        WRITE,
        APPEND
    };

    /************************************************************************
     * Stream
    ************************************************************************/

    enum Seek
    {
        BEG,
        CUR,
        END
    };

    enum StreamTarget
    {
        FILE,
        STRING
    };

    class Stream
    {
    public:
        typedef char             CharType;
        typedef CharType       * String;
        typedef CharType const * ConstString;
        typedef int64_t          Pos;
        typedef uint64_t         size_type;

    public:
        virtual ~Stream();

        virtual bool     open(ConstString path, Mode mode)          = 0;
        virtual bool  is_open() const                               = 0;
        virtual void    close()                                     = 0;

        virtual void     seek(         Pos offset, Seek origin)     = 0;
        virtual Pos    tell()                                       = 0;

        virtual size_type write(ConstString buffer, size_type size) = 0;
        virtual size_type read (     String buffer, size_type size) = 0;

        virtual Buffer  dump()                                      = 0;

    public:
        static Stream * build(StreamTarget type);
    };
}

CV_FS_PRIVATE_END
