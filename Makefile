# Makefile
CC = gcc
CFLAGS = -Wall -O2 -Werror -ggdb -fPIC

all: runner performance gprof_performance
support.o: support.c support.h
csbrk.o: csbrk.c csbrk.h
err_handler.o: err_handler.c err_handler.h 
csbrk_tracked.o: csbrk.c csbrk.h
	$(CC) $(CFLAGS) -DTRACK_CSBRK -o csbrk_tracked.o -c csbrk.c
umalloc.o: umalloc.c umalloc.h
check_heap.o: umalloc.c umalloc.h


libmemory.so: csbrk.o umalloc.o
	gcc -shared -o libmemory.so csbrk.o umalloc.o

libmemory_tracked.so: csbrk_tracked.o umalloc.o check_heap.o
	gcc -shared -o libmemory_tracked.so csbrk_tracked.o umalloc.o check_heap.o

runner: runner.c libmemory_tracked.so err_handler.o support.o
	$(CC) $(CFLAGS) -Wl,-rpath='.' -o runner runner.c -L. umalloc.h -lmemory_tracked err_handler.o support.o

performance: performance.c libmemory.so support.o
	$(CC) $(CFLAGS) -Wl,-rpath='.' -o performance performance.c -L. umalloc.h -lmemory err_handler.o support.o


# GPROF
gprof_csbrk.o: csbrk.c csbrk.h
	$(CC) -O0 -c -g -pg -fPIC -o gprof_csbrk.o csbrk.c 

gprof_umalloc.o: umalloc.c umalloc.h
	$(CC) -O0 -c -g -pg -fPIC -o gprof_umalloc.o umalloc.c	

libmemory_gprof.so: gprof_csbrk.o gprof_umalloc.o
	gcc -shared -o libmemory_gprof.so gprof_csbrk.o gprof_umalloc.o

gprof_performance: performance.c libmemory_gprof.so support.o
	$(CC) -Wl,-rpath='.' -O0 -g -pg -o gprof_performance performance.c -L. umalloc.h -lmemory_gprof err_handler.o support.o

clean:
	rm -f *.o *.so runner gprof_performance performance