INCLUDES := -I../../include

target = trap.o user_trap.o kernel_trap.o timer.o tlb_exception.o syscall.o sysarg.o interrupt.o

.PHONY: build clean

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target)

clean:
	rm -rf *~ *.o *.d

include ../../include.mk