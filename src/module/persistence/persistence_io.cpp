/****************************************************************************
 *  license
 ***************************************************************************/

#include <cstdio>
#include <cstring>
#include <limits>
#include <sstream>
#include "persistence_io.hpp"
#include "persistence_private.hpp"
#include "persistence_utility.hpp"

CV_FS_PRIVATE_BEGIN

namespace exception
{
    /***********************************************************************
     * exception
     ***********************************************************************/

    using chars::soss_t;
    using chars::fmt;

    inline static void file_too_large(long long size, POS_TYPE_)
    {
		error(0,
            ( soss_t<char>()
                * "file `"
                | fmt<32>(size)
                | " byte` is too large"
            ), POS_ARGS_
        );
    }
}

namespace io
{
    /************************************************************************
     * stringstream_t
    ************************************************************************/
    class stringstream_t : public stream_t
    {
    public:
        stringstream_t()
            : stream()
            , is_writing(false)
        {
            stream.setstate(std::ios_base::badbit);
        }
        ~stringstream_t()
        {
            if (is_open())
                close();
        }
    public:
        virtual bool open(const_string_t str, mode_t mode)       /*override*/
        {
            if (is_open())
                close();
            stream.clear();
            switch (mode)
            {
            case READ:  { if(str) stream.str(str); is_writing = false;break;}
            case WRITE: {                          is_writing = true; break;}
            case APPEND:{ if(str) stream.str(str); is_writing = true; break;}
            default:    {                                             break;}
            }
            return is_open();
        }
        virtual bool is_open() const                             /*override*/
        {
            return !stream.bad();
        }
        virtual void close()                                     /*override*/
        {
            stream.str();
            stream.setstate(std::ios_base::badbit);
        }
        virtual void seek(pos_t offset, seek_t origin)           /*override*/
        {
            switch (origin)
            {
            case BEG: { stream.seekp(offset, std::ios::beg); break; }
            case CUR: { stream.seekp(offset, std::ios::cur); break; }
            case END: { stream.seekp(offset, std::ios::end); break; }
            default:  { break; }
            }
        }
        virtual pos_t tell() /* override */
        {
            return static_cast<pos_t>
                (is_writing ? stream.tellp() : stream.tellg());
        }
        virtual size_type write(const_string_t buffer, size_type size)
            /* override */
        {
            stream.write(buffer, size);
            return size;
        }
        virtual size_type read(string_t buffer, size_type size)
            /* override */
        {
            typedef std::streamsize arg_t;
            ASSERT
            (uint64_t(size) <= uint64_t(std::numeric_limits<arg_t>::max()));

            stream.read(buffer, static_cast<arg_t>(size));
            ASSERT(stream.gcount() >= 0);

            return static_cast<size_type>(stream.gcount());
        }
        virtual buffer_t dump() /* override */
        {
            pos_t backup = tell();
            seek(0, END);
            pos_t count  = tell() / sizeof(char_t);
            seek(0, BEG);

            ASSERT(count >= 0);
            if (uint64_t(count)>uint64_t(std::numeric_limits<size_t>::max()))
                exception::file_too_large(count, POS_);

			buffer_t buffer(static_cast<size_t>(count));
            write(buffer, static_cast<size_type>(count));
            seek(backup, BEG);
            return buffer;
        }

    private:
        std::stringstream stream;
        bool is_writing;
    };

    /************************************************************************
     * filestream_t
    ************************************************************************/

    class filestream_t : public stream_t
    {
    public:
        filestream_t()
            : stream(NULL)
        {}
        ~filestream_t()
        {
            if (is_open())
                close();
        }
    public:
        virtual bool open(const char * path, mode_t mode)/*override*/
        {
            const char * fmode = NULL;

            switch (mode)
            {
            case READ:  { fmode = "r"; break; }
            case WRITE: { fmode = "w"; break; }
            case APPEND:{ fmode = "a"; break; }
            default:    { return false; }
            }

            if (is_open())
                close();

            stream = std::fopen(path, fmode);
            return is_open();
        }
        virtual bool is_open() const                             /*override*/
        {
            return stream != NULL;
        }
        virtual void close()                                     /*override*/
        {
            std::fclose(stream);
            stream = NULL;
        }
        virtual void seek(pos_t offset, seek_t origin)           /*override*/
        {
            int  way  = 0;
            bool is_valid = true;

            switch (origin)
            {
            case BEG: { way   = SEEK_SET; break; }
            case CUR: { way   = SEEK_CUR; break; }
            case END: { way   = SEEK_END; break; }
            default:  { is_valid = false; break; }
            }

            ASSERT(pos_t(offset) <= pos_t(std::numeric_limits<long>::max()));
            if (is_valid)
                std::fseek(stream, static_cast<long>(offset), way);
        }
        virtual pos_t tell()                                     /*override*/
        {
            /* warning: std::numeric_limits<long>::max() < 2.1G */
            long result = std::ftell(stream);
            if (result < 0)
                return pos_t(-1);
            else
                return static_cast<pos_t>(result);
        }
        virtual size_type write(const_string_t buffer, size_type size)
            /* override */
        {
            ASSERT
            (uint64_t(size) <= uint64_t(std::numeric_limits<size_t>::max()));
            return std::fwrite(buffer, sizeof(char_t), size_t(size), stream);
        }
        virtual size_type read(string_t buffer, size_type size)
            /* override */
        {
            ASSERT
            (uint64_t(size) <= uint64_t(std::numeric_limits<size_t>::max()));
            return std::fread(buffer, sizeof(char_t), size_t(size), stream);
        }
        virtual buffer_t dump()                       /*override*/
        {
            pos_t backup = tell();
            seek(0, END);
            pos_t count  = tell() / sizeof(char_t);
            seek(0, BEG);

            ASSERT(count >= 0);
            if (uint64_t(count)>uint64_t(std::numeric_limits<size_t>::max()))
                exception::file_too_large(count, POS_);

			buffer_t buffer(static_cast<size_t>(count));
            write(buffer, static_cast<size_type>(count));
            seek(backup, BEG);
            return buffer;
        }

    private:
        filestream_t            (filestream_t const &);
        filestream_t & operator=(filestream_t const &);

    private:
        std::FILE * stream;
    };

    /************************************************************************
     * stream_t
    ************************************************************************/

    stream_t::~stream_t() {}

    stream_t * stream_t::build(stream_type type)
    {
        switch (type)
        {
        case FILE   : { return new   filestream_t(); }
        case STRING : { return new stringstream_t(); }
        default:      { return NULL; }
        }
    }
}

CV_FS_PRIVATE_END
