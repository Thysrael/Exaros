INCLUDES := -I../../include

target = console.o print.o sd.o virtio.o macb.o phy_mscc.o

.PHONY: build clean

%.o: %.S
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

%.o: %.c
	$(GCC) $(CFLAGS) $(INCLUDES) -c $<

build: $(target)

clean:
	rm -rf *~ *.o *.d

include ../../include.mk