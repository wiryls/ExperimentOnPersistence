/****************************************************************************
 *  license
 ***************************************************************************/

#ifndef __PERSISTENCE_HPP__
#define __PERSISTENCE_HPP__

namespace experimental
{
    /************************************************************************
     * Declaration
    ************************************************************************/

    class FileNode;
    class FileStorage;

    /************************************************************************
     * FileNode
    ************************************************************************/

    class FileNode
    {
    public:
         FileNode();
        ~FileNode();

        FileNode              (const FileNode & rhs);
        FileNode & operator = (const FileNode & rhs);

    public:
        FileNode operator [] (      size_t index) const;
        FileNode operator [] (const char * key  ) const;

        operator          int() const;
        operator       double() const;
        operator const char *() const;

    public:
        bool empty() const;

    private:
        friend class FileStorage;

    private:
        class Impl;
        Impl* impl; // std::unique_ptr<Impl> impl;
    };

    /************************************************************************
     * FileStorage
    ************************************************************************/

    class FileStorage
    {
    public:
        //enum Mode
        //{
        //    READ        = 0000,
        //    WRITE       = 0001,
        //    APPEND      = 0002,
        //    MEMORY      = 0004,

        //    FORMAT_XML  = 0010,
        //    FORMAT_YML  = 0020,
        //    FORMAT_JSON = 0030,
        //    FORMAT_YAML = FORMAT_YML,
        //};

        enum Mode
        {
            READ   = 0,
            WRITE  = 1,
            APPEND = 2,
            MEMORY = 4
        };

        enum Format
        {
            AUTO = 0,
            XML  = 1,
            YAML = 2,
            JSON = 3,
            YML  = YAML
        };

    public:
         FileStorage();
         FileStorage(const char * filename, int mode, int format = AUTO);
         // FileStorage(const char * filename, int flags);
        ~FileStorage();

    public:
        // bool open(const char * filename, int flags);
        bool open(const char * filename, int mode, int format = AUTO);
        bool isOpen() const;
        void release();
        // TODO: string releaseAndGetString();

        FileNode root(int streamidx = 0) const;

        void test_dump() const;

    public:
        void write(int val);
        void write(double val);
        void write(const char * val, size_t len = 0);

    private:
        FileStorage              (const FileStorage & rhs) /* = delete */;
        FileStorage & operator = (const FileStorage & rhs) /* = delete */;

    private:
        class Impl;
        Impl* impl;
    };

    template<typename T> static inline
    FileStorage & operator << (FileStorage& fs, T const & value)
    {
        if (fs.isOpen() == false)
            return fs;
        fs.write(value);
        return fs;
    }

}

#endif
