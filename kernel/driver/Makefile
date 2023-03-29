INCLUDES := -I../../include

target = print.o console.o

.PHONY: build clean

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target)

clean:
	rm -rf *~ *.o *.d

include ../../include.mk