all:
	g++ -Wall -D_FILE_OFFSET_BITS=64 ytfs.cpp ytfs_list.cpp `pkg-config fuse --cflags --libs` -o ytfs -l sqlite3

clean:
	rm -f ytfs *.o
