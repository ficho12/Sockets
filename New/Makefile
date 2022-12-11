CC = gcc
CFLAGS = 
#Descomentar la siguiente linea para nogal
LIBS = -pthread 
#Descomentar la siguiente linea para linux
#LIBS =

PROGS = servidor cliente 

all: ${PROGS}

servidor: servidor.o log.o
	${CC} ${CFLAGS} -o $@ servidor.o log.o ${LIBS}
	
cliente: cliente.o log.o
	${CC} ${CFLAGS} -o $@ cliente.o log.o ${LIBS}

log.o: log.c log.h
	$(CC) -c log.c

clean:
	rm *.o ${PROGS}
