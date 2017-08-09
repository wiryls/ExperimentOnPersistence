/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once
#include <stdio.h>
#include "persistence_private.hpp"
#include "persistence_utility.hpp"
#include "persistence_chars.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 * convert to string
 ***************************************************************************/

namespace chars { namespace internal
{
    template<typename char_t> inline bool
        to_string(uint64_t src, char_t * dst, size_t len) NOTHROW
    {
        if (len == 0 || dst == NULL)
            return false;

        char_t * end = dst;
        char_t * beg = dst;

        do {
            *end++ = char_t('0') + static_cast<char_t>(src % 10);
            src /= 10;
        } while (--len && src);

        if (len == 0)
            return false;

        *end-- = char_t();
        while (beg < end) {
            *beg ^= *end ^= *beg ^= *end;
            ++ beg;
            -- end;
        }
        return true;
    }

    template<typename char_t> inline bool
        to_string(int64_t src, char_t * dst, size_t len) NOTHROW
    {
        if (len == 0 || dst == NULL)
            return false;

        if (src < 0)
        {
             src = -src;
            *dst = char_t('-');
            dst++;
            len--;
        }
        return to_string(static_cast<uint64_t>(src), dst, len);
    }

} }

namespace chars
{
    template<size_t N, typename char_t> inline bool
        make_string(char_t const * src, char_t (&dst)[N]) NOTHROW
    {
        if (src == NULL) {
            dst[0] = char_t();
            return true;
        }

        char_t const * rhs = src;
        char_t       * lhs = dst;
        char_t       * end = dst + N;
        while (lhs != end && (*lhs = *rhs) != char_t())
            lhs++, rhs++;
        return lhs != end;
    }

    template<typename T, size_t N, typename char_t> inline
    typename utility::enable_if_t<
        utility::isinteger_t<T>::value, bool
    >::type
        make_string(T src, char_t (&dst)[N]) NOTHROW
    {
        typedef typename utility::if_t<
            utility::issigned_t<T>::value,
             int64_t,
            uint64_t
        >::type int_t;

        static const size_t MAX_DIGITS = 21;
        char_t buf[MAX_DIGITS]; /* big enough buffer for int types */

        if (!internal::to_string(static_cast<int_t>(src), buf, MAX_DIGITS))
            return false; /* should never appear */

        char_t * rhs = buf;
        char_t * lhs = dst;
        char_t * end = dst + N;
        while (lhs != end && (*lhs = *rhs) != char_t())
            lhs++, rhs++;
        return lhs != end;
    }

    template<size_t N, typename char_t> inline bool
        make_string(void * ptr, char_t (&dst)[N]) NOTHROW
    {
        static const char mapping[] = "0123456789abcdef";

        char_t buf[(sizeof(void*) << 1) + 2 + 1] = {
            char_t('0'), char_t('x')
        };
        char_t * cur = buf + 2;
        for (size_t i = (sizeof(void*) << 1); i --> 0;)
            *cur++ = mapping[0xF&(reinterpret_cast<size_t>(ptr) >> (i<<2))];

        char_t * rhs = buf;
        char_t * lhs = dst;
        char_t * end = dst + N;
        while (lhs != end && (*lhs = *rhs) != char_t())
            lhs++, rhs++;
        return lhs != end;
    }

    template<size_t N, typename char_t> inline bool
        make_string(bool src, char_t (&dst)[N]) NOTHROW
    {
        static const char t[] = "true";
        static const char f[] = "false";
        char const * rhs = src ? t: f;
        char_t     * lhs = dst;
        char_t     * end = dst + N;
        while (lhs != end && (*lhs = char_t(*rhs)) != char_t())
            lhs++, rhs++;
        return lhs != end;
    }

    template<size_t N, typename char_t> inline bool
        make_string(float src, char_t (&dst)[N]) NOTHROW
    {
        static const char  nan[] =  ".Nan";
        static const char  inf[] =  ".Inf";
        static const char ninf[] = "-.Inf";

        static const uint32_t MASK = 0x7f800000;
        static const size_t MAX_DIGITS = 1080;

        union { float flt; uint32_t u32; };
        flt = src;

        char buf[MAX_DIGITS];
        const char * rhs = buf;

        if ((u32 & MASK) != MASK)
        {
            /* TODO: cvRound(flt) */
            int64_t integer = static_cast<int64_t>(flt);
            if (static_cast<float>(integer) == flt)
            {
                if (!internal::to_string(integer, buf, MAX_DIGITS))
                    return false; /* should never appaer */

                char * ptr = buf;
                while (chars::isdigit(*ptr))
                    ptr++;
                *ptr++ = '.';
                *ptr++ = '0';
                *ptr++ = 0;
            }
            else
            {
                static const char * fmt = "%.8e";
                /* TODO: */
                sprintf(buf, fmt, flt);

                char * ptr = buf;
                if (*ptr == '+' || *ptr == '-')
                    ptr++;
                while (chars::isdigit(*ptr))
                    ptr++;
                if (*ptr == ',')
                    *ptr = '.';
            }
        }
        else if ((u32 & 0x7fffffff) != MASK)
        {
            rhs = nan;
        }
        else if (u32 >> 31)
        {
            rhs = ninf;
        }
        else
        {
            rhs = inf;
        }

        char_t * lhs = dst;
        char_t * end = dst + N;
        while (lhs != end && (*lhs = char_t(*rhs)) != char_t())
            lhs++, rhs++;
        return lhs != end;
    }

    template<size_t N, typename char_t> inline bool
        make_string(double src, char_t (&dst)[N]) NOTHROW
    {
        static const char  nan[] =  ".Nan";
        static const char  inf[] =  ".Inf";
        static const char ninf[] = "-.Inf";

        static const uint32_t MASK = 0x7ff00000;
        /* [maximum length of double]
         * (http://stackoverflow.com/a/1701272)
         */
        static const size_t MAX_DIGITS = 1080;

        union { double dbl; uint64_t u64; };
        dbl = src;
        uint32_t ieee754_hi = static_cast<uint32_t>(u64 >> 32);
        uint32_t ieee754_lo = static_cast<uint32_t>(u64);

        char buf[MAX_DIGITS];
        const char * rhs = buf;

        if ((ieee754_hi & MASK) != MASK)
        {
            /* TODO: cvRound(dbl) */
            int64_t integer = static_cast<int64_t>(dbl);
            if (static_cast<double>(integer) == dbl)
            {
                if (!internal::to_string(integer, buf, MAX_DIGITS))
                    return false; /* should never appaer */

                char * ptr = buf;
                while (chars::isdigit(*ptr))
                    ptr++;
                *ptr++ = '.';
                *ptr++ = '0';
                *ptr++ = 0;
            }
            else
            {
                static const char * fmt = "%.16e";
                /* TODO: */
                sprintf(buf, fmt, dbl);

                char * ptr = buf;
                if (*ptr == '+' || *ptr == '-')
                    ptr++;
                while (chars::isdigit(*ptr))
                    ptr++;
                if (*ptr == ',')
                    *ptr = '.';
            }
        }
        else if ((ieee754_hi & 0x7fffffff) + (ieee754_lo != 0) > MASK)
        {
            rhs = nan;
        }
        else if (ieee754_hi >> 31)
        {
            rhs = ninf;
        }
        else
        {
            rhs = inf;
        }

        char_t * lhs = dst;
        char_t * end = dst + N;
        while (lhs != end && (*lhs = char_t(*rhs)) != char_t())
            lhs++, rhs++;
        return lhs != end;
    }

    template<typename char_t, size_t N> inline bool
        make_string_readable(char_t (&dst)[N]) NOTHROW
    {
        char_t * cur = dst;

        for (size_t cnt = N-1; cnt --> 0 && *cur != char_t(); ++cur)
            if (chars::iscntrl(*cur))
                *cur = char_t('\\');

        if (*cur != char_t())
        {   /* string is too long */
            const char omit[] = "...";
            const char * ptr  = omit;
            for (size_t cnt = N-1; cnt --> 0 && *ptr != char_t(); ++ptr);

            *cur = char_t();
            while (ptr != omit)
                *(--cur) = char_t(*(--ptr));
        }
        return true;
    }
}

/****************************************************************************
 * string buffer
 ***************************************************************************/

namespace chars
{
    /************************************************************************
     * declaration - buffer_t
     ***********************************************************************/

    //template<
    //    typename T,
    //    size_t N,
    //    template<typename> class Alloc_T
    //> class buffer_t;

    template<
        typename T,
        size_t N,
        template<typename> class Alloc_T
    > class buffer_t
    {
    public:
        typedef T                   value_type;
        typedef value_type       *  pointer;
        typedef value_type       &  reference;
        typedef value_type const *  const_pointer;
        typedef value_type const &  const_reference;
        typedef Alloc_T<value_type> allocator_t;

    public:
        ~buffer_t();

        explicit buffer_t();
        explicit buffer_t(allocator_t & alloc);
        explicit buffer_t(size_t siz, bool is_reserved = false);
        explicit buffer_t(size_t siz, bool is_reserved, allocator_t & alloc);

		buffer_t             (buffer_t const & rhs);
        buffer_t & operator= (buffer_t const & rhs);

        inline operator       pointer ();
        inline operator const_pointer () const;

        inline void  push_back(const_pointer   mem, size_t siz);
        inline void  push_back(const_reference val);
        inline void   pop_back(size_t n = 1);
        inline void  pop_front(size_t n = 1);   /* inefficient */
        inline void       swap(buffer_t & rhs); /* inefficient */
        inline void      clear();

        inline void     resize(size_t size);
        inline void    reserve(size_t size);

		inline const_pointer begin() const;
		inline const_pointer   end() const;
		inline pointer begin();
		inline pointer   end();

        inline const_reference  back() const;
		inline const_reference front() const;
		inline reference  back();
		inline reference front();

        inline bool      empty() const;
        inline size_t     size() const;
        inline size_t capacity() const;

    private:
        inline allocator_t & alloc();

    private:
        static inline size_t ceil2(size_t y);

    private:
        allocator_t  alc_;
        allocator_t *alp_;
        size_t       siz_;
        size_t       cap_;
        pointer      ptr_;
        value_type   buf_[N + 1];
    };

    /************************************************************************
     * implementation - buffer_t
     ***********************************************************************/

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline buffer_t<T, N, Alloc_T>::
        ~buffer_t()
    {
        if (ptr_ != buf_)
            alloc().deallocate(ptr_, cap_);
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline buffer_t<T, N, Alloc_T>::
        buffer_t()
        : alc_()
        , alp_()
        , siz_()
        , cap_(N)
        , ptr_(buf_)
    {}

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline buffer_t<T, N, Alloc_T>::
        buffer_t(allocator_t & allocator)
        : alc_()
        , alp_(&allocator)
        , siz_()
        , cap_(N)
        , ptr_(buf_)
    {}

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline buffer_t<T, N, Alloc_T>::
        buffer_t(size_t n, bool is_reserved)
        : alc_()
        , alp_()
        , siz_(is_reserved ? 0 : n)
        , cap_(n <= N ? N : ceil2(n))
        , ptr_(n <= N ? buf_ : alloc().allocate(cap_))
    {}

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline buffer_t<T, N, Alloc_T>::
        buffer_t(size_t n, bool is_reserved, allocator_t & allocator)
        : alc_()
        , alp_(&allocator)
        , siz_(is_reserved ? 0 : n)
        , cap_(n <= N ? N : ceil2(n))
        , ptr_(n <= N ? buf_ : alloc().allocate(cap_))
    {}

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline buffer_t<T, N, Alloc_T>::
        buffer_t(buffer_t const & rhs)
        : alc_()
        , alp_(rhs.alp_)
        , siz_(rhs.siz_)
        , cap_(rhs.cap_)
        , ptr_(cap_ <= N ? buf_ : alloc().allocate(cap_))
    {
        ::memcpy(ptr_, rhs.ptr_, siz_ * sizeof(value_type));
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline buffer_t<T, N, Alloc_T> & buffer_t<T, N, Alloc_T>::
        operator= (buffer_t const & rhs)
    {
        if (this == &rhs)
            return *this;

        /* ensure cap_ >= rhs.siz_ */
        siz_ = rhs.siz_;
        alp_ = rhs.alp_;
        if (cap_ < siz_) {
            if (ptr_ != buf_)
                alloc().deallocate(ptr_, cap_);
            cap_ = rhs.cap_;
            ptr_ = alloc().allocate(cap_);
        }

        /* copy data */
        ::memcpy(ptr_, rhs.ptr_, siz_ * sizeof(value_type));

        return *this;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline buffer_t<T, N, Alloc_T>::
        operator pointer ()
    {
        return ptr_;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline buffer_t<T, N, Alloc_T>::
        operator const_pointer () const
    {
        return ptr_;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline void buffer_t<T, N, Alloc_T>::
        push_back(const_pointer mem, size_t siz)
    {
		if (mem == NULL)
			return;

        size_t new_siz_ = siz_ + siz;
		reserve(new_siz_);

        ::memcpy(ptr_ + siz_, mem, siz * sizeof(value_type));
        siz_ = new_siz_;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline void buffer_t<T, N, Alloc_T>::
        push_back(const_reference val)
    {
        size_t old_siz = siz_;
        siz_ += size_t(1);
        reserve(siz_);

        ptr_[old_siz] = val;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline void buffer_t<T, N, Alloc_T>::
        pop_back(size_t n)
    {
        if (siz_ <= n) {
            siz_ = 0;
        } else {
            siz_ -= n;
        }
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline void buffer_t<T, N, Alloc_T>::
        pop_front(size_t n)
    {
        if (siz_ <= n) {
            siz_ = 0;
        } else {
            siz_ -= n;
            ::memcpy(ptr_, ptr_ + n, siz_ * sizeof(value_type));
        }
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline void buffer_t<T, N, Alloc_T>::
        swap(buffer_t & rhs)
    {
        if (&rhs == this)
            return;

        pointer lhs_old_ptr =     ptr_;
        pointer rhs_old_ptr = rhs.ptr_;

            ptr_ = (rhs_old_ptr == rhs.buf_ ?     buf_ : rhs_old_ptr);
        rhs.ptr_ = (lhs_old_ptr ==     buf_ ? rhs.buf_ : lhs_old_ptr);

        if (ptr_ == buf_ || rhs.ptr_ == rhs.buf_) {
            size_t max_siz = siz_ > rhs.siz_ ? siz_ : rhs.siz_;
            for (size_t i = size_t(); i < max_siz; ++i)
                buf_[i] ^= rhs.buf_[i] ^= buf_[i] ^= rhs.buf_[i];
        }

        {
            allocator_t * ptr = alp_;
                alp_ = rhs.alp_;
            rhs.alp_ =     ptr;
        }

        siz_ ^= rhs.siz_ ^= siz_ ^= rhs.siz_;
        cap_ ^= rhs.cap_ ^= cap_ ^= rhs.cap_;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline void buffer_t<T, N, Alloc_T>::
        clear()
    {
        siz_ = 0;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline void buffer_t<T, N, Alloc_T>::
        resize(size_t n)
    {
        reserve(n);
        siz_ = n;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline void buffer_t<T, N, Alloc_T>::
        reserve(size_t n)
    {
        if (n <= cap_)
            return;

        /* alloc new space */
        size_t  new_cap_ = ceil2(n);
        pointer new_ptr_ = alloc().allocate(new_cap_);

        /* copy */
        ::memcpy(new_ptr_, ptr_, siz_ * sizeof(value_type));

        /* dealloc old space */
        if (ptr_ != buf_)
            alloc().deallocate(ptr_, cap_);

        /* set to new */
        cap_ = new_cap_;
        ptr_ = new_ptr_;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline typename buffer_t<T, N, Alloc_T>::const_pointer
	buffer_t<T, N, Alloc_T>::
		begin() const
	{
		return ptr_;
	}

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline typename buffer_t<T, N, Alloc_T>::const_pointer
	buffer_t<T, N, Alloc_T>::
		end() const
	{
		return ptr_ + siz_;
	}

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline typename buffer_t<T, N, Alloc_T>::pointer
	buffer_t<T, N, Alloc_T>::
		begin()
	{
		return const_cast<pointer>
            (static_cast<buffer_t const &>(*this).begin());
	}

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline typename buffer_t<T, N, Alloc_T>::pointer
	buffer_t<T, N, Alloc_T>::
		end()
	{
		return const_cast<pointer>
            (static_cast<buffer_t const &>(*this).end());
	}

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline typename buffer_t<T, N, Alloc_T>::const_reference
	buffer_t<T, N, Alloc_T>::
		back() const
	{
		return ptr_[siz_ - size_t(1)];
	}

	template<typename T, size_t N, template<typename> class Alloc_T>
    inline typename buffer_t<T, N, Alloc_T>::const_reference
	buffer_t<T, N, Alloc_T>::
		front() const
	{
		return *ptr_;
	}

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline typename buffer_t<T, N, Alloc_T>::reference
	buffer_t<T, N, Alloc_T>::
		back()
	{
		return const_cast<reference>
            (static_cast<buffer_t const &>(*this).back());
	}

	template<typename T, size_t N, template<typename> class Alloc_T>
    inline typename buffer_t<T, N, Alloc_T>::reference
	buffer_t<T, N, Alloc_T>::
		front()
	{
		return const_cast<reference>
            (static_cast<buffer_t const &>(*this).front());
	}

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline bool buffer_t<T, N, Alloc_T>::
        empty() const
    {
        return siz_ == size_t();
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline size_t buffer_t<T, N, Alloc_T>::
        size() const
    {
        return siz_;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline size_t buffer_t<T, N, Alloc_T>::
        capacity() const
    {
        return cap_;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline typename buffer_t<T, N, Alloc_T>::allocator_t &
    buffer_t<T, N, Alloc_T>::
        alloc()
    {
        if (alp_ == NULL)
            return alc_;
        else
            return *alp_;
    }

    template<typename T, size_t N, template<typename> class Alloc_T>
    inline size_t buffer_t<T, N, Alloc_T>::
        ceil2(size_t x)
    {
        if (~(~size_t() >> 1) < x)
            return x;

        size_t y = size_t(1);
        while (y < x)
            y <<= 1;
        return y;
    }
}

/****************************************************************************
 * static output string stream
 * - noexpt
 * e.g.:
 * soss_t<char, 60>()
 *  * fmt<16>(test_string)
 *  | ", float:`"
 *  | fmt<2>(3.14f)
 *  | '`'
 *  | "double:`"
 *  | fmt<10>(2.718)
 *  | "`"
 ***************************************************************************/

namespace chars
{
    /************************************************************************
     * characters
     ***********************************************************************/

    template<typename char_t, size_t N> class chs_t
    {
    public:
        typedef char_t type[N];
        inline operator type       &()       NOTHROW;
        inline operator type const &() const NOTHROW;
    private:
        type buffer;
    };

    template<typename char_t, size_t N> inline chs_t<char_t, N>::
        operator type const &() const NOTHROW
    {
        return buffer;
    }

    template<typename char_t, size_t N> inline chs_t<char_t, N>::
        operator type &() NOTHROW
    {
        return buffer;
    }

    /************************************************************************
     * formatter
     ***********************************************************************/

    template<typename T> struct isprintable_t
    {
    private:
        typedef typename utility::tl::make_list<
            void       *,
            void const *
        >::type list;
    public:
        static const bool value
            =  utility::tl::contain<list, T>::value
            || utility::isprimitive_t<T>::value
            ;
    };

    template<typename char_t, size_t N, typename T> inline
    typename utility::enable_if_t<
        isprintable_t<T>::value, chs_t<char_t, N+1>
    >::type fmt(T value) NOTHROW
    {
        typedef chs_t<char_t, N + 1> buf_t;
        typedef typename buf_t::type & type;

        buf_t buf;
        if (make_string(value,   static_cast<type>(buf)) == false)
            make_string_readable(static_cast<type>(buf));
        return buf;
    }

    template<size_t N, typename char_t> inline
    chs_t<char_t, N+1> fmt(char_t const * value) NOTHROW
    {
        typedef chs_t<char_t, N + 1> buf_t;
        typedef typename buf_t::type & type;

        buf_t buf;
        if (make_string(value,   static_cast<type>(buf)) == false)
            make_string_readable(static_cast<type>(buf));
        return buf;
    }

    template<size_t N, typename T> inline
    typename utility::enable_if_t<
        isprintable_t<T>::value,
        chs_t<char, N+1>
    >::type fmt(T value) NOTHROW
    {
        return fmt<char, N>(value);
    }

    template<size_t N, typename T> inline
    typename utility::enable_if_t<
        isprintable_t<T>::value,
        chs_t<wchar_t, N+1>
    >::type wfmt(T value) NOTHROW
    {
        return fmt<wchar_t, N>(value);
    }

    /************************************************************************
     * declaration - soss_t
     ***********************************************************************/

    template<typename char_t = char, size_t N = 128> class soss_t
    {
    private:
        /* a fixed size buffer */
        class buffer_t
        {
        public:
             buffer_t() NOTHROW;
            ~buffer_t() NOTHROW;
            buffer_t(buffer_t const & rhs) NOTHROW;
            inline buffer_t & operator=(buffer_t const & rhs) NOTHROW;
            inline operator const char_t *() const NOTHROW;
            inline void put(char_t c) NOTHROW;
            inline void put(char_t const * src, size_t len) NOTHROW;
            inline void clr() NOTHROW;
        private:
            inline void fin() NOTHROW;
        private:
            static const size_t capacity = N + 1; /* last one for '\0' */
            char_t *cur;
            char_t *end;
            char_t  beg[capacity];
        };

    private:
        /* a counter_t for recording the rest size of buffer */
        template<size_t I> class counter_t
        {
        public:
            counter_t(buffer_t & buffer) NOTHROW;
            inline operator const char_t *() const NOTHROW;
        public:
            template<typename T> inline
            typename utility::enable_if_t<
                (1 <= I && I <= N && utility::is_same_t<T, char_t>::value),
                counter_t<I - 1>
            >::type
            operator|(T rhs) NOTHROW;

            template<size_t R> inline
            typename utility::enable_if_t<
                (R<=I+1 && I <= N), counter_t<I+1-R>
            >::type
            operator|(char_t const(&rhs)[R]) NOTHROW;

            template<size_t R> inline
            typename utility::enable_if_t<
                (R<=I+1 && I <= N), counter_t<I+1-R>
            >::type
            operator|(chs_t<char_t, R> const & rhs) NOTHROW;
        private:
            buffer_t & buffer;
        };

    public:
        /* method */
        soss_t() NOTHROW;
        inline operator const char_t *() const NOTHROW;

    public:
        template<typename T> inline
        typename utility::enable_if_t<
            (1 <= N && utility::is_same_t<T, char_t>::value),
            counter_t<N - 1>
        >::type
        operator*(T rhs) NOTHROW;

        template<size_t R> inline
        typename utility::enable_if_t<(R<=N+1), counter_t<N+1-R> >::type
        operator*(char_t const(&rhs)[R]) NOTHROW;

        template<size_t R> inline
        typename utility::enable_if_t<(R<=N+1), counter_t<N+1-R> >::type
        operator*(chs_t<char_t, R> const & rhs) NOTHROW;

    private:
        buffer_t buffer;
    };

    /************************************************************************
     * implementation soss_t::buffer
     ***********************************************************************/

    template<typename T, size_t N> inline soss_t<T, N>::buffer_t::
        buffer_t() NOTHROW
        : cur(beg)
        , end(beg + N)
    {
        fin();
    }

    template<typename T, size_t N> inline soss_t<T, N>::buffer_t::
        ~buffer_t() NOTHROW
    {}

    template<typename T, size_t N> inline soss_t<T, N>::buffer_t::
        buffer_t(buffer_t const & rhs) NOTHROW
        : cur(beg + (rhs.cur - rhs.beg)), end(beg + N)
    {
        ::memcpy(beg, rhs.beg, capacity);
    }

    template<typename T, size_t N> inline typename soss_t<T, N>::buffer_t &
        soss_t<T, N>::buffer_t::
        operator=(buffer_t const & rhs) NOTHROW
    {
        ::memcpy(beg, rhs.beg, capacity);
        cur = beg + (rhs.cur - rhs.beg);
        end = beg + N;
    }

    template<typename T, size_t N> inline soss_t<T, N>::buffer_t::
        operator const T*() const NOTHROW
    {
        return beg;
    }

    template<typename T, size_t N> inline void soss_t<T, N>::buffer_t::
        put(T c) NOTHROW
    {
        if (cur != end)
            *cur++ = c;
        fin();
    }

    template<typename T, size_t N> inline void soss_t<T, N>::buffer_t::
        put(T const * src, size_t len) NOTHROW
    {
        while (cur != end && len --> 0 && *src != T())
            *cur++ = *src++;
        fin();
    }

    template<typename T, size_t N> inline void soss_t<T, N>::buffer_t::
        clr() NOTHROW
    {
        cur = beg;
        fin();
    }

    template<typename T, size_t N> inline void soss_t<T, N>::buffer_t::
        fin() NOTHROW
    {
        *cur = T();
    }

    /************************************************************************
     * implementation soss_t::counter_t
     ***********************************************************************/

    template<typename T, size_t N> template<size_t I>
    soss_t<T, N>::counter_t<I>::
        counter_t(buffer_t & buf) NOTHROW : buffer(buf)
    {}

    template<typename T, size_t N> template<size_t I>
    inline soss_t<T, N>::counter_t<I>::
        operator const T *() const NOTHROW
    {
        return buffer;
    }

    template<typename T, size_t N> template<size_t I> template<typename Ch_T>
    inline typename utility::enable_if_t<
        (1 <= I && I <= N && utility::is_same_t<Ch_T, T>::value),
        typename soss_t<T, N>::AVOID_MSVC_C2244_ counter_t<I - 1>
    >::type soss_t<T, N>::counter_t<I>::
        operator|(Ch_T rhs) NOTHROW
    {
        buffer.put(rhs);
        return counter_t<I - 1>(buffer);
    }

    template<typename T, size_t N> template<size_t I> template<size_t R>
    inline typename utility::enable_if_t<
        (R<=I+1 && I <= N),
        typename soss_t<T, N>::AVOID_MSVC_C2244_ counter_t<I+1-R>
    >::type soss_t<T, N>::counter_t<I>::
        operator|(T const(&rhs)[R]) NOTHROW
    {
        buffer.put(rhs, R - 1);
        return counter_t<I+1-R>(buffer);
    }

    template<typename T, size_t N> template<size_t I> template<size_t R>
    inline typename utility::enable_if_t<
        (R<=I+1 && I <= N),
        typename soss_t<T, N>::AVOID_MSVC_C2244_ counter_t<I+1-R>
    >::type soss_t<T, N>::counter_t<I>::
        operator|(chs_t<T, R> const & rhs) NOTHROW
    {
        buffer.put(rhs, R - 1);
        return counter_t<I+1-R>(buffer);
    }

    /************************************************************************
     * implementation soss_t
     ***********************************************************************/

    template<typename T, size_t N> inline soss_t<T, N>::soss_t() NOTHROW
        : buffer()
    {}

    template<typename T, size_t N> inline soss_t<T, N>::
        operator const T *() const NOTHROW
    {
        return buffer;
    }

    template<typename T, size_t N> template<typename char_t> inline
    typename utility::enable_if_t<
        (1 <= N && utility::is_same_t<char_t, T>::value),
        typename soss_t<T, N>::AVOID_MSVC_C2244_ counter_t<N - 1>
    >::type soss_t<T, N>::
        operator*(char_t rhs) NOTHROW
    {
        buffer.clr();
        return (counter_t<N>(buffer) | rhs);
    }

    template<typename T, size_t N> template<size_t R> inline
    typename utility::enable_if_t<
        (R<=N+1),
        typename soss_t<T, N>::AVOID_MSVC_C2244_ counter_t<N+1-R>
    >::type soss_t<T, N>::
        operator*(T const(&rhs)[R]) NOTHROW
    {
        buffer.clr();
        return (counter_t<N>(buffer) | rhs);
    }

    template<typename T, size_t N> template<size_t R> inline
    typename utility::enable_if_t<
        (R<=N+1),
        typename soss_t<T, N>::AVOID_MSVC_C2244_ counter_t<N+1-R>
    >::type soss_t<T, N>::
        operator*(chs_t<T, R> const & rhs) NOTHROW
    {
        buffer.clr();
        return (counter_t<N>(buffer) | rhs);
    }
}

CV_FS_PRIVATE_END
