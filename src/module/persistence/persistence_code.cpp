/****************************************************************************
 *  license
 ***************************************************************************/

#include <climits>
#include <cstring>
#include "persistence_code.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 *  Base64
 ***************************************************************************/
namespace code { namespace base64
{
    /************************************************************************
     * constant
     ***********************************************************************/

    #if CHAR_BIT != 8 /* defined in <climits> */
    #error "`char` should be 8 bit."
    #endif

    uint8_t const padding   = '=';
    uint8_t const mapping[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    uint8_t const demapping[] = {
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
        0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63, 52, 53, 54,
       55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,
        3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
       20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,  0, 26, 27, 28, 29, 30,
       31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
       48, 49, 50, 51,  0,  0,  0,  0,
    };

    /*    `demapping` above is generated in this way:
     *    ```````````````````````````````````````````````````````````````````
     *    std::string mapping((const char *)base64_mapping);
     *    for (auto ch = 0; ch < 127; ch++) {
     *        auto i = mapping.find(ch);
     *        printf("%3u, ", (i != std::string::npos ? i : 0));
     *    }
     *    ```````````````````````````````````````````````````````````````````
     */

    /************************************************************************
     * function
     ***********************************************************************/

    size_t encode(uint8_t const * src, uint8_t * dst, size_t off, size_t cnt)
    {
        if (src == NULL || dst == NULL || cnt == 0)
            return 0;

        /* initialize beginning and end */
        uint8_t       * dst_beg = dst;
        uint8_t       * dst_cur = dst_beg;

        uint8_t const * src_beg = src + off;
        uint8_t const * src_cur = src_beg;
        uint8_t const * src_end = src_cur + cnt / 3U * 3U;

        /* integer multiples part */
        while (src_cur < src_end) {
            uint8_t _2 = *src_cur++;
            uint8_t _1 = *src_cur++;
            uint8_t _0 = *src_cur++;
            *dst_cur++ = mapping[ _2          >> 2];
            *dst_cur++ = mapping[(_1 & 0xF0U) >> 4 | (_2 & 0x03U) << 4];
            *dst_cur++ = mapping[(_0 & 0xC0U) >> 6 | (_1 & 0x0FU) << 2];
            *dst_cur++ = mapping[ _0 & 0x3FU];
        }

        /* remainder part */
        size_t rst = static_cast<size_t>(src_beg + cnt - src_cur);
        if (rst == 1U) {
            uint8_t _2 = *src_cur++;
            *dst_cur++ = mapping[ _2          >> 2];
            *dst_cur++ = mapping[(_2 & 0x03U) << 4];
        } else if (rst == 2U) {
            uint8_t _2 = *src_cur++;
            uint8_t _1 = *src_cur++;
            *dst_cur++ = mapping[ _2          >> 2];
            *dst_cur++ = mapping[(_2 & 0x03U) << 4 | (_1 & 0xF0U) >> 4];
            *dst_cur++ = mapping[(_1 & 0x0FU) << 2];
        }

        /* padding */
        switch (rst)
        {
        case 1U: *dst_cur++ = padding;
        case 2U: *dst_cur++ = padding;
        default: break;
        }

        return static_cast<size_t>(dst_cur - dst_beg);
    }

    size_t decode(uint8_t const * src, uint8_t * dst, size_t off, size_t cnt)
    {
        /* check parameters */
        if (src == NULL || dst == NULL || cnt == 0)
            return 0;
        if (cnt & 0x3U)
            return 0;

        /* initialize beginning and end */
        uint8_t       * dst_beg = dst;
        uint8_t       * dst_cur = dst_beg;

        uint8_t const * src_beg = src + off;
        uint8_t const * src_cur = src_beg;
        uint8_t const * src_end = src_cur + cnt;

        /* start decoding */
        while (src_cur < src_end) {
            uint8_t d50 = demapping[*src_cur++];
            uint8_t c50 = demapping[*src_cur++];
            uint8_t b50 = demapping[*src_cur++];
            uint8_t a50 = demapping[*src_cur++];

            uint8_t b10 = b50 & 0x03U;
            uint8_t b52 = b50 & 0x3CU;
            uint8_t c30 = c50 & 0x0FU;
            uint8_t c54 = c50 & 0x30U;

            *dst_cur++ = (d50 << 2) | (c54 >> 4);
            *dst_cur++ = (c30 << 4) | (b52 >> 2);
            *dst_cur++ = (b10 << 6) | (a50 >> 0);
        }

        return size_t(dst_cur - dst_beg);
    }

    bool is_valid(uint8_t const * src,                size_t off, size_t cnt)
    {
         /* check parameters */
        if (src == NULL || src + off == NULL)
            return false;
        if (cnt == 0U)
            cnt = std::strlen(reinterpret_cast<char const *>(src));
        if (cnt == 0U)
            return false;
        if (cnt & 0x3U)
            return false;

        /* initialize beginning and end */
        uint8_t const * beg = src + off;
        uint8_t const * end = beg + cnt;

        /* skip padding */
        if (*(end - 1U) == padding) {
            end--;
            if (*(end - 1U) == padding)
                end--;
        }

        /* find illegal characters */
        for (uint8_t const * iter = beg; iter < end; iter++)
            if (*iter > 126U || (!demapping[*iter] && *iter != mapping[0]))
                return false;

        return true;
    }

    size_t encode(char const * src, char * dst, size_t off, size_t cnt)
    {
        if (cnt == 0)
            cnt = std::strlen(src);

        size_t rv = encode
        (
            reinterpret_cast<uint8_t const *>(src),
            reinterpret_cast<uint8_t       *>(dst),
            off,
            cnt
        );

        if (rv != 0)
            dst[rv] = 0;

        return rv;
    }

    size_t decode(char const * src, char * dst, size_t off, size_t cnt)
    {
        if (cnt == 0)
            cnt = std::strlen(src);

        size_t rv = decode
        (
            reinterpret_cast<uint8_t const *>(src),
            reinterpret_cast<uint8_t       *>(dst),
            off,
            cnt
        );

        if (rv != 0)
            dst[rv] = 0;

        return rv;
    }

    bool is_valid(char const * src,             size_t off, size_t cnt)
    {
        if (cnt == 0U)
            cnt = std::strlen(src);
        return is_valid(reinterpret_cast<uint8_t const *>(src), off, cnt);
    }

    size_t encode_buffer_size(size_t cnt, bool with_zero)
    {
        size_t addend = static_cast<size_t>(with_zero == true);
        return (cnt + 2U) / 3U * 4U + addend;
    }

    size_t decode_buffer_size(size_t cnt, bool with_zero)
    {
        size_t addend = static_cast<size_t>(with_zero == true);
        return cnt / 4U * 3U + addend;
    }

    size_t decode_buffer_size(size_t cnt, char const * src, bool with_zero)
    {
        return decode_buffer_size
            (cnt, reinterpret_cast<uint8_t const *>(src), with_zero);
    }

    size_t decode_buffer_size(size_t cnt, uint8_t const *src, bool with_zero)
    {
        size_t padding_cnt = 0U;
        for (uint8_t const * ptr = src + cnt - 1U; *ptr == padding; ptr--)
            padding_cnt ++;
        return decode_buffer_size(cnt, with_zero) - padding_cnt;
    }
}}

CV_FS_PRIVATE_END
