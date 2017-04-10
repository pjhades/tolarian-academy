#SFINAE

Substitution Failure Is Not An Error，编译器在解析函数模板的重载时，
如果发现模板参数替换失败，那就不使用当前这个特化版本，而不是报错。

有啥用？`std::enable_if` 就得用它

#`std::enable_if`

考虑 `vector` 的两参数构造函数，可以传元素个数 + 值，也可以传两个迭代器，如果定义为：

```c++
template <typename T>
class vector {
    vector(size_type n, const T val);

    template <class InputIterator>
    vector(InputIterator first, InputIterator last);

    //...
}
```

那么 `vector<int> v(4, 10);` 两个版本都可以调用，无法区分。因为
这里 `InputIterator` 并没有任何语义上的限制，只是一个类型名而已。

而实际的定义会使用 `std::enable_if` 来根据模板参数类型选择合适的版本：

```c++
template <class _InputIterator>
vector(_InputIterator __first,
       typename enable_if<__is_input_iterator<_InputIterator>::value &&
                          !__is_forward_iterator<_InputIterator>::value &&
                          // ... more conditions ...
                          _InputIterator>::type __last);
```

这里根据 `__first` 的类型做判断，只有当它满足后面那些条件时才会使用当前这个模板。
SFINAE 保证了这个模板替换即时失败也不会报错，而是放弃使用之。

优点：

* 允许根据模板的参数类型来区分到底使用哪个版本，而不是让模板成为一个啥都能匹配的大杂烩。
考虑如果没有 `enable_if` 的话，`vector` 的实现就必须定义出所有可能的非迭代器两参数版本，非常麻烦

* 可以早在编译时就报错。同样对于 `vector` 而言，如果把两参数构造写成一锅端模板，在其内部来进行各种判断
处理，或者报错，则会在运行时才得到报错信息
