ytfs : ytfs.o
	gcc -g `pkg-config fuse --libs` -o ytfs ytfs.o

ytfs.o : ytfs.c
	gcc -g -Wall `pkg-config fuse --cflags` -c ytfs.c

clean:
	rm -f ytfs *.o
