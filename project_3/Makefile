all:
	g++ -Wall -D_FILE_OFFSET_BITS=64 -std=c++11 ytfs.cpp `pkg-config fuse --cflags --libs` -o ytfs -l sqlite3

clean:
	rm -f ytfs *.o
