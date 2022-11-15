#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PUERTO 65657

void serverTCP(int s, struct sockaddr_in clientaddr_in);


int main(argc, argv)
int argc;
char *argv[];
{
    int fd_S, fd_LS;
	int addrlen;

    struct sockaddr_in myaddr_in;
    struct sockaddr_in clientaddr_in;

    fd_S = socket (AF_INET, SOCK_STREAM, 0);
    if (fd_socket == -1) {
        perror(argv[0]);
        fprintf(stderr, "%s: no se pudo crear el socket TCP\n", argv[0]);
        exit(1);
    }

	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

    addrlen = sizeof(struct sockaddr_in);

	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	if (bind(fd_S, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: no se ha podido emperejar la direccion TCP\n", argv[0]);
		exit(1);
	}

	if (listen(fd_S, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: no se ha podido escuchar al socket\n", argv[0]);
		exit(1);
	}

    fd_LS = accept(fd_S, (struct sockaddr *) &clientaddr_in, &addrlen);
    if (s_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: no se ha podido escuchar al socket\n", argv[0]);
		exit(1);
	}

}