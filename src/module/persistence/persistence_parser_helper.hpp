/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once

#include <cstdio>
#include <exception>

#include "persistence_private.hpp"
#include "persistence_chars.hpp"
#include "persistence_utility.hpp"
#include "persistence_parser.hpp"

CV_FS_PRIVATE_BEGIN

/***************************************************************************
 * Declaration
 ***************************************************************************/

namespace exception
{
    using chars::Soss;
    using chars::fmt;

    class ParseError : public std::exception
    {
    public:
        ParseError()                 NOTHROW :soss() {}
        ParseError(const char * msg) NOTHROW :soss() { soss*fmt<512>(msg); }
        virtual ~ParseError()        NOTHROW         {}
        virtual const char *what() const NOTHROW { return soss; }

    private:
        Soss<char, 512> soss;
    };

    inline static void cannot_compare(
        size_t buf_len,
        size_t str_len,
        const char * str,
        POS_TYPE_)
    {
        error(0,
            ( Soss<char, 256>()
                * "buffer size `"
                | fmt<32>(buf_len)
                | "` of stream helper is less than string `"
                | fmt<32>(str)
                | "` size `"
                | fmt<32>(str_len)
                | "`"
            ), POS_ARGS_
        );
    }
}

namespace parser
{
    /************************************************************************
     *  Keyword
     ***********************************************************************/

    template<typename CharType> class Keyword
    {
    public:
        typedef CharType CharType;

    public:
        template<size_t N> Keyword(CharType const (&str)[N]) NOTHROW
            : ptr(str ? str : dmy)
            , dmy()
            , fst(ptr[0])
            , len(N ? (N - 1) : 0)
        {}

        Keyword(CharType chr) NOTHROW
            : ptr(dmy)
            , dmy()
            , fst(chr)
            , len(1)
        {
            dmy[0] = chr;
        }

        inline operator const CharType * () const NOTHROW { return ptr; }
        inline operator       CharType   () const NOTHROW { return fst; }
        inline size_t                size() const NOTHROW { return len; }

    private:
        CharType const * const ptr;
        CharType               dmy[2];
        CharType const         fst;
        size_t const         len;
    };

    /************************************************************************
     * Declaration StreamHelper
     ***********************************************************************/

    /* support userdata in StreamHelper */
    template<typename DataType> class Holder;
    template<>                  class Holder<void> {};
    template<typename DataType> class Holder
    {
    public:
        typedef DataType           value_type;
        typedef value_type       & reference;
        typedef value_type const & const_reference;
    public:
        explicit Holder(reference ref) : data_(ref) {}
        inline reference       get()       { return data_; }
        inline const_reference get() const { return data_; }
    protected:
        ~Holder() {};
    private:
        value_type & data_;
    };

    template<typename StreamType, typename ExtraDataType>
    class StreamHelper : public Holder<ExtraDataType>
    {
    public:
        typedef ExtraDataType             ExtraDataType;
        typedef StreamType                Stream;
        typedef typename Stream::CharType CharType;

        typedef StreamHelper              This;

    public:
        static const size_t MIN_BUFFER_SIZE = 32U;

    public:
        ~StreamHelper();

        /* valid only if ExtraDataType == void */
        StreamHelper(Stream & stream, Settings const & settings);

        /* valid only if ExtraDataType != void */
        template<typename T>
        StreamHelper
        (
            Stream         & stream,
            Settings const & settings,
            T                & userdata,
            typename utility::EnableIf<
                utility::IsSame<T, ExtraDataType>::value, void *
            >::type = NULL /* compatible with C++98 */
        );

    public:
        inline size_t        line() const;
        inline size_t        col () const;
        inline size_t        pos () const;

        inline bool          eof () const;
        inline CharType      ch  () const;
        inline CharType const *data() const;
        inline size_t        size() const;
        inline size_t        capacity() const;
        inline bool          empty() const;

        inline This &   skip();
        inline This &   skip(size_t skip_n_chars);
        inline This &   skip(CharType           ch,   bool expect=true);
        inline This &   skip(CharType const list[],   bool expect=true);
        inline This &   skip(bool (is_skip)(CharType),bool expect=true);
        inline bool          reload();

        inline size_t             count_warning();
        inline Settings const & get_settings() const;

    private:
        inline void count_char(CharType ch);

    private:
        StreamHelper            (StreamHelper const &);
        StreamHelper & operator=(StreamHelper const &);

    private:
        Stream & stream;
        size_t     buffer_size;
        CharType *   buffer;
        CharType *   buf_beg;
        CharType *   buf_cur;
        CharType *   buf_end;

        size_t     line_number;
        size_t     column_number;
        size_t     position;

        size_t     warning_counter;
        Settings settings;
    };

    /************************************************************************
     * Implementation StreamHelper
     ***********************************************************************/

    template<typename StreamType, typename ExtraDataType>
    inline StreamHelper<StreamType, ExtraDataType>::
        StreamHelper(Stream &stream_ref, Settings const &settings_ref)
        : Holder<ExtraDataType>()
        , stream(stream_ref)
        , buffer_size(
            utility::max(settings_ref.stream_buffer_size, MIN_BUFFER_SIZE)
        )
        , buffer(new CharType[buffer_size + 4U])
        , buf_beg(buffer)
        , buf_cur(buf_beg)
        , buf_end(buf_beg)

        , line_number(1U)
        , column_number(1U)
        , position(1U)

        , warning_counter(0)
        , settings(settings_ref)
    {
        if (stream.is_open())
            reload();

        /* a mark of end of buffer */
        buf_end[0] = CharType('.');
        buf_end[1] = CharType('.');
        buf_end[2] = CharType('.');
        buf_end[3] = CharType('\0');
    }

    template<typename StreamType, typename ExtraDataType> template<typename T>
    inline StreamHelper<StreamType, ExtraDataType>::
        StreamHelper
        (
            Stream         & stream_ref,
            Settings const & settings_ref,
            T                & userdata_ref,
            typename utility::EnableIf<
                utility::IsSame<T, ExtraDataType>::value, void *
            >::type
        )
        : Holder<ExtraDataType>(userdata_ref)
        , stream(stream_ref)
        , buffer_size(
            utility::max(settings_ref.stream_buffer_size, MIN_BUFFER_SIZE)
        )
        , buffer(new CharType[buffer_size + 4U])
        , buf_beg(buffer)
        , buf_cur(buf_beg)
        , buf_end(buf_beg)

        , line_number(1U)
        , column_number(1U)
        , position(1U)

        , warning_counter(0)
        , settings(settings_ref)
    {
        if (stream.is_open())
            reload();

        /* a mark of end of buffer */
        buf_end[0] = CharType('.');
        buf_end[1] = CharType('.');
        buf_end[2] = CharType('.');
        buf_end[3] = CharType('\0');
    }

    template<typename StreamType, typename ExtraDataType>
    inline StreamHelper<StreamType, ExtraDataType>::
    ~StreamHelper()
    {
        delete buffer;
        buffer = NULL;
    }

    template<typename StreamType, typename ExtraDataType>
    inline size_t StreamHelper<StreamType, ExtraDataType>::
        line() const
    {
        return line_number;
    }

    template<typename StreamType, typename ExtraDataType>
    inline size_t StreamHelper<StreamType, ExtraDataType>::
        col() const
    {
        return column_number;
    }

    template<typename StreamType, typename ExtraDataType>
    inline size_t StreamHelper<StreamType, ExtraDataType>::
        pos() const
    {
        return position;
    }

    template<typename StreamType, typename ExtraDataType>
    inline bool StreamHelper<StreamType, ExtraDataType>::
        eof() const
    {
        return buf_beg == buf_end;
    }

    template<typename StreamType, typename ExtraDataType>
    inline typename StreamHelper<StreamType, ExtraDataType>::CharType
        StreamHelper<StreamType, ExtraDataType>::
        ch() const
    {
        return (eof() ? EOF : *buf_cur);
    }

    template<typename StreamType, typename ExtraDataType>
    inline typename StreamHelper<StreamType, ExtraDataType>::CharType
        const * StreamHelper<StreamType, ExtraDataType>::
        data() const
    {
        return buf_cur;
    }

    template<typename StreamType, typename ExtraDataType>
    inline size_t StreamHelper<StreamType, ExtraDataType>::
        size() const
    {
        ASSERT_DBG(buf_end >= buf_cur);
        return static_cast<size_t>(buf_end - buf_cur);
    }

    template<typename StreamType, typename ExtraDataType>
    inline size_t StreamHelper<StreamType, ExtraDataType>::
        capacity() const
    {
        return buffer_size;
    }

    template<typename StreamType, typename ExtraDataType>
    inline bool StreamHelper<StreamType, ExtraDataType>::empty() const
    {
        return buf_cur == buf_end;
    }

    template<typename StreamType, typename ExtraDataType>
    inline typename StreamHelper<StreamType, ExtraDataType>::This &
        StreamHelper<StreamType, ExtraDataType>::
        skip()
    {
        /* note: make sure `empty() == eof()` */
        /* so always do `if (empty()) reload();` */
        if (!eof()) {
            count_char(*buf_cur++);
            if (empty())
                reload();
        }
        return *this;
    }

    template<typename StreamType, typename ExtraDataType>
    inline typename StreamHelper<StreamType, ExtraDataType>::This &
        StreamHelper<StreamType, ExtraDataType>::
        skip(size_t skip_n_chars)
    {
       while (skip_n_chars >= size()) {
           for (skip_n_chars -= size(); !empty();)
               count_char(*buf_cur++);
            if (reload() == false)
                return *this;
        }
       for (CharType *end = buf_cur + skip_n_chars; buf_cur != end;)
           count_char(*buf_cur++);
        return *this;
    }

    template<typename StreamType, typename ExtraDataType>
    inline typename StreamHelper<StreamType, ExtraDataType>::This &
        StreamHelper<StreamType, ExtraDataType>::
        skip(CharType c, bool expect)
    {
       if (eof())
            return *this;
        while ((*buf_cur == c) == expect) {
            count_char(*buf_cur++);
            if (empty() && reload() == false)
                break;
        }
        return *this;
    }

    template<typename StreamType, typename ExtraDataType>
    inline typename StreamHelper<StreamType, ExtraDataType>::This &
        StreamHelper<StreamType, ExtraDataType>::
        skip(CharType const list[], bool expect)
    {
       if (eof())
            return *this;
        while ((chars::strchr(list, *buf_cur) != NULL) == expect) {
            count_char(*buf_cur++);
            if (empty() && reload() == false)
                break;
        }
        return *this;
    }

    template<typename StreamType, typename ExtraDataType>
    inline typename StreamHelper<StreamType, ExtraDataType>::This &
        StreamHelper<StreamType, ExtraDataType>::
        skip(bool(is_skip)(CharType), bool expect)
    {
        if (eof())
            return *this;
        while (is_skip(*buf_cur) == expect) {
            count_char(*buf_cur++);
            if (empty() && reload() == false)
                break;
        }
        return *this;
    }

    template<typename StreamType, typename ExtraDataType>
    inline bool StreamHelper<StreamType, ExtraDataType>::
        reload()
    {
        size_t  rest = size();
        size_t total = capacity();

        if (buf_cur != buf_beg)
            std::memcpy(buf_beg, buf_cur, rest);

        uint64_t read = stream.read(buf_beg + rest, total - rest);

        buf_end = buf_beg + read + rest;
        buf_cur = buf_beg;

        return read != 0;
    }

    template<typename StreamType, typename ExtraDataType>
    inline size_t StreamHelper<StreamType, ExtraDataType>::
        count_warning()
    {
        return ++warning_counter;
    }

    template<typename StreamType, typename ExtraDataType>
    inline Settings const & StreamHelper<StreamType, ExtraDataType>::
        get_settings() const
    {
        return settings;
    }

    template<typename StreamType, typename ExtraDataType>
    inline void StreamHelper<StreamType, ExtraDataType>::
        count_char(CharType c)
    {
        static const CharType CR  = CharType(0xd); /* \r */
        static const CharType LF  = CharType(0xa); /* \n */
        static const CharType TAB = CharType(0x9); /* \t */

        position += 1;
        switch (c)
        {
        case TAB:
            column_number += settings.indent_width;
            break;
        case CR:
            if (buf_cur + 1 == buf_end && reload() == false)
                break;
            if (buf_cur[1] == LF) /* CR LF */
                break;
        case LF:
            line_number   += size_t(1);
            column_number  = size_t(1);
            break;
        default:
            column_number += size_t(1);
            break;
        }
    }
}

/****************************************************************************
 *  useful function of StreamHelper
 ***************************************************************************/

namespace parser
{
    /* check if there is at least `min_size` data in buffer */
    template<typename InType> inline bool
        ensure(InType & in, size_t min_size)
    {
        return in.size() >= min_size
            || (in.reload() && in.size() >= min_size)
            ;
    }

    /*  */
    template<typename InType> inline bool
        is_equ(InType & in, typename InType::CharType const *str, size_t len = 0)
    {
        if (len == 0)
            len = chars::strlen(str);

        if (len >= in.capacity())
            /* as this method should NOT call `skip`. */
            exception::cannot_compare(in.capacity(), len, str, POS_);

        return ensure(in, len)
            && chars::strncmp(in.data(), str, len) == 0;
    }

    template<typename InType> inline bool
        is_equ(InType & in, Keyword<typename InType::CharType> const & kwd)
    {
        if (kwd.size() >= in.capacity())
            /* as this method should NOT call `skip`. */
            exception::cannot_compare(in.capacity(), kwd.size(), kwd, POS_);

        return ensure(in, kwd.size())
            && chars::strncmp<typename InType::CharType>(
                in.data(), kwd, kwd.size()
                ) == 0;
    }

    /* check if there is at least `min_size` data in buffer */
    template<typename InType> inline bool
        match(InType & in, Keyword<typename InType::CharType> const & kwd)
    {
        size_t need = kwd.size();
        if (need == size_t(1))
        {
            if (in.ch() == kwd) {
                in.skip();
                return true;
            } else {
                return false;
            }
        }
        else
        {
            if (is_equ(in, kwd)) {
                in.skip(need);
                return true;
            } else {
                return false;
            }
        }
    }

    template<typename InType> inline bool
        skip_block(
        InType & in,
        Keyword<typename InType::CharType> const & beg,
        Keyword<typename InType::CharType> const & end)
    {
        /* skip beg */
        if (is_equ(in, beg) == false)
            return true;

        in.skip(beg.size());

        /* skip mid */
        while (in.ch() != EOF) {
            in.skip(static_cast<typename InType::CharType>(end), false);
            if (is_equ(in, end))
                break;
        }

        /* skip end */
        if (in.ch() == EOF)
            return false;

        in.skip(end.size());
        return true;
    }

    template<typename InType> inline void raise_error(
        InType & /* in */,
        typename InType::CharType const * msg)
    {
        throw exception::ParseError(msg);
    }

    template<typename InType> inline void raise_warning(
        InType & in,
        typename InType::CharType const * msg)
    {
        Settings const & settings = in.get_settings();
        if (settings.enable_warning_message == false) {
            return;
        } if (settings.treate_warning_as_error) {
            raise_error(in, msg);
        } else {
            // TODO:
            size_t used = in.count_warning();
            if (settings.warning_maximum + 1 == used)
                printf("parsing warning: ...\n");
            else if (settings.warning_maximum >= used)
                printf("parsing warning: %s\n", msg);
        }
    }

}

CV_FS_PRIVATE_END
