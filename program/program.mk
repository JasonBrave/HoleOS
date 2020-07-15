CFLAGS += -I../../library/libsys/include -I../../library/libc/include \
	-I../../library

$(APP): $(OBJS)
	$(LD) -L../../library -rpath-link=../../library -I/lib/ld.so -e _start \
	-Ttext-segment=0 -o $(APP) $(OBJS) ../../library/crt/crt0.o $(LIB) -lc -lsys

%.o : %.asm
	nasm -felf32 $< -o $@

.PHONY: install
install: $(APP)
	cp $(APP) ../../rootfs/bin/

.PHONY: clean
clean:
	rm -f $(APP) $(OBJS)
