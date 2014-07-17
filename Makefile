# GNU Makefile
# Author: Ethan Gordon

all:

clean:
	rm -rf marp
	rm -rf *.o

clobber: clean
	rm -rf *~
	rm -rf \#*\#
