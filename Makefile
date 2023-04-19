include include.mk

kernel_dir	:= 	kernel
link_dir	:= 	linkscript
target_dir	:= 	target

linkscript	:= 	$(link_dir)/kernel.lds
exaros_elf	:=	$(target_dir)/exaros.elf
exaros_bin	:=	kernel-qemu
exaros_sys	:= 	$(target_dir)/exaros_sys.txt

modules := 	$(kernel_dir)
objects := $(kernel_dir)/*/*.o 

.PHONY: build clean $(modules) run

DEBUG 	:= n
ifeq ($(MAKECMDGOALS), debug)
	DEBUG = y
endif

all: $(modules)
	mkdir -p $(target_dir)
	$(LD) -o $(exaros_elf) -T $(linkscript) $(LDFLAGS) $(objects)
	$(OBJDUMP) -alDS $(exaros_elf) > $(exaros_sys)
	$(OBJCOPY) -O binary $(exaros_elf) $(exaros_bin)


$(modules):
	$(MAKE) DEBUG=$(DEBUG) --directory=$@

clean:
	for module in $(modules);						\
		do 											\
			$(MAKE) --directory=$$module clean;		\
		done;										\
	rm -rf *.o *~ $(target_dir)
	rm $(exaros_bin)

run: 
	$(QEMU) -kernel $(exaros_bin) $(QFLAGS)

asm: 
	$(QEMU) -kernel $(exaros_bin) $(QFLAGS) -d in_asm

int:  
	$(QEMU) -kernel $(exaros_bin) $(QFLAGS) -d int

debug: all
	$(QEMU) -kernel $(exaros_bin) $(QFLAGS) -s -S

gdb: 
	$(GDB) $(exaros_elf)


