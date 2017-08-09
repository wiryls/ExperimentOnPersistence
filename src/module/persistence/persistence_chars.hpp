/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once
#include <cstring>
#include <cctype>
#include "persistence_private.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 * chars
 ***************************************************************************/

namespace chars
{
    template<typename Ch_T> inline bool isalpha(Ch_T ch);

    template<typename Ch_T> inline bool isdigit(Ch_T ch);

    template<typename Ch_T> inline bool ishexdigit(Ch_T ch);

    template<typename Ch_T> inline bool isalnum(Ch_T ch);

    template<typename Ch_T> inline bool isspace(Ch_T ch);

    template<typename Ch_T> inline bool isnewline(Ch_T ch);

    template<typename Ch_T> inline bool iscntrl(Ch_T ch);

    /* note: 0x20 ~ 0x7E, not included '\t' '\n' '\r' */
    template<typename Ch_T> inline bool isprint(Ch_T ch);

    template<typename Ch_T> inline Ch_T tolower(Ch_T ch);

    template<typename Ch_T> inline Ch_T toupper(Ch_T ch);

    template<typename Ch_T> inline size_t strlen(Ch_T const * src);

    template<typename Ch_T> inline
        int strcmp(Ch_T const * lhs, Ch_T const * rhs);

    template<typename Ch_T> inline
        int strncmp(Ch_T const * lhs, Ch_T const * rhs, size_t len);

    template<typename Ch_T> inline
        int strcmpi(Ch_T const * lhs, Ch_T const * rhs);

    template<typename Ch_T> inline
        int strncmpi(Ch_T const * lhs, Ch_T const * rhs, size_t len);

    template<typename Ch_T> inline
        Ch_T const * strchr(Ch_T const * str, Ch_T ch);

    template<typename Ch_T> inline
        Ch_T       * strchr(Ch_T       * str, Ch_T ch);

    template<typename Ch_T> inline
        Ch_T const * strrchr(Ch_T const * str, Ch_T ch);

    template<typename Ch_T> inline
        Ch_T       * strrchr(Ch_T       * str, Ch_T ch);

    template<typename Ch_T> inline
        Ch_T const * strpbrk(Ch_T const * str, Ch_T const * ch_set);

    template<typename Ch_T> inline
        Ch_T       * strpbrk(Ch_T       * str, Ch_T const * ch_set);

    template<typename Ch_T> inline
        Ch_T const * strstr(Ch_T const * str, Ch_T const * sub_str);

    template<typename Ch_T> inline
        Ch_T       * strstr(Ch_T       * str, Ch_T const * sub_str);


    // TODO: function for skip BOM
}

namespace chars
{
    /************************************************************************
     * for <char>
     ***********************************************************************/

    template<> inline bool isalpha(char c)
    {
        return ::isalpha(c) != 0;
    }
    template<> inline bool isdigit(char c)
    {
        return ::isdigit(c) != 0;
    }
    template<> inline bool ishexdigit(char c)
    {
        return isdigit(c)
            || ('a' <= c && c <= 'f')
            || ('A' <= c && c <= 'F')
            ;
    }
    template<> inline bool isalnum(char c)
    {
        return ::isalnum(c) != 0;
    }
    template<> inline bool iscntrl(char c)
    {
        /* 0x00 ~ 0x1F | 0x7F */
        return ::iscntrl(c) != 0;
    }
    template<> inline bool isprint(char c)
    {
        /* 0x20 ~ 0x7E */
        return ::isprint(c) != 0;
    }
    template<> inline bool isspace(char c)
    {
        /* '\x09', '\x0D', '\x0A', '\x20' */
        return ::isspace(c) != 0;
    }
    template<> inline bool isnewline(char c)
    {
        return c == '\n'
            || c == '\r';
    }
    template<> inline char tolower(char c)
    {
        return static_cast<char>(::tolower(c));
    }
    template<> inline char toupper(char c)
    {
        return static_cast<char>(::toupper(c));
    }
    template<> inline size_t strlen(char const * src)
    {
        return ::strlen(src);
    }
    template<> inline int strcmp(char const * lhs, char const * rhs)
    {
        return ::strcmp(lhs, rhs);
    }
    template<> inline
        int strncmp(char const * lhs, char const * rhs, size_t len)
    {
        return ::strncmp(lhs, rhs, len);
    }
    template<> inline int strcmpi(char const * lhs, char const * rhs)
    {
        for (;;) {
            int delta = tolower(*lhs) - tolower(*rhs);
            if (delta != 0 || *lhs == 0)
                return delta;
            ++ lhs;
            ++ rhs;
        }
    }
    template<> inline
        int strncmpi(char const * lhs, char const * rhs, size_t len)
    {
        while (len --> 0) {
            int delta = tolower(*lhs) - tolower(*rhs);
            if (delta != 0 || *lhs == 0)
                return delta;
            ++ lhs;
            ++ rhs;
        }
        return 0;
    }
    template<> inline char const * strchr(char const * str, char ch)
    {
        return ::strchr(str, ch);
    }
    template<> inline char       * strchr(char       * str, char ch)
    {
        return ::strchr(str, ch);
    }
    template<> inline char const * strrchr(char const * str, char ch)
    {
        return ::strrchr(str, ch);
    }
    template<> inline char       * strrchr(char       * str, char ch)
    {
        return ::strrchr(str, ch);
    }
    template<> inline
        char const * strpbrk(char const * str, char const * ch_set)
    {
        return ::strpbrk(str, ch_set);
    }
    template<> inline
        char       * strpbrk(char       * str, char const * ch_set)
    {
        return ::strpbrk(str, ch_set);
    }
    template<> inline
        char const * strstr(char const * str, char const * sub_str)
    {
        return ::strstr(str, sub_str);
    }
    template<> inline
        char       * strstr(char       * str, char const * sub_str)
    {
        return ::strstr(str, sub_str);
    }
}

namespace chars
{
    /************************************************************************
     * for <wchar>
     ***********************************************************************/

    // TODO:
}

CV_FS_PRIVATE_END
