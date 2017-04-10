#happens-before relationship

happens-before 指的是一个读写操作结果要在另一个操作开始之前生效。

注意
此关系不一定蕴含着时间上的先后。如果两条指令不相关，编译器仍然可以 reorder
时间上需要先后发生，不一定蕴含着此关系的实现。只有遵循语言规范的读写才能保证此关系的实现


#synchronizes-with relationship

此关系是一种 happens-before 关系。happens-before 还有别的形式，比如单个线程中按 program order 执行。

注意找出两点

* guard variable，保护 payload 的读写
* payload，要在线程间传递的数据

release 可以 synchronize with acquire

具体到 c++11 中，可以直接用 release 语义的 atomic 写来 synchronize with acquire 语义的 atomic 读。

synchronizes-with relationship 是运行时的关系，而非代码间的关系。如果 acquire 发生在 release 修改 guard variable 之前，就没有这种关系。

其他实现方法

* acquire/release，release synchronizes with next acquire
* thread create/join，join synchronizes with next create
* mutex，unlock synchronizes with next lock

synchronizes-with relationship 仅在语言和 API 规范说它存在时才存在。规范作出约定，代码里面才能这么写。


#strong and weak memory models

x86、x64 都是 strong model，除了特殊情况（极端情况查手册），除了 store/load reordering 之外其他三种都不允许出现。


#acquire/release semantics

主要是在 lock-free programming 中遇到，而且，既然有 ordering 问题，显然没有 sequential consistency 的保证

* acquire semantics 只作用于读操作，此读操作后面的读写不能提到它之前
* release semantics 只作用于写操作，此写操作之前的读写不能提到它之后

可以用 barrier 来实现该语义，barrier 必须放在 acquire 之后，或者 release 之前，才能保证严格地限定 acquire 和 release 与 critical section 的相对顺序

保持 acquire/release semantics 只需要三种 barrier

* load load - acquire
* load store - acquire + release
* store store - release

要求 acquire 和 release 两个操作
* 作用于同一个变量（否则还谈什么顺序）
* 对该变量的操作都是原子的（否则该变量会被其他线程修改导致不一致）

##实现方法

* 汇编指令
    + 如 powerpc 的 `lwsync` 指令，对应的编译器接口为 `__lwsync()`，编译器将 emit 对应的 `lwsync`
* c++11 fences
    + `atomic_thread_fence()`，参数指定 fence 的类别，这里只需要
    + `memory_order_acquire`
    + `memory_order_release`
    + 一般类型的原子性用 atomic<T> 来保证
* c++11 acquire/release operation
    + atomic<T> 变量的 store 和 load 方法即可指定 `memory_order_acquire`
    + 此法不如前一方法严格，因为
        - acquire/release operation 只限制前后的读写不能和它本身乱序
        - 但 fence 限制前后的读写不能和它后前的所有读写乱序
* locking
    + lock 的 acquire 和 release 操作即名字的来历
    + lock 的实现必须保证 critical section 被 acquire 和 release 包围起来，防止跨越边界的 reordering，如此才能保证 critical section 的操作（信息）正确地在线程之间传递




#acquire/release fences

* An acquire fence prevents the memory reordering of any read which precedes it in program order with any read or write which follows it in program order.

* A release fence prevents the memory reordering of any read or write which precedes it in program order with any write which follows it in program order.




#Double Checked Locking Pattern，DCLP

就是用一个 guard variable 来减少 lock cnotention，比如 c++11 的 singleton 实现

```c++
Singleton* Singleton::getInstance() {
    Lock lock;      // scope-based lock, released automatically when the function returns
    if (m_instance == NULL) {
        m_instance = new Singleton;
    }
    return m_instance;
}
```

普通实现用到了 lock，可以满足要求，但 singleton 创建之后，其他线程都会先检查 lock 再检查指针，所以即使 singleton 已经创建，还是会有较大的 lock contention 开销。

如果改写为 DCLP：

```c++
std::atomic<Singleton*> Singleton::m_instance;
std::mutex Singleton::m_mutex;

Singleton* Singleton::getInstance() {
    Singleton* tmp = m_instance.load(std::memory_order_relaxed);
    std::atomic_thread_fence(std::memory_order_acquire);
    if (tmp == nullptr) {
        std::lock_guard<std::mutex> lock(m_mutex);
        tmp = m_instance.load(std::memory_order_relaxed);
        if (tmp == nullptr) {
            tmp = new Singleton;
            std::atomic_thread_fence(std::memory_order_release);
            m_instance.store(tmp, std::memory_order_relaxed);
        }
    }
    return tmp;
}
```

这里用 singleton 指针来作为 guard variable，并用 barrier 保证创建 singleton 的线程和后续线程之间正确传递 singleton 已经创建好的消息。

此例中 barrier 也可换成 c++11 的 atomic acquire/release peration 或者默认的 sequentially consistent atomic operation



#consume semantics 和 c++11 `std::memory_order_consume`

此语义的特点

* 与 `memory_order_acquire` 一样，要和 `memory_order_release` 配对使用
* 性能比 acquire 好，但约束比 acquire 要弱
* 在 data dependency ordering 得到保证时，可以替代 acquire 提高性能

通常 acquire/release 在一些平台上会编译为 barrier，但一些 CPU 如 itanium，x86，x64，weakly ordered CPU 如 powerpc 遇到 data-dependent 指令时会保持内存访问的顺序。所谓 data-dependent 指令是指两条指令之间存在数据依赖，比如前一条指令写寄存器，后一条指令要用到刚写入的寄存器。

btw，注意此时 cache coherence 很重要，如果 CPU 不能很好地保证 cache coherence，则可能另一个 CPU 上的线程修改了内存中的变量，但本 CPU cache line 并未更新，则第二条读指令会读到旧的数据。

多个 data dependency 可以组成 chain，data dependency ordering 保证这些 chain 会按照代码指令的顺序执行，但 chain 之间仍然可能 reorder。

`memory_order_consume` 的引入主要是为了在 data dependency ordering 有保障的情况下替代 acquire 提高效率。但正确的使用必须注意

* 和 release 配对
* consume operation 作用的变量必须和之后的某个操作形成 chain

linux RCU 利用了 data dependency ordering

consume/release 和 acquire/release 的对比参见 cppreference.com 的例子

acquire/release 保证了 release 前的所有写，都能在 acquire 之后可见：

```C++
#include <thread>
#include <atomic>
#include <cassert>
#include <string>

std::atomic<std::string*> ptr;
int data;

void producer()
{
    std::string* p  = new std::string("Hello");
    data = 42;
    ptr.store(p, std::memory_order_release);
}

void consumer()
{
    std::string* p2;
    while (!(p2 = ptr.load(std::memory_order_acquire)))
        ;
    assert(*p2 == "Hello"); // never fires
    assert(data == 42); // never fires
}

int main()
{
    std::thread t1(producer);
    std::thread t2(consumer);
    t1.join(); t2.join();
}
```

但 consume/release 强调的是数据依赖，即只有对 release store 的那个变量在 release 之前的写，在
consume load 之后依赖 load 的变量的语句中可见：

```C++
#include <thread>
#include <atomic>
#include <cassert>
#include <string>

std::atomic<std::string*> ptr;
int data;

void producer()
{
    std::string* p  = new std::string("Hello");
    data = 42;
    ptr.store(p, std::memory_order_release);
}

void consumer()
{
    std::string* p2;
    while (!(p2 = ptr.load(std::memory_order_consume)))
        ;
    assert(*p2 == "Hello"); // never fires: *p2 carries dependency from ptr
    assert(data == 42); // may or may not fire: data does not carry dependency from ptr
}

int main()
{
    std::thread t1(producer);
    std::thread t2(consumer);
    t1.join(); t2.join();
}
```
