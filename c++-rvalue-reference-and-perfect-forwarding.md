# Rvalue Reference

用途主要有二：
1. 實現 move semantics
2. 實現 perfect forwarding

move，比如有時候會 `obj = Ctor()` 這樣賦一個 rvalue 給變量，
其實此刻 `operator =` 無必要拷貝，因為賦值的語義希望讓 `obj` 持有的資源析構，
所以不如交換 `obj` 和 rvalue 對象的“內在”。

C++11 提供的方法即支持 `T &&` 類型的 rvalue reference，用 lvalue
和 rvalue 調用 `operator =` 時會區分兩個版本。

注意，對一個重載函數，

* 若只有 `f(T &)`，則只能對 lvalue 調用
* 若只有 `f(const T &)`，則可對 lvalue 和 rvalue 調用，但不會區分兩者
* 若有 `f(const T &)` 和 `f(T &&)`， 則可對 lvalue 和 rvalue 調用，且能區分兩者
* 若只有 `f(T &&)`，則只能對 rvalue 調用

## 區分 lvalue 與 rvalue
聲明為 rvalue reference 的有可能是 rvalue 或 lvalue，要根據該對象是否有名字來區分。

> Things that are declared as rvalue reference can be lvalues or rvalues.
> The distinguishing criterion is: if it has a name, then it is an lvalue.
> Otherwise, it is an rvalue.

所以有以下情形：

```cpp
void foo(X&& x)
{
    X anotherX = x; // calls X(X const & rhs)
}

Derived(Derived&& rhs) 
: Base(rhs) // wrong: rhs is an lvalue
{
    // Derived-specific stuff
}
```

現代編譯器往往有編譯優化，所以不要濫用 `std::move`，例如以下代碼編譯器會直接
在返回值處構造 `x` 而非拷貝

```cpp
X foo()
{
    X x;
    // perhaps do something to x
    return x;
}
```

## `operator =` 之注意事項
實現 `operator =` 時要注意做一些必要的清除工作，避免 move 導致無法跟蹤的析構。

例如 `a = std::move(b)`，若此 `operator =` 的實現僅僅簡單交換雙方資源，則當 `b`
退出作用域時，它原來持有的，当前 `a` 的資源將被析構，但若 `b` 又成了 move 的目標，
則可能無法跟蹤原來 `a` 所有的資源到底何時被析構。尤其是析構函數要做一些帶副作用的
操作，比如釋放 lock。

## 強制 move
即對 lvalue 強制使用 `std::move` 生成對應的 rvalue，來避免不必要的拷貝動作，例如

```C++
template<class T> 
void swap(T& a, T& b) 
{ 
    T tmp(std::move(a));
    a = std::move(b); 
    b = std::move(tmp);
} 
```

好處：
1. 若實現了 move，則一些標準庫算法會調用
2. 一些 STL 容器只用可 move 不可 copy 的元素即可


## Perfect Forwarding 問題

```cpp
template<typename T, typename Arg> 
shared_ptr<T> factory(Arg arg)
{ 
    return shared_ptr<T>(new T(arg));
} 
```

此處 `factory` 的調用者希望直接在調用處完成 `T` 類型的構造，
但 `factory` 的調用引入了一次額外的拷貝。如果 `T` 類型的構造
需要的是 `Arg` 類型的引用則會出問題。

可能的解決方案是將 `factory` 改為接收引用參數

```cpp
template<typename T, typename Arg> 
shared_ptr<T> factory(Arg& arg)
{ 
    return shared_ptr<T>(new T(arg));
} 
```

但這樣做的問題在於，`factory` 無法通過 rvalue 來調用

```cpp
factory<X>(hoo()); // error if hoo returns by value
factory<X>(41);    // error
```

另一解決方案是讓參數改為接收 const 引用

```cpp
template<typename T, typename Arg> 
shared_ptr<T> factory(Arg const & arg)
{ 
    return shared_ptr<T>(new T(arg));
} 
```

這樣做的問題是 move semantics 無法實現，即使 `T` 類型的構造支持 move，但 `factory` 的調用使得傳入 `T` 類型構造函数的是 lvalue `arg`。

解決這個問題需要兩條規則

1. Reference collapsing
  * rvalue reference to rvalue reference collapses to rvalue reference
  * all other combinations form lvalue reference
2. 模板參數類型推斷
  * `template<typename T> void foo(T &&);` 用 `A` 類型 lvalue 調用時，`T` 為 `A &`，根據前一規則，`foo` 的參數類型為 `A &`
  * `template<typename T> void foo(T &&);` 用 `A` 類型 rvalue 調用時，`T` 為 `A`，根據前一規則，`foo` 的參數類型為 `A &&`

於是可以將前面的 `factory` 改為

```cpp
template<typename T, typename Arg> 
shared_ptr<T> factory(Arg&& arg)
{ 
    return shared_ptr<T>(new T(std::forward<Arg>(arg)));
} 
```

其中 `forward` 的作用是在有 move 語義時保持 move 語義，也就是说，把
一个函数调用“完美地”传给另一个函数调用。其定義為

```cpp
template<class S>
S&& forward(typename remove_reference<S>::type& a) noexcept
{
    return static_cast<S&&>(a);
} 
```

各种调用情形：

* `arg` 类型为 `X &`，则 `Arg` 为 `X &`，`S` 为 `X &`，`forward` 返回 `X &`
* `arg` 类型为 `X &&`，则 `Arg` 为 `X &&`，`S` 为 `X &&`，`forward` 返回 `X &&`


## 關於 `std::forward` 中的 `remove_reference`

`remove_reference<S>::type &` 的作用是去掉 `S` 上的任何引用，只留下 `S`，來保證 `a` 具有 `S &` 類型。

如果不加這個變換，則 `forward` 只有在顯式地指定了模板參數時，即 `forward<...>(...)` 這樣調用才能保證正確。

考慮這一段程序

```cpp
template<class S>
S &&myforward(S &a) noexcept
{
    return static_cast<S&&>(a);
}

template<typename Arg>
Person *factory(Arg &&arg)
{
    return new Person(myforward(arg));
}

int main()
{
    std::cout << std::boolalpha;
    std::string lv("hello");
    Person *p1 = factory(lv);                    // should call lvalue ctor [*]
    const std::string clv("hello");
    Person *p2 = factory(clv);                   // should call lvalue ctor
    Person *p3 = factory(std::string("hello"));  // should call rvalue ctor

    return 0;
}
```

標記星號的一行和 `std::forward` 行為不一致，因為在 `factory` 中，`Arg` 推斷為 `std::string &`，
而 `arg` 具有 `std::string &` 類型，傳給 `myforward` 後，`S` 推斷為 `std::string` 類型，所以
`myforward` 會返回一個 `std::string &&`，導致 `Person` 的 rvalue ctor 被調用。

而如果顯式地指定了 `myforward<Arg>`，則會調用 `myforward<std::string &>`，這樣 `S` 將具有類型
`std::string &`，最後返回一個 lvalue reference。

## `std::move` 實現

```cpp
template<class T> 
typename remove_reference<T>::type&&
std::move(T&& a) noexcept
{
    typedef typename remove_reference<T>::type&& RvalRef;
    return static_cast<RvalRef>(a);
} 
```

注意兩點

1. 參數要聲明為 `T &&` 來保證對 lvalue 和 rvalue 的正確調用
2. `remove_reference<T>::type &&` 提取出一個純的 `T` 類型 rvalue reference


## Move 和异常
实现 move 构造函数时最好确保不抛异常（一般都是交换指针而已），并用 `noexcept` 修饰。


## 參考

1. [http://thbecker.net/articles/rvalue_references/section_01.html](http://thbecker.net/articles/rvalue_references/section_01.html)
2. [http://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/](http://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/)
