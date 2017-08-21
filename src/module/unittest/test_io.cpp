/****************************************************************************
 *  license
 ***************************************************************************/

#include <gtest/gtest.h>
#include "../persistence/persistence.hpp"

TEST(io, input)
{
    using namespace experimental;

    FileStorage fs("test.json", FileStorage::READ);
    FileNode root = fs.root();
    
    {
        ;
    }

    fs.release();
}

TEST(io, input_bigfile)
{
    using namespace experimental;

    //FileStorage fs("citylots.json", FileStorage::READ);
    //fs.release();
}

TEST(io, output)
{
    using namespace experimental;

    FileStorage fs("sample.json", FileStorage::WRITE);
    {
        fs << "[";
        {
            fs << "{" << "empty" << "[" << "]" << "}";
            fs << "[" << "[" << "]" << "{" << "}" << "]";
            fs << "[";
            fs << 1 << 2 << 2.33;
            fs << "{" << "excited!" << 1.2 << "}";
            fs << "]";
        }
        fs << "]";
    }
    fs.release();
}

TEST(io, memory)
{
    using namespace experimental;

    FileStorage fs
    (
        "{\"12345678901234\":1}",
        FileStorage::READ | FileStorage::MEMORY,
        FileStorage::AUTO
    );
    FileNode root = fs.root();

    EXPECT_EQ((int)root["12345678901234"], 1);
    fs.release();
}
