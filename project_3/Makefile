ytfs : ytfs.o
	gcc -g -o ytfs ytfs.o `pkg-config fuse --libs`

ytfs.o : ytfs.c
	gcc -g -Wall `pkg-config fuse --cflags` -c ytfs.c

clean:
	rm -f ytfs *.o
