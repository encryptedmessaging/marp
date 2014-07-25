# Simple makefile to build inih as a static library using g++

SRC = ini.c
OBJ = $(SRC:.c=.o)
INCLUDES = -I..
CCFLAGS = -pthread -m64 -std=c99 -pedantic -Wall -Wshadow -Wpointer-arith -Wstrict-prototypes -Wmissing-prototypes
CC = gcc

default: $(OBJ)

.c.o:
	$(CC) $(INCLUDES) $(CCFLAGS) $(EXTRACCFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(OUT)
