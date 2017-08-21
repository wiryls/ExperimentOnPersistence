Introduction
===
This page is about a new design of the **node** in an abstract syntax tree of some **data exchange format**.

The following will be covered:
- The node in details.
- Associated parts.
- Performance.

Related code: [persistence](src/module/persistence).

[Node](src/module/persistence/persistence_ast_node.hpp#L91-L92)
===

The node is a basic and important part of an abstract syntax tree. And here, this new design only aims at some data interchange format.

Features
---

The new design:

 - keep always 16 bytes in x86 and x64;
 - is trivial and standard layout (plain old data);
 - support type: null, int64, double, (w)string, sequence, and map;
 - have small string optimization;
 - set growth factor of built-in containers to the golden ratio;
 - require memory pool.

Design
---

### Overview

An overview of underlying union `Node`.

**Source Code Snippet** (from [persistence_ast_node.hpp](src/module/persistence/persistence_ast_node.hpp#L91-L467))

```C++
template<typename CharType> union Node
{
public:
    typedef Node   Pair[2];
    /* ... some typedef ... */

public:
    template<typename PoolType> void construct(Tag tag, Pool & pool);
    /* ... some methods and no ctors, dtor ... */

private:
    typename Traits<Node, NIL>::Layout nil;
    typename Traits<Node, I64>::Layout i64;
    typename Traits<Node, DBL>::Layout dbl;
    typename Traits<Node, STR>::Layout str;
    typename Traits<Node, SEQ>::Layout seq;
    typename Traits<Node, MAP>::Layout map;
};
```

The idea is simple:

 - **Lower memory cost**:
    - Use top level `union` and,
    - Compress `capacity` to keep `sizeof(Node<CharType>) == 16`. (SSE friendly!)
 - **Lower coupling**:
    - Use generic programming to reduce code and bugs.
    - It's easy to add a new built-in type.
 - **Good compatibility**:
    - Keep `Node` trivial and standard layout.
    - Reserve a template parameter `Pool` for a memory pool.

### Memory layout

A common union-style AST node is like:

```C++
struct Node
{
    int flag_;
    union Data {
        double dbl_;
        /* ... */
    } data_;
};
```

Once we choose this solution, in x64, it is hard to keep the size of node smaller than 16 bytes. In `union Data` part, a pointer will be 8 bytes, so containers such as `string` with pointer, size, and capacity will cost more than 16 bytes easily. `sizeof(Node)` may be more than 24 bytes due to memory alignment.

So how to keep `sizeof(Node) == 16`?

After reading relevant materials and learning from others, I come up with an interesting new idea:
 - Use a top level union and,
 - Compress "capacity".

Let's take the sequence type as an example:

**Source Code Snippet** (from [persistence_ast_node.hpp](src/module/persistence/persistence_ast_node.hpp#L958-L969))

```C++
struct Layout
{
    union
    {
        typename container::pointer ptr; /* pointer to raw data */
        uint64_t                    pad;
    }        raw;
    uint32_t siz;                        /* size */
    uint8_t  exp;                        /* index */
    uint8_t  pad[2];
    uint8_t  tag;                        /* node type */
};
```

**Memory Layout Digram**:
```
+---------------------------------------------------------------+
|                            pointer                            |
+---------------------------------------------------------------+
|               size            | index |               |  tag  |
+---------------------------------------------------------------+
```

 - The pointer field is forced to be 8 bytes.
 - The size of a sequence is limited to 4 bytes. It costs 64 GB memory if full. It's enough in most cases.
 - The capacity of a sequence is compressed to be 1 byte. It only stores an index. The actual capacity is the n-th number in the Fibonacci series.
 - The tag is fixed at the last byte of all nodes whatever type. In this way, we can use `nil.tag` to access node tag.
 - We can make sure struct members won't rearrange. As it is standard layout.

### Generic Programming

With the help of traits paradigm, life is easier.

All built-in type is created in the following way:

    1. Add its type name to `enum Tag`.
    2. Create `template<typename CharType> struct Traits<Node<CharType>, NEW_TAG_NAME>`.
    3. Add its memory layout to `union Node`.
    4. Add new methods to `Node` if necessary. 

Mostly, methods of `Node` take the advantage of SFINAE. For example:
```C++
template<Tag TAG> inline
typename Traits<Node, TAG>::size_type
capacity() const;
```

We must firstly define `size_type` in `Traits` to call method `capacity<TAG>()` with a specific TAG. Or we will get a compile-time error.

Details
---

### Small String Optimization

As small strings appear in data exchange format quite often. It is worth to add small string optimization. Also because the size of a node is fixed to 16 bytes, it is easier to optimize.

**Source Code Snippet** (from [persistence_ast_node.hpp](src/module/persistence/persistence_ast_node.hpp#L710-L737))

```C++
union Layout
{
    /* small string */
    union
    {
        CharType raw[14 / sizeof(CharType)];
        struct
        {
            uint8_t pad[14];
            uint8_t siz;
            uint8_t tag;
        } ext;
    } sht;
    /* normal(long) string */
    struct
    {
        union
        {
            CharType * ptr;
            uint64_t   pad;
        }        raw;
        uint32_t siz;
        uint8_t  exp;
        uint8_t  pad[2];
        uint8_t  tag;
    } lng;
};
```

**Memory Layout Digram**:

```
+---------------------------------------------------------------+
|                        CharType array                         |
+---------------------------------------------------------------+
|                                               | size  |  type |
+---------------------------------------------------------------+
```
```
+---------------------------------------------------------------+
|                            pointer                            |
+---------------------------------------------------------------+
|               size            | index |               |  type |
+---------------------------------------------------------------+
```

As mentioned above, `uint8_t tag` is fixed at the last byte of a node. The rest part is used to store necessary data of a string. If it is `Node<char>` ( or`Node<char16_t>`), all string less than 13 (or 6) characters will be stored as a small string. If not, `str.sht.ext.siz` will be set to 0xFF and it means NOT a small string.

### Growth Factor of Built-in Containers

In this design, the capacity of a built-in container is limited to some fixed number. And only the index of a number in series is stored. So it is necessary to pre-compute the series and provide some methods to look up.

In my initial design, the growth factor of a container such as string was 2. I found it wastes about 0.25n memory if there are n nodes, compared to some normal designs with a growth factor 1.5.

Finally, I choose the Fibonacci series because of the ideal growth factor, golden ratio. The growth factor 1.618 keeps a balance of memory cost and time cost. And also it is realloc-friendly.

[Three functions](src/module/persistence/persistence_fibonacci.hpp#L159-L202) (and [three template structs](src/module/persistence/persistence_fibonacci.hpp#L204-L232)) are provided for calculation:

 - `IndexType left (ValueType y)`, get the index of the left Fibonacci number of y.
 - `IndexType right(ValueType y)`, get the index of the right Fibonacci number of y.
 - `ValueType at   (IndexType x)`, get the x-th Fibonacci number.

We can customize it to control the growth factor of built-in containers.

### Memory Pool Requirement

Most methods of `Node` have a template parameter `Pool` for a memory pool. It is not a good idea but still needed. With this template parameter, we can wrap up something like `CvMemStorage` to maintain compatibility.

```C++
Pool       pool;
Node<char> node;
node.construct(pool);
```

The `pool` must support allocate and deallocate `CharType` and `Node<CharType>` type.

There is a simple and specially [customized memory pool](src/module/persistence/persistence_pool.hpp#L126) in my source code for test. The memory pool is optimized according to the Fibonacci series and based on `std::allocator`.

About the simple and [customized memory pool](src/module/persistence/persistence_pool.hpp#L126), a picture is worth words:

```
      +-----------+  +-----------------+  +-----------------+
 +----------+     |  |                 |  |                 |
 |Chunk List|   +-v--+-+----------+  +-v--+-+----------+  +-v----+----------+
 +----------+   | head |          |  | head |          |  | head |          |
                +------+          |  +------+          |  +------+          |
                |                 |  |                 |  |                 |
                |                 |  |                 |  |                 |
                |                 |  |                 |  |                 |
                |                 |  |                 |  |                 |
                |   size = 6765   |  |   size = 10946  |  |   size = 6765   |
                |                 |  |                 |  |                 |
                |                 |  |                 |  |                 |
                |  +--+     +--+  |  |                 |  |  +--+           |
                |  |04+-+   |15+---------+             |  |  |16|           |
                |  +^-+ |   +^-+  |  |   |             |  |  +^-+           |
                |   |   |    |    |  |   |             |  |   |             |
                +-----------------+  |   |             |  +-----------------+
                    |   |    |       |  +v-+     +--+  |      |
                    |   |    |       |  |15|     |04|  |      |
                    |   |    |       |  +--+     +^-+  |      |
                    |   |    |       |            |    |      |
                    |   |    |       |            |    |      |
                    |   |    |       |  +--+      |    |      |
                    |   +--------------->04+------+    |      |
                    |        |       |  +--+           |      |
                    |        |       |                 |      |
                    |        |       +-----------------+      |
                    |        |                                |
                    +------+ +---------+  +-------------------+
                           |           |  |
                +----------+-----------+--+------------------------------+
      +--------->01|02|03|04| ... |14|15|16|17|18|19|20| ... |44|45|46|47|
+---------+     +--------------------------------------------------------+
|Free List|
+---------+
```

Usage
---

Here a simple example shows what `Node` looks like.
```C++
Pool       pool;
Node<char> node;
node.construct(pool);
const char str[] = "string";
{   /* move back a pair */
    Node<char>::Pair pair;
    pair[0].construct(pool);
    pair[1].construct(pool);
    pair[0].set<DBL>(1.0, pool);
    pair[1].set<STR>(str, str + sizeof(str), pool);
    node.move_back<MAP>(pair, pool);
}
{   /* search it */
    Node<char> key, val;
    key.construct(pool);
    val.construct(pool);
    key.set<DBL>(1.0, pool);
    val.set<STR>(str, str + sizeof(str), pool);
    assert((*node.find<MAP>(key))[1].equal(val), true);
    key.destruct(pool);
    val.destruct(pool);
}
{	/* pop back */
    node.pop_back<MAP>(pool);
}
node.destruct(pool);
```

Obviously, the interface looks unfriendly. Because it is designed to be low-level and plain old data, there are no constructors, destructor, and operators. Because it needs compatibility for some stateful memory pool, there is always a template parameter for the pool. They make the interface quite unfriendly. Besides, if we only use some stateful allocators, we can remove the template parameter `Pool`.

Generally speaking, we may need a high-level interface to wrap it up. For example:

```C++
class FileNode
{
public:
    /* ...               */
    /* constructor       */
    /* destructor        */
    /* operator overload */
    /* other methods     */
    /* ...               */

private:
    Node<char> * node_;
    Pool       * pool_;
};
```

And then the interface will be friendly via function overloading.

Comparison
---

**Original**:

```C++
typedef struct CvFileNode
{
    int tag;
    struct CvTypeInfo* info;
    union
    {
        double f;
        int i;
        CvString str;
        CvSeq* seq;
        CvFileNodeHash* map;
    } data;
}
CvFileNode;
```

**New**:

```C++
template<typename CharType> union Node
{
    /* ...... */

private:
    typename Traits<Node, NIL>::Layout nil;
    typename Traits<Node, I64>::Layout i64;
    typename Traits<Node, DBL>::Layout dbl;
    typename Traits<Node, STR>::Layout str;
    typename Traits<Node, SEQ>::Layout seq;
    typename Traits<Node, MAP>::Layout map;
};
```

**The differences**:

 - Remove member `struct CvTypeInfo* info`. Member `info` is used in RTTI but seems not used in most nodes. To minimize the size of nodes, I removed it. And an additional node is needed to record the type.
 - `Ordered Map` instead of `Hash Map`. As we all know, the main advantage of `ordered map` is it preserves order. So we can open a file and do some insertion or deletion but the order will not change.
 - Minimal pointer members. For example, it is `struct ...::Layout seq` instead of `Seq * seq` in the union. Memory allocation is reduced here.
 - Top level union. It shows above. Size of a node is always 16 bytes.

Performance
===

To roughly test its improvement, I choose a quite big JSON file to parse.

Environment
---
 - **OS**: Windows 10 (64-bit)
 - **IDE**: Visual Studio Community 2017
 - **CPU**: Intel Core i7-7700HQ
 - **SSD**: SAMSUNG MZVPW256HEGL

Test
---

### Test Code Snippet

From [test_io.cpp](src/module/unittest/test_io.cpp#L26-L27).

```C++
{
    FileStorage fs("citylots.json", FileStorage::READ);
    fs.release();	
}
```

Note:
 - The test file is `citylots.json` (189.9 MB, contains tens of millions of nodes). Please see references[3].

### Result

|  Node Type   |      Memory Pool       |  Condition  | Time(s) | Memory(MB) |
| :----------: | :--------------------: | :---------: | ------: | ---------: |
| `Node<char>` | Customized Memory Pool |  Debug x86  |      90 |        431 |
| `Node<char>` |    `std::allocator`    |  Debug x86  |      92 |        525 |
| `CvFileNode` |     `CvMemStorage`     |  Debug x86  |      45 |       1017 |
| `Node<char>` | Customized Memory Pool |  Debug x64  |      85 |        431 |
| `Node<char>` |    `std::allocator`    |  Debug x64  |      86 |        586 |
| `Node<char>` | Customized Memory Pool | Release x86 |    4.72 |        372 |
| `Node<char>` |    `std::allocator`    | Release x86 |    5.76 |        363 |
| `CvFileNode` |     `CvMemStorage`     | Release x86 |    6.48 |       1011 |
| `Node<char>` | Customized Memory Pool | Release x64 |    4.05 |        372 |
| `Node<char>` |    `std::allocator`    | Release x64 |    4.96 |        393 |

Note:
 - Time measure and memory footprint measure are provided by Visual Studio 2017.
 - Each test ran for three times and took its mean.
 - `null` has been replaced with 0.
 - Customized Memory Pool is mentioned above and based on `std::allocator`.
 - About `CvFileNode`, OpenCV version is 3.3.

Reference
===

1. Modern C++ Design
2. [Optimal memory reallocation and the golden ratio](https://crntaylor.wordpress.com/2011/07/15/optimal-memory-reallocation-and-the-golden-ratio/)
3. [City Lots San Francisco in .json](https://github.com/zeMirco/sf-city-lots-json)
4. [RapidJSON Documentation](http://rapidjson.org/)
