# Signal

## 信号处理流程

* 未决信号集（pending set）
* 阻塞信号集（blocking set）
  * 事实上就是 mask
* signalAction
  * 进程内的线程组共享一个处理函数

* 进程在由中断 / 异常返回前检查是否有未处理的信号：`userTrapReturn()`
* 处理信号：
  * 未注册信号：默认处理
    * 默认处理的方式需要注意
  * 已注册信号：
    * 系统注册的特殊信号，如 SIGKILL，SIGSTOP，由系统注册的处理函数处理
    * 用户注册的信号，由用户注册的处理函数在用户态处理
      * 此时可能再次被信号打断（信号重入）
* 从内核态返回用户态

我们选取 [Linux 5.10](https://github.com/torvalds/linux/blob/v5.10/include/uapi/asm-generic/unistd.h) 中 signal 的系统调用的一个子集进行实现

```c
#define SYSCALL_KILL 129
#define SYSCALL_TKILL 130
#define SYSCALL_TGKILL 131
#define SYSCALL_SIGNAL_ACTION 134
#define SYSCALL_SIGNAL_PROCESS_MASK 135
#define SYSCALL_SIGRETURN 139
```

