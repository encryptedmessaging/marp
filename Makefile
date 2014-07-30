# GNU Makefile
# Author: Ethan Gordon

CC=gcc 
CFLAGS=-pthread -m64 -std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Ioaes/inc
DEVFLAGS=-O3 -DNDEBUG
LDFLAGS=-Loaes -loaes_lib -lpthread

# Object Files
%.o: %.c
	$(CC) $(CFLAGS) $(DEVFLAGS) -c $< -o $@

# Main Targets
all: marpd

dev: DEVFLAGS=-O0 -g
dev: all

data/inih/inih.o:
	make default -C data/inih

sha256.o:
	gcc -c libsha2/sha256.c

oaes/liboaes_lib.a: CFLAGS=
oaes/liboaes_lib.a:
	cd oaes; cmake .
	make -C oaes

marpd: data/inih/ini.o marpd.o frame.o signal.o network/socket.o object/query.o object/response.o data/cache.o data/local.o network/peers.o network/recursor.o sha256.o oaes/liboaes_lib.a
	$(CC) $< *.o */*.o $(LDFLAGS) -o $@

# Phony Targets
clean:
	make clean -C data/inih
	rm -rf marpd
	rm -rf *.o
	rm -rf */*.o

clobber: clean
	make clean -C oaes
	cd oaes; rm -rf oaes_setup.vdproj cmake_install.cmake Makefile CMakeFiles/ CMakeCache.txt
	rm -rf *~
	rm -rf \#*\#
