# Signal

## 信号处理流程

一个信号被产生时，它进入待处理状态 *pending*。
如果这种类型的信号被阻塞 *blocked*，则直到该类型的信号被解除阻塞，该信号才会结束待处理状态，被进程所接收并进行处理。
进程处理信号时，若之前使用 `sigaction` 等为该信号注册了处理函数，则交由其处理函数处理，
否则由信号的默认处理函数处理。

注意，如果对某种信号的指定操作是忽略，那么即使当时信号被阻塞，也将直接丢弃（忽略的优先级更高）

## 信号处理函数

信号处理函数是由用户定义的，
用以规定程序收到特定信号后的行为的函数。

## 信号注册函数

创建新进程时，它会从其父进程继承信号处理函数
使用 `exec` 时，信号处理函数将会重置

以下函数为信号注册函数

```c
int sigaction (int signum, const struct sigaction *restrict action, struct sigaction *restrict old-action);
```

## 信号发送函数

以下函数为信号发送函数

```c
int kill(int pid, int sig);
int tgkill(int tgid, int tid, int sig);
int tkill(int tid, int sig);
```

## 处理函数运行时另一个信号到达

* 当调用特定信号的处理程序时，该信号会自动阻塞，直到处理程序返回。
* 当处理程序返回时，阻塞信号集将恢复到处理程序运行之前的值。
* 当异常返回前发现多个信号处理函数时，内核会在进程栈中依次堆叠多个信号栈帧，后到的信号被优先处理。

## 具体实现

由于信号有线程和进程两种粒度，
我们在每个进程和每个都设计了一个信号栈，
用以表示当前待处理的信号。

在线程陷入内核的时候，检查自己和对应进程的信号栈，并进行处理。
由于处理时处理程序会破坏当前程序状态，
我们使用信号上下文 `SignalContext` 来保存信号处理程序结束后应当恢复的现场，
在进入信号处理时，将其放入 `handlingSignal` 信号上下文栈，在退出时利用其恢复现场，并从栈中取出。
栈顶的信号上下文标识着当前正在处理的信号。
