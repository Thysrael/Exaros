include ../include.mk

modules	:= $(shell ls -d */)

.PHONY:	build clean $(modules)

build: $(modules)

$(modules):
	$(MAKE) build --directory=$@

clean:
	for d in $(modules); 					\
		do 									\
			$(MAKE) --directory=$$d clean; 	\
		done
	rm -rf *.o *~