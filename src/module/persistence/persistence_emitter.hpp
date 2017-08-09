/****************************************************************************
 *  license
 ***************************************************************************/

// TODO: define _HPP_
#pragma once

#include <iostream>
#include "persistence_private.hpp"
#include "persistence_string.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 * event
 ***************************************************************************/

namespace emitter
{
    /************************************************************************
     * event type
     ***********************************************************************/

    enum event_type
    {
        OUT_INT,
        OUT_DBL,
        OUT_STR,
        BEG_SEQ,
        BEG_MAP,
        END_SEQ,
        END_MAP
    };

    /************************************************************************
     * event data
     ***********************************************************************/

    template<event_type EVENT> struct event_t
    {
        /* default: empty */
    };

    template<> struct event_t<OUT_INT>
    {
        int64_t val;
    };

    template<> struct event_t<OUT_DBL>
    {
        double val;
    };

    template<> struct event_t<OUT_STR>
    {
        char const * val;
        size_t       len;
    };
}

/****************************************************************************
 * state
 ***************************************************************************/

namespace emitter
{
    /************************************************************************
     * state type
     ***********************************************************************/

    enum state_type
    {
        NIL,
        VAL,
        //BIN,
        SEQ_VAL,
        MAP_KEY,
        MAP_VAL
    };
}

/****************************************************************************
 * finite state machine
 ***************************************************************************/

namespace emitter
{
    class handler_t
    {
    public:
        virtual inline ~handler_t() {};

    public:
        virtual void change(state_type state) = 0;
        virtual void push  (state_type state) = 0;
        virtual void pop   () = 0;

        virtual void out(double  val                 ) = 0;
        virtual void out(int64_t val                 ) = 0;
        virtual void out(char const * val, size_t len) = 0;

        virtual state_type top() const = 0;

	public:
        virtual void error(event_type event) const = 0;
    };
}

/****************************************************************************
 * transition
 ***************************************************************************/

namespace emitter
{
    /************************************************************************
     * reject
     ***********************************************************************/

    template<state_type STATE, event_type EVENT> inline
    void transition(handler_t & handler, event_t<EVENT> const &)
    {
        handler.error(EVENT);
    }

    /************************************************************************
     * accept
     ***********************************************************************/

    /************************************************************************
     * state: VAL
     ***********************************************************************/

    template<> inline void transition
    <VAL, OUT_INT>
    (handler_t & handler, event_t<OUT_INT> const & event)
    {
        handler.out(event.val);
        handler.pop();
    }

    template<> inline void transition
    <VAL, OUT_DBL>
    (handler_t & handler, event_t<OUT_DBL> const & event)
    {
        handler.out(event.val);
        handler.pop();
    }

    template<> inline void transition
    <VAL, OUT_STR>
    (handler_t & handler, event_t<OUT_STR> const & event)
    {
        handler.out(event.val, event.len);
        handler.pop();
    }

    template<> inline void transition
    <VAL, BEG_SEQ>
    (handler_t & handler, event_t<BEG_SEQ> const &)
    {
        handler.change(SEQ_VAL);
    }

    template<> inline void transition
    <VAL, BEG_MAP>
    (handler_t & handler, event_t<BEG_MAP> const &)
    {
        handler.change(MAP_KEY);
    }

    /************************************************************************
     * state: SEQ_VAL
     ***********************************************************************/

    template<> inline void transition
    <SEQ_VAL, OUT_INT>
    (handler_t & handler, event_t<OUT_INT> const & event)
    {
        handler.out(event.val);
    }

    template<> inline void transition
    <SEQ_VAL, OUT_DBL>
    (handler_t & handler, event_t<OUT_DBL> const & event)
    {
        handler.out(event.val);
    }

    template<> inline void transition
    <SEQ_VAL, OUT_STR>
    (handler_t & handler, event_t<OUT_STR> const & event)
    {
        handler.out(event.val, event.len);
    }

    template<> inline void transition
    <SEQ_VAL, BEG_SEQ>
    (handler_t & handler, event_t<BEG_SEQ> const &)
    {
        handler.push(SEQ_VAL);
    }

    template<> inline void transition
    <SEQ_VAL, BEG_MAP>
    (handler_t & handler, event_t<BEG_MAP> const &)
    {
        handler.push(MAP_KEY);
    }

    template<> inline void transition
    <SEQ_VAL, END_SEQ>
    (handler_t & handler, event_t<END_SEQ> const &)
    {
        handler.pop();
    }

    /************************************************************************
     * state: MAP_KEY
     ***********************************************************************/

    template<> inline void transition
    <MAP_KEY, OUT_STR>
    (handler_t & handler, event_t<OUT_STR> const & event)
    {
        handler.out(event.val, event.len);
        handler.change(MAP_VAL);
    }

    template<> inline void transition
    <MAP_KEY, END_MAP>
    (handler_t & handler, event_t<END_MAP> const &)
    {
        handler.pop();
    }

    /************************************************************************
     * state: MAP_VAL
     ***********************************************************************/

    template<> inline void transition
    <MAP_VAL, OUT_INT>
    (handler_t & handler, event_t<OUT_INT> const & event)
    {
        handler.out(event.val);
        handler.change(MAP_KEY);
    }

    template<> inline void transition
    <MAP_VAL, OUT_DBL>
    (handler_t & handler, event_t<OUT_DBL> const & event)
    {
        handler.out(event.val);
        handler.change(MAP_KEY);
    }

    template<> inline void transition
    <MAP_VAL, OUT_STR>
    (handler_t & handler, event_t<OUT_STR> const & event)
    {
        handler.out(event.val, event.len);
        handler.change(MAP_KEY);
    }

    template<> inline void transition
    <MAP_VAL, BEG_SEQ>
    (handler_t & handler, event_t<BEG_SEQ> const &)
    {
        handler.change(MAP_KEY);
        handler.push  (SEQ_VAL);
    }

    template<> inline void transition
    <MAP_VAL, BEG_MAP>
    (handler_t & handler, event_t<BEG_MAP> const &)
    {
        handler.change(MAP_KEY);
        handler.push  (MAP_KEY);
    }

    /************************************************************************
     * transition
     ***********************************************************************/

    template<event_type EVENT> inline
     handler_t & operator <<
    (handler_t & handler, event_t<EVENT> const & event)
    {
        switch (handler.top())
        {
        case VAL    : transition<VAL    >(handler, event); break;
        case SEQ_VAL: transition<SEQ_VAL>(handler, event); break;
        case MAP_KEY: transition<MAP_KEY>(handler, event); break;
        case MAP_VAL: transition<MAP_VAL>(handler, event); break;
        default     : transition<NIL    >(handler, event); break;
        }
        return handler;
    }
}

/****************************************************************************
 * finite state machine
 ***************************************************************************/

namespace io
{
	class stream_t;
}

namespace emitter
{
    /************************************************************************
     * state machine
     ***********************************************************************/

    class json_fsm_t : public handler_t
    {
    public:
         json_fsm_t(io::stream_t * stream);
        ~json_fsm_t();

    public:
        void change(state_type state);
        void push  (state_type state);
        void pop   ();

        void out(double  val);
        void out(int64_t val);
        void out(char const * val, size_t len);

    public:
        state_type top() const;
        void error(event_type event) const;

    public:
        operator bool() const;

    public:
        typedef chars::buffer_t<state_type, 128U, std::allocator> stack_t;
        typedef chars::buffer_t<state_type, 128U, std::allocator> strbuf_t;

    private:
        stack_t stack_;
		io::stream_t * stream_;
        bool    is_container_empty_;
    };
}

CV_FS_PRIVATE_END
