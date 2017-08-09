/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once

#include <cstddef>

/****************************************************************************
 *  C++ version differences
 ***************************************************************************/

#define NOTHROW throw()

/****************************************************************************
 *  Compiler differences
 ***************************************************************************/

#ifndef _MSC_VER
#define AVOID_MSVC_C2244_ template
#else
#define AVOID_MSVC_C2244_
#define _CRT_SECURE_NO_WARNINGS
#endif

/****************************************************************************
 *  namespace
 ***************************************************************************/

#ifndef CV_PRIVATE
#define CV_PRIVATE
#endif

#if (defined CV_FS_PRIVATE_BEGIN) || (defined CV_FS_PRIVATE_END)
#error "conflicts!"
#else
#define CV_FS_PRIVATE_BEGIN namespace test {
#define CV_FS_PRIVATE_END   }
#endif

#if (defined CV_FS_PRIVATE_NS)
#error "conflicts!"
#else
#define CV_FS_PRIVATE_NS test
#endif

/****************************************************************************
 *  primitive type
 ***************************************************************************/

CV_FS_PRIVATE_BEGIN

typedef signed char        int8_t;
typedef short              int16_t;
typedef int                int32_t;
typedef long long          int64_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;

//typedef ::int8_t         int8_t;
//typedef ::int16_t        int16_t;
//typedef ::int32_t        int32_t;
//typedef ::int64_t        int64_t;
//
//typedef ::uint8_t        uint8_t;
//typedef ::uint16_t       uint16_t;
//typedef ::uint32_t       uint32_t;
//typedef ::uint64_t       uint64_t;

typedef uint8_t          byte_t;

using ::size_t;

CV_FS_PRIVATE_END

/****************************************************************************
 *  exception
****************************************************************************/

#if (defined POS_) || (defined POS_TYPE_) || (defined POS_ARGS_)
#error "conflicts!"
#else
#define POS_            __FUNCTION__,          __FILE__, __LINE__
#define POS_TYPE_  char const * func, char const * file, int line
#define POS_ARGS_               func,              file,     line
#endif

CV_FS_PRIVATE_BEGIN
namespace exception
{
    void error(int code, char const * err , POS_TYPE_);
}
CV_FS_PRIVATE_END

/****************************************************************************
 *  assert
****************************************************************************/

#if (defined ASSERT) || (defined ASSERT_DBG)
#error "conflicts!"
#else
#define ASSERT(expr) (void)((expr) || \
                     (::CV_FS_PRIVATE_NS::exception::error(1,#expr,POS_),0))

#if (defined _DEBUG) || (defined DEBUG)
#define ASSERT_DBG(expr) ASSERT(expr)
#else
#define ASSERT_DBG(expr)
#endif
#endif
