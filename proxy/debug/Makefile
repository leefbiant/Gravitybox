CC=gcc
CFLAGS= -g -Wall -O2 
LIB=libdebug.a
OBJ=debug.o
all:$(LIB)
	ar rc $(LIB) $(OBJ)	
$(LIB):$(OBJ)

clean:	
	rm -rf libdebug.a debug.o
