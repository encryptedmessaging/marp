# GNU Makefile
# Author: Ethan Gordon

CC=gcc 
CFLAGS=-pthread -m64 -std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes -Ioaes/inc
DEVFLAGS=-O3 -DNDEBUG
LDFLAGS=-Loaes -loaes_lib -lpthread
OBJECTS=data/inih/ini.o frame.o signal.o network/socket.o object/query.o object/response.o data/cache.o data/local.o network/peers.o network/recursor.o sha256.o oaes/liboaes_lib.a micro-ecc/uECC.o

# Basic .o Targets
%.o: %.c %.h
	$(CC) $(CFLAGS) $(DEVFLAGS) -c $< -o $@

# Main Targets
all: marpd mlookup

dev: DEVFLAGS=-O0 -g
dev: all

mlookup: client/mlookup.o $(OBJECTS)
	$(CC) client/mlookup.o $(OBJECTS) $(LDFLAGS) -o $@

marpd: marpd.o $(OBJECTS)
	$(CC) marpd.o $(OBJECTS) $(LDFLAGS) -o $@

# Specific Object Files
client/mlookup.o: client/mlookup.c
	$(CC) $(CFLAGS) $(DEVFLAGS) -c $< -o $@

marpd.o: marpd.c frame.h signal.h network/socket.h network/peers.h data/cache.h data/local.h
	$(CC) $(CFLAGS) $(DEVFLAGS) -c $< -o $@

frame.o: frame.c frame.h network/socket.h
	$(CC) $(CFLAGS) $(DEVFLAGS) -c $< -o $@

data/inih/inih.o:
	make default -C data/inih

sha256.o:
	gcc -c libsha2/sha256.c

oaes/liboaes_lib.a: CFLAGS=
oaes/liboaes_lib.a:
	cd oaes; cmake .
	make -C oaes

# Debugging Targets
splint:
	splint *.c */*.c +posixlib -I/usr/include/x86_64-linux-gnu -Ioaes/inc

# Phony Targets
clean:
	make clean -C data/inih
	rm -rf marpd marpd.o mlookup client/mlookup.o
	rm -rf $(OBJECTS)

clobber: clean
	rm -rf *~
	rm -rf */*~
	rm -rf \#*\#
	make clean -C oaes
	cd oaes; rm -rf oaes_setup.vdproj cmake_install.cmake CMakeFiles/ CMakeCache.txt Makefile
