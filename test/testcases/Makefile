INCLUDES	:=	-I../include
USER_LIB	:=	
USER_SRC	:=	$(wildcard ./*.c)
USER_TARGET	:=	$(patsubst %.c, %.o, $(USER_SRC))
MOUNT_DIR	:=	./target
$(info    VAR is $(wildcard ./*.c))
$(info    USER_TARGET is $(USER_TARGET))

.PHONY: build clean

build: $(USER_LIB) $(USER_TARGET) 

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c -o $@ $<

clean:
	rm -rf *~ *.o *.d

include ../../include.mk