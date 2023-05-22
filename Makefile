include include.mk

kernel_dir	:= 	kernel
user_dir	:= 	user
link_dir	:= 	linkscript
target_dir	:= 	target

linkscript	:= 	$(link_dir)/kernel.lds
exaros_elf	:=	$(target_dir)/exaros.elf
exaros_bin	:=	kernel-qemu
exaros_sys	:= 	$(target_dir)/exaros_sys.txt

modules := 	$(kernel_dir) $(user_dir)
objects := $(kernel_dir)/*/*.o $(user_dir)/*.x

mnt_path 	:=	/mnt
fs_img	:=	sdcard.img

.PHONY: all clean $(modules) $(user_dir) run fat

DEBUG 	:= n
ifeq ($(MAKECMDGOALS), debug)
	DEBUG = y
endif

all: fat $(modules)
	mkdir -p $(target_dir)
	$(LD) -o $(exaros_elf) -T $(linkscript) $(LDFLAGS) $(objects)
	$(OBJDUMP) -alDS $(exaros_elf) > $(exaros_sys)
	$(OBJCOPY) -O binary $(exaros_elf) $(exaros_bin)


$(modules):
	$(MAKE) DEBUG=$(DEBUG) --directory=$@

# 制作 FAT 格式的文件系统镜像
fat: $(user_dir)
	if [ ! -f "$(fs_img)" ]; then \
		echo "making fs image..."; \
		dd if=/dev/zero of=$(fs_img) bs=512k count=1024; fi
	mkfs.vfat -F 32 $(fs_img); 
	@sudo mount $(fs_img) $(mnt_path)
	@sudo cp -r user/target/* $(mnt_path)/
	@sudo umount $(mnt_path)

umount:
	sudo umount $(mnt_path)

mount:
	sudo mount -t vfat $(fs_img) $(mnt_path)

clean:
	for module in $(modules);						\
		do 											\
			$(MAKE) --directory=$$module clean;		\
		done;										\
	rm -rf *.o *~ $(target_dir)
	rm $(fs_img)
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


