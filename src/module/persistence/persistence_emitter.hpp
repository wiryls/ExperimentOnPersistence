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

    enum EventTag
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

    template<EventTag EVENT> struct Event
    {
        /* default: empty */
    };

    template<> struct Event<OUT_INT>
    {
        int64_t val;
    };

    template<> struct Event<OUT_DBL>
    {
        double val;
    };

    template<> struct Event<OUT_STR>
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

    enum StateTag
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
    class Handler
    {
    public:
        virtual inline ~Handler() {};

    public:
        virtual void change(StateTag state) = 0;
        virtual void push  (StateTag state) = 0;
        virtual void pop   () = 0;

        virtual void out(double  val                 ) = 0;
        virtual void out(int64_t val                 ) = 0;
        virtual void out(char const * val, size_t len) = 0;

        virtual StateTag top() const = 0;

    public:
        virtual void error(EventTag event) const = 0;
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

    template<StateTag STATE, EventTag EVENT> inline
    void transition(Handler & handler, Event<EVENT> const &)
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
    (Handler & handler, Event<OUT_INT> const & event)
    {
        handler.out(event.val);
        handler.pop();
    }

    template<> inline void transition
    <VAL, OUT_DBL>
    (Handler & handler, Event<OUT_DBL> const & event)
    {
        handler.out(event.val);
        handler.pop();
    }

    template<> inline void transition
    <VAL, OUT_STR>
    (Handler & handler, Event<OUT_STR> const & event)
    {
        handler.out(event.val, event.len);
        handler.pop();
    }

    template<> inline void transition
    <VAL, BEG_SEQ>
    (Handler & handler, Event<BEG_SEQ> const &)
    {
        handler.change(SEQ_VAL);
    }

    template<> inline void transition
    <VAL, BEG_MAP>
    (Handler & handler, Event<BEG_MAP> const &)
    {
        handler.change(MAP_KEY);
    }

    /************************************************************************
     * state: SEQ_VAL
     ***********************************************************************/

    template<> inline void transition
    <SEQ_VAL, OUT_INT>
    (Handler & handler, Event<OUT_INT> const & event)
    {
        handler.out(event.val);
    }

    template<> inline void transition
    <SEQ_VAL, OUT_DBL>
    (Handler & handler, Event<OUT_DBL> const & event)
    {
        handler.out(event.val);
    }

    template<> inline void transition
    <SEQ_VAL, OUT_STR>
    (Handler & handler, Event<OUT_STR> const & event)
    {
        handler.out(event.val, event.len);
    }

    template<> inline void transition
    <SEQ_VAL, BEG_SEQ>
    (Handler & handler, Event<BEG_SEQ> const &)
    {
        handler.push(SEQ_VAL);
    }

    template<> inline void transition
    <SEQ_VAL, BEG_MAP>
    (Handler & handler, Event<BEG_MAP> const &)
    {
        handler.push(MAP_KEY);
    }

    template<> inline void transition
    <SEQ_VAL, END_SEQ>
    (Handler & handler, Event<END_SEQ> const &)
    {
        handler.pop();
    }

    /************************************************************************
     * state: MAP_KEY
     ***********************************************************************/

    template<> inline void transition
    <MAP_KEY, OUT_STR>
    (Handler & handler, Event<OUT_STR> const & event)
    {
        handler.out(event.val, event.len);
        handler.change(MAP_VAL);
    }

    template<> inline void transition
    <MAP_KEY, END_MAP>
    (Handler & handler, Event<END_MAP> const &)
    {
        handler.pop();
    }

    /************************************************************************
     * state: MAP_VAL
     ***********************************************************************/

    template<> inline void transition
    <MAP_VAL, OUT_INT>
    (Handler & handler, Event<OUT_INT> const & event)
    {
        handler.out(event.val);
        handler.change(MAP_KEY);
    }

    template<> inline void transition
    <MAP_VAL, OUT_DBL>
    (Handler & handler, Event<OUT_DBL> const & event)
    {
        handler.out(event.val);
        handler.change(MAP_KEY);
    }

    template<> inline void transition
    <MAP_VAL, OUT_STR>
    (Handler & handler, Event<OUT_STR> const & event)
    {
        handler.out(event.val, event.len);
        handler.change(MAP_KEY);
    }

    template<> inline void transition
    <MAP_VAL, BEG_SEQ>
    (Handler & handler, Event<BEG_SEQ> const &)
    {
        handler.change(MAP_KEY);
        handler.push  (SEQ_VAL);
    }

    template<> inline void transition
    <MAP_VAL, BEG_MAP>
    (Handler & handler, Event<BEG_MAP> const &)
    {
        handler.change(MAP_KEY);
        handler.push  (MAP_KEY);
    }

    /************************************************************************
     * transition
     ***********************************************************************/

    template<EventTag EVENT> inline
     Handler & operator <<
    (Handler & handler, Event<EVENT> const & event)
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
    class Stream;
}

namespace emitter
{
    /************************************************************************
     * state machine
     ***********************************************************************/

    class JsonFSM : public Handler
    {
    public:
         JsonFSM(io::Stream * stream);
        ~JsonFSM();

    public:
        void change(StateTag state);
        void push  (StateTag state);
        void pop   ();

        void out(double  val);
        void out(int64_t val);
        void out(char const * val, size_t len);

    public:
        StateTag top() const;
        void error(EventTag event) const;

    public:
        operator bool() const;

    public:
        typedef chars::Buffer<StateTag, 128U, std::allocator> Stack;
        typedef chars::Buffer<StateTag, 128U, std::allocator> Strbuf;

    private:
        Stack        stack_;
        io::Stream * stream_;
        bool         is_container_empty_;
    };
}

CV_FS_PRIVATE_END
