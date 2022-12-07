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
	int addrlen, i, j, errcode;
    /* This example uses TAM_BUFFER byte messages. */
	char buf[TAM_BUFFER];
	char logString[1024];
	char logFileName[99];
	FILE * log;

	//Nuevas variables
	char *contents;
	size_t cont_size = 512*sizeof(char);
	char delim[] = " ";
	char tipo[2];
	char *pagina;
	char *get_s;
	int get = 0;
    size_t len = 0;
	int cont = 1;
	char *ptr,*ptr2;
	char *mensaje;
	char keep_alive[200];
	int flag = 0;
	char *respuesta;
	long long length, length2 = 0, lengthRecibido;
	int e;
	char longitud[256];
	char fichero[256];
	int cuerpoCorreo = 0;

	respuesta = (char*) malloc ((1024)*sizeof(char));

	mkdir("logs", S_IRWXU | S_IRWXG | S_IRWXO);
	
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset ((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));	
	
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
		if(recv(s, respuesta, 1024*sizeof(char), 0) < 0){
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
		//printf("\nLog abierto\n");

		FILE* input_file = fopen(filename, "r");

		if (!input_file)
		{
			fprintf(stdout,"Error al leer el archivo de ordenes.\n");
			exit(EXIT_FAILURE);
		}

		

		fputs(respuesta, log);
		//fseek (log, 0, SEEK_END);
		free(respuesta);

		//TODO: Leer archivo de ordenes y enviar mensajes \r\l

		/*
			FILE * fp;
			fp = fopen(argv[3], "z");
			fscanf(fp, "", cad);
			while(!feof(fp)){
				send()
				fscanf(fp,...)
			}
			fclose(fp);
		*/

		contents = (char*) malloc ((1024)*sizeof(char));
		respuesta = (char*) malloc ((1024)*sizeof(char));
		//printf("Aquí\n");

		while(getline(&contents,&cont_size,input_file) != -1)
		{
			//mensaje = (char*) malloc ((1024)*sizeof(char));
			
			//snprintf(mensaje, 1024,"%s\r\n",contents);


			//printf("%s",mensaje);

			if ((strstr(contents, ".\r\n") != NULL) && (strlen(contents) == 3))
				cuerpoCorreo = 0;

			if (send(s, contents, 1024, 0) == -1) {
				fprintf(stderr, "%s: Connection aborted on error %s",get_s,strerror(errno));
				exit(1);
			}

			printf("Enviado: \"%s\"\tLength: %d\tCuerpoCorreo: %d\n", contents, (int) strlen(contents),cuerpoCorreo);

			if(!cuerpoCorreo)
			{
				if(recv(s, respuesta, 1024*sizeof(char), 0) <= 0){
					fprintf(stderr, "Connection aborted on error %s", strerror(errno));
					exit(1);
				}

				if(reg(contents,regDATA) && reg(respuesta,reg354))
					cuerpoCorreo = 1;

				printf("Respuesta: %s\n", respuesta);

				fputs(respuesta, log);
				//fseek (log, 0, SEEK_END);
				free(respuesta);
				respuesta = (char*) malloc ((1024)*sizeof(char));
			}
			
			free(contents);
			contents = (char*) malloc ((1024)*sizeof(char));
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

	}

	exit (0);
}
