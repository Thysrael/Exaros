INCLUDES	:=	-I./include
USER_LIB	:=	crt.o main.o syscall.o print.o string.o 
# 如果希望拷到磁盘中，则需要无后缀文件名，否则只需要 x 即可嵌入内核
USER_TARGET	:=	processA.x processB.x processC.x sh.x syscall_test.x sh test.x
BINTOC		:=	../utility/bintoc
MOUNT_DIR	:=	./target

.PHONY: build clean

build: $(BINTOC) $(USER_LIB) $(USER_TARGET) 

%.x: %.b.c
	$(GCC) $(CFLAGS) -c -o $@ $<

%.b.c: %.b
	$(BINTOC) -f $< -o $@

%: %.o $(USER_LIB)
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ./include/arch/riscv/user.ld $^
	mkdir -p $(MOUNT_DIR)
	mv $@ $(MOUNT_DIR)

%.b: %.o $(USER_LIB)
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ./include/arch/riscv/user.ld $^
	$(OBJDUMP) -S -D $@ > $@.txt

# %: %.o $(USER_LIB)
# 	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ../linkscript/user.lds $^
# 	mv $@ $(MOUNT_DIR)

# %.b: %.o $(USER_LIB)
# 	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ../linkscript/user.lds $^
# 	$(OBJDUMP) -S -D $@ > $@.txt

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *~ *.o *.d *.b.c *.b.txt *.x *.b $(MOUNT_DIR)/*
	
include ../include.mk