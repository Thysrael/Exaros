# 串口驱动

## 背景

内核从控制台读写数据默认通过 OpenSBI 的 SBI_CONSOLE_GETCHAR 和 SBI_CONSOLE_PUTCHAR，
需要陷入到 S 级，这导致许多不必要的切换。

我们通过在 M 级实现 UART 驱动，使得内核能够直接对控制台进行输出输出

## 实现

我们通过向 sifive 的 UART0 的 txdata/rxdata 寄存器读写从而实现数据传输

在写数据时，需要读寄存器保证目前 FIFO 队列未满。
同时，读写寄存器前后需要使用 `fence` 指令来保证 UART 的 FIFO 队列及时清空，以防止阻塞