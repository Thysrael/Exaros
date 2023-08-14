# use 64 bit arch cross compiler
CROSS_COMPILE	:= riscv64-unknown-elf-

# tools chain
GCC		:= $(CROSS_COMPILE)gcc
OBJDUMP := $(CROSS_COMPILE)objdump
OBJCOPY	:= $(CROSS_COMPILE)objcopy
LD 		:= $(CROSS_COMPILE)ld
GDB		:= $(CROSS_COMPILE)gdb
QEMU	:= qemu-system-riscv64

# gcc options
# `-MMD` 配合 `-include *.d` 可以达到最终头文件的目的
CFLAGS 	:= 	-Wall -Werror -O -MMD
CFLAGS 	+=	-fno-omit-frame-pointer -ffreestanding -fno-common -nostdlib -mno-relax
CFLAGS 	+= 	$(shell $(GCC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
CFLAGS 	+= 	-MD -ggdb -g
CFLAGS 	+= -I.
CFLAGS 	+= -D QEMU

DEBUG 	:= n
ARCH	:= sifive
ifeq ($(MAKECMDGOALS), debug)
	DEBUG = y
endif
ifeq ($(MAKECMDGOALS), virt)
	ARCH = virt
endif
# These options are about riscv arch
ifeq ($(DEBUG),y)
	CFLAGS 	+= -mabi=lp64
else
	CFLAGS  += -march=rv64g -mabi=lp64f
endif
CFLAGS 	+= -mcmodel=medany

# ld options
LDFLAGS	:= -z max-page-size=4096

# qemu options
ifeq ($(ARCH), virt)
	QFLAGS 	:= -machine virt 
	QFLAGS 	+= -drive file=sdcard.img,if=none,format=raw,id=x0
	QFLAGS 	+= -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
	QFLAGS	+= -m 128M
else 
	QFLAGS	:= -machine sifive_u
	QFLAGS 	+= -drive file=sdcard.img,if=sd,format=raw
	QFLAGS	+= -m 8G
endif 

QFLAGS	+= -smp 2
QFLAGS	+= -bios default
QFLAGS	+= -nographic
