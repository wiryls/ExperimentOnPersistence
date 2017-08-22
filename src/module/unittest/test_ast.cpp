/****************************************************************************
 *  license
 ***************************************************************************/

#include <type_traits>
#include <gtest/gtest.h>
#include "../persistence/persistence_ast.hpp"

TEST(ast, basic)
{
    using namespace CV_FS_PRIVATE_NS::ast;

    EXPECT_EQ(std::is_standard_layout< Node<char    > >::value, true);
    EXPECT_EQ(std::is_trivial< Node<char    > >::value, true);
    EXPECT_EQ(sizeof(Node<char    >), 16);

    union CommonInitialSequence
    {
        Traits<Node<char>, NIL>::Layout nil;
        Traits<Node<char>, I64>::Layout i64;
        Traits<Node<char>, DBL>::Layout dbl;
        Traits<Node<char>, STR>::Layout str;
        Traits<Node<char>, SEQ>::Layout seq;
        Traits<Node<char>, MAP>::Layout map;
    } ls;
    ls.nil.tag = SEQ;
    EXPECT_EQ(ls.nil.tag, SEQ);
    EXPECT_EQ(ls.i64.tag, SEQ);
    EXPECT_EQ(ls.dbl.tag, SEQ);
    EXPECT_EQ(ls.str.tag, SEQ);
    EXPECT_EQ(ls.seq.tag, SEQ);
    EXPECT_EQ(ls.map.tag, SEQ);
}

TEST(ast, number)
{
    using namespace CV_FS_PRIVATE_NS::ast;

    Pool pool;
    Node<char> node;
    node.construct(pool);

    {
        double number = -1e-10;
        node.set<DBL>(number, pool);
        EXPECT_EQ(node.val<DBL>(), number);
    }
    {
        int64_t number = 0x123456789ABCDEF;
        node.set<I64>(number, pool);
        EXPECT_EQ(node.val<I64>(), number);
    }
    {
        double number = 1e100;
        node.set<DBL>(number, pool);
        EXPECT_EQ(node.val<DBL>(), number);
    }

    node.destruct(pool);
}

TEST(ast, string)
{
    using namespace CV_FS_PRIVATE_NS::ast;

    Pool pool;
    Node<char> node;
    node.construct(pool);

    {   /* build short string */
        const char s13[] = "1234567890123";
        node.set<STR>(s13, s13 + 13, pool);
        EXPECT_EQ(::memcmp(node.raw<STR>(), s13, 14), 0);

        /* push back (to normal string) */
        const char s15[] = "123456789012345";
        node.push_back<STR>('4', pool);
        node.push_back<STR>('5', pool);
        EXPECT_EQ(::memcmp(node.raw<STR>(), s15, 16), 0);

        /* erase */
        const char s5[] = "12345";
        node.erase<STR>(node.begin<STR>(), node.begin<STR>() + 10, pool);
        EXPECT_EQ(::memcmp(node.raw<STR>(), s5, 6), 0);

        /* pop back */
        const char s3[] = "123";
        node.pop_back<STR>(pool);
        node.pop_back<STR>(pool);
        EXPECT_EQ(::memcmp(node.raw<STR>(), s3, 4), 0);

        /* erase */
        const char s1[] = "1";
        char * ptr = node.erase<STR>
            (node.begin<STR>() + 1, node.begin<STR>() + 3, pool);
        EXPECT_EQ(::memcmp(node.raw<STR>(), s1, 2), 0);
        EXPECT_EQ(ptr, node.end<STR>());
    }
    {   /* build normal string */
        const char s31[] = "this is not a small test string";
        node.set<STR>(s31, s31 + 31, pool);
        EXPECT_EQ(::memcmp(node.raw<STR>(), s31, 32), 0);
    }

    node.destruct(pool);
}

TEST(ast, seq)
{
    using namespace CV_FS_PRIVATE_NS::ast;

    Pool pool;
    Node<char> tmp;
    Node<char> node;
    node.construct<SEQ>(pool);

    {   /* push back */

        tmp.construct<DBL>(pool);
        tmp.set<DBL>(1.0,  pool);
        node.move_back<SEQ>(tmp, pool);
        EXPECT_EQ(tmp.type(), NIL);
        tmp.destruct(pool);
        /* SEQ --- DBL
         */

        node.push_back<SEQ>(node, pool);
        /* SEQ --+-- DBL
         *       |
         *       +-- SEQ --- DBL
         */

        node.push_back<SEQ>(node, pool);
        /* SEQ --+-- DBL
         *       |
         *       +-- SEQ --- DBL
         *       |
         *       +-- SEQ --+-- DBL
         *                 |
         *                 +-- SEQ --- DBL
         */

        node.push_back<SEQ>(node, pool);
        /* SEQ --+-- DBL
         *       |
         *       +-- SEQ --- DBL
         *       |
         *       +-- SEQ --+-- DBL
         *       |         |
         *       |         +-- SEQ --- DBL
         *       |
         *       +-- SEQ --+-- DBL
         *                 |
         *                 +-- SEQ --- DBL
         *                 |
         *                 +-- SEQ --+-- DBL
         *                           |
         *                           +-- SEQ --- DBL
         */

        EXPECT_EQ(node.at<SEQ>(0)->val<DBL>(), 1.0);
        EXPECT_EQ(node.at<SEQ>(1)->at<SEQ>(0)->val<DBL>(), 1.0);
        EXPECT_EQ(node.at<SEQ>(2)->at<SEQ>(0)->val<DBL>(), 1.0);
        EXPECT_EQ(node.at<SEQ>(2)->at<SEQ>(1)->at<SEQ>(0)->val<DBL>(), 1.0);
        EXPECT_EQ(node.at<SEQ>(3)->at<SEQ>(0)->val<DBL>(), 1.0);
        EXPECT_EQ(node.at<SEQ>(3)->at<SEQ>(1)->at<SEQ>(0)->val<DBL>(), 1.0);
        EXPECT_EQ(node.at<SEQ>(3)->at<SEQ>(2)->at<SEQ>(0)->val<DBL>(), 1.0);
        EXPECT_EQ(node.at<SEQ>(3)->at<SEQ>(2)->at<SEQ>(1)->at<SEQ>(0)
                                 ->val<DBL>(), 1.0);
    }

    {   /* move */
        tmp.construct<SEQ>(pool);
        tmp.move(node, pool);
        EXPECT_EQ(tmp.at<SEQ>(3)->at<SEQ>(2)->at<SEQ>(0)->val<DBL>(), 1.0);

        node.move(tmp, pool);
        tmp.destruct(pool);
        EXPECT_EQ(node.size<SEQ>(), 4);
    }

    {   /* erase */
        node.erase<SEQ>(node.begin<SEQ>(), pool);
        EXPECT_EQ(node.at<SEQ>(2)->at<SEQ>(2)->at<SEQ>(0)->val<DBL>(), 1.0);
        EXPECT_EQ(node.size<SEQ>(), 3);

        node.erase<SEQ>(node.begin<SEQ>() + 1, node.begin<SEQ>() + 3, pool);
        EXPECT_EQ(node.at<SEQ>(0)->at<SEQ>(0)->val<DBL>(), 1.0);
        EXPECT_EQ(node.size<SEQ>(), 1);
    }

    {   /* pop back */
        node.pop_back<SEQ>(pool);
        EXPECT_EQ(node.size<SEQ>(), 0);
    }

    {   /* infinitely recursive */
        tmp.construct(pool);
        node.move_back<SEQ>(tmp, pool);
        tmp.destruct(pool);
        EXPECT_EQ(node.at<SEQ>(0)->type(), NIL);

        Node<char> & child = *node.at<SEQ>(0);

        child.move(node, pool);

        EXPECT_EQ(child.at<SEQ>(0)->type(), SEQ);
        EXPECT_EQ(child.at<SEQ>(0)->at<SEQ>(0)->type(), SEQ);
        EXPECT_EQ(child.at<SEQ>(0)->at<SEQ>(0)->at<SEQ>(0)->type(), SEQ);

        node.move(child, pool);
        EXPECT_EQ(node.at<SEQ>(0)->type(), NIL);
    }

    node.destruct(pool);
}

TEST(ast, map)
{
    using namespace CV_FS_PRIVATE_NS::ast;

    Pool pool;
    const char key[] = "key";

    Node<char> node;
    node.construct<MAP>(pool);

    {   /* move back */
        Node<char>::Pair pair;
        pair[0].construct<STR>(pool);
        pair[0].set<STR>(key, key + sizeof(key), pool);
        pair[1].construct<DBL>(pool);
        pair[1].set<DBL>(1.0, pool);

        node.move_back<MAP>(pair, pool);
        EXPECT_EQ(pair[0].type(), NIL);
        EXPECT_EQ(pair[1].type(), NIL);
        EXPECT_EQ(node.size<MAP>(), 1);
    }

    {   /* push back */
        Node<char>::Pair pair;
        pair[0].construct<DBL>(pool);
        pair[0].set<DBL>(1.0, pool);
        pair[1].construct<STR>(pool);
        pair[1].set<STR>(key, key + sizeof(key), pool);

        node.push_back<MAP>(pair, pool);
        EXPECT_EQ(pair[0].type(), DBL);
        EXPECT_EQ(pair[1].type(), STR);
        EXPECT_EQ(node.size<MAP>(), 2);

        pair[0].destruct(pool);
        pair[1].destruct(pool);
    }
    {
        /* make key */
        Node<char> num;
        num.construct<DBL>(pool);
        num.set<DBL>(1.0, pool);

        Node<char> str;
        str.construct<STR>(pool);
        str.set<STR>(key, key + sizeof(key), pool);

        /* find */
        EXPECT_EQ((*node.find<MAP>(str))[1].equal(num), true);
        EXPECT_EQ((*node.find<MAP>(num))[1].equal(str), true);

        /* erase */
        node.erase<MAP>(node.begin<MAP>() + 1, node.begin<MAP>() + 2, pool);
        EXPECT_EQ((*node.find<MAP>(str))[1].equal(num), true);
        EXPECT_EQ(node.find<MAP>(num), node.end<MAP>());
        EXPECT_EQ(node.size<MAP>(), 1);

        /* pop back */
        node.pop_back<MAP>(pool);
        EXPECT_EQ(node.size<MAP>(), 0);

        num.destruct(pool);
        str.destruct(pool);
    }
}
