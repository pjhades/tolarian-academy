# 进程管理

## 进程生命期
进程可能处于：
* running 正在运行
* waiting 可以运行，但当前 CPU 被其他进程占用。下次调度可以被选中运行
* sleeping 因等待某种事件发生而暂时无法运行。下次调度不能选则

用户态切换到内核态的方法：
* 系统调用，主动，发起调用的进程请求切换
* 中断，随机，可能和当前进程无关，比如网络数据包到达

## 抢占式调度
* 普通进程始终可以被抢占
* 执行系统调用时，不可被 scheduler 抢占，但可能被中断（`EINTR`）
* 中断可以 suspend 用户进程和内核进程，有最高优先级，因为其需要被快速响应
* kernel preemption - switch to another process even during execution of syscall

查看进程资源限制 `cat /proc/PID/limits`

## namespace
- 资源隔离，将资源组织为 namespace，以此把进程划分到不同的、独立的容器里
- 用 vm 的话需要 one kernel + userland per user，对资源需求比较高
- `chroot` 文件系统的 namespace

```C
    struct task_struct --- struct nsproxy --- struct xxx_namespace
    进程                   the view，进程     被管理的资源
                           可见的一系列
                           namespace
```

### `struct uts_namspace`
- 用 `struct new_utsname` 记录 domain name、node name 等信息
- 初始 `init_uts_ns`
- `copy_utsname` 复制

### `struct user_namespace`
- `struct user_struct` 记录 namespace 中的用户信息，例如 `uid_map`、`gid_map`

### `struct pid_namespace`
各种 identifier
- thread group id，TGID
  - 通过 `CLONE_THREAD` 调用 `clone` 创建的进程同属一个 thread group，
  - 同一 thread group 中的任意进程调用 `getpid` 返回 TGID
  - thread group 内部的进程通过 thread id，TID 区分
  - `task_struct->group_leader` 指向 thread group leader
- process group id，PGID
  - `setpgrp`
  - 管道连接的一系列进程同属一个 process group
- session id，SID
  - `setsid`
  - 多个 process group 组成一个 session

全局 PID 和 TGID 保存在 `task_struct->pid` 和 `task_struct->tgid`

- `pid_namespace->child_reaper` 保存此 namespace 对应的 `init`
- `pid_namespace->parent` 指向上级 namespace
- `pid_namespace->level` 记录 namespace 层次中的深度，根深度为 0

数据结构：
```C
    // 哈西表存 upid
    static struct hlist_head *pid_hash;

    // 某个特定 PID 在某个 namespace 可见的具体数值
    struct upid {
    	/* Try to keep pid_chain in the same cacheline as nr for find_vpid */
    	int nr;
    	struct pid_namespace *ns;
        // pid_hash 哈希表串起来
    	struct hlist_node pid_chain;
    };
    
    // 某个特定 PID
    struct pid
    {
    	atomic_t count;
        // 此 PID 在几个 namespace 中可见，即最深到哪层
    	unsigned int level;
    	/* lists of tasks that use this pid */
        // 每种 PID 类型一个链表保存使用此 PID 的 task_struct
    	struct hlist_head tasks[PIDTYPE_MAX];
    	struct rcu_head rcu;
        // 此 PID 在每个 namespace level 中的具体数值，默认 1 因为只有 global namespace 深度为 0
    	struct upid numbers[1];
    };

    // TGID 不在此列，因为 TGID 即 leader 的 PID
    enum pid_type
    {
    	PIDTYPE_PID,
    	PIDTYPE_PGID,
    	PIDTYPE_SID,
    	PIDTYPE_MAX
    };

    struct pid_link
    {
        struct hlist_node node;
        struct pid *pid;
    };

    struct task_struct {
        // ...
    
        // 此 task 的各种 PID
    	struct pid_link pids[PIDTYPE_MAX];
    
        // ...
    };
```

- `attach_pid` 对 task 和 PID 建立双向关联
  - `task_struct->pids[type].pid = pid`
  - 将 `task_struct->pids[type]` 结点加入哈希表 `pid->tasks[type]`
- 各种查找操作
  - PID value, namespace -> task
    - `find_pid_ns` 根据 PID value 和 namespace 查哈希表 `pid_hash` 得到 PID
    - `pid_task` 根据 PID 和 type 从 `pid->tasks` 找出
    - `find_task_by_pid_type_ns`
  - task, PID type, namespace -> PID value
    - `task_pid_nr_ns`
    - `task_tgid_nr_ns`
    - `task_pgrp_nr_ns`
    - `task_session_nr_ns`
- 生成 PID
  - `alloc_pidmap` 操作 bitmap
  - `free_pidmap`
  - `alloc_pid` 创建 PID，关联到目标 namespace 及其所有上级 namespace



### 进程切换
`switch_to(prev, next, last)` 宏完成切换操作，注意 `last` 输出参数，
用来保存从谁切换回来的：

1. push eflags 和 `%ebp` 
2. 将当前栈 `%esp` 保存到 `prev->thread.esp`
3. 加载新的栈 `next->thread.esp` 到 `%esp`，切换到 `next` 的 kernel stack
4. 保存一个 label 到 `prev->thread.eip`，切换回来时从 label 出继续运行
5. push `next->thread.eip` 到它自己的 stack
6. `jmp __switch_to`
7. label，控制已经切换回来
  1. pop `%ebp`
  2. pop eflags
  3. 将 `%eax` 的内容输出到 `last`

`__switch_to(prev_p, next_p)` 函数最后会 `return prev_p`，将 `prev_p`
保存到 `%eax`，配合之前栈上准备好的 `%eip`，将控制切换到 `next_p`。

此时切换过去的进程将从 label 处执行，从 `%eax` 中把刚才 `return` 的
`prev_p` 保存到它对应 `switch_to` 的 `last` 中，标明控制从 `prev_p` 转移而来。


### 进程关系
- `task_struct->children` 子进程列表
- `task_struct->sibling` 同一父进程 fork 出的其他进程列表，按 fork 先后排序


### 进程管理 API
`fork` `vfork` `clone` 入口是 `sys_xxx`，然后统一调用 `do_fork`

## `do_fork`
- `copy_process`
- 确定子进程 PID 和给父进程返回的 PID，如果创建了 namespace，父进程得到的应该是子进程在父进程 namespace 中的 PID
- 若 `clone` 指定 `CLONE_PTRACE` 且父进程被跟踪，则子进程也应该被跟踪，要发 `SIGSTOP`
- 若来自 `vfork`，则有 `CLONE_VFORK`，启动子进程 completion，让父进程等待直到子进程终止或调用 `execve`
- 调用 `wake_up_new_task` 将子进程加入 scheduler 队列等待被调度

## `copy_process`
- 检查 `CLONE_XXX` flag 是否有语义上的冲突或不相容，例如若指定了 `CLONE_THREAD`，
则必须指定 `CLONE_SIGHAND` 在父子之间共享信号设置
- `dup_task_struct` 创建新的 `task_struct`，父子进程 `task_struct` 差别只有内核栈 `task_struct->stack` 不一样
  - 内核栈通常和 `struct thread_info` 共同保存在 `union thread_union` 中
  - 默认 8KB 保存 `thread_union`，可通过 `4KSTACKS` 改为 4KB
- 检查 fork 操作是否超过 `RLIMIT_NPROC` 资源限制
