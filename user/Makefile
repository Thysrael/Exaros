INCLUDES	:=	-I./lib/

USER_LIB	:= print.o lib_main.o 

USER_TARGET	:= processA.o processA.b processA.b.c processB.b.c

BINTOC := ../utility/bintoc

.PHONY: build clean

build: $(BINTOC) $(USER_LIB) $(USER_TARGET) 

%.x: %.b.c
	$(GCC) $(CFLAGS) -c -o $@ $<

%.b.c: %.b
	$(BINTOC) -f $< -o $@

%: Entry.o %.o $(USER_LIB)
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ../linkscript/user.lds $^
	mv $@ $(MOUNT_DIR)

%.b: Entry.o %.o $(USER_LIB)
	$(LD) -o $@ $(LDFLAGS) -G 0 -static -n -nostdlib -T ../linkscript/user.lds $^
	$(OBJDUMP) -D $@ > $@.txt

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	for d in $(modules); \
		do \
			$(MAKE) --directory=$$d clean; \
		done
	rm -rf *~ *.o *.d *.b.c *.b.txt *.x *.b
	
include ../include.mk