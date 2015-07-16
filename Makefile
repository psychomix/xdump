all:
	gcc -Wall -lssl -lcrypto -o xdump xdump.c
	strip --strip-debug xdump

clean:
	rm -f *.o xdump
