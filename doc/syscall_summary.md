# 系统调用汇总

## 整理和查询

测试样例的文档中没有具体的系统调用说明，只有一个使用如下命令打印出的每个程序涉及的系统调用

```shell
strace -c <app>
```

这种命令显然是不适合开发的，所以我们用如下命令可以将系统调用名提取出来

```shell
awk 'NR>1 && !/total/ && !/----------------/ {print $5}' * | sort -u
```

还有一种是过滤汇编得到的，同样可以用命令提取出来

```shell
awk '{sub("__NR_", ""); print $4}' lmbench_libc_syscall.txt | sort -u
```

对于系统调用的具体查询，可以使用如下命令

```shell
man 2 <syscall_name>
```

但是这样查询是没有办法查找到系统调用号的，我最终是在网上搞了一份，但是是 X86 的，不知道有没有影响。https://blog.csdn.net/Embed_coder/article/details/75155809。

---



## 网络编程


| 调用名      | 简介                                                           | 情况 |
| ----------- | -------------------------------------------------------------- | ---- |
| accept      | 接受一个 socket 的连接                                         |      |
| bind        | 给 socket 绑定一个名字                                         |      |
| connect     | 初始化一个 socket 连接                                         |      |
| getpeername | 获得与 socket 关联的远程主机                                   |      |
| getsockname | 用于获取一个已经绑定到某个 socket 的本地地址信息               |      |
| getsockopt  | 用于获取指定 socket 的选项值                                   |      |
| listen      | 将 socket 设置为监听状态                                       |      |
| recvfrom    | 从指定的套接字接收数据，并获取发送数据的源地址                 |      |
| recvmsg     | 从指定的套接字接收数据，并获取发送数据的源地址和其他相关信息。 |      |
| sendfile    | 进行文件传输                                                   |      |
| sendmsg     | 发送消息                                                       |      |
| sendto      | 向指定的目标地址发送数据                                       |      |
| sethostname | 设置主机名                                                     |      |
| setsockipt  | 设置套接字选项                                                 |      |
| socket      | 创建一个套接字                                                 |      |



## 进程线程管理

这里其实还是比较繁杂的，起始可以考虑分成权限管理，线程管理，资源管理，进程间通信和调度管理多个部分。（香老师分一下）

| 调用名                 | 简介                                                                          | 情况   |
| ---------------------- | ----------------------------------------------------------------------------- | ------ |
| acct                   | 开始或停止对于进程信息的记录（在一个指定文件中）                              |        |
| capget                 | 获得线程的能力集（一组功能权限）                                              |        |
| capset                 | 设置线程的能力集                                                              |        |
| clone                  | 创建一个子进程                                                                | 已实现 |
| execve                 | 执行程序                                                                      | 已实现 |
| exit                   | 终止进程                                                                      | 已实现 |
| exit_group             | 终止一个进程的所有线程                                                        |        |
| futex                  | 线程等待直到条件为真                                                          |        |
| getegid                | 获得当前进程的有效组 ID                                                       |        |
| geteuid                | 获得当前进程的有效用户 ID                                                     |        |
| getgid                 | 获得当前进程的组 ID                                                           |        |
| getgroups              | 获得当前进行的附加组 ID 列表                                                  |        |
| getpgid                | 获得指定进程的组 ID                                                           |        |
| getpid                 | 获得进程 ID                                                                   | 已实现 |
| getppid                | 获得父进程 ID                                                                 | 已实现 |
| getpriority            | 获得进程优先级                                                                |        |
| getresgid              | 获取当前进程的实际、有效和保存的组ID（Real, Effective, and Saved Group ID）   |        |
| getresuid              | 获取当前进程的实际、有效和保存的用户ID（Real, Effective, and Saved Group ID） |        |
| getrlimit              | 获取指定进程的资源的软限制和硬限制（Soft Limit and Hard Limit）               |        |
| getrusage              | 用于获取指定进程或当前进程的资源使用情况（Resource Usage）                    |        |
| getsid                 | 用于获取指定进程或当前进程的会话 ID（Session ID）                             |        |
| gettid                 | 获得当前线程 ID                                                               |        |
| getuid                 | 获取当前进程的用户ID（User ID）                                               |        |
| kill                   | 用于指定要发送信号的进程                                                      |        |
| msgctl                 | 控制消息队列的属性和状态                                                      |        |
| msgget                 | 创建或获取一个消息队列                                                        |        |
| personality            | 设置进程的执行环境                                                            |        |
| pivot_root             | 更改进程的根文件系统                                                          |        |
| prctl                  | 管理进程的属性和状态                                                          |        |
| prlimit64              | 设置或获取进程的资源限制                                                      |        |
| rt_sigaction           | 设置指定信号的处理函数                                                        |        |
| rt_sigprocmask         | 设置或修改当前进程的信号屏蔽字                                                |        |
| rt_sigreturn           | 将信号处理程序的执行返回到原始程序的指令处                                    |        |
| sched_getaffinity      | 获取指定进程或线程的 CPU 亲和性                                               |        |
| sched_getparam         | 获取指定进程或线程的调度参数                                                  |        |
| sched_get_priority_max | 获得进程的调度最大优先级                                                      |        |
| sched_get_priority_min | 获得进程的调度最小优先级                                                      |        |
| sched_getscheduler     | 获取指定进程的调度策略                                                        |        |
| sched_setaffinity      | 设置指定进程或线程的 CPU 亲和性                                               |        |
| sched_setparam         | 设置指定进程的调度参数                                                        |        |
| sched_setscheduler     | 设置指定进程的调度策略                                                        |        |
| sched_yield            | 让当前进程主动放弃 CPU 时间片                                                 | 已实现 |
| semctl                 | 对信号量进行控制操作                                                          |        |
| semget                 | 创建一个新的信号量集                                                          |        |
| semop                  | 对信号量进行操作                                                              |        |
| setgid                 | 设置进程的组 ID                                                               |        |
| setgroups              | 设置进程的附加组 ID                                                           |        |
| setns                  | 将进程加入到指定的命名空间中                                                  |        |
| setpgid                | 设置进程的进程组 ID                                                           |        |
| setpriority            | 设置进程的优先级                                                              |        |
| setrlimit              | 设置进程的资源限制                                                            |        |
| setsid                 | 创建一个新的会话，并将调用进程设置为该会话的领头进程                          |        |
| set_tid_address        | 设置线程 ID 地址                                                              |        |
| setuid                 | 设置进程的用户 ID                                                             |        |
| shmat                  | 将共享内存区域附加到进程的地址空间中                                          |        |
| shmctl                 | 对共享内存区域进行控制操作                                                    |        |
| shmdt                  | 将共享内存区域从进程的地址空间中分离                                          |        |
| socketpair             | 创建一对相互连接的套接字                                                      |        |
| tgkill                 | 向指定线程发送信号（用 pid）                                                  |        |
| tkill                  | 向指定线程发送信号（用 tid）                                                  |        |
| unshare                | 创建一个新的命名空间                                                          |        |



## 时钟时间

| 调用名        | 简介                         | 情况 |
| ------------- | ---------------------------- | ---- |
| adjtimex      | 调整系统时钟的精度和偏移量   |      |
| clock_adjtime | 用于调整系统时钟的行为和频率 |      |
| clock_getres  | 获取指定时钟的精度或分辨率   |      |
| clock_gettime | 获得时钟时间                 |      |
| clock_settime | 设置时钟时间                 |      |
| gettimeofday  | 获取当前时间和日期           |      |
| nanosleep     | 让当前进程睡眠指定的时间     |      |
| setitimer     | 设置计时器                   |      |
| times         | 获取进程的 CPU 时间          |      |



## 内存管理

| 调用名   | 简介                                                             | 情况   |
| -------- | ---------------------------------------------------------------- | ------ |
| brk      | 更改数据段的尾部地址                                             | 已实现 |
| madvise  | 通知内核对指定的内存区域采取特定的操作，例如，预读、预写、释放等 | 已实现 |
| mlock    | 将指定的内存区域锁定在物理内存中，以便防止被交换出去。           | 已实现 |
| mmap     | 将一个文件或设备映射到进程的地址空间中                           | 已实现 |
| mprotect | 修改指定内存区域的访问权限                                       | 已实现 |
| mremap   | 重新映射一个已经映射的内存区域                                   |        |
| msync    | 将内存中的数据同步到磁盘上                                       | 已实现 |
| munlock  | 解锁指定的内存区域                                               |        |
| munmap   | 解除一个进程地址空间中的内存映射                                 | 已实现 |
| swapon   | 将交换分区或交换文件打开并启用                                   |        |
| wait4    | 等待进程                                                         | 已实现 |



## 文件系统

| 调用名       | 简介                                     | 情况   |
| ------------ | ---------------------------------------- | ------ |
| chdir        | 切换工作目录                             | 已实现 |
| chroot       | 切换根目录                               |        |
| close        | 关闭文件                                 | 已实现 |
| dup          | 为已经打开的文件分配一个新的描述符       | 已实现 |
| dup3         | 让新的描述符和已经打开的文件对应起来     | 已实现 |
| faccessat    | 测试文件访问权限（可以指定相对目录）     |        |
| fallocate    | 预分配文件空间                           |        |
| fchdir       | 切换工作目录（可以指定相对目录）         |        |
| fchmod       | 更改文件的模式                           |        |
| fchmodat     | 更改文件的模式（用路径名）               | 已实现 |
| fchown       | 更改文件的所有者                         |        |
| fchownat     | 更改文件的所有者（用路径名）             |        |
| flock        | 对文件进行锁定                           |        |
| fstat        | 查询文件状态                             | 已实现 |
| fstatat      | 查询文件状态（用路径名）                 | 已实现 |
| fsync        | 内存文件同步                             | 已实现 |
| ftruncate    | 截断文件                                 |        |
| getcwd       | 获得当前路径                             | 已实现 |
| getdents64   | 获得目录项                               | 已实现 |
| linkat       | 创建软链接（似乎在 linux 上是硬链接）    | 已实现 |
| lremovexattr | 删除指定文件的扩展属性                   |        |
| lseek        | 设置文件的偏移量                         |        |
| lsetxattr    | 设置指定文件的扩展属性                   |        |
| mkdirat      | 创建目录                                 | 已实现 |
| mount        | 挂载文件系统                             | 已实现 |
| newfstatat   | 获得文件状态（功能强大）                 | 已实现 |
| openat       | 打开文件                                 | 已实现 |
| pipe2        | 创建一个管道                             | 已实现 |
| read         | 读取文件                                 | 已实现 |
| readahead    | 预读取文件的内容到内存中                 |        |
| readlinkat   | 读取符号链接文件的目标路径               | 已实现 |
| readv        | 从文件中读取多个数据块到多个缓冲区中     | 已实现 |
| removexattr  | 删除指定文件的扩展属性                   |        |
| renameat2    | 对指定目录下的文件或目录进行重命名或移动 |        |
| setxattr     | 设置文件或目录的扩展属性                 |        |
| statfs       | 获取文件系统的状态信息                   | 已实现 |
| symlinkat    | 用于创建一个符号链接                     |        |
| sync         | 将文件系统中的缓存数据写入磁盘中         |        |
| syncfs       | 将文件系统中的缓存数据写入磁盘中         |        |
| umask        | 设置文件的默认权限掩码                   | 已实现 |
| umount2      | 卸载文件系统                             | 已实现 |
| unlinkat     | 取消链接                                 |        |
| utimensat    | 更改文件的访问和修改时间                 |        |
| write        | 写入文件                                 | 已实现 |
| writev       | 向文件写入多个缓冲区的数据               | 已实现 |



## 系统设备管理

| 调用名   | 简介                                 | 情况 |
| -------- | ------------------------------------ | ---- |
| ioctl    | 对设备进行控制操作                   |      |
| mknodat  | 指定目录下创建一个设备文件或命名管道 |      |
| ppoll    | 等待一个或多个文件描述符的I/O事件    |      |
| pselect6 | 等待一个或多个文件描述符的I/O事件    |      |
| reboot   | 重启                                 |      |
| shutdown | 关机                                 |      |
| sysinfo  | 获取系统的信息                       |      |
| syslog   | 记录系统的运行状态和事件             |      |
| uname    | 获取系统的信息                       |      |

