ALL:tfc

CPPFLAGS=-fpermissive
tfc:encript.o util_hexdump.c
	g++ $(CFLAGS) $(CPPFLAGS) -o $@ $?
%.o: %.c
	g++ -c $(CFLAGS) $(CPPFLAGS) $< -o $@
