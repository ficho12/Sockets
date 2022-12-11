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

void cierreOrdenado(char *mensaje, int s,char* logFileName, char *aborrar1, char *aborrar2)
{
	close(s);
	if (aborrar1 != NULL)
	{
		remove(aborrar1);
	}
	if (aborrar2 != NULL)
	{
		remove(aborrar2);
	}
	escribirRespuestaLogCliente(mensaje, logFileName);
	exit(1);
}

int main(argc, argv)
int argc;
char *argv[];
{
	char filename[50];
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
	char errorString[512];
	char logFileName[99];
	FILE * log;
	char *contents;
	size_t cont_size = 516;
	char *respuesta;
	int cuerpoCorreo = 0;

	if (argc != 4) {
		fprintf(stderr, "Usage:  %s <remote host> <mode (TCP or UDP)> <orders.txt>\n", argv[0]);
		exit(1);
	}

	if(argv[3] != NULL)
		strcpy(filename, argv[3]);
	else{
		fprintf(stderr, "Usage:  %s <remote host> <mode (TCP or UDP)> <orders.txt>\n", argv[0]);
		exit(1);
	}

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
		snprintf(errorString, 512, "%s: No es posible resolver la IP de %s\n",
				argv[0], argv[1]);
		cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
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
			snprintf(errorString, 512, "%s: No se ha podido creo el socket TCP\n",argv[0]);
			cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
		}

		if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
			snprintf(errorString, 512, "%s: No se ha podido conectar al remoto\n",argv[0]);
			cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
		}

	}else if(!strcmp(argv[2],"UDP")){
		myaddr_in.sin_port = htons(0);
		myaddr_in.sin_family = AF_INET;
		myaddr_in.sin_addr.s_addr = INADDR_ANY;

		signal(SIGALRM, manejadora);

		s = socket (AF_INET, SOCK_DGRAM, 0);
		if (s == -1) {
			snprintf(errorString, 512, "%s: No se ha podido crear el socket UDP\n",argv[0]);
			cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
		}

		if(bind(s, (const struct sockaddr *)&myaddr_in, addrlen) == -1){
			snprintf(errorString, 512, "%s: No se ha podido hacer bind a la direccion UDP\n",argv[0]);
			cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
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
		snprintf(errorString, 512, "%s: No se ha podido leer la direccion del socket\n",argv[0]);
		cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
	}

	if(!strcmp(argv[2],"TCP")){
		if(recv(s, respuesta, 516, 0) < 0){
			snprintf(errorString, 512, "%s: Conexion abortada en recv por el error %s\n",argv[0], strerror(errno));
			cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
		}

		//printf("Respuesta: %s\n", respuesta);

		snprintf(logFileName, sizeof(logFileName), "logs/%d.txt", ntohs(myaddr_in.sin_port));

		//Abrimos el archvio de log, sino existe se crea
		log = fopen(logFileName, "a");
		if(log == NULL)
		{
			snprintf(errorString, 512, "%s: Error al crear archivo log.\n",argv[0]);
			cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
		}

		FILE* input_file = fopen(filename, "r");

		if (!input_file)
		{
			snprintf(errorString, 512, "%s: Error al leer el archivo de ordenes %s.\n",argv[0],filename);
			cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
		}		

		fputs(respuesta, log);
		free(respuesta);

		contents = (char*) malloc (516);
		respuesta = (char*) malloc (516);

		while(fgets(contents,516,input_file) != NULL)
		{
			if ((strstr(contents, ".\r\n") != NULL) && (strlen(contents) == 3))
				cuerpoCorreo = 0;

			comprobarCRLF(contents);

			if (send(s, contents, 516, 0) == -1) {
				snprintf(errorString, 512, "%s: Conexion abortada en send por el error %s\n",argv[0], strerror(errno));
				cierreOrdenado(errorString, s, logFileName, respuesta, contents);
			}

			//printf("Enviado: \"%s\"\tLength: %d\tCuerpoCorreo: %d\n", contents, (int) strlen(contents),cuerpoCorreo);

			if(!cuerpoCorreo)
			{
				if(recv(s, respuesta, 516, 0) <= 0){
					snprintf(errorString, 512, "%s: Conexion abortada en recv por el error %s\n",argv[0], strerror(errno));
					cierreOrdenado(errorString, s, logFileName, respuesta, contents);
				}

				if(reg(contents,regDATA) && reg(respuesta,reg354))
					cuerpoCorreo = 1;

				//printf("Respuesta: %s\n", respuesta);

				fputs(respuesta, log);
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
			snprintf(errorString, 512, "%s: Conexion abortada en sendto por el error %s\n",argv[0], strerror(errno));
			cierreOrdenado(errorString, s, logFileName, respuesta, contents);
		}

		recvfrom(s,respuesta, 516, 0,(struct sockaddr *)&servaddr_in, &addrlen); //Recibe " " para actualizar los datos del socket nuevo para este cliente

		//printf("Respuesta: %s\n", respuesta);

		recvfrom(s,respuesta, 516, 0,(struct sockaddr *)&servaddr_in, &addrlen); //Recibe 220, primer mensaje real.

		//printf("Respuesta: %s\n", respuesta);

		snprintf(logFileName, sizeof(logFileName), "logs/%d.txt", ntohs(myaddr_in.sin_port));

		//Abrimos el archvio de log, sino existe se crea
		log = fopen(logFileName, "a");
		if(log == NULL)
		{
			snprintf(errorString, 512, "%s: Error al crear archivo log.\n",argv[0]);
			cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
		}

		FILE* input_file = fopen(filename, "r");

		if (!input_file)
		{
			snprintf(errorString, 512, "%s: Error al leer el archivo de ordenes %s.\n",argv[0],filename);
			cierreOrdenado(errorString, s, logFileName, respuesta, NULL);
		}		

		fputs(respuesta, log);
		free(respuesta);

		contents = (char*) malloc (516);
		respuesta = (char*) malloc (516);
		int n_intentos;
		while(fgets(contents,516,input_file) != NULL)
		{
			n_intentos = 0;
			while(n_intentos < MAX_INTENTOS){					
				if ((strstr(contents, ".\r\n") != NULL) && (strlen(contents) == 3))
					cuerpoCorreo = 0;

				comprobarCRLF(contents);

				if(sendto(s,contents,cont_size,0, (struct sockaddr *)&servaddr_in, addrlen)== -1) {
					snprintf(errorString, 512, "%s: Conexion abortada en sendto por el error %s\n",argv[0], strerror(errno));
					cierreOrdenado(errorString, s, logFileName, respuesta, contents);
				}

				//printf("Enviado: \"%s\"\tLength: %d\tCuerpoCorreo: %d\n", contents, (int) strlen(contents),cuerpoCorreo);

				if(!cuerpoCorreo)
				{
					alarm(TIMEOUT);
					if(recvfrom(s,respuesta, 516, 0,(struct sockaddr *)&servaddr_in, &addrlen) == -1){
						if (errno == EINTR){
							n_intentos++;
							//fprintf(stderr, "Se encontro una señal mientras se esperaba un mensaje. Aumentando número de intentos a %d", n_intentos);
							if(n_intentos == 5)
							{
								snprintf(errorString, 512, "%s: Número máximo de intentos de recepción de mensaje en el servidor UDP.\nCerrando ordenadamente el servidor\n", argv[0]);
								cierreOrdenado(errorString, s, logFileName, respuesta, contents);
							}
						}
						else{
							snprintf(errorString, 512, "%s: No se ha podido recibir una respuesta en el servidor UDP\n", argv[0]);
							cierreOrdenado(errorString, s, logFileName, respuesta, contents);
						}
					}else{
						alarm(0);
						if(reg(contents,regDATA) && reg(respuesta,reg354))
							cuerpoCorreo = 1;

						//printf("Respuesta: %s\n", respuesta);

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
