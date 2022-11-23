 /*
 *          		S E R V I D O R
 *
 *	This is an example program that demonstrates the use of
 *	sockets TCP and UDP as an IPC mechanism.  
 *
 */
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



#define PUERTO 17278
#define ADDRNOTFOUND	0xffffffff	/* return address for unfound host */
#define BUFFERSIZE	1024	/* maximum size of packets to be received */
#define TAM_BUFFER 10
#define MAXHOST 128

extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */
 
void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in);
void errout(char *);		/* declare error out routine */

int FIN = 0;             /* Para el cierre ordenado */
void finalizar(){ FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{

    int s_TCP, s_UDP;		/* connected socket descriptor */
    int ls_TCP;				/* listen socket descriptor */
    
    int cc;				    /* contains the number of bytes read */
     
    struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
    
    struct sockaddr_in myaddr_in;	/* for local socket address */
    struct sockaddr_in clientaddr_in;	/* for peer socket address */
	int addrlen;
	
    fd_set readmask;
    int numfds,s_mayor;
    
    char buffer[BUFFERSIZE];	/* buffer for packets to be read into */
    
    struct sigaction vec;

		/* Create the listen socket. */
	ls_TCP = socket (AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
	memset ((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
   	memset ((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

    addrlen = sizeof(struct sockaddr_in);

		/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
		/* The server should listen on the wildcard address,
		 * rather than its own internet address.  This is
		 * generally good practice for servers, because on
		 * systems which are connected to more than one
		 * network at once will be able to have one server
		 * listening on all networks at once.  Even when the
		 * host is connected to only one network, this is good
		 * practice, because it makes the server program more
		 * portable.
		 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
		/* Initiate the listen on the socket so remote users
		 * can connect.  The listen backlog is set to 5, which
		 * is the largest currently supported.
		 */
	if (listen(ls_TCP, 5) == -1) {
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}
	
	
	/* Create the socket UDP. */
	s_UDP = socket (AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1) {
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	   }
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *) &myaddr_in, sizeof(struct sockaddr_in)) == -1) {
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	    }

		/* Now, all the initialization of the server is
		 * complete, and any user errors will have already
		 * been detected.  Now we can fork the daemon and
		 * return to the user.  We need to do a setpgrp
		 * so that the daemon will no longer be associated
		 * with the user's control terminal.  This is done
		 * before the fork, so that the child will not be
		 * a process group leader.  Otherwise, if the child
		 * were to open a terminal, it would become associated
		 * with that terminal as its control terminal.  It is
		 * always best for the parent to do the setpgrp.
		 */
	setpgrp();

	switch (fork()) {
	case -1:		/* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0:     /* The child process (daemon) comes here. */

			/* Close stdin and stderr so that they will not
			 * be kept open.  Stdout is assumed to have been
			 * redirected to some logging file, or /dev/null.
			 * From now on, the daemon will not report any
			 * error messages.  This daemon will loop forever,
			 * waiting for connections and forking a child
			 * server to handle each one.
			 */
		fclose(stdin);
		fclose(stderr);

			/* Set SIGCLD to SIG_IGN, in order to prevent
			 * the accumulation of zombies as each child
			 * terminates.  This means the daemon does not
			 * have to make wait calls to clean them up.
			 */
		if ( sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror(" sigaction(SIGCHLD)");
            fprintf(stderr,"%s: unable to register the SIGCHLD signal\n", argv[0]);
            exit(1);
            }
            
		    /* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
        vec.sa_handler = (void *) finalizar;
        vec.sa_flags = 0;
        if ( sigaction(SIGTERM, &vec, (struct sigaction *) 0) == -1) {
            perror(" sigaction(SIGTERM)");
            fprintf(stderr,"%s: unable to register the SIGTERM signal\n", argv[0]);
            exit(1);
            }
        
		while (!FIN) {
            /* Meter en el conjunto de sockets los sockets UDP y TCP */
            FD_ZERO(&readmask);
            FD_SET(ls_TCP, &readmask);
            FD_SET(s_UDP, &readmask);
            /* 
            Seleccionar el descriptor del socket que ha cambiado. Deja una marca en 
            el conjunto de sockets (readmask)
            */ 
    	    if (ls_TCP > s_UDP) s_mayor=ls_TCP;
    		else s_mayor=s_UDP;

            if ( (numfds = select(s_mayor+1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) {
                if (errno == EINTR) {
                    FIN=1;
		            close (ls_TCP);
		            close (s_UDP);
                    perror("\nFinalizando el servidor. Se�al recibida en elect\n "); 
                }
            }
           else { 

                /* Comprobamos si el socket seleccionado es el socket TCP */
                if (FD_ISSET(ls_TCP, &readmask)) {
                    /* Note that addrlen is passed as a pointer
                     * so that the accept call can return the
                     * size of the returned address.
                     */
    				/* This call will block until a new
    				 * connection arrives.  Then, it will
    				 * return the address of the connecting
    				 * peer, and a new socket descriptor, s,
    				 * for that connection.
    				 */
    			s_TCP = accept(ls_TCP, (struct sockaddr *) &clientaddr_in, &addrlen);
    			if (s_TCP == -1) exit(1);
    			switch (fork()) {
        			case -1:	/* Can't fork, just exit. */
        				exit(1);
        			case 0:		/* Child process comes here. */
                    			close(ls_TCP); /* Close the listen socket inherited from the daemon. */
        				serverTCP(s_TCP, clientaddr_in);
        				exit(0);
        			default:	/* Daemon process comes here. */
        					/* The daemon needs to remember
        					 * to close the new accept socket
        					 * after forking the child.  This
        					 * prevents the daemon from running
        					 * out of file descriptor space.  It
        					 * also means that when the server
        					 * closes the socket, that it will
        					 * allow the socket to be destroyed
        					 * since it will be the last close.
        					 */
        				close(s_TCP);
        			}
             } /* De TCP*/
          /* Comprobamos si el socket seleccionado es el socket UDP */
          if (FD_ISSET(s_UDP, &readmask)) {
                /* This call will block until a new
                * request arrives.  Then, it will
                * return the address of the client,
                * and a buffer containing its request.
                * BUFFERSIZE - 1 bytes are read so that
                * room is left at the end of the buffer
                * for a null character.
                */
                cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0,
                   (struct sockaddr *)&clientaddr_in, &addrlen);
                if ( cc == -1) {
                    perror(argv[0]);
                    printf("%s: recvfrom error\n", argv[0]);
                    exit (1);
                    }
                /* Make sure the message received is
                * null terminated.
                */
                buffer[cc]='\0';
                serverUDP (s_UDP, buffer, clientaddr_in);
                }
          }
		}   /* Fin del bucle infinito de atenci�n a clientes */
        /* Cerramos los sockets UDP y TCP */
        close(ls_TCP);
        close(s_UDP);
    
        printf("\nFin de programa servidor!\n");
        
	default:		/* Parent process comes here. */
		exit(0);
	}

}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;		/* keeps count of number of requests */
	char buf[TAM_BUFFER];		/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST];		/* remote host's name string */

	int len, len1, status;
    struct hostent *hp;		/* pointer to host info for remote host */
    long timevar;			/* contains time returned by time() */
    
    struct linger linger;		/* allow a lingering, graceful close; */
    				            /* used when setting SO_LINGER */

	//Variables añadidas
	char *mensaje_r;
	char *html;
	char *comando;
	int comando_b = 0;
	char *tipo,*tipo_aux;
	char *aux;
	FILE * log;
	long long final_log;
	char logString[1024];
	char logFileName[99];
	int contador = 0;
	
	int smtp_number = 501;
	char * buffer = 0;
	long long length;
	FILE * f;
	//char html2[200];
	char *respuesta = 0;

	char longitud[256];
    				
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */
	 
     status = getnameinfo((struct sockaddr *)&clientaddr_in,sizeof(clientaddr_in),
                           hostname,MAXHOST,NULL,0,0);
     if(status){
		/* The information is unavailable for the remote
		* host.  Just format its internet address to be
		* printed out in the logging information.  The
		* address will be shown in "internet dot format".
		*/
		/* inet_ntop para interoperatividad con IPv6 */
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}
    
	//Creamos la String para el nombre del archivo de log dinámicamente con el numero de puerto que se recibe
	snprintf(logFileName, sizeof(logFileName), "logs/%d.log", ntohs(clientaddr_in.sin_port));
	//Abrimos el archvio de log, sino existe se crea
	log = fopen(logFileName, "a");
	if(log == NULL)
	{
		fprintf(stdout,"Error al crear archivo log.\n");
		exit(EXIT_FAILURE);
	}
	printf("\nLog abierto\n");

	/* Log a startup message. */
    time (&timevar);
	//Guardamos la string del primer mensaje de log de Comunicación Realizada
	snprintf(logString,sizeof(logString), "Comunicación Realizada. Fecha: %s Ejecutable: clientcp Nombre del host:%s IP: %d Protocolo: TCP Puerto: %d", (char *) ctime(&timevar), hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port));

	//Escribimos la String en el archivo de log
	fputs(logString, log);
	
	//Cerramos archivo de log
    fclose(log);
	printf("\nLog Cerrado\n");
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */

	printf("Startup from %s port %u at %s",
		hostname, ntohs(clientaddr_in.sin_port), (char *) ctime(&timevar));

		/* Set the socket for a lingering, graceful close.
		 * This will cause a final close of this socket to wait until all of the
		 * data sent on it has been received by the remote host.
		 */
	linger.l_onoff  =1;
	linger.l_linger =1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
					sizeof(linger)) == -1) {
		errout(hostname);
	}

		/* Go into a loop, receiving requests from the remote
		 * client.  After the client has sent the last request,
		 * it will do a shutdown for sending, which will cause
		 * an end-of-file condition to appear on this end of the
		 * connection.  After all of the client's requests have
		 * been received, the next recv call will return zero
		 * bytes, signalling an end-of-file condition.  This is
		 * how the server will know that no more requests will
		 * follow, and the loop will be exited.
		 */

	//Se recibe de KB en KB, mirar que cantidad es la adecuada
	while (recv(s, mensaje_r, 1024, 0) == 1024) {// while (recv(s, mensaje_r, 1024, 0) <= 1024)? Devuelve el Nº de números leídos. Puede que lea <1024

		aux = (char*) malloc(1024*sizeof(char));

		printf("%s\n",mensaje_r);

		//TODO: Leer y separar cadenas del mensaje recibido para saber que mensaje devolver al cliente
		/*
		aux = strtok(mensaje_r, " ");

		while(aux != NULL)
		{
			printf("%s", aux);
			contador++;

			if(contador == 1){
				comando = aux;
			}else if(contador == 2){

				html = aux;

			}else if(contador == 5){

				tipo_aux = aux;

			}

			aux = strtok(NULL, " ");

		}

		tipo = strtok(tipo_aux, "<");

		contador = 0;

		printf("\n%s.....%s.....%s\n",comando, html,tipo);

		if(0 == strcmp(comando,"GET")){
			comando_b = 1;
			printf("Comando_b = true\n");
		}else{
			comando_b = 0;
		}
		*/

		//Abrimos el html del mensaje
		/* Probablemente esto no se utilice
		if(comando_b)
		{
			//strcat(html2,"www");
			//strcat(html2, html);
			snprintf(html2,sizeof(html2), "www%s",html);
			printf("%s\n",html2);
			f = fopen (html2, "r");

			if (f)
			{
				fseek (f, 0, SEEK_END);
				length = ftell (f);
				fseek (f, 0, SEEK_SET);
				buffer = malloc (length);
				if (buffer)
				{
					fread (buffer, 1, length, f);
				}
				fclose (f);
				html_number = 200;
				sprintf(longitud, "%lld", length);
				printf("Cerrando HTML Longitud: %s\n",longitud);
			}else
				html_number = 404;
		}
		*/
		
		respuesta = (char*) malloc ((length+1000)*sizeof(char));
		
		//html_number = 404;

		//Una vez decidido el mensaje que se va a enviar se (smtp_number) se crea la cadena	
		switch(smtp_number){
			case 220:	//Respuesta cuando el cliente realiza la conexión
					printf("220 Servicio de transferencia simple de correo preparado\n");	//Cambiar Respuesta
					snprintf(respuesta,length+1000, "HTTP/1.1 200 OK<CR><LF>Server: %s<CR><LF>Connection: %s<CR><LF>Content-Length: %s<CR><LF>%s",hostname,tipo,longitud,buffer);
				break;
			case 221:	//Respuesta a la orden QUIT
					printf("221 Cerrrando el servicio\n");	//Cambiar Respuesta
					snprintf(respuesta,length+1000, "HTTP/1.1 200 OK<CR><LF>Server: %s<CR><LF>Connection: %s<CR><LF>Content-Length: %s<CR><LF>%s",hostname,tipo,longitud,buffer);
				break;
			case 250:	//Respuesta correcta a las ordenes MAIL, RCPT, DATA
					printf("250 OK\n");	//Cambiar Respuesta
					snprintf(respuesta,length+1000, "HTTP/1.1 200 OK<CR><LF>Server: %s<CR><LF>Connection: %s<CR><LF>Content-Length: %s<CR><LF>%s",hostname,tipo,longitud,buffer);
				break;
			case 354:	//Respuesta al envío de la orden DATA
					printf("250 Comenzando con el texto del correo, finalice con .\n");	//Cambiar Respuesta
					snprintf(respuesta,length+1000, "HTTP/1.1 200 OK<CR><LF>Server: %s<CR><LF>Connection: %s<CR><LF>Content-Length: %s<CR><LF>%s",hostname,tipo,longitud,buffer);
				break;
			case 500:	//Respuesta a errores de sintaxis en cualquier orden
					printf("500 Error de sintaxis\n");	//Cambiar Respuesta
					snprintf(respuesta,length+1000, "HTTP/1.1 200 OK<CR><LF>Server: %s<CR><LF>Connection: %s<CR><LF>Content-Length: %s<CR><LF>%s",hostname,tipo,longitud,buffer);
				break;
		}


		printf("Respuesta: %s\n", respuesta);
		
		//Comienzo LOG
		log = fopen(logFileName, "a");

		if(log == NULL)
		{
			fprintf(stderr,"Error al abrir el archivo log.\n");
			exit(EXIT_FAILURE);
		}

		fputs(respuesta, log);

		/*
		if (!strcmp(tipo, "keep-alive "))
		{
			fseek (log, 0, SEEK_END);
			//final_log = ftell (log);
			snprintf(logString,sizeof(logString), "Fecha: %s Ejecutable: clientcp Nombre del host:%s IP: %d Protocolo: TCP Puerto: %d", (char *) ctime(&timevar), hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port));
			printf("close\n");
			fputs(logString, log);
		}else{
			printf("keep-alive\n");
		}
		*/

		

		//Cerramos archivo de log
    	fclose(log);
		printf("X_\n");
		free(respuesta);
		printf("X2_\n");
		free(mensaje_r);
		printf("%s",html);
		free(aux);
		printf("X3_\n");
		printf("Y_\n");
		sleep(1); // Mirar esto porque creo recordar que si lo quitabas daba error XD
		
		if (send(s, logString, length+1000, 0) != 200) { //Comprobar mensaje respuesta funcion
			fprintf(stderr, "%s: Connection aborted on error ",hostname);
			exit(1);
		}
		
		 mensaje_r = (char *) malloc(1024*sizeof(char));
  
	}

	/*--------------------------------------------------------------------------------*/
		/* The loop has terminated, because there are no
		 * more requests to be serviced.  As mentioned above,
		 * this close will block until all of the sent replies
		 * have been received by the remote host.  The reason
		 * for lingering on the close is so that the server will
		 * have a better idea of when the remote has picked up
		 * all of the data.  This will allow the start and finish
		 * times printed in the log file to reflect more accurately
		 * the length of time this connection was used.
		 */
	close(s);

		/* Log a finishing message. */
	time (&timevar);
		/* The port number must be converted first to host byte
		 * order before printing.  On most hosts, this is not
		 * necessary, but the ntohs() call is included here so
		 * that this program could easily be ported to a host
		 * that does require it.
		 */
	printf("Completed %s port %u, %d requests, at %s\n",
		hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *) ctime(&timevar));
	
}


/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);     
}


/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int s, char * buffer, struct sockaddr_in clientaddr_in)
{
    struct in_addr reqaddr;	/* for requested host's address */
    struct hostent *hp;		/* pointer to host info for requested host */
    int nc, errcode;

    struct addrinfo hints, *res;

	int addrlen;
    
   	addrlen = sizeof(struct sockaddr_in);

      memset (&hints, 0, sizeof (hints));
      hints.ai_family = AF_INET;
		/* Treat the message as a string containing a hostname. */
	    /* Esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta. */
    errcode = getaddrinfo (buffer, NULL, &hints, &res); 
    if (errcode != 0){
		/* Name was not found.  Return a
		 * special value signifying the error. */
		reqaddr.s_addr = ADDRNOTFOUND;
      }
    else {
		/* Copy address of host into the return buffer. */
		reqaddr = ((struct sockaddr_in *) res->ai_addr)->sin_addr;
	}
     freeaddrinfo(res);

	nc = sendto (s, &reqaddr, sizeof(struct in_addr),
			0, (struct sockaddr *)&clientaddr_in, addrlen);
	if ( nc == -1) {
         perror("serverUDP");
         printf("%s: sendto error\n", "serverUDP");
         return;
         }   
 }
