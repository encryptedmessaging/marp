# GNU Makefile
# Author: Ethan Gordon

CC=gcc 
CFLAGS=-pthread -m64 -std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wcast-qual -Wstrict-prototypes -Wmissing-prototypes
# DEVFLAGS=
DEVFLAGS=-O0 -g
PRODFLAGS=
# PRODFLAGS=-O3 -NDEBUG 
LDFLAGS=

# Object Files
%.o: %.c
	$(CC) $(CFLAGS) $(DEVFLAGS) $(PRODFLAGS) -c $< -o $@

# Main Targets
all: marpd

marpd: marpd.o frame.o network/socket.o
	$(CC) $(LDFLAGS) *.o */*.o -o $@

# Phony Targets
clean:
	rm -rf marpd
	rm -rf *.o
	rm -rf */*.o

clobber: clean
	rm -rf *~
	rm -rf \#*\#
