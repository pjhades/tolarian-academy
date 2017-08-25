`std::addressof` 的作用是，在重载的 `operator &` 存在时，取到对象真正的地址。

如 cppreference 上的例子：

```c++
template<class T>
struct Ptr {
    T* pad; // add pad to show difference between 'this' and 'data'
    T* data;
    Ptr(T* arg) : pad(nullptr), data(arg) 
    {
        std::cout << "Ctor this = " << this << std::endl;
    }
 
    ~Ptr() { delete data; }
    T** operator&() { return &data; }
};`c++
```

实现：

```c++
template< class T >
T* addressof(T& arg) 
{
    return reinterpret_cast<T*>(
               &const_cast<char&>(
                  reinterpret_cast<const volatile char&>(arg)));
}
```

1. 将 `arg` 转为 `char &` 并加上 cv。原因是
  * 如果直接转型为 `char &`，`arg` 上可能带 cv，而 `reinterpret_cast` 不能去掉 cv
  * 如果转型为其他类型，如 `int &`，则后面取地址时，可能会因为对齐的原因得不到正确地址
2. 将上一步结果转为 `char &`，去掉 cv
3. 取地址。此时由于取的是内置类型 `char` 的地址，所以不会受重载 `operator &` 的影响
4. 转回 `T *` 返回

参考：

* [http://stackoverflow.com/questions/16195032/implementation-of-addressof](http://stackoverflow.com/questions/16195032/implementation-of-addressof)
