CC	= gcc -std=c99
CFLAGS	= -Wall -Wextra

.PHONY: build clean

build: send recv

send: send.o link_emulator/lib.o
	$(CC) $(CFLAGS) send.o link_emulator/lib.o -o send

recv: recv.o link_emulator/lib.o
	$(CC) $(CFLAGS) recv.o link_emulator/lib.o -o recv

send.o: send.c window.h link_emulator/lib.h
	$(CC) $(CFLAGS) -c send.c

recv.o: recv.c window.h link_emulator/lib.h
	$(CC) $(CFLAGS) -c recv.c

link_emulator/lib.o:
	$(MAKE) -C link_emulator

clean:
	$(MAKE) -C link_emulator clean; \
	rm -f *.o send recv recv_fileX
