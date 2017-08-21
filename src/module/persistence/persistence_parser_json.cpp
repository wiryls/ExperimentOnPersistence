/****************************************************************************
 *  license
 ***************************************************************************/

#include <cmath>
#include <limits>

#include "persistence_private.hpp"
#include "persistence_chars.hpp"
#include "persistence_string.hpp"
#include "persistence_parser_helper.hpp"
#include "persistence_parser.hpp"

CV_FS_PRIVATE_BEGIN

/****************************************************************************
 *  Forward Declaration
 ***************************************************************************/

namespace parser { namespace json
{
    template<typename CharType> class Builder;
    typedef StreamHelper<Stream, Builder<char> > In;
}}

/****************************************************************************
 *  exception
 ***************************************************************************/

namespace exception
{
    using chars::Soss;
    using chars::fmt;

    inline static bool opt_error(
        parser::json::In & in,
        char const * option,
        char const * status)
    {
        parser::raise_error(in,
            ( Soss<char, 256>()
                * "option `"
                | fmt<96>(option)
                | "` is `"
                | fmt<32>(status)
                | "`, but got `"
                | fmt<16>(in.eof() ? "End Of File" : in.data())
                | "`, at("
                | fmt<32>(in.line())
                | ", "
                | fmt<32>(in.col())
                | ')'
            )
        );
        /* if not throw return false */
        return false;
    }

    inline static bool expect(
        parser::json::In & in,
        char const * expected,
        char const * hint)
    {
        parser::raise_error(in,
            ( Soss<char, 256>()
                * "expecting `"
                | fmt<32>(expected)
                | "` but got `"
                | fmt<16>(in.eof() ? "End Of File" : in.data())
                | "` ["
                | fmt<96>(hint)
                | "], at("
                | fmt<32>(in.line())
                | ", "
                | fmt<32>(in.col())
                | ')'
            )
        );
        /* if not throw return false */
        return false;
    }

    inline static bool warning(parser::json::In & in, char const * message)
    {
        if (in.get_settings().enable_warning_message == false)
            return true;

        parser::raise_warning(in,
            ( Soss<char, 256>()
                * fmt<128>(message)
                | ", at ("
                | fmt<32>(in.line())
                | ", "
                | fmt<32>(in.col())
                | ')'
            )
        );
        return true;
    }
}

/****************************************************************************
 *  JSON ast builder
 ***************************************************************************/

namespace parser { namespace json
{
    /************************************************************************
     * declaration Builder
     ***********************************************************************/

    template<typename CharType>
    class Builder
    {
    public:
        typedef ast::Tree<CharType> Tree;
        typedef ast::Node<CharType> Node;

    public:
        Builder(Tree & tree);

    public:
        void map_beg();
        void map_key();
        void map_val();
        void map_end();

        void seq_beg();
        void seq_val();
        void seq_end();

        void str_beg();
        void str_end();

    public:
        void on_nil();
        void on_int(int64_t i);
        void on_dbl(double  d);
        void on_chr(CharType ch);

    public:

    private:
        enum status_t
        {
            MAP_STR,
            MAP_OBJ,
            SEQ_OBJ,
            STR_CHR
        };

    private:
        typedef chars::Buffer<status_t, 128, std::allocator> sstack_t;
        typedef chars::Buffer<Node *, 128, std::allocator> nstack_t;
        typedef chars::Buffer<  CharType, 128, std::allocator> Buffer;

        Tree    & tree_;
        nstack_t  nstack_;
        sstack_t  sstack_;
        Buffer  buffer_;
    };

    /************************************************************************
     * implementation Builder
     ***********************************************************************/

    template<typename CharType> inline Builder<CharType>::
        Builder(Tree & tree)
        : tree_(tree)
        , nstack_()
        , sstack_()
        , buffer_()
    {
        nstack_.push_back(&tree_.root());
    }

    template<typename CharType> inline void Builder<CharType>::
        map_beg()
    {
        using namespace ast;
        Node & top = *nstack_.back();
        top.template construct<MAP>(tree_.pool());
    }

    template<typename CharType> inline void Builder<CharType>::
        map_key()
    {
        using namespace ast;
        typedef typename Node::Pair Pair;
        Pair * pair = NULL;
        {
            Node & top = *nstack_.back();
            Pair dummy;
            dummy[0].construct(tree_.pool());
            dummy[1].construct(tree_.pool());
            top.template move_back<MAP>(dummy, tree_.pool());

            pair = top.template rbegin<MAP>();
        }
        nstack_.push_back(&((*pair)[1]));
        nstack_.push_back(&((*pair)[0]));
    }

    template<typename CharType> inline void Builder<CharType>::
        map_val()
    {
        ;
    }

    template<typename CharType> inline void Builder<CharType>::
        map_end()
    {
        nstack_.pop_back();
    }

    template<typename CharType> inline void Builder<CharType>::
        seq_beg()
    {
        using namespace ast;
        Node & top = *nstack_.back();
        top.template construct<SEQ>(tree_.pool());
    }

    template<typename CharType> inline void Builder<CharType>::
        seq_val()
    {
        using namespace ast;
        Node * node = NULL;
        {
            Node & top = *nstack_.back();
            Node dummy;
            dummy.construct(tree_.pool());
            top.template move_back<SEQ>(dummy, tree_.pool());
            node = top.template rbegin<SEQ>();
        }
        nstack_.push_back(node);
    }

    template<typename CharType> inline void Builder<CharType>::
        seq_end()
    {
        nstack_.pop_back();
    }

    template<typename CharType> inline void Builder<CharType>::
        str_beg()
    {
        buffer_.clear();
    }

    template<typename CharType> inline void Builder<CharType>::
        str_end()
    {
        using namespace ast;
        Node & top = *nstack_.back();
        top.template set<STR>(buffer_.begin(), buffer_.end(),tree_.pool());
        buffer_.clear();
        nstack_.pop_back();
    }

    template<typename CharType> inline void Builder<CharType>::
        on_int(int64_t val)
    {
        using namespace ast;
        Node & top = *nstack_.back();
        top.template set<I64>(val, tree_.pool());
        nstack_.pop_back();
    }

    template<typename CharType> inline void Builder<CharType>::
        on_dbl(double val)
    {
        using namespace ast;
        Node & top = *nstack_.back();
        top.template set<DBL>(val, tree_.pool());
        nstack_.pop_back();
    }

    template<typename CharType> inline void Builder<CharType>::
        on_nil()
    {
        nstack_.pop_back();
    }

    template<typename CharType> inline void Builder<CharType>::
        on_chr(CharType ch)
    {
        buffer_.push_back(ch);
    }

}}

/****************************************************************************
 *  JSON Keyword
 ***************************************************************************/

namespace parser { namespace json
{
    template<typename CharType> struct KeywordTable
    {
        typedef CharType CharType;
        typedef const Keyword<CharType> value_type;

        static value_type COLON;
        static value_type COMMA;
        static value_type ESCAPE;
        static value_type HEX;
        static value_type MINUS;
        static value_type PLUS;
        static value_type ZERO;
        static value_type DOT;
        static value_type EXP_U;
        static value_type EXP_L;

        static value_type STR_BEG;
        static value_type STR_END;
        static value_type SEQ_BEG;
        static value_type SEQ_END;
        static value_type MAP_BEG;
        static value_type MAP_END;

        static value_type VAL_FALSE;
        static value_type VAL_TRUE;
        static value_type VAL_NULL;

        /* first of {C_COMMENT_1_BEG, C_COMMENT_2_BEG} */
        static value_type   COMMENT_FIRST;
        static value_type C_COMMENT_1_BEG;
        static value_type C_COMMENT_1_END;
        static value_type C_COMMENT_2_BEG;
        static value_type C_COMMENT_2_END;
    };

#ifdef DEFINE_KEYWORD
#error "conflicts!"
#else
#define DEFINE_KEYWORD(type, name, value) \
    template<> const Keyword<type> KeywordTable<type>::name = value

    DEFINE_KEYWORD(char, COLON , ':');
    DEFINE_KEYWORD(char, COMMA , ',');
    DEFINE_KEYWORD(char, ESCAPE, '\\');
    DEFINE_KEYWORD(char, HEX   , 'u');
    DEFINE_KEYWORD(char, MINUS , '-');
    DEFINE_KEYWORD(char, PLUS  , '+');
    DEFINE_KEYWORD(char, ZERO  , '0');
    DEFINE_KEYWORD(char, DOT   , '.');
    DEFINE_KEYWORD(char, EXP_U , 'E');
    DEFINE_KEYWORD(char, EXP_L , 'e');

    DEFINE_KEYWORD(char, STR_BEG , '"');
    DEFINE_KEYWORD(char, STR_END , '"');
    DEFINE_KEYWORD(char, SEQ_BEG , '[');
    DEFINE_KEYWORD(char, SEQ_END , ']');
    DEFINE_KEYWORD(char, MAP_BEG , '{');
    DEFINE_KEYWORD(char, MAP_END , '}');

    DEFINE_KEYWORD(char, VAL_FALSE, "false");
    DEFINE_KEYWORD(char, VAL_TRUE , "true" );
    DEFINE_KEYWORD(char, VAL_NULL , "null" );

    DEFINE_KEYWORD(char,   COMMENT_FIRST, '/');
    DEFINE_KEYWORD(char, C_COMMENT_1_BEG, "/*");
    DEFINE_KEYWORD(char, C_COMMENT_1_END, "*/");
    DEFINE_KEYWORD(char, C_COMMENT_2_BEG, "//");
    DEFINE_KEYWORD(char, C_COMMENT_2_END, '\n');

#undef DEFINE_KEYWORD
#endif
}}

/****************************************************************************
 *  JSON parser
 ***************************************************************************/

namespace parser { namespace json
{
    /************************************************************************
     * helper
     ***********************************************************************/

    template<typename CharType> inline CharType chr_to_esc(CharType);

    template<> inline char chr_to_esc(char ch)
    {
        switch (ch)
        {
        case '\\':
        case '\'':
        case '"' : return ch;
        case 'n' : return '\n';
        case 'r' : return '\r';
        case 't' : return '\t';
        case 'b' : return '\b';
        case 'f' : return '\f';
        default  : return '\0';
        }
    }
}}

namespace parser { namespace json
{
    /************************************************************************
     * Declaration
     ***********************************************************************/

    /* references:
     * 0. http://json.org/
     * 1. https://tools.ietf.org/html/rfc7159
     * 2. ECMA-404.pdf
     */

    /*
     *  object
     *      {}
     *      { members }
     *  members
     *      pair
     *      pair , members
     *  pair
     *      string : value
     */
    template<typename InType> inline bool parse_object(InType & in);

    /*
     *  array
     *      []
     *      [ elements ]
     *  elements
     *      value
     *      value , elements
     */
    template<typename InType> inline bool parse_array(InType & in);

    /*
     *  value
     *      string
     *      number
     *      object
     *      array
     *      keyword
     */
    template<typename InType> inline bool parse_value(InType & in);

    /*
     *  keyword
     *      true
     *      false
     *      null
     */
    template<typename InType> inline bool parse_keyword(InType & in);

    /*
     *  string
     *      ""
     *      " chars "
     *  chars
     *      char
     *      char chars
     *  char
     *      any-Unicode-character-except-"-or-\-or-control-character
     *      \"
     *      \\
     *      \/
     *      \b
     *      \f
     *      \n
     *      \r
     *      \t
     *      \u four-hex-digits
     */
    template<typename InType> inline bool parse_string(InType & in);

    /*
     *  number
     *      int
     *      int frac
     *      int exp
     *      int frac exp
     *  int
     *      digit
     *      digit1-9 digits
     *      - digit
     *      - digit1-9 digits
     *  frac
     *      . digits
     *  exp
     *      e digits
     *  digits
     *      digit
     *      digit digits
     *  e
     *      e
     *      e+
     *      e-
     *      E
     *      E+
     *      E-
     */
    template<typename InType> inline bool parse_number(InType & in);

    /*
     *  cpp style comments
     *      / / any-character-except-newline-or-eof (newline | eof)
     *      / * any-character-except-*-and-/ * /
     */
    template<typename InType> inline bool skip_comments(InType & in);

    /************************************************************************
     * Implementation
     ***********************************************************************/

    /* note: each function must do `skip(chars::isspace)` at the end */

    template<typename InType> inline bool parse_object(InType & in)
    {
        typedef typename InType::CharType CharType;
        typedef KeywordTable<CharType> kwd;

        /* { */
        if (! match(in, kwd::MAP_BEG))
            return exception::expect(in, kwd::MAP_BEG, "JSON object");

        if (! skip_comments(in.skip(chars::isspace)))
            return false;

        Builder<CharType> & builder = in.get();
        builder.map_beg();

        /* { } */
        if (match(in, kwd::MAP_END)) {
            builder.map_end();
            return skip_comments(in.skip(chars::isspace));
        }

        bool is_continue = false;
        do { /* { members } */

            builder.map_key();
            if (! parse_string(in))
                return false;

            if (! skip_comments(in))
                return false;

            if (! match(in, kwd::COLON))
                return exception::expect(in, kwd::COLON, "JSON pair");

            if (! skip_comments(in.skip(chars::isspace)))
                return false;

            builder.map_val();
            if (! parse_value(in))
                return false;

            if (     match(in, kwd::COMMA))
                is_continue = true;
            else if (match(in, kwd::MAP_END))
                is_continue = false;
            else
                return exception::expect(in, ",` or `}", "JSON object");

            if (! skip_comments(in.skip(chars::isspace)))
                return false;
        } while(is_continue);

        builder.map_end();
        return true;
    }

    template<typename InType> inline bool parse_array(InType & in)
    {
        typedef typename InType::CharType CharType;
        typedef KeywordTable<CharType> kwd;

        /* [ */
        if (! match(in, kwd::SEQ_BEG))
            return exception::expect(in, kwd::SEQ_BEG, "JSON array");

        if (! skip_comments(in.skip(chars::isspace)))
            return false;

        Builder<CharType> & builder = in.get();
        builder.seq_beg();

        /* [ ] */
        if (match(in, kwd::SEQ_END)) {
            builder.seq_end();
            return skip_comments(in.skip(chars::isspace));
        }

        /* [ elements ] */
        bool is_continue = false;
        do {
            builder.seq_val();
            if (! parse_value(in))
                return false;

            if (! skip_comments(in))
                return false;

            if (     match(in, kwd::COMMA))
                is_continue = true;
            else if (match(in, kwd::SEQ_END))
                is_continue = false;
            else
                return exception::expect(in, ",` or `]", "JSON array");

            if (! skip_comments(in.skip(chars::isspace)))
                return false;
        } while(is_continue);

        builder.seq_end();
        return true;
    }

    template<typename InType> inline bool parse_value(InType & in)
    {
        typedef typename InType::CharType   CharType;
        typedef KeywordTable<CharType> kwd;

        bool (* func)(InType &) = NULL;
        CharType ch = in.ch();
        if (       ch == kwd::STR_BEG) {
            func = parse_string;
        } else if (ch == kwd::MAP_BEG) {
            func = parse_object;
        } else if (ch == kwd::SEQ_BEG) {
            func = parse_array;
        } else if (chars::isdigit(ch) || ch == kwd::MINUS) {
            func = parse_number;
        } else if (chars::isalpha(ch)) {
            func = parse_keyword;
        } else {
            return exception::expect(in, "value", "JSON value");
        }

        if (! func(in))
            return false;

        return skip_comments(in.skip(chars::isspace));
    }

    template<typename InType> inline bool parse_keyword(InType & in)
    {
        typedef typename InType::CharType   CharType;
        typedef KeywordTable<CharType> kwd;
        using namespace ast;

        typename kwd::value_type const * keyword = NULL;
        CharType                                ch = in.ch();
        Builder<CharType> &              builder = in.get();

        if (     ch == kwd::VAL_TRUE)
            keyword =& kwd::VAL_TRUE;
        else if (ch == kwd::VAL_FALSE)
            keyword =& kwd::VAL_FALSE;
        else if (ch == kwd::VAL_NULL)
            keyword =& kwd::VAL_NULL;

        if (keyword != NULL && match(in, *keyword))
        {
            if (       ch == kwd::VAL_TRUE) {
                exception::warning(in, "JSON value 'true' is not supported"
                                       " and will be treated as int 1");
                builder.on_int(int64_t(1));
            } else if (ch == kwd::VAL_FALSE) {
                exception::warning(in, "JSON value 'false' is not supported"
                                       " and will be treated as int 0");
                builder.on_int(int64_t(0));
            } else if (ch == kwd::VAL_NULL) {
                builder.on_nil();
            } else {
                keyword = NULL;
            }
            in.skip(chars::isspace);
        }

        if (keyword == NULL)
            return exception::expect(in, "KEYWORD", "JSON value");

        return true;
    }

    template<typename InType> inline bool parse_string(InType & in)
    {
        typedef typename InType::CharType   CharType;
        typedef KeywordTable<CharType> kwd;

        /* " */
        if (! match(in, kwd::STR_BEG))
            return exception::expect(in, kwd::STR_BEG, "JSON string");

        Builder<CharType> & builder = in.get();
        builder.str_beg();

        bool is_char = true;

        /* [ chars ] */
        do {
            if (in.ch() == kwd::ESCAPE) { /* escape */
                in.skip();
                CharType ch = chr_to_esc(in.ch());
                if (ch != '\0') {
                    builder.on_chr(ch);
                } else if (in.ch() == kwd::HEX) {
                    exception::warning(in, "`\\uXXXX` is not implemented and"
                                           " will be preserved");
                    builder.on_chr(kwd::ESCAPE);
                    builder.on_chr(kwd::HEX);
                    for (size_t i = 0U; i < 4U; i++) {
                        ch = in.skip().ch();
                        if (chars::ishexdigit(ch)) {
                            builder.on_chr(ch);
                        } else {
                            return exception::expect
                            (in, "DIGIT(HEX)", "\\uXXXX");
                        }
                    }
                } else {
                    return exception::expect
                    (in, "ESCAPED CHARACTER", "JSON char");
                }
            } else if (in.ch() == kwd::STR_END) {
                is_char = false;
            } else { /* unescaped */
                CharType ch = in.ch();
                if (chars::iscntrl(ch))
                    return exception::expect(in, "CHAR", "JSON char");
                builder.on_chr(ch);
            }

            in.skip();
        } while(is_char);

        builder.str_end();

        in.skip(chars::isspace);
        return true;
    }

    template<typename InType> inline bool parse_number(InType & in)
    {
        typedef typename InType::CharType CharType;
        typedef KeywordTable<CharType> kwd;
        using namespace ast;

        Builder<CharType> & builder = in.get();

        uint64_t    integral = 0,    integral_length = 0;
        uint64_t  fractional = 0,  fractional_length = 0;
        int      exponential = 0, exponential_length = 0;

        /* - */
        bool is_integral_negtive = false;
        if (in.ch() == kwd::MINUS) {
            is_integral_negtive = true;
            in.skip();
        }

        /* 0 or [1-9]* */
        if (in.ch() == kwd::ZERO) {
            in.skip();
            integral_length ++;
        } else if (chars::isdigit(in.ch())) {
            integral *= uint64_t(10);
            integral += static_cast<int>(in.ch() - kwd::ZERO);
            integral_length ++;
            while (chars::isdigit(in.skip().ch())) {
                integral *= uint64_t(10);
                integral += static_cast<int>(in.ch() - kwd::ZERO);
                integral_length ++;
            }
        } else {
            return exception::expect(in, "DIGIT", "JSON number");
        }

        /* .[0-9]* */
        if (in.ch() == kwd::DOT) {
            while (chars::isdigit(in.skip().ch())) {
                fractional *= uint64_t(10);
                fractional += static_cast<int>(in.ch() - kwd::ZERO);
                fractional_length ++;
            }
        }

        /* e[+-][0-9]* */
        if (in.ch() == kwd::EXP_U || in.ch() == kwd::EXP_L) {
            bool is_exponential_negtive = false;
            in.skip();
            if (in.ch() == kwd::MINUS || in.ch() == kwd::PLUS  ) {
                is_exponential_negtive = (in.ch() == kwd::MINUS);
                in.skip();
            }
            while (chars::isdigit(in.ch())) {
                exponential *= uint64_t(10);
                exponential += static_cast<int>(in.ch() - kwd::ZERO);
                exponential_length ++;
                in.skip();
            }
            if (is_exponential_negtive)
                exponential = -exponential;
            if (exponential_length == 0)
                return exception::expect(in, "DIGIT", "JSON number");
        }

        /* get value */
        if (fractional_length > 0 || exponential_length > 0) {    /* real */
            static const int EXPONENT_MAX_LENGTH = static_cast<int>
                (::log10(std::numeric_limits<double>::max_exponent10)) + 1;

            if (fractional_length + integral_length >=
                std::numeric_limits<double>::digits10 + 5) {
                exception::warning(in, "`double` precision may be lost");
            }
            if (exponential_length > EXPONENT_MAX_LENGTH ||
                exponential <= std::numeric_limits<double>::min_exponent10 ||
                exponential >= std::numeric_limits<double>::max_exponent10) {
                exception::warning(in, "too big for `double` type");
            }

            double var = 0;
            if (integral != 0)
                var = static_cast<double>(integral);
            if (fractional_length != 0)
                var += static_cast<double>(fractional)
                     / ::pow(10, fractional_length);
            if (exponential_length != 0)
                var *= ::pow(10.0, exponential);
            if (is_integral_negtive)
                var = -var;
            builder.on_dbl(var);

        } else {    /* integer */
            static const int64_t MAX = std::numeric_limits<int64_t>::max();
            static const int64_t MIN = std::numeric_limits<int64_t>::min();

            if (integral_length > std::numeric_limits<uint64_t>::digits10) {
                exception::warning(in, "too big for `uint64`");
                if (is_integral_negtive)
                    builder.on_int(MIN);
                else
                    builder.on_int(MAX);
            } else if (integral & uint64_t(0x8000000000000000)) {
                if (is_integral_negtive)
                    builder.on_int(MIN);
                else
                    builder.on_int(MAX);
            } else {
                if (is_integral_negtive)
                    builder.on_int(-static_cast<int64_t>(integral));
                else
                    builder.on_int(static_cast<int64_t>(integral));
            }
        }

        in.skip(chars::isspace);
        return true;
    }

    /* actually (comment*) */
    template<typename InType> bool skip_comments(InType & in)
    {
        typedef KeywordTable<typename InType::CharType> kwd;
        if (in.ch() != kwd::COMMENT_FIRST)
            return true;

        if (in.get_settings().enable_json_comment == false)
            return exception::opt_error(in, "ENABLE_JSON_COMMENT", "FALSE");

        size_t pos = 0;
        while (pos != in.pos()) {
            pos = in.pos();

            /* try skip '/ * ... * /' */
            if (!skip_block(in, kwd::C_COMMENT_1_BEG, kwd::C_COMMENT_1_END))
                return exception::expect(in,kwd::C_COMMENT_1_END, "Comment");

            /* try skip '/ / ...' */
            skip_block(in, kwd::C_COMMENT_2_BEG, kwd::C_COMMENT_2_END);

            /* try skip 'SPACE' */
            in.skip(chars::isspace);
        }
        return true;
    }
}}

/****************************************************************************
 * [extern]parse
 ***************************************************************************/

namespace parser { namespace json
{
    extern bool parse(
        Stream         & stream,
        Tree<char>     & tree,
        Message        & message,
        Settings const & settings)
    {
        Builder<char> builder(tree);
        In in(stream, settings, builder);

        bool status = false;
        try {
            status = parse_value(in);
        } catch (exception::ParseError const & e) {
            message.push_back(e.what(), chars::strlen(e.what()));
        }
        return status;
    }
}}

CV_FS_PRIVATE_END
