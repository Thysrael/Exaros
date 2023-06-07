# Pre Competion

## 仓库说明

[https://github.com/oscomp/testsuits-for-oskernel/tree/pre-2023](https://github.com/oscomp/testsuits-for-oskernel/tree/main) 

这个仓库主要由两个文件夹组成， `riscv-syscalls-testing` 存放样例和自动化脚本，`riscv-linux-rootfs` 存储用于对拍的 Linux OS。

## riscv-syscalls-testing/

这个里面主要是 `user/` 文件夹较为重要，里面有许多待编译的文件，通过编译这些用户文件然后将其存入硬盘，即可进行测试。

测试思路大致是这样的，首先规定了一组系统调用，涉及文件系统，进程管理，内存管理等多个方面，位于 `oscomp_syscalls.md` 中，然后我们首先用 `user/lib` 中的文件将这些系统调用封装成了多个比较基础的静态库函数，在 `user/include/` 中列出，然后再利用这些函数，在 `user/src/` 中构造出样例，并利用 `python` 进行正确性检验。

---

## riscv-linux-rootfs

在修正了诸多问题后，依然跑不起来。



