INCLUDES := -I../../include

target = $(shell ls *.c | awk '{gsub(/\.c/, ".o"); print $0}')

.PHONY: build clean
-include *.d
%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target)

clean:
	rm -rf *~ *.o *.d

include ../../include.mk