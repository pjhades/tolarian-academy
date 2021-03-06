# `kobject`
作为“基类”嵌入到其他数据结构中，实现对象引用计数等管理。
kobject 与 sysfs 相关，可通过 sysfs 将各种系统信息导出到 `/sys` 供用户空间查看。

但要注意，引用计数和 kobject hierarchy 不一定要同时使用，可以创建 kobject 且不加入 sysfs，
只使用引用计数。

文档见 `Documentation/kobject.txt`。

```c
struct kobject {
	const char		* k_name;     // 通过 sysfs 导出到用户空间的名字
	struct kref		kref;         // 引用计数，struct kref 对引用计数的具体操作做数据抽象
	struct list_head	entry;    // kobject 可以组织在一起
	struct kobject		* parent;
	struct kset		* kset;       // 此 kobject 所属的 kset，表示一组要做类似处理的 kobject
	struct kobj_type	* ktype;
	struct sysfs_dirent	* sd;
};

// 某种 kobject 类别与 sysfs 间的交互
struct kobj_type {
	void (*release)(struct kobject *);   // 引用计数减到 0 时调用，释放此 kobject 所用内存
	struct sysfs_ops	* sysfs_ops;
	struct attribute	** default_attrs;
};

// 实现引用计数，做数据抽象，避免直接操作具体的类型
struct kref {
	atomic_t refcount;
};

// 管理一组同类的 kobject
struct kset {
	struct kobj_type	*ktype;  // 此 kset 中所有 kobject 的共同类别
	struct list_head	list;    // 属于此 kset 的 kobject 列表
	spinlock_t		list_lock;   // 控制对 kset 中 kobject 的并发访问
	struct kobject		kobj;    // kset 本身也是一个 kobject，也要嵌入一个 kobject 来管理，也可以把一整个 kset 注册到 sysfs
	struct kset_uevent_ops	*uevent_ops; // 一些回调函数，此 kset 中发生一些事件时调用
};

// 保存一些回调函数，将一个 kset 的状态传播到用户空间
struct kset_uevent_ops {
	int (*filter)(struct kset *kset, struct kobject *kobj);
	const char *(*name)(struct kset *kset, struct kobject *kobj);
	int (*uevent)(struct kset *kset, struct kobject *kobj,
		      struct kobj_uevent_env *env);
};
```

# Reference

1. Professional Linux Kernal Architecture
