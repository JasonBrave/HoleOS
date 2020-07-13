CFLAGS += -I../libsys/include -I../libc/include -I..

$(LIB).a : $(OBJS)
	$(AR) -rcs $(LIB).a $(OBJS)

$(LIB).so : $(OBJS)
	$(LD) -Bsymbolic -L.. -shared $(OBJS) $(DEPLIBS) -o $(LIB).so

%.o : %.asm
	nasm -felf32 -gdwarf $< -o $@

.PHONY: install
install: $(LIB).a $(LIB).so
	cp $(LIB).a $(LIB).so ..
	mkdir -p ../../rootfs/devel/include/$(HEADERDIR)
	cp -r $(HEADERS) ../../rootfs/devel/include/$(HEADERDIR)

.PHONY: clean
clean:
	rm -f $(LIB).a $(LIB).so $(OBJS)
