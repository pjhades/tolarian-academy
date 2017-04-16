## 1
重点是模板参数推断和 `auto` 的推断规则，尤其是对于 universal reference。

```c++
template<typename T>
void f(ParamType param); // ParamType 形参

f(expr); // expr 实参
```

模板参数推断分为三种情况

* 形参是指针或引用，但不是 universal reference（类型推断中的右值引用）
  1. 若实参是引用，先忽略 `&`
  2. 然后对比实参和形参类型，推断出 `T`

* 形参是 universal reference
  * 若实参是左值，`T` 和 `ParamType` 都是左值引用
  * 应用 reference collapsing 规则推断 `T`（相当于按前一种情况处理）

* 形参不是指针或引用（传值调用）
  * 会发生参数的复制，产生新对象
  * 实参上的 `const` 和 `volatile` 会被忽略，因为那些都是实参的性质

另外对于数组和函数参数而言
* 传值时，数组参数退化为指向首元素的指针
* 传引用时，实参类型推断为数组的引用，类似 `const char (&)[10]`，所以函数内修改对外部可见
* 传值时，函数参数退化为指向函数的指针
* 传引用时，实参类型推断为函数的引用

利用传引用，可以实现编译时获取数组参数，并作为常量表达式：

```c++
template<typename T, std::size_t N>
constexpr std::size_t arraySize(T (&)[N]) noexcept
{
    return N;
}
```

此处声明为 `constexpr` 告诉编译器可以把这个函数调用作为常量表达式使用，如

```c++
int keyVals[] = { 1, 3, 7, 9, 11, 22, 35 };
int mappedVals[arraySize(keyVals)];
```

## 2

* `auto` 大部分时候与模板参数推断规则一致，但 `auto` 会把 `{...}` 花括号初始值推断为 `std::initializer_list`
  * `auto x = {1, 2, 3}` 实际包含两次推断，一次推断 `auto` 为 `std::initializer_list`，一次推断 `std::initializer_list` 的实参类型

* `auto` 用在函数返回值和 `lambda` 参数时相当于模板参数推断，故不会把 `{...}` 推断为 `std::initializer_list`

## 3

`decltype` 一般都会返回某个名字或者变量声明的类型：

```c++
template<typename Container, typename Index>
auto authAndAccess(Container& c, Index i)
  -> decltype(c[i]) // C++11 trailing return type
{
  authenticateUser();
  return c[i];
}
```

这里用了 C++11 trailing return type，返回值的 `auto` 就是为了说明我要用这个语法了，可以在声明返回值类型时
使用参数。但不能放在最前面，因为那里参数定义还没出现。

C++14 中，在一些使用 `auto` 的地方要用 `decltype(auto)` 来保证 1. 模板参数推断；2. 拿到原表达式的类型。
这样可以避免模板推断导致类型不一致，如：

```c++
template<typename Container, typename Index>
decltype(auto) authAndAccess(Container& c, Index i)
{
  authenticateUser();
  return c[i];
}
```

以上如果不加 `decltype`，则 `auto` 对`c[i]`（一般 `[]` 操作符返回左值）进行模板参数推断，由于这里不是 universal reference，
所以会丢掉 `c[i]` 的引用，因此得到的是右值，无法对此函数的调用结果进行赋值。

`decltype` 对“复杂的”左值表达式返回左值，这个非常暧昧：

```c++
decltype(auto) f1() {
  int x = 0;
  return x;  // 返回 int 类型
}

decltype(auto) f2() {
  int x = 0;
  return (x); // 返回 int& 类型
}
```


## 4

人造编译错误，让编译器告诉你类型：

```c++
const int theAnswer = 42;
auto x = theAnswer;
auto y = &theAnswer;
template<typename T>
class TD; // incomplete class
TD<decltype(x)> xType; // int
TD<decltype(y)> yType; // const int *
```

用 `typeid()` 运行时打印出类型信息：

```c++
// clang++ -std=c++14 test.cpp -o test && ./test | c++filt
int main()
{
    const int a = 42;
    auto x = a;
    auto y = &a;

    cout << typeid(x).name() << endl; // prints i, meaning Int
    cout << typeid(y).name() << endl; // prints PKi, meaning Pointer to Konst Int

    return 0;
}
```

`typeid()` 打印的信息取决于编译器。GNU 和 clang 可以用 `c++filt` 命令解码出原始类型。

但 `typeid()` 打印的信息可能不准确。比如标准规定 `typeid(xxx).name()` 会把传入的类型当作传给了传值调用的模板函数，
也就是说，打印出的内容会丢掉 `const` 和 `&`。

用 Boost TypeIndex 库可以得到正确的类型名。



## 5
给了几种具体的情况，表明 `auto` 优于显式类型声明：

* vs `std::function`。`auto` 定义出来的类型和闭包类型一致，但 `std::function` 是一个模板，且可能消耗更多的内存（可能在堆上）。

* vs type shortcuts。type shortcuts 即程序员简写了某种类型，比如 `std::vector<int>::size_type` 写为 `unsigned int`，可能
导致不同平台上因长度不一致出现 bug。使用 `auto` 既能减轻输入负担，也能保证各个平台上都是用了正确的类型。

* vs 一些细微的 bug：比如用 ranged-for 遍历 `std::unordered_map`，但写成 `for (const pair<string, int>& p : m) { ... }`，
因为 `unordered_map` 的 key 是 `const` 的，所以这里会创建一个临时对象并把循环变量 `p` 绑定上去，增加不必要的负担，而且万一
循环里面要取 pair 的地址保存到指针，这样写取到的是临时对象的地址。用 `auto` 即可避免这一情况。


另外一个好处是增加可读性。作者的观点是，有时候我们看代码只需要直到一个名字是什么就够了，并不需要知道具体的类型，
`auto` 带来的简洁再加上漂亮的变量名完全可以弥补“不知道确切类型”这一点。而显式类型声明在之前几种情况下可能造成更大的麻烦，
为了一点“准确性”不值得花这种代价。


## 6
某些地方会出现 proxy object，比如 `vector<bool>` 元素是按位保存，取元素引用会返回一个内置的 `reference` 类型的对象来模拟引用
，此时用 `auto` 会导致推断出的类型是那个 proxy object，在后面的代码中使用可能出问题。此问题关键不在 `auto` 而在 proxy object，
所以要对自己用的库、接口都比较熟悉。

如果同时又想得到 Item 5 中提到的 `auto` 的各种好处，可以用 `auto` 时加上 `static_cast` 将类型强制指定为你需要的。


主要讲新特性的一些使用方面的细节。

## 7
C++11 以前，有这么几个问题

* `Widget w1; Widget w2 = w1; w1 = w2;` 三条语句中，第三条才是调用 `=` 操作，第二条是调用 copy ctor，这个容易搞混
* 不论用哪种初始化语法，都无法实现任意的初始化，比如构造一个含有元素 1 2 3 的 `vector`

所以新版引入了使用花括号的 uniform initialization，因为这个语法在所有要做初始化的地方都可以用。
（`=` 可以用在 class 成员初始化，但 `()` 不行。`()` 可以初始化不可 copy 的对象，但 `=` 不行）

uniform initialization 的好处：
* 避免 narrowing conversion，花括号里的表达式类型如果无法用被初始化对象的类型表示，无法通过编译，比如 `double x, y; int s{x + y};`
* 不受 most vexing parse 影响。C++ 会把所有可能的语句都 parse 为声明，所以 `Widget w(1);` 调用构造函数，但 `Widget w();` 则会被当作声明

uniform initialization 的问题（跟函数重载、对象构造纠缠不休）：
* call hijack。如果重载了以 `std::initializer_list` 为参数的函数，则以 `{}` 调用的函数优先匹配 `std::initializer_list` 版本。更诡异的是，如果
定义了 convert operator，可能先隐式转换，再调用 `std::initializer_list` 版本。哪怕调用 `std::initializer_list` 版本可能造成 narrowing conversion
报编译错误，也会优先调用。除非无法从转换实参类型转换到 `std::initializer_list` 元素类型，才会尝试其他重载版本。

* 无参数 vs 空 `std::initializer_list`：`Widget w{};` 会调用默认 ctor，而 `Widget w({});` 或 `Widget w{{}};` 会调用空 `std::initializer_list` 版本的 ctor。

* 用 `()` 和 `{}` 可能实现不同的构造：如 `vector<int> v(10, 20);` vs `vector<int> v{10, 20};`，前者含 10 个元素 20，后者含 2 个元素 10 和 20。


相关 tips：
* 实现重载和构造函数的时候要注意，尽量让对象构造不受使用 `()` 或 `{}` 的影响（我觉得 `vector` 那个是 feature，`{}` 跟数组初始化一样），
注意不要让新加的 `std::initializer_list` 版本 hijack 了以前的调用

* 作为用户，调用的时候注意是否存在 `()` 和 `{}` 的区别。

## 8
0 和 `NULL` 会和整型冲突，在重载和模板参数推断时出问题，传 0 和 `NULL` 可能调不到指针版本。

所以，请用 `nullptr`，尽量不要同时编写整型和指针参数的重载版本。

## 9
这个主要还是跟模板相关。

Alias declaration 可以用在模板里面，实现 alias template，但 `typedef` 不行，需要用一个 `struct` 成员来实现：

```c++
template<typename T>
using MyAllocList = std::list<T, MyAlloc<T>>;
MyAllocList<Widget> lw;

template<typename T>
struct MyAllocList {
    typedef std::list<T, MyAlloc<T>> type;
};
MyAllocList<Widget>::type lw;
```

用 `struct` 成员来表示类型有缺点。比如在模板中使用：

```c++
template<typename T>
class Widget {
private:
    typename MyAllocList<T>::type list;
    //...
};
```

此时需要标注 `typename`，因为 `MyAllocList<T>::type` 编译器会认为是 dependent type，即这个东西
到底是不是类型，取决于 `T`。可能存在某个 `MyAllocList` 的特化版本中 `type` 其实是一个变量，
所以需要 `typename` 告诉编译器这是类型。

同理，`type_traits` 提供的模板，如 `std::remove_const<T>::type` 用于模板时也要加 `typename`。

alias 不受此影响。C++14 中加入了各种 type traits 对应的 alias 版本，如`std::remove_const_t<T>`。


## 10
C++98 的 `enum` 称 unscoped，缺点如下：
* 其中定义的名字会存在于 `enum` 所在的作用于下导致命名空间污染
* C++11 以前不支持前向声明
* 存在隐式转换，无法保证类型安全

C++11 提供 enum class，即 scoped，解决上述问题。

作者推荐的将 enum class 枚举值转换为整型的函数模板：

```c++
template<typename E>
constexpr typename std::underlying_type<E>::type
toUType(E enumerator) noexcept
{
    // 将枚举值 enumerator 显式转换为枚举类型 E 的底层类型，用 type traits 取得之
    return static_cast<typename std::underlying_type<E>::type>(enumerator);
}
```

另外要注意

* C++11 unscoped enum 可以前向声明，但必须注明底层类型 `enum Color: std::uint8_t;`
* 编译器对 unscoped enum 的底层类型选择会考虑性能优化，所以未指定时不知道其确切类型
* scoped enum 的默认底层类型为 `int`


## 11
C++98 禁用某个函数的方法一般是将其声明为 `private` 且不实现。缺点是：

* 仅对 class 成员有效
* `private` 函数本 class 和友元仍可以调用，而且因为没有实现，错误要等到链接时才出现
* 无法只禁止某个成员函数模板的某个特化版本

C++11 用 `= delete` 来禁用函数。除了禁用成员，还可以：

禁用某些重载版本：

```c++
bool isLucky(int number);
bool isLucky(char) = delete;
bool isLucky(bool) = delete;
bool isLucky(double) = delete;
```

禁用函数模板的特化：

```c++
template<typename T>
void processPointer(T* ptr);

template<>
void processPointer<void>(void*) = delete;
template<>
void processPointer<char>(char*) = delete;
```

禁用类成员函数模板的特化：

```c++
class Widget {
public:
    //...
    template<typename T>
    void processPointer(T* ptr) { /*...*/ }
    //...
};

template<>
void Widget::processPointer<void>(void*) = delete;
```

## 12

虚函数覆盖必须满足函数名、参数类型、`const`、reference qualifier  相同，且返回值类型和异常声明必须兼容。
如果覆盖不成功，编译器可能不会报任何错，导致运行时 bug。

用 `override` 标记相当于告诉编译器：我要覆盖基类方法。如果覆盖不成功，编译器会报错。便于抓出一些细小的问题，
而且万一不小心修改了基类虚函数的原型，也可以根据报错信息查找并修改子类的函数。

Reference qualifier 类似于成员函数的 `const` 修饰符，`const` 会区分调用函数的对象是否 `const`，reference qualifier
可以区分调用对象是左值还是右值。在一些场合可以根据左值右值来实现不同的行为，比如对右值对象 move 而左值对象复制。

## 13
C++98 中容器的 `const_iterator` 无法直接取得，而且存在类型转换问题，比如 `insert` 不接受 `const_iterator`，
又无法把非 const 的迭代器转换为 const，不好用。

C++11 可通过 `cbegin`，`cend` 等容器成员取得 `const_iterator`，也提供了全局的 `std::begin` 等函数，如 `std::vector` 有：

```c++
terator begin();
const_iterator begin() const;
const_iterator cbegin() const;
```

C++14 增加了更多的全局 `begin`、`end` 函数，以及针对数组的特化版本，可取得指向其元素的指针：

```c++
template< class T, std::size_t N > 
constexpr T* begin( T (&array)[N] );

template< class C > 
constexpr auto cbegin( const C& c ) -> decltype(std::begin(c));
```

## 14
* 将函数声明为 `noexcept` 告诉编译器此函数不抛出异常。编译器对此可以进行一些优化。

* 一些库函数，或容器操作，对 `noexcept` 的函数会做优化，比如 `vector` 在成员类型具有 `noexcept` 的 move ctor
时，会把复制操作替换为 move。再如对 `pair` 的 `swap` 操作，它抛不抛异常取决于对具体 `pair` 成员的 `swap`
操作是否抛异常。

* 有些时候声明为 `noexcept` 需要做大量的“吞异常”工作，即处理此函数调用的其他函数可能抛出的异常，引入额外的消耗，
使 `noexcept` 带来的优化效果打折。

* 两种函数的概念
  * wide contract，表示此函数的调用不需要假设任何先决条件
  * narrow contract，与上相反，调用者负责保证先决条件得到满足，比如参数字符串长度不超过 100。
    但 narrow contract 函数一般可能需要通过异常来通知调用者先决条件没有满足，所以 `noexcept` 与此
    矛盾，库的作者如果要区分这两种函数，一般只对 wide contract 函数保留 `noexcept`

## 15
对于对象而言，`constexpr` 表明对象是 `const`，而且它的值在编译时即可得到：

```c++
int x;
constexpr auto y = x; // x 编译时未知
std::array<int, x> z; // 同上

constexpr auto a = 10;
std::array<int, a> b; // OK
```

对于函数而言，`constexpr` 的作用是：

* 如果调用时传入的参数都是编译时常量，则该函数调用也可以作为编译时常量使用

* 否则该函数和普通函数一样。如果在这种情况下，函数被用在需要编译时常量的上下文中，则无法通过编译

C++11 的 `constexpr` 函数只能包含 `return` 语句，类成员函数默认为 `const`。C++14 放宽了此限制。


## 16
`const` 成员函数虽说是“只读”，但可能修改 `mutable` 成员，所以要保证线程安全。

## 17
编译器在缺少各种 ctor、dtor 且被使用时会生成默认版本：
* ctor：生成 ctor 仅当 class 没有定义 ctor
* dtor：默认 dtor `noexcept`
* copy ctor：默认版本逐个 copy 非 static 成员，仅当 class 没有定义 copy ctor 时生成，定义了 move 时删除
* copy assignment：类似 copy ctor。两个 copy 操作是独立的，定义了一个无法阻止编译器生成另一个（deprecated，不要依赖这种行为）
* move ctor 和 move assignment：默认版本逐个 move 非 static 成员，仅当 class 没有定义 copy、move 和 dtor 时生成

用 `=default` 显式地让编译器生成默认版本的好处是，避免在默认情况下因为代码修改，增删了 ctor 之类的特殊函数而导致另一些特殊函数被隐式地删除，
比如，若 class 里增加了 move，那么原来默认生成的 copy 就被隐式地标为 `=delete` 了，影响后面使用这个 class 的代码。
当然用 `=default` 的前提是它行为是对的。

总之最好的办法就是尽量不要依靠默认行为，一般需要这些东西都是要管理资源，管理资源你还敢用默认行为啊？！



## 18
`unique_ptr` 用于表达 exclusive ownership，只能 move 不能 copy。

默认删除方式是 `delete`，但也可以在构造智能指针时指定一个自定义的删除函数。此时用 lambda 比用函数指针构造出的 `unique_ptr` 要省内存。
一般函数指针比 lambda 多 1-2 个 word，用函数对象的话，大小取决于对象中保存的其他状态。

`unique_ptr` 可以转换为 `shared_ptr`。

## 19
`shared_ptr` 用于表达 shared ownership，相当于提供了 GC 式的自动内存管理，而且内存释放时间还是确定的（最后一个指针不再指向对象、被 `delete` 等）。
但会带来一些开销：

* `shared_ptr` 通常是普通指针的两倍：一个指针指向动态分配的对象，另一个指向 control block（包含引用计数）
* 引用计数需要用原子操作维护

与 `unique_ptr` 的区别在于：
* `shared_ptr` 也可以指定自定义的 deleter（和 allocator），但这些东西不是 `shared_ptr` 类型的一部分。而 `unique_ptr` 如果对象类型相同但 deleter 不同，
得到的 `unique_ptr` 类型就不同
* `unique_ptr` 可以转换为 `shared_ptr` 但反过来不行
* `shared_ptr` 不能从普通数组对象构造

Control block 的创建的规则：
* `std::make_shared` 总会创建
* 从 `unique_ptr` 或 `auto_ptr` 等等遵循 unique-ownership 的指针构造时创建
* 从普通指针构造时创建

因为以上第三点存在，使用 `shared_ptr` 时要注意尽量不要用普通指针来构造，否则多个 control block 意味着多份引用计数和多次析构，引发未定义行为。

如果存在从 `this` 指针构造 `shared_ptr` 的情形，需要让 class 继承自 `std::enable_shared_from_this<Klass>`。这个类模板中定义了一个
`shared_from_this()` 成员函数，用于从 `this` 创建 `shared_ptr`，使用现有的 control block 而非新建。但调用时必须保证已经从这个类构造
过 `shared_ptr`，否则会引发未定义行为。所以通常在这种情况下，会将类的构造函数设置为 `private`，并定义一些 factory function 来获得
这个类的对象。

## 20
`weak_ptr` “像 `shared_ptr`”一样可以访问 shared ownership，但不参与到被指对象的引用计数管理中，
所以被指对象可能已经被释放了，`weak_ptr` 可以判断这种情况。

* 不能解引用，要先用 `wptr.lock()` 或 `shared_ptr` 构造函数转换为 `shared_ptr` 再访问被指对象
* 不能和 `nullptr` 比较，要调用 `wptr.expire()` 判断此指针是否已经 dangle

## 21
用 `make_unique`，`make_shared`，`allocate_shared` 的好处是

* 避免用构造函数时重复输入模板参数类型，如 `std::shared_ptr<Widget> spw2(new Widget)`

* 避免指针构造中的异常不安全情形，如 `processWidget(std::shared_ptr<Widget>(new Widget), foo())`，如果 `foo` 在 `Widget` 和 `shared_ptr` 的
构造中间执行，且出现异常，则由于 `Widget` 已被 `new` 出来但并未交给智能指针管理，`Widget` 的内存会泄漏。用 `make_shared` 可以将两个操作合并在
一起保证不被打断

* 减少内存分配次数。对 `make_shared` 而言，会一次分配出保存对象的内存和保存 control block 的内存，而调用构造函数会分配两次：先 `new` 再构造

不适合调用 `make_xxx` 的情形：

* 要使用自定义的 deleter

* 要使用 initializer list（`make_xxx` 调用小括号版本的构造函数，如 `auto upv = std::make_unique<std::vector<int>>(10, 20)` 之后 `vector` 包含
10 个 20）

* 对 `make_shared` 而言，它会一次性分配对象和 control block 的内存。而 `weak_ptr` 的 dangle 判断需要检查它对应的 `shared_ptr` 的 control block，
所以当有 `weak_ptr` 引用一个 control block 的时候，即时引用计数减为 0，control block 的内存也不能释放。在此情况下，如果被管理的对象很大，则它
虽然已被析构，但其占用的内存仍无法释放

## 22
pimpl idiom 主要解决编译时间问题。

当 class 中有其他 class 的对象作为成员时，可能需要 `#include` 一堆头文件，
这会产生编译依赖，如果一个头文件改了，`#include` 它的代码也要重新编译。

pimpl idiom 的做法是将依赖的部分全部移到 `.cpp` 文件中，而包含 class 定义的头文件只保留
一个内部的 `struct` 和指向它的指针：

```cpp
class Widget {
public:
    Widget();
    // ...
private:
    std::string name;
    std::vector<double> data;
    Gadget g1, g2, g3;
};

class Widget {
public:
    Widget();
    ~Widget();
    // ...
private:
    struct Impl;
    Impl *pImpl;
};
```

此时为了实现 `Impl` 对象的自动析构，一般把普通指针改为 `unique_ptr`。
但要注意，此处 `Impl` 只声明未定义，需要在对应的 `.cpp` 文件中，在 `Impl` 的实现
之后显式地定义 `Widget` 的析构函数，否则编译器自动生成的析构函数（对 `unique_ptr` 管理的对象指针使用 `delete`）
会因找不到 `Impl` 的定义（imcomplete type）而报错。

`unique_ptr` 和 `shared_ptr` 的区别：

* 对 `unique_ptr` 而言，deleter 是其类型的一部分，编译器生成的特殊函数在调用时，必须保证被指对象是 complete type
* 对 `shared_ptr` 而言，deleter 不是类型的一部分，没有以上要求

## 23

之前已经有笔记记录了。

补充一点：

```c++
class Annotation {
public:
    explicit Annotation(const std::string text): value(std::move(text)) 
    {
        // ...
    }
    // ...
private:
    std::string value;
};
```

这个里面，`Annotation` 有一个 `string` 成员，构造的时候声明为了 `const`，看起来很自然。
但是如果想把 `text` `move` 给成员，则实际调用的是 `string` 的 copy ctor。因为这里 `text`
声明为了 `const`，这个 `const` 属性会一直保持，于是会调用 `const string &` 版本的 ctor，
因为从一个对象 `move` 出来相当于对对象的修改，不符合 `const`。

这种情况下要注意，参数不要声明为 `const`。

## 24
`T&&` 不一定都是右值引用，在有参数推断的时候可能是 universal reference，要注意区分。

## 25
对最后一次使用的右值引用用 `move`，对 universal reference 用 `forward`。但要注意 return value optimization
的情形，如果某个函数局部变量和返回值类型相同，且该局部变量被返回，则满足优化的条件，此时不用 `move`，现代
编译器都会自动在分配给返回值的内存里构造局部对象。即使满足条件但编译器无法做此优化，C++ 标准也规定编译器必须
把返回值当作右值处理，所以相当于有一个隐式的 `move`。自己加一个 `move` 上去只能增加额外的 `move` 到返回值内存的操作。

## 26
参见关于 SNIFAE 的笔记。universal reference 函数模板往往能够覆盖很多调用场景，所以很可能会覆盖掉非模板的版本，
特别是当非模板版本和实参类型不匹配，需要类型提升时，编译器会认为模板才是更好的选择。

## 27
主要就是讲怎么解决 Item 26 里面说到的，universal reference 和重载同时出现时，调用某个函数容易出现被调用
的版本不符合预期。

一种处理方法是使用所谓的 tag dispatch，使用 `std::true_type` 和 `std::false_type`
作为 tag 来决定使用哪个版本的模板。这两个东西实际是 `std::integral_constant`，是其他 `is_xxx` type traits 的基类。

```c++
template<typename T>
void logAndAdd(T&& name)
{
    logAndAddImpl(
        std::forward<T>(name),
        std::is_integral<typename std::remove_reference<T>::type>() // 去掉 T 的引用判断其是否为整型
    );
}

template<typename T>
// 只在第二个参数为 false 时才调用
void logAndAddImpl(T&& name, std::false_type)
{
    auto now = std::chrono::system_clock::now();
    log(now, "logAndAdd");
    names.emplace(std::forward<T>(name));
}
```

有几个 type traits 模板值得注意

* `std::enable_if` 见另一篇专门记录它的笔记
* `std::is_integral` 判断某个类型是否是整型（包括 `bool`）
* `std::decay` 去掉类型上的 `constant` 和 `volatile` 修饰（`std::remove_cv`），或者对数组类型退化为第一维的指针，函数类型退化为 `std::add_pointer<F>::type`
* `std::is_base_of<A, B>` 判断 `B` 是否是 `A` 的派生类
* `std::is_constructible<T, Args...>` 判断 `T` 类型对象是否能用 `Args...` 构造出来

## 28
炒冷饭。略。

## 29
就是说，不要总认为 move 比 copy 来得划算，有些时候表面上看是 move，实际会调用 copy，
比如没有定义 move，或者要不抛异常才能 move，但实际 move 没有定义为 `noexcept`。

## 30
注意有些时候 forward 会出问题

* bitfield
* initializer list
* 重载函数
* 没有定义的整型 static 成员（编译器会做 const propagation，只要没有取地址就行，但 forward 会跟引用打交道，
  所以会出链接问题）

#Item 31: Avoid default capture modes
lambda capture by-ref 可能导致 dangling reference。by-value 也可能导致 dangling：

```c++
class Example {
    private:
        int base_value;

    public:
        Example(int base_value_=0): base_value(base_value_) { cout << "ctor" << endl; }
        ~Example() { cout << "dtor" << endl; }

        auto get_adder(int v) -> function<int(int)> {
            return [=](int x) {
                return base_value + v + x;
            };
        }
};

auto get_adder_from_example() -> function<int(int)>{
    Example e(10);
    return e.get_adder(15);
}

int main()
{
    auto f = get_adder_from_example();
    cout << f(1) << endl;
    return 0;
}
```

此处 `get_adder_from_example` 中的对象 `e` 已经析构，`main` 中调用闭包试图通过被捕获的 `this` 访问 class 成员 `base_value` 会 dangle。

## 32
C++14 开始支持让 lambda capture 支持 move，称为 init capture：

```c++
auto func = [pw = std::move(pw)] {
    return pw->isValidated() && pw->isArchived();
};
```

C++11 的一种朴素的模拟：

```c++
class IsValAndArch {
public:
    using DataType = std::unique_ptr<Widget>;

    explicit IsValAndArch(DataType&& ptr)
    : pw(std::move(ptr)) {}

    bool operator()() const
    {
        return pw->isValidated() && pw->isArchived();
    }
private:
    DataType pw;
};

auto func = IsValAndArch(std::make_unique<Widget>());
```

也可以用 `std::bind` 模拟：

```c++
auto func =
    std::bind(
        [](const std::vector<double>& data)
        { /* uses of data */ },
    std::move(data)
);
```

默认 lambda 创建出来是 closure class 的 `operator ()` 是 `const` 的，
所以为了模拟 init capture，这里用了 `const` 引用来防止 lambda body 中修改了 `data`。

## 33

C++14 支持 generic lambda，lambda 的参数支持泛型：

```c++
auto f = [](auto x){ return func(normalize(x)); };
```

如果要保持 `auto` 参数的值特性，由于此时不是模板，没有模板参数可以在 `forward` 中使用，要配合 `decltype`：

```c++
auto f =
     [](auto&& param)
     {
return func(normalize(std::forward<decltype(param)>(param)));
};
```

## 34
尽量用 lambda 而不是 `std::bind`，因为

* lambda 比 `std::bind` 可读性更好
* lambda 在适当的时候可以对 body 里面的调用做内联， 但传给 `std::bind` 的通常是函数指针，无法内联
* 参数在传给 `std::bind` 时求值，而非真正要调用的函数处，在某些时候可能有问题，比如设置 timer 的时间
* `std::bind` 总会复制它的参数，bind object 调用时参数 pass by ref，而 lambda 可以在 capture 部分显式地指明

## 35
一般情况下尽量用 `std::async(f)` 得到 future 来异步执行，而不是用 `std::thread`，因为

* 代码更清晰
* 不用纠结线程管理的细节问题，比如 oversubscription（线程太多切换频繁带来额外开销，cache 不友好），load balancing 等等

## 36
`std::async` 默认的 policy 是 `std::launch::async | std::launch::deferred`，特别是在线程 oversubscription 的时候
无法保证一定在新线程中执行，可能同步，可能根本不执行，所以会导致不好调试的 bug，最好是显式地指定 policy。

或者写一个默认异步执行的版本：

```c++
template<typename F, typename... Ts>
inline
std::future<typename std::result_of<F(Ts...)>::type>
reallyAsync(F&& f, Ts&&... params)
{
    return std::async(std::launch::async,
                      std::forward<F>(f),
                      std::forward<Ts>(params)...);
}
```
C++14 返回值可以改为 `auto`。

## 37
实现支持 RAII 的 `std::thread`，在析构函数中测试 `t.joinable()` 并根据需要决定 join 还是 detach：

```c++
class ThreadRAII {
  public:
    enum class DtorAction { join, detach };
    ThreadRAII(std::thread&& t, DtorAction a) 
      : action(a), t(std::move(t)) {}
    ~ThreadRAII()
    {
      if (t.joinable()) {
        if (action == DtorAction::join) {
          t.join();
        } else {
          t.detach();
        }
      }
    }
    std::thread& get() { return t; }
  private:
    DtorAction action;
    std::thread t;
};
```

## 38
分清三个东西

* `std::future` 通过它来访问异步执行对象
* `std::promise` 保存异步执行对象的结果或异常，可通过 promise 获得一个 future
* `std::packaged_task` 封装一个 callable 对象，使它可以异步调用

每个 `std::promise` 都和一个 shared state 绑定。future 析构的时候，若 shared state 是通过
`std::async` 创建出来的且没有 ready，而这个 future 又是最后一个引用该 shared state 的，
析构操作会 block 直到 task 结束。

## 39
首先注意，`std::conditional_variable` 的 `wait` 方法可以按 `cv.wait(lock, func)` 形式来调用，
`lock` 是保护变量的锁，`func` 返回 `bool`，用于判断当前线程的唤醒是否是 spurious wakeup。

One-shot 条件变量（只等待-唤醒一次）可以用 promise 和 future 来实现：

```c++
std::promise<void> p; // 此处用 void promise 因为不关心具体值，值关心 set_value 这一行为
void react(); // 等条件变量的线程要执行的函数
void detect()
{
  std::thread t([] {
                p.get_future().wait();
                react();
              });
  // ...
  p.set_value();
  // ...
  t.join();
}
```

用 shared future 同时唤醒多个等待线程：

```c++
std::promise<void> p;
void detect()
{
  auto sf = p.get_future().share();
  std::vector<std::thread> vt;
  for (int i = 0; i < threadsToRun; ++i) {
    vt.emplace_back([sf] {  // 此处每个线程要 copy 一份自己的 shared future 来共享 state（参见 cppreference）
                      sf.wait();
                      react();
                    });
  }
  // ...
  p.set_value();
  // ...
  for (auto& t : vt) {
    t.join();
  }
}
```

## 40
`std::atomic` 保证原子性但不能保证 volatile 内存的正确访问。`volatile` 相反，不能保证原子性。

## 41
move 操作快的时候，可以考虑用 pass-by-value 来简化一些重载或者模板的实现。
但这样做可能有其他问题，比如 slicing problem（把派生类对象 by-value 传给基类形参，只能访问基类部分）

## 42
容器的 emplace 操作往往比对应的 insert 操作要高效，因为省去了不必要的临时对象构造析构。
但有两种情况要注意：

* 容器里装的是资源管理类，比如 `std::shared_ptr`。emplace 由于用到了 forward，所以不能像 `std::make_shared`
一样接收参数之后就立即构造出智能指针，从而保证即时出现异常，指针也能正确析构。emplace 进去之后如果容器操作（比如内存分配）
出现异常，由于智能指针构造并未结束，所以无法保证资源的释放，而外面又没有资源的 handle 所以会出现泄漏

* emplace 传进去的是构造函数的参数，而 insert 类型的操作传的是对象。前者算显式构造，后者算复制，
所以如果容器里面装的是比如 `std::regex`，emplace 一个 `nullptr` 也能通过编译，但运行时因为 regex 
必须要接收正常的正则表达式，所以会产生未定义行为。这里要区别的是 direct initialization `A a(x);`
和 copy initialization `A a = x;`，emplace 算直接初始化，不报错，但实际上是有问题的
