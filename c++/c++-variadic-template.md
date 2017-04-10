#变参模板

感觉写起来很像递归，先取变参表中的头几个类型做操作，调用一个以后续变参为参数的模板，直到 base case。

与 C 提供的 `va_xxx` 宏相比，宏有运行时开销，调用时会对栈做操作。并且 `va_xxx` 系列
取参数都靠强制转换，无法保证类型安全，出问题几乎都是运行时的问题，不好调试。

变参模板相当于一种模式匹配，而且更能保证类型安全，没有运行时开销。


##求加法
```c++
template<typename T>
T adder(T v) {
  return v;
}

template<typename T, typename... Args>
T adder(T first, Args... args) {
  return first + adder(args...);
}
```


##tuple 数据结构

整体来看就是一个从空的基类开始的继承链，每个派生类都定义一个新的成员，代表 tuple 中的元素，支持各种不同类型混合在一个 tuple 中：

```c++
// base case，空 struct
template <class... Ts> struct tuple {};

// general case，继承自 用后续参数构造的 tuple
template <class T, class... Ts>
struct tuple<T, Ts...> : tuple<Ts...> {
  // 构造函数，基类 tuple<Ts...> 部分用 ts... 初始化，tail 用 t 初始化
  tuple(T t, Ts... ts) : tuple<Ts...>(ts...), tail(t) {}

  // 每层只定义一个成员类型
  T tail;
};
```

tuple 的 `get` 操作：

```c++
// 表示第 k 个成员类型的辅助模板
// base case，只定义当前变参表第一个元素的类型
template <class T, class... Ts>
struct elem_type_holder<0, tuple<T, Ts...>> {
  typedef T type;
};
// general case
template <size_t k, class T, class... Ts>
struct elem_type_holder<k, tuple<T, Ts...>> {
  typedef typename elem_type_holder<k - 1, tuple<Ts...>>::type type;
};

// get 操作
template <size_t k, class... Ts>
typename std::enable_if<
  k == 0, typename elem_type_holder<0, tuple<Ts...>>::type&>::type
get(tuple<Ts...>& t) {
  return t.tail;
}

template <size_t k, class T, class... Ts>
typename std::enable_if<
  k != 0, typename elem_type_holder<k, tuple<T, Ts...>>::type&>::type
get(tuple<T, Ts...>& t) {
  tuple<Ts...>& base = t;
  return get<k - 1>(base);
}
```

注意上面 `get` 操作用到了 `std::enable_if`，因为这里与 `elem_type_holder` 不同，`elem_type_holder`
中 base case 里的 `k` 不是模板形参，而 `get` 中的 `k` 是模板形参，如果不用 `std::enable_if`，无法
区分 `k != 0` 和 `k == 0` 两种模板。所以要用 `std::enable_if` 来判断模板参数 `k` 的取值，然后决定
`get` 的返回值类型。

这里返回的是引用，所以可以 `get` 出来后对它赋值。

```c++
tuple<double, uint64_t, const char*> t1(12.2, 42, "big");

std::cout << "0th elem is " << get<0>(t1) << "\n";
std::cout << "1th elem is " << get<1>(t1) << "\n";
std::cout << "2th elem is " << get<2>(t1) << "\n";

get<1>(t1) = 103;
std::cout << "1th elem is " << get<1>(t1) << "\n";
```

##通用函数
比如遍历并打印任意 STL 容器元素的函数：

```c++
template <template <typename, typename...> class ContainerType,
          typename ValueType, typename... Args>
void print_container(const ContainerType<ValueType, Args...>& c) {
  for (const auto& v : c) {
    std::cout << v << ' ';
  }
  std::cout << '\n';
}
```

这里需要变参模板是因为各种 STL 容器类型需要的模板参数 `ContainerType<...>` 不一定数量一致。


##与 `std::forward` 联用
变参模板一般和 `std::forward` 联用，forward 参数给某个构造函数。这种情况的例子如 `std::make_unique()` 和 `std::vector::emplace_back()`。


#Parameter Pack
打包一堆的模板参数，可以包含类型、非类型和其他模板。

```c++
template <typename... Args>
void pprint(const char *fmt)
{
    cout << fmt;
}

template <typename T, typename... Args>
void pprint(const char *fmt, T&& first, Args&& ... rest)
{
    while (*fmt != '~')
        cout << *fmt++;
    if (*(fmt + 1) == '~')
        cout << *fmt++;
    else
        cout << first;
    pprint<Args...>(fmt + 1, rest...);
}
```

注意几点：

* 类模板中，pack 只能是最后一个模板参数
* 函数模板中，pack 后面可以有其他模板参数，但必须要能根据模板实参推断出来，或者有默认参数，如 `template<typename ...Ts, typename U, typename=void> void valid(Ts..., U);`
* pack 展开时，`...` 之前的内容（the largest expression）充当一个 pattern，表示按照这种格式来展开其中的 pack name。如果多个一个 pattern 中有多个 pack name，长度必须相同
* pack 嵌套时，从内到外展开

```c++
template<typename...> struct Tuple {};
template<typename T1, typename T2> struct Pair {};

template<class ...Args1> struct zip {
    template<class ...Args2> struct with {
        typedef Tuple<Pair<Args1, Args2>...> type;
        //        Pair<Args1, Args2>... is the pack expansion
        //        Pair<Args1, Args2> is the pattern，按这种格式来展开两个 pack，即将里面的依次组合起来
    };
};
```

总结一下，`...` 有两个意思：
* 这部分表示 pack 展开的 pattern
* 展开后包含任意个符合此 pattern 的参数

比如：

```c++
template<typename ...Ts, int... N> void g(Ts (&...arr)[N]) {}
int n[1];
g<const char, int>("a", n); // Ts (&...arr)[N] expands to 
                            // const char (&)[2], int(&)[1]
```

这里函数模板 `g` 的形参为 `Ts (&...arr)[N]`，和判断类型的方法一样，可见 `...` 前面的类型是“含有 `N` 个 `Ts` 类型元素的数组的引用”，
这里 `N` 和 `Ts` 都是 pack，所以每个这样的数组引用的长度和元素类型都可以不同。而且这个 `g` 函数会接受任意个这样的引用，
所以下面 `g` 的调用是合法的，类型 pack `Ts` 包含 `const char` 和 `int`，长度 pack `N` 包含 2 和 1。


#Tips
* `std::cout << __PRETTY_FUNCTION__ << std::endl` 打印当前函数的签名和传入的参数类型
* `sizeof...` 操作符可返回 parameter pack 的长度

#参考
1. [http://eli.thegreenplace.net/2014/variadic-templates-in-c/](http://eli.thegreenplace.net/2014/variadic-templates-in-c/)
2. [http://en.cppreference.com/w/cpp/language/parameter_pack](http://en.cppreference.com/w/cpp/language/parameter_pack)
