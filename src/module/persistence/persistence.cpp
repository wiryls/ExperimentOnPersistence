/****************************************************************************
 *  license
 ***************************************************************************/

#include <iostream> //  TODO: remove It

#include "persistence_private.hpp"
#include "persistence_utility.hpp"
#include "persistence_io.hpp"
#include "persistence_ast.hpp"
#include "persistence_parser.hpp"
#include "persistence_emitter.hpp"
#include "persistence_string.hpp"
#include "persistence.hpp"

CV_FS_PRIVATE_BEGIN

namespace exception
{
    /***********************************************************************
     * exception
     ***********************************************************************/

    using chars::Soss;
    using chars::fmt;

    inline static void failed_to_parse(
        const char * filename, const char * msg, POS_TYPE_)
    {
        error(0,
            ( Soss<char, 512>()
                * "failed to parse file `"
                | fmt<64>(filename)
                | "`, hint: "
                | fmt<256>(msg)
                | '.'
            ), POS_ARGS_
        );
    }
    inline static void failed_to_open(const char * filename, POS_TYPE_)
    {
        error(0,
            ( Soss<char, 256>()
                * "failed to open file `"
                | fmt<64>(filename)
                | '`'
            ), POS_ARGS_
        );
    }
    inline static void failed_to_build_stream(POS_TYPE_) /* internal error */
    {
        error(0, "internal error - failed to build stream", POS_ARGS_);
    }
    inline static void invalid_format(int format, POS_TYPE_)
    {
        error(0,
            ( Soss<char, 256>()
                * "unable to determine file format, or"
                | " format `"
                | fmt<16>(format)
                | "` is invalid"
            ), POS_ARGS_
        );
    }
    inline static void invalid_mode(int mode, POS_TYPE_)
    {
        error(0,
            ( Soss<char, 256>()
                * "mode `"
                | fmt<16>(mode)
                | "` is invalid"
            ), POS_ARGS_
        );
    }
    inline static void invalid_filestorage(POS_TYPE_) /* internal error */
    {
        error(0, "FileStorage failed to initialize", POS_ARGS_);
    }
    inline static void index_out_of_range(size_t index, POS_TYPE_)
    {
        error(0,
            ( Soss<char, 256>()
                * "index `"
                | fmt<32>(index)
                | "` is out of range"
            ), POS_ARGS_
        );
    }
    inline static void invalid_key(const char * key, POS_TYPE_)
    {
        error(0,
            ( Soss<char, 256>()
                * "key `"
                | fmt<128>(key)
                | "` is invalid"
            ), POS_ARGS_
        );
    }
    inline static void invalid_filenode(POS_TYPE_) /* internal error */
    {
        error(0, "FileNode is empty or failed to initialize", POS_ARGS_);
    }
    inline static void type_not_match(
        const char * expected, ast::Tag get, POS_TYPE_)
    {
        error(0,
            ( Soss<char, 256>()
                * "expect filenode type `"
                | fmt<64>(expected)
                | "`,  but get `"
                | fmt<64>(ast::to_string(get))
                | "`"
            ), POS_ARGS_
        );
    }
    inline static void type_not_match(
        ast::Tag expected, ast::Tag get, POS_TYPE_)
    {
        error(0,
            ( Soss<char, 256>()
                * "expect filenode type `"
                | fmt<64>(ast::to_string(expected))
                | "`,  but get `"
                | fmt<64>(ast::to_string(get))
                | "`"
            ), POS_ARGS_
        );
    }
}

CV_FS_PRIVATE_END

/****************************************************************************
 * Declaration
 ***************************************************************************/

namespace experimental
{
    using namespace CV_FS_PRIVATE_NS;

    typedef parser::Message String;
}

/****************************************************************************
 * FileNode
 ***************************************************************************/

namespace experimental
{
    /************************************************************************
     * FileNode::Impl
     ***********************************************************************/

    class FileNode::Impl
    {
    public:
        typedef ast::Node<char>  value_type;
        typedef value_type       * pointer;
        typedef value_type       & reference;
        typedef value_type const * const_pointer;
        typedef value_type const & const_reference;
        typedef ast::Pool        allocator_t;

    public:
        reference     node_;
        allocator_t & pool_;

    public:
        explicit Impl(reference node, allocator_t & alloc)
            : node_(node)
            , pool_(alloc)
        {}
    };

    /************************************************************************
     * FileNode::constructor\destructor\copier
     ***********************************************************************/

    FileNode::FileNode()
        : impl(NULL)
    {}

    FileNode::~FileNode()
    {
        delete impl;
    }

    FileNode::FileNode(const FileNode & rhs)
        : impl(rhs.impl == NULL ? NULL : new Impl(*rhs.impl))
    {}

    FileNode & FileNode::operator=(const FileNode & rhs)
    {
        if (this != &rhs) {
            delete this->impl;
            this->impl = (rhs.impl == NULL ? NULL : new Impl(*rhs.impl));
        }
        return *this;
    }

    /************************************************************************
     * FileNode::methods
     ***********************************************************************/

    FileNode FileNode::operator[](size_t index) const
    {
        using namespace ast;

        if (empty())
            exception::invalid_filenode(POS_);

        Impl::reference node = impl->node_;
        if (node.type() != SEQ)
            exception::type_not_match(SEQ, node.type(), POS_);

        Impl::pointer child = node.at<SEQ>(static_cast<uint32_t>(index));
        if (child == NULL)
            exception::index_out_of_range(index, POS_);

        FileNode rv;
        rv.impl = new Impl(*child, impl->pool_);
        return rv;
    }

    FileNode FileNode::operator[](const char * key) const
    {
        using namespace ast;

        if (key == NULL)
            exception::null_argument("const char * key", POS_);
        if (empty())
            exception::invalid_filenode(POS_);

        Impl::reference node = impl->node_;
        if (node.type() != MAP)
            exception::type_not_match(MAP, node.type(), POS_);

        size_t len = chars::strlen(key);
        Impl::value_type key_node;
        key_node.construct(impl->pool_);
        key_node.set<STR>(key, key + len, impl->pool_);

        Impl::value_type::Pair * child = node.find<MAP>(key_node);
        if (child == NULL)
            exception::invalid_key(key, POS_);

        FileNode rv;
        rv.impl = new Impl((*child)[1], impl->pool_);
        return rv;
    }

    FileNode::operator int() const
    {
        using namespace ast;

        if (empty())
            exception::invalid_filenode(POS_);

        Impl::reference node = impl->node_;
        if (node.type() != I64)
            exception::type_not_match(I64, node.type(), POS_);

        return static_cast<int>(node.val<I64>());
    }

    FileNode::operator double() const
    {
        using namespace ast;

        if (empty())
            exception::invalid_filenode(POS_);

        Impl::reference node = impl->node_;
        if (node.type() != DBL)
            exception::type_not_match(DBL, node.type(), POS_);

        return node.val<DBL>();
    }

    FileNode::operator const char *() const
    {
        using namespace ast;
        if (empty())
            exception::invalid_filenode(POS_);

        Impl::reference node = impl->node_;
        if (node.type() != STR)
            exception::type_not_match(STR, node.type(), POS_);

        return node.raw<STR>();
    }

    bool FileNode::empty() const
    {
        using namespace ast;
        return impl == NULL
            || impl->node_.type() == NIL
            ;
    }
}

/****************************************************************************
 * FileStorage
 ***************************************************************************/

namespace experimental
{
    /************************************************************************
     * helper
     ***********************************************************************/

    struct filestorage_settings_t
    {
        filestorage_settings_t()
            : filename()
            , data()
            , mode(FileStorage::READ)
            , format(FileStorage::AUTO)
            , enable_memory(false)
            , enable_base64(false)
        {}

        String filename;
        String data;
        int      mode;
        int      format;
        bool     enable_memory;
        bool     enable_base64;
    };

    static inline void
        analyze_query(const char * query, filestorage_settings_t & settings)
    {
        static const size_t PATH_MAX_LENGTH   = 256U; /* ext4 */
        static const char NOT_FILE_NAME     []= "\n\r";
        static const char PARAM_EQUAL         = '=';
        static const char PARAM_BEGIN         = '?';
        static const char PARAM_SEPARATOR     = '&';
        static const char DOT                 = '.';
        static const char XML_SIGNATURE     []= "<?xml";
        static const char YAML_SIGNATURE    []= "%YAML";
        static const char JSON_SIGNATURE    []= "{";
        static const char XML_SUFFIX        []= ".xml";
        static const char YML_SUFFIX        []= ".yml";
        static const char YAML_SUFFIX       []= ".yaml";
        static const char JSON_SUFFIX       []= ".json";
        static const char OPT_ENABLE_BASE64 []= "base64";

        /* [0]create a copy of `query` */
        String string;
        string.push_back(query, chars::strlen(query));
        string.push_back(0);
        char * buffer = string; /* not safe if string realloced */

        if (settings.mode & FileStorage::MEMORY) {
            settings.mode &= ~int(FileStorage::MEMORY);
            settings.enable_memory = true;
        }

        /* [1]stringstream mode, analyze type of stringstream */
        if (settings.enable_memory                       ||
            string.size() > PATH_MAX_LENGTH              ||
            chars::strpbrk(buffer, NOT_FILE_NAME) != NULL)
        {
            /* string may be too long, we only take first 1024 bytes */
            char flag = 0;
            if (string.size() > 1024U) {
                flag = buffer[1024U];
                buffer[1024U] = 0;
            }

            if (settings.format == FileStorage::AUTO) {
                if      (chars::strstr(buffer, XML_SIGNATURE ) != NULL)
                    settings.format = FileStorage::XML;
                else if (chars::strstr(buffer, YAML_SIGNATURE) != NULL)
                    settings.format = FileStorage::YML;
                else if (chars::strstr(buffer, JSON_SIGNATURE) != NULL)
                    settings.format = FileStorage::JSON;
            }

            if (flag)
                buffer[1024] = flag;

            settings.enable_memory = true;
            settings.data.push_back(buffer, chars::strlen(buffer));
            settings.data.push_back(0);
            return;
        }

        /* [2]filestream mode */
        char * end = buffer + chars::strlen(buffer);
        char * beg = chars::strchr(buffer, PARAM_BEGIN);
        if (beg == NULL) beg   = end;
        else            *beg++ = '\0';

        /* convert "aa.bb?cc&dd=2&&ee" to {"cc", "dd""2", "ee"} */
        if (beg != end) {
            for (char * cur = beg, * nxt = beg; cur < end; cur = nxt) {
                nxt = chars::strchr(cur, PARAM_SEPARATOR);
                if (nxt == NULL) nxt   = end;
                else            *nxt++ = '\0';

                if (cur + 1 < nxt) {
                    char * key = cur;
                    char * val = chars::strchr(cur, PARAM_EQUAL);
                    if (val == NULL) val   = end;
                    else            *val++ = '\0';

                    if (!*val && !chars::strcmp(key, OPT_ENABLE_BASE64)) {
                        settings.enable_base64 = true;
                    } /* else if (key == "...") { } */ else {
                        ;// TODO: warning
                    }
                }
            }
        }

        settings.filename.push_back(buffer, chars::strlen(buffer));
        settings.filename.push_back(0);

        /* analyze type of filestream */
        if (settings.format == FileStorage::AUTO) {
            char * suffix = chars::strrchr(buffer, DOT);
            if (suffix == NULL)
                return;

            if (       chars::strcmp(suffix,  YML_SUFFIX) == 0 ||
                       chars::strcmp(suffix, YAML_SUFFIX) == 0) {
                settings.format = FileStorage::YAML;
            } else if (chars::strcmp(suffix,  XML_SUFFIX) == 0) {
                settings.format = FileStorage::XML;
            } else if (chars::strcmp(suffix, JSON_SUFFIX) == 0) {
                settings.format = FileStorage::JSON;
            }
        }
    }

    /************************************************************************
     * FileStorage::Impl
     ***********************************************************************/

    class FileStorage::Impl
    {
    public:
        Impl() : ast_(), fsm_() {}

        ast::Tree<char>    ast_;
        emitter::Handler * fsm_; // unique_ptr
    };

    /************************************************************************
     * FileStorage::constructor\destructor
     ***********************************************************************/

    FileStorage::FileStorage()
        : impl(new Impl())
    {}

    FileStorage::FileStorage(const char * filename, int mode, int format)
        : impl(new Impl())
    {
        open(filename, mode, format);
    }

    FileStorage::~FileStorage()
    {
        release();
        /* ^^^^ should not throw exception */
        delete impl;
    }

    /************************************************************************
     * FileStorage::methods
     ***********************************************************************/

    bool FileStorage::open(const char * query, int mode, int format)
    {
        if (query == NULL)
            exception::null_argument("const char * filename", POS_);

        if (isOpen())
            release();

        /* [0] analyze settings */
        filestorage_settings_t settings;
        {
            settings.format = format;
            settings.mode   = mode;

            analyze_query(query, settings);

            /* assert `settings.format` is valid */
            switch (settings.format)
            {
            case XML: case YAML: case JSON: break;
            default : exception::invalid_format(settings.format, POS_);break;
            }
        }
        /* args {`format`, `mode`, `query`} should NOT be used below.
         * use `settings` instead.
         */

        /* [1] create stream */
        io::Stream * stream = NULL;
        const char * data     = NULL;
        {
            stream = io::Stream::build(
                ( settings.enable_memory )
                ? io::STRING
                : io::FILE
            );
            if (stream == NULL)
                exception::failed_to_build_stream(POS_);

            /* map `settings.mode` to `stream_mode` */
            io::Mode stream_mode = io::READ;
            switch (settings.mode)
            {
            case READ  : { stream_mode = io::READ;   break; }
            case WRITE : { stream_mode = io::WRITE;  break; }
            case APPEND: { stream_mode = io::APPEND; break; }
            default: { exception::invalid_mode(settings.mode, POS_); break; }
            }

            data =
                ( settings.enable_memory
                ? settings.data
                : settings.filename
                );

            if (stream->open(data, stream_mode) == false)
                exception::failed_to_open(data, POS_);
        }

        /* [2] R or W */
        if (settings.mode == io::READ)
        {
            ast::Tree<char> & tree =  impl->ast_;
            parser::ParseFuncion parse = NULL;
            switch (settings.format)
            {
            //case XML : { parse = parser:: xml::parse; break; }
            //case YAML: { parse = parser::yaml::parse; break; }
            case JSON: { parse = parser::json::parse; break; }
            default: exception::invalid_format(settings.format, POS_); break;
            }

            parser::Message message;
            bool status
                = parse != NULL
                ? parse(*stream, tree, message, parser::Settings())
                : false
                ;

            if (status == false) {
                exception::failed_to_parse(data, message, POS_);
            } else if (!message.empty()) {
                // TODO: warnings
                ;
            }
            delete stream;
        }
        else if (settings.mode == io::WRITE)
        {
            emitter::Handler * & fsm =  impl->fsm_;
            fsm = new emitter::JsonFSM(stream);
        }

        return isOpen();
    }

    bool FileStorage::isOpen() const
    {
        if (impl == NULL)
            exception::invalid_filestorage(POS_);

        return impl != NULL
            &&  (  impl->ast_.empty() == false
                || impl->fsm_ != NULL
                )
            ;
    }

    void FileStorage::release()
    {
        if (impl->ast_.empty() == false) {
            //impl->ast_.pool().allocator<char>().report();
            //impl->ast_.pool().allocator<ast::Node<char>>().report();
            impl->ast_.clear();
            //impl->ast_.pool().allocator<char>().report();
            //impl->ast_.pool().allocator<ast::Node<char>>().report();
        }
        if (impl->fsm_ != NULL) {
            delete (impl->fsm_);
            impl->fsm_ = NULL;
        }
    }

    FileNode FileStorage::root(int /* streamidx*/ ) const
    {
        if (impl == NULL)
            exception::invalid_filestorage(POS_);

        FileNode rv;
        rv.impl = new FileNode::Impl(impl->ast_.root(), impl->ast_.pool());
        return rv;
    }

    static inline void tab(size_t level)
    {
        for (size_t i = level << 1; i-- > 0; std::cout << ' ');
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

    static inline void visit(ast::Node<char> const &node, size_t level, size_t t)
    {
        using namespace ast;
        tab(t);
        switch (node.type())
        {
        case NIL: { std::cout << "<NULL>"; break; }
        case I64: { std::cout << node.val<I64>(); break; }
        case DBL: { std::cout << node.val<DBL>(); break; }
        case STR:
        {
            std::cout << '"';
            typedef char const * const_iter;
            const_iter iter_beg = node.begin<STR>();
            const_iter iter_end = node.  end<STR>();
            for (const_iter iter = iter_beg; iter != iter_end; ++iter) {
                const char * cvt = esc_to_chr(*iter);
                if (cvt == NULL)
                    std::cout << *iter;
                else
                    std::cout << cvt;
            }
            std::cout << '"';
            break;
        }
        case SEQ:
        {
            std::cout << '[';
            typedef ast::Node<char> const * const_iter;
            const_iter iter_beg = node.begin<SEQ>();
            const_iter iter_end = node.  end<SEQ>();
            for (const_iter iter = iter_beg; iter != iter_end; ++iter) {
                std::cout << (iter == iter_beg ? "\n" : ",\n");
                visit(*iter, level + 1, level + 1);
            }
            std::cout << '\n';
            tab(level);
            std::cout << ']';
            break;
        }
        case MAP:
        {
            std::cout << '{';
            typedef ast::Node<char>::Pair const * const_iter;
            const_iter iter_beg = node.begin<MAP>();
            const_iter iter_end = node.  end<MAP>();

            for (const_iter iter = iter_beg; iter != iter_end; ++iter) {
                std::cout << (iter == iter_beg ? "\n" : ",\n");
                visit((*iter)[0], level + 1, level + 1);
                std::cout << ": ";
                visit((*iter)[1], level + 1, 0);
            }
            std::cout << '\n';
            tab(level);
            std::cout << '}';
            break;
        }
        default: { std::cout << "UNEXPECTED NODE!"; return; }
        }
    }

    void FileStorage::test_dump() const
    {
        if (impl == NULL)
            return;

        ast::Node<char> & node = impl->ast_.root();
        visit(node, 0, 0);
        std::cout << '\n';
    }

    void FileStorage::write(int val)
    {
        if (impl == NULL || impl->fsm_ == NULL)
            exception::invalid_filestorage(POS_);


        emitter::Event<emitter::OUT_INT> event; {
            event.val = val;
        }
        *(impl->fsm_) << event;
    }

    void FileStorage::write(double val)
    {
        if (impl == NULL || impl->fsm_ == NULL)
            exception::invalid_filestorage(POS_);

        emitter::Event<emitter::OUT_DBL> event; {
            event.val = val;
        }
        *(impl->fsm_) << event;
    }

    void FileStorage::write(const char * val, size_t len)
    {
        if (impl == NULL || impl->fsm_ == NULL)
            exception::invalid_filestorage(POS_);
        if (val == NULL)
            return;
        if (len == 0)
            len = chars::strlen(val);

        if (*val == '[' && len == 1)
        {
            *(impl->fsm_) << emitter::Event<emitter::BEG_SEQ>();
        }
        else if (*val == '{' && len == 1)
        {
            *(impl->fsm_) << emitter::Event<emitter::BEG_MAP>();
        }
        else if (*val == ']' && len == 1)
        {
            *(impl->fsm_) << emitter::Event<emitter::END_SEQ>();
        }
        else if (*val == '}' && len == 1)
        {
            *(impl->fsm_) << emitter::Event<emitter::END_MAP>();
        }
        else
        {
            emitter::Event<emitter::OUT_STR> event; {
                event.val = val;
                event.len = len;
            }
            *(impl->fsm_) << event;
        }
    }
}
