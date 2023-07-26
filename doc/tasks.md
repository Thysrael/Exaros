# Linux 中进程 / 线程的 ID

## 内核
* pid：线程 id
* tgid：线程组 id，即进程 id
* pgid：进程组 id，等于首进程的 id
* sid：会话 id，由多个进程组组成，等于首进程组的 id

## 系统调用
* getpid：传统意义上的 pid，即 tgid
* gettid：传统意义上的 tid，即 pid
* getpgid：真正的 pgid
* getsid：真正的 sid