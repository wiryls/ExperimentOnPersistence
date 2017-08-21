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

    using chars::Soss;
    using chars::fmt;

    inline static void file_too_large(long long size, POS_TYPE_)
    {
        error(0,
            ( Soss<char>()
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
     * StringStream
    ************************************************************************/
    class StringStream : public Stream
    {
    public:
        StringStream()
            : stream()
            , is_writing(false)
        {
            stream.setstate(std::ios_base::badbit);
        }
        ~StringStream()
        {
            if (is_open())
                close();
        }
    public:
        virtual bool open(ConstString str, Mode mode)       /*override*/
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
        virtual void seek(Pos offset, Seek origin)           /*override*/
        {
            switch (origin)
            {
            case BEG: { stream.seekp(offset, std::ios::beg); break; }
            case CUR: { stream.seekp(offset, std::ios::cur); break; }
            case END: { stream.seekp(offset, std::ios::end); break; }
            default:  { break; }
            }
        }
        virtual Pos tell() /* override */
        {
            return static_cast<Pos>
                (is_writing ? stream.tellp() : stream.tellg());
        }
        virtual size_type write(ConstString buffer, size_type size)
            /* override */
        {
            stream.write(buffer, size);
            return size;
        }
        virtual size_type read(String buffer, size_type size)
            /* override */
        {
            typedef std::streamsize arg_t;
            ASSERT
            (uint64_t(size) <= uint64_t(std::numeric_limits<arg_t>::max()));

            stream.read(buffer, static_cast<arg_t>(size));
            ASSERT(stream.gcount() >= 0);

            return static_cast<size_type>(stream.gcount());
        }
        virtual Buffer dump() /* override */
        {
            Pos backup = tell();
            seek(0, END);
            Pos count  = tell() / sizeof(CharType);
            seek(0, BEG);

            ASSERT(count >= 0);
            if (uint64_t(count)>uint64_t(std::numeric_limits<size_t>::max()))
                exception::file_too_large(count, POS_);

            Buffer buffer(static_cast<size_t>(count));
            write(buffer, static_cast<size_type>(count));
            seek(backup, BEG);
            return buffer;
        }

    private:
        std::stringstream stream;
        bool is_writing;
    };

    /************************************************************************
     * FileStream
    ************************************************************************/

    class FileStream : public Stream
    {
    public:
        FileStream()
            : stream(NULL)
        {}
        ~FileStream()
        {
            if (is_open())
                close();
        }
    public:
        virtual bool open(const char * path, Mode mode)/*override*/
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
        virtual void seek(Pos offset, Seek origin)           /*override*/
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

            ASSERT(Pos(offset) <= Pos(std::numeric_limits<long>::max()));
            if (is_valid)
                std::fseek(stream, static_cast<long>(offset), way);
        }
        virtual Pos tell()                                     /*override*/
        {
            /* warning: std::numeric_limits<long>::max() < 2.1G */
            long result = std::ftell(stream);
            if (result < 0)
                return Pos(-1);
            else
                return static_cast<Pos>(result);
        }
        virtual size_type write(ConstString buffer, size_type size)
            /* override */
        {
            ASSERT
            (uint64_t(size) <= uint64_t(std::numeric_limits<size_t>::max()));
            return std::fwrite(buffer, sizeof(CharType), size_t(size), stream);
        }
        virtual size_type read(String buffer, size_type size)
            /* override */
        {
            ASSERT
            (uint64_t(size) <= uint64_t(std::numeric_limits<size_t>::max()));
            return std::fread(buffer, sizeof(CharType), size_t(size), stream);
        }
        virtual Buffer dump()                       /*override*/
        {
            Pos backup = tell();
            seek(0, END);
            Pos count  = tell() / sizeof(CharType);
            seek(0, BEG);

            ASSERT(count >= 0);
            if (uint64_t(count)>uint64_t(std::numeric_limits<size_t>::max()))
                exception::file_too_large(count, POS_);

            Buffer buffer(static_cast<size_t>(count));
            write(buffer, static_cast<size_type>(count));
            seek(backup, BEG);
            return buffer;
        }

    private:
        FileStream            (FileStream const &);
        FileStream & operator=(FileStream const &);

    private:
        std::FILE * stream;
    };

    /************************************************************************
     * Stream
    ************************************************************************/

    Stream::~Stream() {}

    Stream * Stream::build(StreamTarget type)
    {
        switch (type)
        {
        case FILE   : { return new   FileStream(); }
        case STRING : { return new StringStream(); }
        default:      { return NULL; }
        }
    }
}

CV_FS_PRIVATE_END
