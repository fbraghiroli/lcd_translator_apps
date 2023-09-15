
CC=gcc
CFLAGS=-I.
DEPS = 
OBJ = main.o proto_mtxorb.o proto.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

linux: $(OBJ)
	$(CC) -o lcdlator $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o *~
