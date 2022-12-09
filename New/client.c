/*
 *			C L I E N T C P
 *
 *	This is an example program that demonstrates the use of
 *	stream sockets as an IPC mechanism.  This contains the client,
 *	and is intended to operate in conjunction with the server
 *	program.  Together, these two programs
 *	demonstrate many of the features of sockets, as well as good
 *	conventions for using these features.
 *
 *
 */
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include "regex.c"
#include "log.h"

#define PUERTO 2873
#define TAM_BUFFER 10
#define MAX_INTENTOS 5
#define TIMEOUT 5


/*
 *			M A I N
 *
 *	This routine is the client which request service from the remote.
 *	It creates a connection, sends a number of
 *	requests, shuts down the connection in one direction to signal the
 *	server about the end of data, and then receives all of the responses.
 *	Status will be written to stdout.
 *
 *	The name of the system to which the requests will be sent is given
 *	as a parameter to the command.
 */

void manejadora(){}

// Devuelve 1 si el mensaje ya tiene CRLF, 0 si no lo tiene y lo añade
int comprobarCRLF(char * mensaje){
	if (strstr(mensaje, "\r\n"))
		return 1;
	else{
		strcat(mensaje, "\r\n");
		return 0;
	}
}

int main(argc, argv)
int argc;
char *argv[];
{
	if (argc != 4) {
		fprintf(stderr, "Usage:  %s <remote host> <mode (TCP or UDP)> <orders.txt>\n", argv[0]);
		exit(1);
	}

	const char* filename = argv[3];
	// TODO: Comprobacion de errores
    int s;				/* connected socket descriptor */
   	struct addrinfo hints, *res;
    long timevar;			/* contains time returned by time() */
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in servaddr_in;	/* for server socket address */
	struct sockaddr_in udpaddr_in; //para el nuevo socket de UDP con puerto efímero y único para esta conexión
	int addrlen, errcode;
    /* This example uses TAM_BUFFER byte messages. */
	//Nuevas variables
	char logString[1024];
	char logFileName[99];
	FILE * log;
	char *contents;
	size_t cont_size = 516;
	char *respuesta;
	int cuerpoCorreo = 0;

	respuesta = (char*) malloc (516);
	
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&udpaddr_in, 0, sizeof(struct sockaddr_in));		
	
	addrlen = sizeof(struct sockaddr_in);

	/* Set up the peer address to which we will connect. */
	servaddr_in.sin_family = AF_INET;
	
	/* Get the host information for the hostname that the
	 * user passed in. */
      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
 	 /* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
    errcode = getaddrinfo (argv[1], NULL, &hints, &res); 
    if (errcode != 0){
			/* Name was not found.  Return a
			 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				argv[0], argv[1]);
		exit(1);
        }
    else {
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	    }
    freeaddrinfo(res);

    /* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);

		/* Try to connect to the remote server at the address
		 * which was just built into peeraddr.
		 */
	
	if(!strcmp(argv[2],"TCP")){
		s = socket (AF_INET, SOCK_STREAM, 0);
		if (s == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
			exit(1);
		}

		if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
			exit(1);
		}

	}else if(!strcmp(argv[2],"UDP")){
		myaddr_in.sin_port = htons(0);
		myaddr_in.sin_family = AF_INET;
		myaddr_in.sin_addr.s_addr = INADDR_ANY;

		signal(SIGALRM, manejadora);

		s = socket (AF_INET, SOCK_DGRAM, 0);
		if (s == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to create socket UDP\n", argv[0]);
			exit(1);
		}

		if(bind(s, (const struct sockaddr *)&myaddr_in, addrlen) == -1){
			perror(argv[0]);
			printf("%s: unable to bind address UDP\n", argv[0]);
			exit(1);
		}
	}else{
		fprintf(stderr, "Usage:  %s <remote host> <mode (TCP or UDP)> <orders.txt>\n", argv[0]);
		exit(1);
	}

		/* Since the connect call assigns a free address
		 * to the local end of this connection, let's use
		 * getsockname to see what it assigned.  Note that
		 * addrlen needs to be passed in as a pointer,
		 * because getsockname returns the actual length
		 * of the address.
		 */
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	}

	if(!strcmp(argv[2],"TCP")){
		if(recv(s, respuesta, 516, 0) < 0){
			fprintf(stderr, "Connection aborted on error %s", strerror(errno));
			exit(1);
		}

		printf("Respuesta: %s\n", respuesta);

		/* Print out a startup message for the user. */
		time(&timevar);
		/* The port number must be converted first to host byte
		* order before printing.  On most hosts, this is not
		* necessary, but the ntohs() call is included here so
		* that this program could easily be ported to a host
		* that does require it.
		*/
		printf("Connected to %s on port %u at %s",
				argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

		snprintf(logFileName, sizeof(logFileName), "logs/%d.txt", ntohs(myaddr_in.sin_port));

		//Abrimos el archvio de log, sino existe se crea
		log = fopen(logFileName, "a");
		if(log == NULL)
		{
			fprintf(stdout,"Error al crear archivo log.\n");
			exit(EXIT_FAILURE);
		}

		FILE* input_file = fopen(filename, "r");

		if (!input_file)
		{
			fprintf(stdout,"Error al leer el archivo de ordenes.\n");
			exit(EXIT_FAILURE);
		}		

		fputs(respuesta, log);
		free(respuesta);

		contents = (char*) malloc (516);
		respuesta = (char*) malloc (516);

		while(getline(&contents,&cont_size,input_file) != -1)
		{
			if ((strstr(contents, ".\r\n") != NULL) && (strlen(contents) == 3))
				cuerpoCorreo = 0;

			comprobarCRLF(contents);

			if (send(s, contents, 516, 0) == -1) {
				fprintf(stderr, "Connection aborted on error %s.",strerror(errno));
				exit(1);
			}

			printf("Enviado: \"%s\"\tLength: %d\tCuerpoCorreo: %d\n", contents, (int) strlen(contents),cuerpoCorreo);

			if(!cuerpoCorreo)
			{
				if(recv(s, respuesta, 516, 0) <= 0){
					fprintf(stderr, "Connection aborted on error %s", strerror(errno));
					exit(1);
				}

				if(reg(contents,regDATA) && reg(respuesta,reg354))
					cuerpoCorreo = 1;

				printf("Respuesta: %s\n", respuesta);

				fputs(respuesta, log);
				//fseek (log, 0, SEEK_END);
				free(respuesta);
				respuesta = (char*) malloc (516);
			}
			
			free(contents);
			contents = (char*) malloc (516);
		}

		fclose(input_file);
		free(contents);
		free(respuesta);
		/* Now, shutdown the connection for further sends.
		* This will cause the server to receive an end-of-file
		* condition after it has received all the requests that
		* have just been sent, indicating that we will not be
		* sending any further requests.
		*/
		if (shutdown(s, 1) == -1) {
			perror(argv[0]);
			fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
			exit(1);
		}

		/* Print message indicating completion of task. */
		time(&timevar);
		printf("All done at %s", (char *)ctime(&timevar));
	}
	else	//UDP
	{
		if(sendto(s," ",1,0, (struct sockaddr *)&servaddr_in, addrlen)== -1) {
			perror("clientUDP");
			printf("%s: sendto error 1\n", "clientUDP");
			exit(1);
		}

		recvfrom(s,respuesta, 516, 0,(struct sockaddr *)&servaddr_in, &addrlen); //Recibe " " para actualizar los datos del socket nuevo para este cliente

		printf("Respuesta: %s\n", respuesta);

		recvfrom(s,respuesta, 516, 0,(struct sockaddr *)&servaddr_in, &addrlen); //Recibe 220, primer mensaje real.

		printf("Respuesta: %s\n", respuesta);

		/* Print out a startup message for the user. */
		time(&timevar);
		/* The port number must be converted first to host byte
		* order before printing.  On most hosts, this is not
		* necessary, but the ntohs() call is included here so
		* that this program could easily be ported to a host
		* that does require it.
		*/
		printf("Connected to %s on port %u at %s",
				argv[1], ntohs(myaddr_in.sin_port), (char *) ctime(&timevar));

		snprintf(logFileName, sizeof(logFileName), "logs/%d.txt", ntohs(myaddr_in.sin_port));

		//Abrimos el archvio de log, sino existe se crea
		log = fopen(logFileName, "a");
		if(log == NULL)
		{
			fprintf(stdout,"Error al crear archivo log.\n");
			exit(EXIT_FAILURE);
		}

		FILE* input_file = fopen(filename, "r");

		if (!input_file)
		{
			fprintf(stdout,"Error al leer el archivo de ordenes.\n");
			exit(EXIT_FAILURE);
		}		

		fputs(respuesta, log);
		free(respuesta);

		contents = (char*) malloc (516);
		respuesta = (char*) malloc (516);
		int n_intentos;
		while(getline(&contents,&cont_size,input_file) != -1)
		{
			n_intentos = 0;
			while(n_intentos < MAX_INTENTOS){					
				if ((strstr(contents, ".\r\n") != NULL) && (strlen(contents) == 3))
					cuerpoCorreo = 0;

				comprobarCRLF(contents);

				if(sendto(s,contents,cont_size,0, (struct sockaddr *)&servaddr_in, addrlen)== -1) {
					perror("serverUDP");
					printf("%s: sendto error 2\n", "serverUDP");
					exit(1);
				}

				printf("Enviado: \"%s\"\tLength: %d\tCuerpoCorreo: %d\n", contents, (int) strlen(contents),cuerpoCorreo);

				if(!cuerpoCorreo)
				{
					alarm(TIMEOUT);
					if(recvfrom(s,respuesta, 516, 0,(struct sockaddr *)&servaddr_in, &addrlen) == -1){
						if (errno == EINTR){
							n_intentos++;
							fprintf(stderr, "Se encontro una señal mientras se esperaba un mensaje. Aumentando número de intentos a %d", n_intentos);
							if(n_intentos == 5)
							{
								fprintf(stderr, "Número máximo de intentos de recepción de mensaje en el servidor UDP para %d.\nCerrando ordenadamente el servidor\n", ntohs(servaddr_in.sin_port));
								exit(1);		// TODO: Hacer cierre ordenado
							}
						}
						else{
							fprintf(stderr, "No se ha podido recibir una respuesta en el servidor UDP\n");
							exit(1);
						}
					}else{
						alarm(0);
						if(reg(contents,regDATA) && reg(respuesta,reg354))
							cuerpoCorreo = 1;

						printf("Respuesta: %s\n", respuesta);

						fputs(respuesta, log);
						free(respuesta);
						respuesta = (char*) malloc (516);
						break;
					}
				}else{
					break;
				}
			}
			free(contents);
			contents = (char*) malloc (516);
		}

		fclose(input_file);
		free(contents);
		free(respuesta);

		/* Print message indicating completion of task. */
		time(&timevar);
		printf("All done at %s", (char *)ctime(&timevar));

	}

	exit (0);
}
