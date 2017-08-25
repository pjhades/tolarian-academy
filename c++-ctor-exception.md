# 构造函数中的异常

## 对象生命期
始于构造函数之末，终于析构函数之始。其余时间均表示“没有此对象”。

基于以上原则，构造函数中如果有异常抛出，不管是非 `static` 的成员对象，还是基类对象，由于异常出现时
构造函数尚未结束，所以对象并未被构造出来，此时抛异常是唯一的选择。

## Function Try Block
主要用在构造函数上，其作用在于 translate 构造函数 initializer list 中可能抛出的异常。在 function try block 的 `catch` 中
如果没有抛出任何新异常，则会自动抛出当前已出现的异常。所以在 `catch` 中要么抛原有异常，要么根据原有异常
抛出一个新的，这就是所谓 translate。

据此，无法实现一个绝对不抛任何异常的构造函数。因为要做到不抛异常，就要保证构造函数捕获所有可能在基类对象或成员对象的构造中出现的异常。
但用 function try block 必须抛出新异常，所以即时声明构造函数为 `noexcept(true)`，运行时也会因为实际抛出了异常而导致 `terminate()` 被调用，
进程退出。

如：

```C++
class Base {
    public:
        Base() {
            throw runtime_error("base object failed");
        }
};

class Derived: public Base {
    public:
        // 如果声明为 noexcept(false)，则 Base 的异常捕获后会自动重新抛出
        // 在 main 中可以捕获到
        Derived() noexcept(true) try {
        }
        catch (runtime_error &e) {
            cout << e.what() << endl;
        }
};

int main()
{
    try {
        Derived d;
    }
    catch (runtime_error &e) {
        cout << e.what() << endl;
    }

    return 0;
}
```

析构函数和其他函数基本是用不到 function try block 的。


## Tips
* Unmanged resource 放在构造函数 body 中分配。否则，如果放在 initializer list 中，出现异常时无法自动 `delete` 掉。
此外 unmanaged resource 相关的异常用普通的 try-catch 处理

* 如果可以，尽量用 RAII，省很多麻烦


## 参考
* [http://www.gotw.ca/publications/mill13.htm](http://www.gotw.ca/publications/mill13.htm)
* [http://en.cppreference.com/w/cpp/language/function-try-block](http://en.cppreference.com/w/cpp/language/function-try-block)
