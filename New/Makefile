CC = gcc
CFLAGS = 
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =

PROGS = servidor client 

all: ${PROGS}

servidor: servidor.o log.o
	${CC} ${CFLAGS} -o $@ servidor.o log.o ${LIBS}
	
client: client.o log.o
	${CC} ${CFLAGS} -o $@ client.o log.o ${LIBS}

log.o: log.c log.h
	$(CC) -c log.c

clean:
	rm *.o ${PROGS}
