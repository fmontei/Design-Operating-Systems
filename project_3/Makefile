all:
	gcc -Wall -D_FILE_OFFSET_BITS=64 ytfs.c `pkg-config fuse --cflags --libs` -o ytfs 

clean:
	rm -f ytfs *.o
