INCLUDES	:=	-I./include
USER_LIB	:=	crt.o main.o syscall.o print.o string.o stdlib.o
TEST_DIR 	:=  ./testcases
TEST_SRC	:=  $(wildcard $(TEST_DIR)/*.c)
TEST_TARGET	:=	$(patsubst %.c, %, $(notdir $(TEST_SRC)))
MOUNT_DIR	:=	./target
modules		:=	$(TEST_DIR)

.PHONY: build clean

build: $(modules) $(USER_LIB) $(TEST_TARGET)

$(modules):
	$(MAKE) build --directory=$(TEST_DIR)

%.b.c: %.b
	$(BINTOC) -f $< -o $@

%: $(TEST_DIR)/%.o $(USER_LIB)
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ./include/arch/riscv/user.ld $^
	mkdir -p $(MOUNT_DIR)
	mv $@ $(MOUNT_DIR)

%.b: $(TEST_DIR)/%.o $(USER_LIB)
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ./include/arch/riscv/user.ld $^
	$(OBJDUMP) -S -D $@ > $@.txt

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