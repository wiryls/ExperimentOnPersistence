/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once

#include "persistence_private.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 *  Base64
 ***************************************************************************/
namespace code { namespace base64
{
    extern size_t encode
        (uint8_t const * src, uint8_t * dst, size_t off,size_t cnt);
    extern size_t decode
        (uint8_t const * src, uint8_t * dst, size_t off, size_t cnt);
    extern bool is_valid
        (uint8_t const * src,                size_t off, size_t cnt);

    extern size_t encode
        (char const * src, char * dst, size_t off = 0U, size_t cnt = 0U);
    extern size_t decode
        (char const * src, char * dst, size_t off = 0U, size_t cnt = 0U);
    extern bool is_valid
        (char const * src,             size_t off = 0U, size_t cnt = 0U);

    extern size_t encode_buffer_size
        (size_t cnt,                      bool end_with_zero = true);
    extern size_t decode_buffer_size
        (size_t cnt,                      bool end_with_zero = true);
    extern size_t decode_buffer_size
        (size_t cnt, char    const * src, bool end_with_zero = true);
    extern size_t decode_buffer_size
        (size_t cnt, uint8_t const * src, bool end_with_zero = true);
}}

/****************************************************************************
 *  Binarization
 ***************************************************************************/
namespace code { namespace binarization
{
    template<typename Type_T> inline size_t encode
        (Type_T          src, uint8_t * dst);

    template<typename Type_T> inline size_t encode
        (uint8_t const * src, uint8_t * dst);

    template<typename Type_T> inline size_t decode
        (uint8_t const * src, Type_T  & dst);

    template<typename Type_T> inline size_t decode
        (uint8_t const * src, uint8_t * dst);
}}

namespace code { namespace binarization
{
    template<typename Int_T> inline size_t
        encode(Int_T src, uint8_t * dst)
    {
        size_t cnt = sizeof(Int_T);
        while (cnt --> size_t(0)) {
            *dst++ = static_cast<uint8_t>(src);
            src >>= 8 /* CHAR_BIT */;
        }
        return sizeof(Int_T);
    }

    template<> inline size_t encode(double src, uint8_t * dst)
    {
        union { uint64_t u; double f; };
        f = src;
        return encode(u, dst);
    }

    template<> inline size_t encode(float src, uint8_t * dst)
    {
        union { uint32_t u; float f; };
        f = src;
        return encode(u, dst);
    }

    template<typename Type_T> inline size_t
        encode(uint8_t const * src, uint8_t * dst)
    {
        return encode<Type_T>(*reinterpret_cast<Type_T const *>(src), dst);
    }

    template<typename Int_T> inline size_t
        decode(uint8_t const * src, Int_T & dst)
    {
        dst = Int_T(0);
        for (size_t i = size_t(0); i < sizeof(Int_T); i++)
            dst |= (static_cast<Int_T>(*src++) << (i * 8U /* CHAR_BIT */));
        return sizeof(Int_T);
    }

    template<> inline size_t decode(uint8_t const * src, double & dst)
    {
        union { uint64_t u; double f; };
        decode(src, u);
        dst = f;
        return sizeof(dst);
    }

    template<> inline size_t decode(uint8_t const * src, float & dst)
    {
        union { uint32_t u; float f; };
        decode(src, u);
        dst = f;
        return sizeof(dst);
    }

    template<typename Type_T> inline size_t
        decode(uint8_t const * src, uint8_t * dst)
    {
        return decode<Type_T>(src, *reinterpret_cast<Type_T *>(dst));
    }
}}

CV_FS_PRIVATE_END
