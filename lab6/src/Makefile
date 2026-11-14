CC = gcc
CFLAGS = -pthread
 
all: client server
 
mult_modulo.o: mult_modulo.c mult_modulo.h
	$(CC) $(CFLAGS) -c mult_modulo.c -o mult_modulo.o
 
libmultmodulo.a: mult_modulo.o
	ar rcs libmultmodulo.a mult_modulo.o
 
client: client.c libmultmodulo.a mult_modulo.h
	$(CC) $(CFLAGS) client.c -L. -lmultmodulo -o client
 
server: server.c libmultmodulo.a mult_modulo.h
	$(CC) $(CFLAGS) server.c -L. -lmultmodulo -o server
 
clean:
	rm -f client server mult_modulo.o libmultmodulo.a
 
.PHONY: all clean
