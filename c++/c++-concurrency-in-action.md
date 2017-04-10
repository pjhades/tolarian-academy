`std::thread` 构造

```c++
template< class Function, class... Args > 
explicit thread( Function&& f, Args&&... args );
```
传入参数后会这样调用 `f`:

```c++
std::invoke(decay_copy(std::forward<Function>(f)), decay_copy(std::forward<Args>(args))...)

template <class T>
std::decay_t<T> decay_copy(T&& v) { return std::forward<T>(v); }
```

这里 `decay_copy` 是按值返回，所以会复制参数，如果要给 `f` 传引用，需要用 `std::ref` 或 `std::cref` 封装。


`std::thread` 在线程处于 joinable 状态时析构或赋值，对应线程会调用 `std::terminate()` 终止。


给一些标准算法传成员函数时，要使用 `std::mem_fn` 来封装，否则会因为算法模板内部的调用语法 `f(args)` 与
成员函数调用不符而导致错误。


`std::thread::hardware_concurrency` 返回一个最大支持线程数的参考值，一般等于 CPU core 的数量。
