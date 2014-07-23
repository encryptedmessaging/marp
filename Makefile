# GNU Makefile
# Author: Ethan Gordon

CC=gcc 
CFLAGS=-pthread -m64 -std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
DEVFLAGS= -O3 -DNDEBUG
LDFLAGS=

# Object Files
%.o: %.c
	$(CC) $(CFLAGS) $(DEVFLAGS) -c $< -o $@

# Main Targets
all: marpd

dev: DEVFLAGS=-O0 -g
dev: all

marpd: marpd.o frame.o signal.o network/socket.o object/query.o object/response.o
	$(CC) $(LDFLAGS) *.o */*.o -o $@

# Phony Targets
clean:
	rm -rf marpd
	rm -rf *.o
	rm -rf */*.o

clobber: clean
	rm -rf *~
	rm -rf \#*\#
