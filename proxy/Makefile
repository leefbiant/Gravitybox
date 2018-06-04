CC=gcc
LIB=-ldebug -lpthread 
LIBPATH=-L./debug  
INCLUDE=-I./debug 
CFLAGS=-g -Wall -O2 $(INCLUDE) 
EXE=server_proxy
OBJ=init.o nio.o task.o main.o queue.o mempool.o


all:$(EXE) DEBUGLOG
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ)  $(LIB) $(LIBPATH)
$(EXE):$(OBJ)

DEBUGLOG:
	make -C debug
clean:
	rm -f $(EXE) $(QUEUE_TEST) *.o 
	make clean -C debug

