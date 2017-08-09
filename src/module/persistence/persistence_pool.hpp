/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once
#include <memory>
#include <cstring>
#include <cstdio>
#include "persistence_private.hpp"
#include "persistence_string.hpp"
#include "persistence_fibonacci.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 * exception
 ***************************************************************************/
namespace exception
{
    using chars::soss_t;
    using chars::fmt;

    static inline void size_not_allowed(size_t size , POS_TYPE_)
    {
		error(0,
            ( soss_t<char, 64>()
                * "size `" | fmt<16>(size) | "` "
                | "is too large for this allocator"
            ), POS_ARGS_
        );
    }

    static inline void cnt_not_allowed(POS_TYPE_)
    {
		error(0,
            "this allocator produces too much fragmentation or "
            "an internal error occurred"
            , POS_ARGS_
        );
    }

    static inline void invalid_x2chunk(POS_TYPE_)
    {
        /* chunk was modified and become invalid */
        error(0,
            "memory chunk was modified unexpectedly or "
            "is invalid"
            , POS_ARGS_
        );
    }

    static inline void invalid_x2free(POS_TYPE_)
    {
        /* chunk was modified and become invalid */
        error(0,
            "memory was modified unexpectedly and "
            "some infomation lost"
            , POS_ARGS_
        );
    }

    static inline void invalid_x2memory(POS_TYPE_)
    {
        /* chunk was modified and become invalid */
        error(0,
            "failed to deallocate memory. "
            "maybe it was modified, or "
            "size of memory is wrong, or "
            "has deallocated more than once, or "
            "not allocated by this allocator"
            , POS_ARGS_
        );
    }

    static inline void null_argument(const char arg[], POS_TYPE_)
    {
		error(0,
            ( soss_t<char, 64>()
                * "argument `" | fmt<16>(arg) | "` is null"
            ), POS_ARGS_
        );
    }

    static inline void invalid_argument(const char arg[], POS_TYPE_)
    {
		error(0,
            ( soss_t<char, 64>()
                * "argument `" | fmt<32>(arg) | "` is invalid"
            ), POS_ARGS_
        );
    }

    static inline void alloc_failure(size_t size, POS_TYPE_)
    {
        /* no space or other reason */
		error(0,
            ( soss_t<char, 64>()
                * "failed to allocate `" | fmt<16>(size) | "` bytes memory."
            ), POS_ARGS_
        );
    }

    static inline void invalid_aligned(void * mem, POS_TYPE_)
    {
		error(0,
            ( soss_t<char, 64>()
                * "memory `" | fmt<32>(mem) | "` not aligned."
            ), POS_ARGS_
        );
    }
}

/****************************************************************************
 * storage
 ***************************************************************************/
namespace storage
{
    /************************************************************************
     * type
     ***********************************************************************/
    
    typedef uint8_t exp_type;
}

namespace storage
{
    /************************************************************************
     * x2allocator
     ***********************************************************************/

    template<typename T> class x2allocator_t
    {
    public:
        typedef T                  value_type;
        typedef value_type       *       pointer;
        typedef value_type const * const_pointer;
        typedef value_type       &       reference;
        typedef value_type const * const_reference;

    public:
        x2allocator_t();
        ~x2allocator_t();

    public:
        pointer allocate(             size_t size);
        void  deallocate(pointer mem, size_t size);

    public:
		void report() const;

    private:
        typedef     runtime_fibonacci_t<exp_type, size_t> rt_cap;
        typedef compiletime_fibonacci_t<exp_type, size_t> ct_cap;

    private:
        x2allocator_t            (x2allocator_t const &);
        x2allocator_t & operator=(x2allocator_t const &);

    private:
        struct x2chunk_t
        {
            x2chunk_t * nxt_;
            exp_type    exp_;
            uint16_t    cod_;
            uint32_t    cnt_;
        };

        struct x2chunklist_t
        {
            x2chunk_t * fst_;
            size_t      use_;
        };

        struct x2free_t
        {
            x2chunk_t * own_;
            x2free_t  * nxt_;
        };

        struct x2uesd_t
        {
            x2chunk_t * own_;
            exp_type    exp_;
            uint16_t    cod_;
        };

        union x2meminfo_t
        {
            x2free_t free_;
            x2uesd_t used_;
        };

        typedef x2free_t**                 x2freelist_t;
        typedef std::allocator<value_type> base_alloc_t;
        typedef std::allocator<x2free_t *> list_alloc_t;

    private:
        enum
        {
            VALUE_BYTE = sizeof(value_type),
            ALIGN_BYTE = sizeof(x2free_t), /* TODO: 16 */
            ALIGN_MASK = ALIGN_BYTE - size_t(1),
            HEAD_BYTE  = ((sizeof(x2chunk_t) + ALIGN_MASK) & ~ALIGN_MASK),
            HEAD_SIZE  = ( HEAD_BYTE + VALUE_BYTE - 1) / VALUE_BYTE,
            MIN_SIZE   = (ALIGN_BYTE + VALUE_BYTE - 1) / VALUE_BYTE,
            DEF_SIZE   = 8192U,
            MAX_EXP    = ct_cap::array::size,
            DFT_EXP    = ct_cap:: left<DEF_SIZE>::value,
            MIN_EXP    = ct_cap::right<MIN_SIZE>::value
        };

        typedef typename utility::assert_t
        <
            (VALUE_BYTE % ALIGN_BYTE == 0) || (ALIGN_BYTE % VALUE_BYTE == 0)
        >::type must_satisfy_the_alignment_condition_t;
        /* if you see this error, 
         * it means x2allocator_t cannot solve alignment problem with type T.
         * sizeof(T) may be 1,2,4,8...
         */

    private:
        static exp_type test_exp   (exp_type exp);
        static size_t   align      (size_t size);

        uint16_t        make_code  (void const * src, exp_type exp);

        void            make_chunk (exp_type exp);
        void            free_space (pointer mem, exp_type exp);

        pointer         chunk_alloc(exp_type exp);
        pointer         flist_alloc(exp_type exp);

    private:
        base_alloc_t  base_alloc_;
        list_alloc_t  list_alloc_;
        x2chunklist_t clist_;
        x2freelist_t  flist_;
    };

    /////////////////////////////////////////////////////////////////////////

    template<typename T> inline
        x2allocator_t<T>::x2allocator_t()
        : base_alloc_()
        , list_alloc_()
        , clist_()
        , flist_(list_alloc_.allocate(MAX_EXP))
    {
        if (flist_ == NULL)
            exception::alloc_failure(MAX_EXP * sizeof(x2free_t *), POS_);

        ::memset( flist_, 0, sizeof(*flist_) * MAX_EXP);
        ::memset(&clist_, 0, sizeof( clist_));
    }

    template<typename T>
    x2allocator_t<T>::~x2allocator_t()
    {
        x2chunk_t * iter = clist_.fst_;
        while (iter != NULL) {
            
            if (iter->cod_ != make_code(iter, iter->exp_))
                /* may cause memory leak if ignored */
                exception::invalid_x2chunk(POS_);

            x2chunk_t * next = iter->nxt_;
            pointer mem = reinterpret_cast<pointer>(iter);
            size_t  siz = HEAD_SIZE + MIN_SIZE + rt_cap::at(iter->exp_);
            base_alloc_.deallocate(mem, siz);
            iter = next;
        }

        list_alloc_.deallocate(flist_, MAX_EXP);
        ::memset(&clist_, 0, sizeof(clist_));
    }

    template<typename T> inline
    typename x2allocator_t<T>::pointer x2allocator_t<T>::
        allocate(size_t size)
    {
        exp_type exp = test_exp(rt_cap::right(size));

        /* get space */
        pointer mem = flist_alloc(exp);
        if (mem == NULL)
            mem = chunk_alloc(exp);
        if ((reinterpret_cast<size_t>(mem) & ALIGN_MASK) != 0)
            exception::invalid_aligned(mem, POS_);

        return mem;
    }

    template<typename T> inline
    void x2allocator_t<T>::deallocate(pointer mem, size_t size)
    {
        if (mem == NULL)
            return;
        if ((reinterpret_cast<size_t>(mem) & ALIGN_MASK) != 0)
            exception::invalid_aligned(mem, POS_);

        exp_type exp = test_exp(rt_cap::right(size));
        free_space(mem, exp);
    }

	template<typename T> inline
	void x2allocator_t<T>::report() const
	{
		::printf("=== report begin ===\n");
		::printf("%3s|%9s|%10s\n", "exp", "count", "size");
		{
			size_t total_size = 0;
			size_t total_cnt = 0;
			for (exp_type i = MIN_EXP; i < MAX_EXP; ++i) {
				size_t cnt = 0;
				x2free_t * iter = flist_[i];
				while (iter != NULL) {
					iter = iter->nxt_;
					++cnt;
				}
				if (cnt) {
					size_t siz = cnt * rt_cap::at(i) * sizeof(value_type);
					::printf("%02d, %8d: %f MB\n", i, cnt,siz/1024.0/1024.0);
					total_size += siz;
				}
				total_cnt += cnt * rt_cap::at(i);
			}
			::printf
				( "total %d unused: %f MB\n"
				, total_cnt
				, total_size / 1024.0 / 1024.0
				);
		}
		{
			size_t cnt = 0;
			size_t siz = 0;
			x2chunk_t * iter = clist_.fst_;
			while (iter != NULL) {
				siz += rt_cap::at(iter->exp_) * sizeof(value_type);
				++cnt;
				iter = iter->nxt_;
			}
			::printf("total %d allocated: %f MB\n",cnt,siz/1024.0/1024.0);
		}
		::printf("=== report end ===\n");
	}

    template<typename T> inline
    uint16_t x2allocator_t<T>::make_code(void const * src, exp_type exp)
    {
        /* do something like hash */
        uint16_t hash = exp;
        size_t   addr = reinterpret_cast<size_t>(src)
                      ^ reinterpret_cast<size_t>(this);
        while (addr) {
            hash = hash ^ uint16_t(23333) ^ static_cast<uint16_t>(addr);
            addr >>= 16;
        }
        return hash ^ ((hash >> 8) | (hash << 8));
    }

    template<typename T> inline
    void x2allocator_t<T>::make_chunk(exp_type exp)
    {
        exp = test_exp(exp);
        x2chunk_t * & fst = clist_.fst_;
        size_t      & use = clist_.use_;

        /* add current chunk to freelist */
        if (fst != NULL) {
            
            if (fst->cod_ != make_code(fst, fst->exp_))
                exception::invalid_x2chunk(POS_);

            size_t total = MIN_SIZE + rt_cap::at(fst->exp_);
            size_t rest  = total - use;
            while (rest > MIN_SIZE + ct_cap::at<MIN_EXP>::value) {
                exp_type rexp  = rt_cap::left(rest - MIN_SIZE);
                if (rexp < MIN_EXP)
                    break;

                free_space(chunk_alloc(rexp), rexp);
                rest = total - use;
            }
        }

        /* calc total memory needed */
        size_t size = HEAD_SIZE + MIN_SIZE + rt_cap::at(exp);
        x2chunk_t * mem = reinterpret_cast<x2chunk_t*>(
            base_alloc_.allocate(size)
        );

        /* do some check */
        if (mem == NULL)
            exception::alloc_failure(size * sizeof(value_type), POS_);
        if ((reinterpret_cast<size_t>(mem) & ALIGN_MASK) != 0)
            exception::invalid_aligned(mem, POS_);

        /* init */
        mem->nxt_ = fst;
        mem->exp_ = exp;
        mem->cod_ = make_code(mem, exp);
        mem->cnt_ = 0;

        /* done */
        fst = mem;
        use = 0;
    }

    template<typename T> inline
    typename x2allocator_t<T>::pointer x2allocator_t<T>::
        chunk_alloc(exp_type exp)
    {
        x2chunk_t * & fst = clist_.fst_;
        size_t      & use = clist_.use_;
        if (fst == NULL)
            make_chunk(utility::max(exp, DFT_EXP));

        size_t total = MIN_SIZE + rt_cap::at(fst->exp_);
        size_t rest  = total - use;
        size_t need  = MIN_SIZE + rt_cap::at(exp);
        if (need > rest)
            make_chunk(utility::max(exp, DFT_EXP));

        pointer mem = reinterpret_cast<pointer>(fst);
        mem += HEAD_SIZE + use;

        x2meminfo_t * info = reinterpret_cast<x2meminfo_t*>(mem); {
            info->used_.own_ = fst;
            info->used_.exp_ = exp;
            info->used_.cod_ = make_code(info, exp);
        }

        use  += MIN_SIZE + align(rt_cap::at(exp));
        total = MIN_SIZE + rt_cap::at(fst->exp_);
        if (use > total)
            use = total;
        if (++(fst->cnt_) == 0)
            exception::cnt_not_allowed(POS_);

        mem += MIN_SIZE;
        return mem;
    }

    template<typename T> inline
    typename x2allocator_t<T>::pointer x2allocator_t<T>::
        flist_alloc(exp_type exp)
    {
        x2free_t * & first = flist_[exp];
        if (first == NULL)
            return NULL;

        pointer mem = reinterpret_cast<pointer>(first);
        first = first->nxt_;

        x2meminfo_t * info = reinterpret_cast<x2meminfo_t*>(mem);
        x2chunk_t   * own = info->free_.own_;
        if (own == NULL || own->cod_ != make_code(own, own->exp_))
            exception::invalid_x2free(POS_);
        if (++(own->cnt_) == 0)
            exception::cnt_not_allowed(POS_);        

        info->used_.own_ = own;
        info->used_.exp_ = exp;
        info->used_.cod_ = make_code(info, exp);

        mem += MIN_SIZE;
        return mem;
    }

    template<typename T> inline
    void x2allocator_t<T>::
        free_space(pointer mem, exp_type exp)
    {
        /* do some check */
        if (mem == NULL)
            exception::null_argument("mem", POS_);
        if (exp >= MAX_EXP || exp < MIN_EXP)
            exception::invalid_argument("mem", POS_);

        /* add mem to freelist */
        mem -= MIN_SIZE;

        x2meminfo_t * info = reinterpret_cast<x2meminfo_t*>(mem);
        x2uesd_t    * use = &info->used_;
        if (use->exp_ != exp || use->cod_ != make_code(use, use->exp_))
            exception::invalid_x2memory(POS_);
        x2chunk_t   * own = use->own_;
        if (own == NULL || own->cod_ != make_code(own, own->exp_))
            exception::invalid_x2chunk(POS_);
        if ((own->cnt_)-- == 0)
            exception::invalid_x2chunk(POS_);

        info->free_.own_ = own;
        info->free_.nxt_ = flist_[exp];
    
        flist_[exp] = &info->free_;
    }

    template<typename T> inline
    exp_type x2allocator_t<T>::test_exp(exp_type exp)
    {
        /* assure exp is valid */
        if (exp >= MAX_EXP)
            exception::size_not_allowed(rt_cap::at(exp), POS_);
        if (exp < MIN_EXP)
            exp = MIN_EXP;
        return exp;
    }

    template<typename T> inline
    size_t x2allocator_t<T>::align(size_t size)
    {
        return ((size * VALUE_BYTE + ALIGN_MASK) & ~ALIGN_MASK) / VALUE_BYTE;
    }
}

namespace storage
{
    /************************************************************************
     * fx2allocator, not safe, but a litter smaller and faster
     ***********************************************************************/

    template<typename T> class fx2allocator_t
    {
    public:
        typedef T                  value_type;
        typedef value_type       *       pointer;
        typedef value_type const * const_pointer;
        typedef value_type       &       reference;
        typedef value_type const * const_reference;

    public:
        fx2allocator_t();
        ~fx2allocator_t();

    public:
        pointer allocate(             size_t size);
        void  deallocate(pointer mem, size_t size);

	public:
		void report() const;

    private:
        fx2allocator_t            (fx2allocator_t const &);
        fx2allocator_t & operator=(fx2allocator_t const &);

    private:
        struct x2chunk_t
        {
            x2chunk_t * nxt_;
            exp_type    exp_;
        };

        struct x2chunklist_t
        {
            x2chunk_t * fst_;
            size_t      use_;
        };

        struct x2free_t
        {
            x2free_t  * nxt_;
        };

        typedef x2free_t**                 x2freelist_t;
        typedef std::allocator<value_type> base_alloc_t;
        typedef std::allocator<x2free_t *> list_alloc_t;
        typedef     runtime_fibonacci_t<exp_type, size_t> rt_cap;
        typedef compiletime_fibonacci_t<exp_type, size_t> ct_cap;

    private:
        enum
        {
            VALUE_BYTE = sizeof(value_type),
            ALIGN_BYTE = sizeof(x2free_t), /* TODO: 16 */
            ALIGN_MASK = ALIGN_BYTE - size_t(1),
            HEAD_BYTE  = ((sizeof(x2chunk_t) + ALIGN_MASK) & ~ALIGN_MASK),
            HEAD_SIZE  = ( HEAD_BYTE + VALUE_BYTE - 1) / VALUE_BYTE,
            MIN_SIZE   = (ALIGN_BYTE + VALUE_BYTE - 1) / VALUE_BYTE,
            DEF_SIZE   = 8192U,
            MAX_EXP    = ct_cap::array::size,
            DFT_EXP    = ct_cap:: left<DEF_SIZE>::value,
            MIN_EXP    = ct_cap::right<MIN_SIZE>::value
        };

        typedef typename utility::assert_t
        <
            (VALUE_BYTE % ALIGN_BYTE == 0) || (ALIGN_BYTE % VALUE_BYTE == 0)
        >::type must_satisfy_the_alignment_condition_t;
        /* if you see this error, 
         * it means fx2allocator_t cannot solve alignment problem with type T
         * sizeof(T) may be 1,2,4,8...
         */

    private:
        static exp_type test_exp   (exp_type exp);
        static size_t   align      (size_t size);

        void            make_chunk (exp_type exp);
        void            free_space (pointer mem, exp_type exp);

        pointer         chunk_alloc(exp_type exp);
        pointer         flist_alloc(exp_type exp);

    private:
        base_alloc_t  base_alloc_;
        list_alloc_t  list_alloc_;
        x2chunklist_t clist_;
        x2freelist_t  flist_;
    };

    /////////////////////////////////////////////////////////////////////////

    template<typename T> inline
        fx2allocator_t<T>::fx2allocator_t()
        : base_alloc_()
        , list_alloc_()
        , clist_()
        , flist_(list_alloc_.allocate(MAX_EXP))
    {
        if (flist_ == NULL)
            exception::alloc_failure(MAX_EXP * sizeof(x2free_t *), POS_);

        ::memset( flist_, 0, sizeof(*flist_) * MAX_EXP);
        ::memset(&clist_, 0, sizeof( clist_));
    }

    template<typename T>
    fx2allocator_t<T>::~fx2allocator_t()
    {
        x2chunk_t * iter = clist_.fst_;
        while (iter != NULL) {
            x2chunk_t * next = iter->nxt_;
            pointer mem = reinterpret_cast<pointer>(iter);
            size_t  siz = HEAD_SIZE + rt_cap::at(iter->exp_);
            base_alloc_.deallocate(mem, siz);
            iter = next;
        }

        list_alloc_.deallocate(flist_, MAX_EXP);
        ::memset(&clist_, 0, sizeof(clist_));
    }

    template<typename T> inline
    typename fx2allocator_t<T>::pointer fx2allocator_t<T>::
        allocate(size_t size)
    {
        exp_type exp = test_exp(rt_cap::right(size));

        /* get space */
        pointer mem = flist_alloc(exp);
        if (mem == NULL)
            mem = chunk_alloc(exp);
        if ((reinterpret_cast<size_t>(mem) & ALIGN_MASK) != 0)
            exception::invalid_aligned(mem, POS_);

        return mem;
    }

    template<typename T> inline
        void fx2allocator_t<T>::deallocate(pointer mem, size_t size)
    {
        if (mem == NULL)
            return;
        if ((reinterpret_cast<size_t>(mem) & ALIGN_MASK) != 0)
            exception::invalid_aligned(mem, POS_);

        exp_type exp = test_exp(rt_cap::right(size));
        free_space(mem, exp);
    }

	template<typename T> inline
	void fx2allocator_t<T>::report() const
	{
		::printf("=== report begin ===\n");
		::printf("%3s|%9s|%10s\n", "exp", "count", "size");
		{
			size_t total_size = 0;
			size_t total_cnt = 0;
			for (exp_type i = MIN_EXP; i < MAX_EXP; ++i) {
				size_t cnt = 0;
				x2free_t * iter = flist_[i];
				while (iter != NULL) {
					iter = iter->nxt_;
					++cnt;
				}
				if (cnt) {
					size_t siz = cnt * rt_cap::at(i) * sizeof(value_type);
					::printf("%02d, %8d: %f MB\n", i, cnt,siz/1024.0/1024.0);
					total_size += siz;
				}
				total_cnt += cnt * rt_cap::at(i);
			}
			::printf
				( "total %d unused: %f MB\n"
				, total_cnt
				, total_size / 1024.0 / 1024.0
				);
		}
		{
			size_t cnt = 0;
			size_t siz = 0;
			x2chunk_t * iter = clist_.fst_;
			while (iter != NULL) {
				siz += rt_cap::at(iter->exp_) * sizeof(value_type);
				++cnt;
				iter = iter->nxt_;
			}
			::printf("total %d allocated: %f MB\n",cnt,siz/1024.0/1024.0);
		}
		::printf("=== report end ===\n");
	}

    template<typename T> inline
    void fx2allocator_t<T>::make_chunk(exp_type exp)
    {
        exp = test_exp(exp);
        x2chunk_t * & fst = clist_.fst_;
        size_t      & use = clist_.use_;

        /* add current chunk to freelist */
        if (fst != NULL) {
            size_t   total = rt_cap::at(fst->exp_);
            size_t   rest  = total - use;
            while (rest > ct_cap::at<MIN_EXP>::value) {
                exp_type rexp  = rt_cap::left(rest);
                if (rexp < MIN_EXP)
                    break;

                free_space(chunk_alloc(rexp), rexp);
                rest = total - use;
            }
        }

        /* calc total memory needed */
        size_t size = HEAD_SIZE + rt_cap::at(exp);
        x2chunk_t * mem = reinterpret_cast<x2chunk_t*>(
            base_alloc_.allocate(size)
        );

        /* do some check */
        if (mem == NULL)
            exception::alloc_failure(size * sizeof(value_type), POS_);
        if ((reinterpret_cast<size_t>(mem) & ALIGN_MASK) != 0)
            exception::invalid_aligned(mem, POS_);

        /* init */
        mem->nxt_ = fst;
        mem->exp_ = exp;

        /* done */
        fst = mem;
        use = 0;
    }

    template<typename T> inline
    typename fx2allocator_t<T>::pointer fx2allocator_t<T>::
        chunk_alloc(exp_type exp)
    {
        x2chunk_t * & fst = clist_.fst_;
        size_t      & use = clist_.use_;
        if (fst == NULL)
            make_chunk(utility::max(exp, DFT_EXP));

        size_t total = rt_cap::at(fst->exp_);
        size_t rest  = total - use;
        size_t need  = rt_cap::at(exp);
        if (need > rest)
            make_chunk(utility::max(exp, DFT_EXP));

        pointer mem = reinterpret_cast<pointer>(fst);
        mem += HEAD_SIZE + use;
        use += align(rt_cap::at(exp));
        total = rt_cap::at(fst->exp_);
        if (use > total)
            use = total;
        return mem;
    }

    template<typename T> inline
    typename fx2allocator_t<T>::pointer fx2allocator_t<T>::
        flist_alloc(exp_type exp)
    {
        x2free_t * & first = flist_[exp];
        if (first == NULL)
            return NULL;

        pointer mem = reinterpret_cast<pointer>(first);
        first = first->nxt_;
        return mem;
    }

    template<typename T> inline
    void fx2allocator_t<T>::
        free_space(pointer mem, exp_type exp)
    {
        /* do some check */
        if (mem == NULL)
            exception::null_argument("mem", POS_);

        /* add mem to freelist */

        x2free_t * free_mem = reinterpret_cast<x2free_t *>(mem);
        free_mem->nxt_ = flist_[exp];
        flist_[exp] = free_mem;
    }

    template<typename T> inline
    exp_type fx2allocator_t<T>::test_exp(exp_type exp)
    {
        /* assure exp is valid */
        if (exp >= MAX_EXP)
            exception::size_not_allowed(rt_cap::at(exp), POS_);
        if (exp < MIN_EXP)
            exp = MIN_EXP;
        return exp;
    }

    template<typename T> inline
    size_t fx2allocator_t<T>::align(size_t size)
    {
        return ((size * VALUE_BYTE + ALIGN_MASK) & ~ALIGN_MASK) / VALUE_BYTE;
    }
}

namespace storage { namespace internal
{
    /************************************************************************
     * pool_t helper
     ***********************************************************************/

    using namespace utility;
    using namespace utility::tl;

    template<typename Type, template<typename> class Alloc>
    class base
    {
    protected:
        inline base() : alloc_() {};
        Alloc<Type> alloc_;
    };
    
    template<template<typename> class Alloc>
    class base<end, Alloc> {};

    template<
        typename Type,
        typename Next,
        template<typename> class Alloc
    >
    class base<iter<Type, Next>, Alloc>
        : public base<Type, Alloc>
        , public base<Next, Alloc>
    {};

}}

namespace storage
{
    /************************************************************************
     * pool_t
     ***********************************************************************/

    template<typename List_T, template<typename> class Alloc_T>
    class pool_t;

    template<
        typename Type_T,
        typename Next_T,
        template<typename> class Alloc_T
    >
    class pool_t<internal::iter<Type_T, Next_T>, Alloc_T>
        : protected internal::base<internal::iter<Type_T, Next_T>, Alloc_T>
    {
        typedef internal::iter<Type_T, Next_T> list;
    public:

        template<typename T> inline
        typename internal::enable_if_t<
            internal::contain<list, T>::value, Alloc_T<T> &
        >::type allocator()
        {
            return internal::base<T, Alloc_T>::alloc_;
        }

        template<typename T> inline
        typename internal::enable_if_t<
            internal::contain<list, T>::value, T *
        >::type allocate(size_t size)
        {
            return internal::base<T, Alloc_T>::alloc_.allocate(size);
        }

        template<typename T> inline
        typename internal::enable_if_t<
            internal::contain<list, T>::value
        >::type deallocate(T * mem, size_t size)
        {
            internal::base<T, Alloc_T>::alloc_.deallocate(mem, size);
        }
    };

}

CV_FS_PRIVATE_END
