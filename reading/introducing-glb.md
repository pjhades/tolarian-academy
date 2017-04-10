[http://githubengineering.com/introducing-glb/](http://githubengineering.com/introducing-glb/)

#Github 的 load balancer Github Load Balancer（GLB） 升级

#背景

* bare metal machine
* vertically scaled
* small set of large, monolithic machine
* use haproxy
* specific hardware configuration

#目标

* 可运行在 commodity hardware 上
* scales horizontally
* 支持 high availability，避免运维和 failover 时切断 TCP 连接
* 支持 connection draining，即 LB 后端 node 过载时，LB 不再将其列入 LB rotation，但允许一段时间让 node 处理掉已经给它的连接，过了时限 LB 强制断开连接
* 对每个服务进行 LB，也要支持多个服务共用一个 LB
* Can be iterated on and deployed like normal software（iterate on 啥意思？）
* 在 LB 各层都可以测试，而不是只能集成测试
* Built for multiple POPs and data centers （POP 是啥？）
* 能抵抗 DDoS，有工具来防止后续的攻击

#设计

* 复用 IP 地址
  * 通常的做法，每台物理机器一个 IP 地址，DNS 将流量导向各个 IP/机器
    * 缺点
      * DNS 记录 TTL 常被无视而过度缓存，导致 LB 不均匀
      * 有些用户会直接访问 IP，导致 LB 不均匀
      * 太耗费 IP。Github 保留了一部分 IP 地址给 Github pages 服务，比如用户的静态博客，得给用户 IP 地址让他们去设置自己的域名指向
      * 要用一种方法在多台物理机器上复用 IP
  * 借助 ECMP（Equal-Cost Multi-Path）
    * ECMP 是路由器支持的 multi-path 路由机制
    * 相当于在多个 cost 相等的链路上做负载均衡
    * 支持对 packet IP/port 等信息做一致性 hash，保证同一 TCP 连接的 packet 发往同一主机
    * 缺点：共享 IP 的某台机器挂掉、维护时，约 1/n 的连接要 rehash，而 rehash 到的机器会因没有 TCP 连接状态而断开连接，所以少一台机器就得断 1/n 的连接，n 为共享 IP 的 node 数
* L4+L7 LB 架构
  * 路由器通过 ECMP 连接到后端的 L4 director，如 LVS
  * L4 LB 连接到后端的 L7 proxy，如 Haproxy
  * 优点
    * LVS 可以保持连接状态，所以 L7 proxy 挂掉时，对应的 L4 director 可以继续将该 proxy 服务的 TCP 封包发给其他 proxy，从而实现 connection draining
    * 运维工作主要在 L7 proxy 上，可以 horizontally scale
    * LVS 可以支持多个 node 用 multicast 同步 TCP 连接状态，所以即时 L4 director 挂掉，其他 director 也可以继续服务 rehash 后的 TCP 连接
  * 缺点
    * sync 需要 multicast，Github 不想这么玩，于是只能依赖所有director 使用相同的 hash 算法，且连接到相同的 L7 proxy
    * 这么一来，当 director 挂了或维护时会导致 TCP 连接中断，这个对 git 而言不能接受，因为 git 不能在连接中断时重试或续传
    * director 上记录了 TCP 连接状态，增加了防 DDoS 攻击的复杂度，需要在 director 上开 SYN cookie
* 更好的方案
  * rendezvous hashing：一种分布式 hash 算法，可在多个 client 之间达成共识，根据对象和存储结点计算所有 `hash(object, node[i])` 权重并排序，选最大的一个或多个结点来访问对象
  * 构造一张路由表，表中包含所有的 `hash(object, node[i])` 的 hash 值对应的 L7 proxy 列表
  * 这里 object 是根据请求源 IP hash 得到的 hash 值，node 是 L7 proxy
  * 将这份路由表保存在所有 director 上并同步
  * 请求到来时，根据其源 IP 计算出 hash，然后用此 hash 值查询路由表，找到应该转发给哪个 proxy。发送的方法是利用 [Foo-over-UDP](https://lwn.net/Articles/614348/) 将原来的 IP packet 封装到新的 UDP 报文中发送到 proxy 的内部 IP。proxy 收到先解包再交给协议栈处理，响应直接由后端回复给客户端
  * 优点
    * proxy 挂掉时，director 重新根据 rendezvous hashing 计算出新的路由表并同步。根据 rendezvous hashing 的性质，只有挂掉的 proxy 上的连接会 rehash
    * director 挂掉时，路由器 ECMP 机制会 rehash，但此时 director 不再保留 TCP 连接状态，而是查路由表重新封装出一个 UDP 封包来将请求转发给 L7，所以其他存活的 director 仍能继续服务
  * // 推测后端会用 ECMP 共用的那个 IP 和客户端通信，但客户端发的请求会先经过 LB
