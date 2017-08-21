/****************************************************************************
 *  license
 ***************************************************************************/

#include <iostream>
#include "persistence_private.hpp"
#include "persistence_utility.hpp"
#include "persistence_io.hpp"
#include "persistence_string.hpp"
#include "persistence_emitter.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 *  exception
 ***************************************************************************/

namespace exception
{
    using chars::Soss;
    using chars::fmt;

    static inline void fsm_no_state(POS_TYPE_)
    {
        error(0,
            ( Soss<char, 64>()
                * "finite state machine is empty"
            ), POS_ARGS_
        );
    }

    static inline void error_state(int i, POS_TYPE_)
    {
        error(0,
            ( Soss<char, 64>()
                * "state `"
                | fmt<16>(i)
                | "` is error"
            ), POS_ARGS_
        );
    }

    static inline void error_event(int i, POS_TYPE_)
    {
        error(0,
            ( Soss<char, 64>()
                * "event `"
                | fmt<16>(i)
                | "` is not accepted"
            ), POS_ARGS_
        );
    }
}

/****************************************************************************
 * finite state machine
 ***************************************************************************/

namespace emitter
{
    JsonFSM::JsonFSM(io::Stream * stream)
        : stack_()
        , stream_(stream)
        , is_container_empty_(true)
    {
        stack_.push_back(NIL);
        stack_.push_back(VAL);
    }

    JsonFSM::~JsonFSM()
    {
        while (stack_.back() == SEQ_VAL || stack_.back() == MAP_KEY) {
            while (stack_.back() == SEQ_VAL) {
                transition<SEQ_VAL>(*this, Event<END_SEQ>());
            }
            while (stack_.back() == MAP_KEY) {
                transition<MAP_KEY>(*this, Event<END_MAP>());
            }
        }

        stream_->close();
        delete stream_;
    }

    void JsonFSM::change(StateTag state)
    {
        stack_.back() = state;
    }

    void JsonFSM::push(StateTag state)
    {
        if (is_container_empty_) {
            is_container_empty_ = false;
            if (top() == SEQ_VAL)
                stream_->write("[", 1);
            else if (top() == MAP_KEY)
                stream_->write("{", 1);
        } else {
            if (top() == MAP_KEY)
                stream_->write(": ", 2);
            else
                stream_->write(",", 1);
        }

        stack_.push_back(state);
        is_container_empty_ = true;
    }

    void JsonFSM::pop()
    {
        if (is_container_empty_) {
            is_container_empty_ = false;
            if (top() == SEQ_VAL)
                stream_->write("[", 1);
            else if (top() == MAP_KEY)
                stream_->write("{", 1);
        }

        if (top() == SEQ_VAL)
            stream_->write("]", 1);
        else if (top() == MAP_KEY)
            stream_->write("}", 1);

        stack_.pop_back();
    }

    void JsonFSM::out(double val)
    {
        if (is_container_empty_) {
            is_container_empty_ = false;
            if (top() == SEQ_VAL)
                stream_->write("[", 1);
            else if (top() == MAP_KEY)
                stream_->write("{", 1);
        } else {
            if (top() == MAP_VAL)
                stream_->write(": ", 2);
            else
                stream_->write(",", 1);
        }

        char buffer[30];
        chars::make_string(val, buffer);
        stream_->write(buffer, chars::strlen(buffer));
    }

    void JsonFSM::out(int64_t val)
    {
        if (is_container_empty_) {
            is_container_empty_ = false;
            if (top() == SEQ_VAL)
                stream_->write("[", 1);
            else if (top() == MAP_KEY)
                stream_->write("{", 1);
        } else {
            if (top() == MAP_VAL)
                stream_->write(": ", 2);
            else
                stream_->write(",", 1);
        }

        char buffer[30];
        chars::make_string(val, buffer);
        stream_->write(buffer, chars::strlen(buffer));
    }

    inline char const * esc_to_chr(char ch)
    {
        switch (ch)
        {
        case '\\': return "\\";
        case '\'': return "'";
        case '\"': return "\"";
        case '\n': return "\\n";
        case '\r': return "\\r";
        case '\t': return "\\t";
        case '\b': return "\\b";
        case '\f': return "\\f";
        default  : return NULL;
        }
    }

    void JsonFSM::out(char const * val, size_t len)
    {
        if (is_container_empty_) {
            is_container_empty_ = false;
            if (top() == SEQ_VAL)
                stream_->write("[", 1);
            else if (top() == MAP_KEY)
                stream_->write("{", 1);
        } else {
            if (top() == MAP_VAL)
                stream_->write(": ", 2);
            else
                stream_->write(",", 1);
        }

        stream_->write("\"", 1);

        typedef char const * const_iter;
        const_iter iter_beg = val;
        const_iter iter_end = val + len;
        for (const_iter iter = iter_beg; iter != iter_end; ++iter) {
            const char * cvt = esc_to_chr(*iter);
            if (cvt == NULL)
                stream_->write(iter, 1);
            else
                stream_->write(cvt, chars::strlen(cvt));
        }

        stream_->write("\"", 1);
    }

    StateTag JsonFSM::top() const
    {
        return stack_.back();
    }

    void JsonFSM::error(EventTag event) const
    {
        exception::error_event(event, POS_);
    }

    JsonFSM::operator bool() const
    {
        return !stack_.empty();
    }
}

CV_FS_PRIVATE_END
