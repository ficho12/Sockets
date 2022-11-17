CC = gcc
CFLAGS = 
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
#LIBS =

PROGS = servidortcp clientcp #clientudp

all: ${PROGS}

servidortcp: servidortcp.o
	${CC} ${CFLAGS} -o $@ servidortcp.o ${LIBS}
	
clientcp: clientcp.o
	${CC} ${CFLAGS} -o $@ clientcp.o ${LIBS}

#clientudp: clientudp.o
#	${CC} ${CFLAGS} -o $@ clientudp.o ${LIBS}

clean:
	rm *.o ${PROGS}