[+ autogen5 template +]

## Created by Anjuta

CC = gcc
CFLAGS = -g -Wall
OBJECTS = [+Name+].o
INCFLAGS = 
LDFLAGS = -Wl,-rpath,/usr/local/lib
LIBS = 

all: [+Name+]

[+Name+]: $(OBJECTS)
	$(CC) -o [+Name+] $(OBJECTS) $(LDFLAGS) $(LIBS)

.SUFFIXES:
.SUFFIXES:	.c .cc .C .cpp .o

.c.o :
	$(CC) -o $@ -c $(CFLAGS) $< $(INCFLAGS)

count:
	wc *.c *.cc *.C *.cpp *.h *.hpp

clean:
	rm -f *.o

.PHONY: all
.PHONY: count
.PHONY: clean
