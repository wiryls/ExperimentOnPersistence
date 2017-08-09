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
    using chars::soss_t;
    using chars::fmt;

    class parse_error : public std::exception
    {
    public:
        parse_error()                 NOTHROW :soss() {}
        parse_error(const char * msg) NOTHROW :soss() { soss*fmt<512>(msg); }
        virtual ~parse_error()        NOTHROW         {}
        virtual const char *what() const NOTHROW { return soss; }

    private:
        soss_t<char, 512> soss;
    };

    inline static void cannot_compare(
        size_t buf_len,
        size_t str_len,
        const char * str,
        POS_TYPE_)
    {
		error(0,
            ( soss_t<char, 256>()
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
     *  keyword_t
     ***********************************************************************/

    template<typename Char_T> class keyword_t
    {
    public:
        typedef Char_T char_t;

    public:
        template<size_t N> keyword_t(char_t const (&str)[N]) NOTHROW
            : ptr(str ? str : dmy)
            , dmy()
            , fst(ptr[0])
            , len(N ? (N - 1) : 0)
        {}

        keyword_t(char_t chr) NOTHROW
            : ptr(dmy)
            , dmy()
            , fst(chr)
            , len(1)
        {
            dmy[0] = chr;
        }

        inline operator const char_t * () const NOTHROW { return ptr; }
        inline operator       char_t   () const NOTHROW { return fst; }
        inline size_t              size() const NOTHROW { return len; }

    private:
        char_t const * const ptr;
        char_t               dmy[2];
        char_t const         fst;
        size_t const         len;
    };

    /************************************************************************
     * Declaration stream_helper_t
     ***********************************************************************/

    /* support userdata in stream_helper_t */
    template<typename Data_T> class holder_t;
    template<>                class holder_t<void> {};
    template<typename Data_T> class holder_t
    {
    public:
        typedef Data_T             value_type;
        typedef value_type       & reference;
        typedef value_type const & const_reference;
    public:
        explicit holder_t(reference ref) : data_(ref) {}
        inline reference       get()       { return data_; }
        inline const_reference get() const { return data_; }
    protected:
        ~holder_t() {};
    private:
        value_type & data_;
    };

    template<typename Stream_T, typename ExtraData_T>
    class stream_helper_t : public holder_t<ExtraData_T>
    {
    public:
        typedef ExtraData_T               extradata_t;
        typedef Stream_T                  stream_t;
        typedef typename stream_t::char_t char_t;

        typedef stream_helper_t           this_type;

    public:
        static const size_t MIN_BUFFER_SIZE = 32U;

    public:
        ~stream_helper_t();

        /* valid only if ExtraData_T == void */
        stream_helper_t(stream_t & stream, settings_t const & settings);

        /* valid only if ExtraData_T != void */
        template<typename T>
        stream_helper_t
        (
            stream_t         & stream,
            settings_t const & settings,
            T                & userdata,
            typename utility::enable_if_t<
                utility::is_same_t<T, extradata_t>::value, void *
            >::type = NULL /* compatible with C++98 */
        );

    public:
        inline size_t        line() const;
        inline size_t        col () const;
        inline size_t        pos () const;

        inline bool          eof () const;
        inline char_t        ch  () const;
        inline char_t const *data() const;
        inline size_t        size() const;
        inline size_t        capacity() const;
        inline bool          empty() const;

        inline this_type &   skip();
        inline this_type &   skip(size_t skip_n_chars);
        inline this_type &   skip(char_t           ch,    bool expect=true);
        inline this_type &   skip(char_t const list[],    bool expect=true);
        inline this_type &   skip(bool (is_skip)(char_t), bool expect=true);
        inline bool          reload();

        inline size_t             count_warning();
        inline settings_t const & get_settings() const;

    private:
        inline void count_char(char_t ch);

    private:
        stream_helper_t            (stream_helper_t const &);
        stream_helper_t & operator=(stream_helper_t const &);

    private:
        stream_t & stream;
        size_t     buffer_size;
        char_t *   buffer;
        char_t *   buf_beg;
        char_t *   buf_cur;
        char_t *   buf_end;

        size_t     line_number;
        size_t     column_number;
        size_t     position;

        size_t     warning_counter;
		settings_t settings;
    };

    /************************************************************************
     * Implementation stream_helper_t
     ***********************************************************************/

    template<typename Stream_T, typename ExtraData_T>
    inline stream_helper_t<Stream_T, ExtraData_T>::
        stream_helper_t(stream_t &stream_ref, settings_t const &settings_ref)
        : holder_t<ExtraData_T>()
        , stream(stream_ref)
        , buffer_size(
            utility::max(settings_ref.stream_buffer_size, MIN_BUFFER_SIZE)
        )
        , buffer(new char_t[buffer_size + 4U])
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
        buf_end[0] = char_t('.');
        buf_end[1] = char_t('.');
        buf_end[2] = char_t('.');
        buf_end[3] = char_t('\0');
    }

    template<typename Stream_T, typename ExtraData_T> template<typename T>
    inline stream_helper_t<Stream_T, ExtraData_T>::
        stream_helper_t
        (
            stream_t         & stream_ref,
            settings_t const & settings_ref,
            T                & userdata_ref,
            typename utility::enable_if_t<
                utility::is_same_t<T, extradata_t>::value, void *
            >::type
        )
        : holder_t<ExtraData_T>(userdata_ref)
        , stream(stream_ref)
        , buffer_size(
            utility::max(settings_ref.stream_buffer_size, MIN_BUFFER_SIZE)
        )
        , buffer(new char_t[buffer_size + 4U])
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
        buf_end[0] = char_t('.');
        buf_end[1] = char_t('.');
        buf_end[2] = char_t('.');
        buf_end[3] = char_t('\0');
    }

    template<typename Stream_T, typename ExtraData_T>
    inline stream_helper_t<Stream_T, ExtraData_T>::
    ~stream_helper_t()
    {
        delete buffer;
        buffer = NULL;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline size_t stream_helper_t<Stream_T, ExtraData_T>::
        line() const
    {
        return line_number;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline size_t stream_helper_t<Stream_T, ExtraData_T>::
        col() const
    {
        return column_number;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline size_t stream_helper_t<Stream_T, ExtraData_T>::
        pos() const
    {
        return position;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline bool stream_helper_t<Stream_T, ExtraData_T>::
        eof() const
    {
        return buf_beg == buf_end;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline typename stream_helper_t<Stream_T, ExtraData_T>::char_t
        stream_helper_t<Stream_T, ExtraData_T>::
        ch() const
    {
        return (eof() ? EOF : *buf_cur);
    }

    template<typename Stream_T, typename ExtraData_T>
    inline typename stream_helper_t<Stream_T, ExtraData_T>::char_t
        const * stream_helper_t<Stream_T, ExtraData_T>::
        data() const
    {
        return buf_cur;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline size_t stream_helper_t<Stream_T, ExtraData_T>::
        size() const
    {
        ASSERT_DBG(buf_end >= buf_cur);
        return static_cast<size_t>(buf_end - buf_cur);
    }

    template<typename Stream_T, typename ExtraData_T>
    inline size_t stream_helper_t<Stream_T, ExtraData_T>::
        capacity() const
    {
        return buffer_size;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline bool stream_helper_t<Stream_T, ExtraData_T>::empty() const
    {
        return buf_cur == buf_end;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline typename stream_helper_t<Stream_T, ExtraData_T>::this_type &
        stream_helper_t<Stream_T, ExtraData_T>::
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

    template<typename Stream_T, typename ExtraData_T>
    inline typename stream_helper_t<Stream_T, ExtraData_T>::this_type &
        stream_helper_t<Stream_T, ExtraData_T>::
        skip(size_t skip_n_chars)
    {
       while (skip_n_chars >= size()) {
           for (skip_n_chars -= size(); !empty();)
               count_char(*buf_cur++);
            if (reload() == false)
                return *this;
        }
       for (char_t *end = buf_cur + skip_n_chars; buf_cur != end;)
           count_char(*buf_cur++);
        return *this;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline typename stream_helper_t<Stream_T, ExtraData_T>::this_type &
        stream_helper_t<Stream_T, ExtraData_T>::
        skip(char_t c, bool expect)
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

    template<typename Stream_T, typename ExtraData_T>
    inline typename stream_helper_t<Stream_T, ExtraData_T>::this_type &
        stream_helper_t<Stream_T, ExtraData_T>::
        skip(char_t const list[], bool expect)
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

    template<typename Stream_T, typename ExtraData_T>
    inline typename stream_helper_t<Stream_T, ExtraData_T>::this_type &
        stream_helper_t<Stream_T, ExtraData_T>::
        skip(bool(is_skip)(char_t), bool expect)
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

    template<typename Stream_T, typename ExtraData_T>
    inline bool stream_helper_t<Stream_T, ExtraData_T>::
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

    template<typename Stream_T, typename ExtraData_T>
    inline size_t stream_helper_t<Stream_T, ExtraData_T>::
        count_warning()
    {
        return ++warning_counter;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline settings_t const & stream_helper_t<Stream_T, ExtraData_T>::
        get_settings() const
    {
        return settings;
    }

    template<typename Stream_T, typename ExtraData_T>
    inline void stream_helper_t<Stream_T, ExtraData_T>::
        count_char(char_t c)
    {
        static const char_t CR  = char_t(0xd); /* \r */
        static const char_t LF  = char_t(0xa); /* \n */
        static const char_t TAB = char_t(0x9); /* \t */

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
 *  useful function of stream_helper_t
 ***************************************************************************/

namespace parser
{
    /* check if there is at least `min_size` data in buffer */
    template<typename In_T> inline bool
        ensure(In_T & in, size_t min_size)
    {
        return in.size() >= min_size
            || (in.reload() && in.size() >= min_size)
            ;
    }

    /*  */
    template<typename In_T> inline bool
        is_equ(In_T & in, typename In_T::char_t const *str, size_t len = 0)
    {
        if (len == 0)
            len = chars::strlen(str);

        if (len >= in.capacity())
            /* as this method should NOT call `skip`. */
            exception::cannot_compare(in.capacity(), len, str, POS_);

        return ensure(in, len)
            && chars::strncmp(in.data(), str, len) == 0;
    }

    template<typename In_T> inline bool
        is_equ(In_T & in, keyword_t<typename In_T::char_t> const & kwd)
    {
        if (kwd.size() >= in.capacity())
            /* as this method should NOT call `skip`. */
            exception::cannot_compare(in.capacity(), kwd.size(), kwd, POS_);

        return ensure(in, kwd.size())
            && chars::strncmp<typename In_T::char_t>(
                in.data(), kwd, kwd.size()
                ) == 0;
    }

    /* check if there is at least `min_size` data in buffer */
    template<typename In_T> inline bool
        match(In_T & in, keyword_t<typename In_T::char_t> const & kwd)
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

    template<typename In_T> inline bool
        skip_block(
        In_T & in,
        keyword_t<typename In_T::char_t> const & beg,
        keyword_t<typename In_T::char_t> const & end)
    {
        /* skip beg */
        if (is_equ(in, beg) == false)
            return true;

        in.skip(beg.size());

        /* skip mid */
        while (in.ch() != EOF) {
            in.skip(static_cast<typename In_T::char_t>(end), false);
            if (is_equ(in, end))
                break;
        }

        /* skip end */
        if (in.ch() == EOF)
            return false;

        in.skip(end.size());
        return true;
    }

    template<typename In_T> inline void raise_error(
        In_T & /* in */,
        typename In_T::char_t const * msg)
    {
        throw exception::parse_error(msg);
    }

    template<typename In_T> inline void raise_warning(
        In_T & in,
        typename In_T::char_t const * msg)
    {
        settings_t const & settings = in.get_settings();
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
