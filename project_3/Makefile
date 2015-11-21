ytfs : ytfs.o
	gcc -Wall ytfs.c `pkg-config fuse --cflags --libs` -o ytfs
	
clean:
	rm -f ytfs *.o

