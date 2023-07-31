# 调度

Linux 中 PR 的范围为 [-100,39]，([0,139])
在调度过程中，优先调度优 PR 低的，
对于相同优先级的线程，则根据其调度策略进行调度。

PR 计算公式如下：

* 非实时进程
  * PR = 20 + nice
* 实时进程
  * PR = -1 - sched_priority

Linux 的支持的调度策略有：

* 非实时
  * SCHED_OTHER, SCHED_IDLE, SCHED_BATCH
  * sched_priority 恒为 0，不会被使用
* 实时
  * SCHED_FIFO, SCHED_RR
  * sched_priority 在 (1-99) 之间

显然 sched_priority 越高，PR 越低，优先级越高，
实时进程的优先级永远高非实时。

目前我们主要关注 SCHED_OTHER 和 SCHED_FIFO，不考虑 nice

* PR = -1 - sched_priority [-100, -1]
* SCHED_OTHER
  * 直接采取时间片轮转策略
* SCHED_FIFO
  * 被更高优先级抢占的进程将保留在优先级队列头部
  * blocked 进程转变为 runnable 时插入到优先级队列尾部
  * 正在运行或者可运行的线程改变优先级时
    * 线程优先级提高，则置于新优先级列表末尾
    * 线程优先级不变，则位置不变
    * 线程优先级降低，则置于新优先级列表前端
    * 改变优先级会导致当前进程被移动到优先级列表末尾 // 忽略
  * 调用 sched_yield ，则置于优先级队列的末尾

* theadFork / processFork ptid 作用