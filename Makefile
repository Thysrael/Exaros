include include.mk

kernel_dir	:= 	kernel
link_dir	:= 	linkscript
target_dir	:= 	target

linkscript	:= 	$(link_dir)/kernel.lds
exaros_elf	:=	$(target_dir)/exaros.elf
exaros_bin	:=	os.bin
exaros_sys	:= 	$(target_dir)/exaros_sys.txt

modules := 	$(kernel_dir)
objects := $(kernel_dir)/*/*.o 

.PHONY: all clean $(modules) run

DEBUG 	:= n
ifeq ($(MAKECMDGOALS), debug)
	DEBUG = y
endif

all: $(modules)
	mkdir -p $(target_dir) 
	$(LD) -o $(exaros_elf) -T $(linkscript) $(LDFLAGS) $(objects)
	$(OBJDUMP) -alD $(exaros_elf) > $(exaros_sys)
	$(OBJCOPY) -O binary $(exaros_elf) $(exaros_bin)


$(modules):
	$(MAKE) DEBUG=$(DEBUG) --directory=$@

clean:
	for module in $(modules);						\
		do 											\
			$(MAKE) --directory=$$module clean;		\
		done;										\
	rm -rf *.o *~ $(target_dir)/*
	rm $(exaros_bin)


run: clean all
	$(QEMU) -kernel $(exaros_bin) $(QFLAGS)

asm: clean all
	$(QEMU) -kernel $(exaros_bin) $(QFLAGS) -d in_asm

int: clean all
	$(QEMU) -kernel $(exaros_bin) $(QFLAGS) -d int

debug: clean all
	$(QEMU) -kernel $(exaros_bin) $(QFLAGS) -s -S

gdb: 
	$(GDB) $(exaros_elf)


