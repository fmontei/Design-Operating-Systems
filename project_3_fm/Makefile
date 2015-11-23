all:
	gcc -Wall -D_FILE_OFFSET_BITS=64 ytfs.c ytfs_list.c `pkg-config fuse --cflags --libs` -o ytfs -l sqlite3

clean:
	rm -f ytfs *.o
