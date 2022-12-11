/*
 *          		S E R V I D O R
 *
 *	This is an example program that demonstrates the use of
 *	sockets TCP and UDP as an IPC mechanism.
 *
 */

/*
 *
 *	Autores:
 *	Fiz Rey Armesto 		34292873B
 *
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
#include <sys/stat.h>
#include <semaphore.h>
#include "regex.c"
#include "log.h"

#define PUERTO 2873
#define ADDRNOTFOUND 0xffffffff /* return address for unfound host */
#define BUFFERSIZE 1024			/* maximum size of packets to be received */
#define TAM_BUFFER 10
#define MAXHOST 128
#define MAX_INTENTOS 5
#define TIMEOUT 5
#define resp220 "220 Servicio de transferencia simple de correo preparado\r\n"
#define resp221 "221 Cerrando el servicio\r\n"
#define resp250 "250 OK\r\n"
#define resp354 "354 Comenzando con el texto del correo, finalice con .\r\n"
#define resp500 "500 Error de sintaxis\r\n"

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
void serverUDP(int s, struct sockaddr_in clientaddr_in);
void errout(char *); /* declare error out routine */
void cierreOrdenado(char *mensaje, sem_t *sem, int s, char *aborrar1, char *aborrar2);

void cierreOrdenado(char *mensaje, sem_t *sem, int s, char *aborrar1, char *aborrar2)
{

	fprintf(stderr, "%s\n. Cerrando ordenadamente el servidor.", mensaje);
	close(s);
	if (aborrar1 != NULL)
	{
		remove(aborrar1);
	}
	if (aborrar2 != NULL)
	{
		remove(aborrar2);
	}
	sem_destroy(sem);
	exit(0);
}

int FIN = 0; /* Para el cierre ordenado */
void finalizar() { FIN = 1; }
sem_t sem; // Semaforo

int main(argc, argv)
int argc;
char *argv[];
{

	int s_TCP, s_UDP;							   /* connected socket descriptor */
	int ls_TCP;									   /* listen socket descriptor */
	int cc;										   /* contains the number of bytes read */
	struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */
	struct sockaddr_in myaddr_in;				   /* for local socket address */
	struct sockaddr_in clientaddr_in;			   /* for peer socket address */
	int addrlen;
	fd_set readmask;
	int numfds, s_mayor;
	char buffer[BUFFERSIZE]; /* buffer for packets to be read into */
	struct sigaction vec;

	/* Create the listen socket. */
	ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}

	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

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
	if (bind(ls_TCP, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		close(ls_TCP);
		exit(1);
	}
	/* Initiate the listen on the socket so remote users
	 * can connect.  The listen backlog is set to 5, which
	 * is the largest currently supported.
	 */
	if (listen(ls_TCP, 5) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		close(ls_TCP);
		exit(1);
	}

	/* Create the socket UDP. */
	s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket UDP\n", argv[0]);
		close(ls_TCP);
		exit(1);
	}

	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address UDP\n", argv[0]);
		close(ls_TCP);
		close(s_UDP);
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

	switch (fork())
	{
	case -1: /* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		close(ls_TCP);
		close(s_UDP);
		exit(1);

	case 0: /* The child process (daemon) comes here. */

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
		if (sigaction(SIGCHLD, &sa, NULL) == -1)
		{
			perror(" sigaction(SIGCHLD)");
			fprintf(stderr, "%s: unable to register the SIGCHLD signal\n", argv[0]);
			close(ls_TCP);
			close(s_UDP);
			exit(1);
		}

		/* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
		vec.sa_handler = (void *)finalizar;
		vec.sa_flags = 0;
		if (sigaction(SIGTERM, &vec, (struct sigaction *)0) == -1)
		{
			perror(" sigaction(SIGTERM)");
			fprintf(stderr, "%s: unable to register the SIGTERM signal\n", argv[0]);
			close(ls_TCP);
			close(s_UDP);
			exit(1);
		}

		// Semaforo
		sem_init(&sem, 0, 1);

		while (!FIN)
		{
			/* Meter en el conjunto de sockets los sockets UDP y TCP */
			FD_ZERO(&readmask);
			FD_SET(ls_TCP, &readmask);
			FD_SET(s_UDP, &readmask);
			/*
			Seleccionar el descriptor del socket que ha cambiado. Deja una marca en
			el conjunto de sockets (readmask)
			*/
			if (ls_TCP > s_UDP)
				s_mayor = ls_TCP;
			else
				s_mayor = s_UDP;

			if ((numfds = select(s_mayor + 1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0)
			{
				if (errno == EINTR)
				{
					FIN = 1;
					close(ls_TCP);
					close(s_UDP);
					perror("\nFinalizando el servidor. Se�al recibida en elect\n ");
				}
			}
			else
			{

				/* Comprobamos si el socket seleccionado es el socket TCP */
				if (FD_ISSET(ls_TCP, &readmask))
				{
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
					s_TCP = accept(ls_TCP, (struct sockaddr *)&clientaddr_in, &addrlen);
					if (s_TCP == -1)
						exit(1);
					switch (fork())
					{
					case -1: /* Can't fork, just exit. */
						exit(1);
					case 0:			   /* Child process comes here. */
						close(ls_TCP); /* Close the listen socket inherited from the daemon. */
						serverTCP(s_TCP, clientaddr_in);
						exit(0);
					default: /* Daemon process comes here. */
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
				if (FD_ISSET(s_UDP, &readmask))
				{
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
					if (cc == -1)
					{
						perror(argv[0]);
						printf("%s: recvfrom error\n", argv[0]);
						exit(1);
					}

					switch (fork())
					{
					case -1: // Can't fork, just exit.
						exit(1);
					case 0:			   // Child process comes here.
									   // Cerrar sockets del padre antes de hacer el nuevo socket
						close(ls_TCP); // Close the listen socket inherited from the daemon.
						close(s_UDP);

						int nuevoSocketUDP = socket(AF_INET, SOCK_DGRAM, 0);
						if (nuevoSocketUDP == -1)
						{
							perror(argv[0]);
							fprintf(stderr, "%s: unable to create socket UDP\n", argv[0]);
							exit(1);
						}

						// La direccion local tiene que ser un puerto efimero --> 0

						// Struct Nuevo socket UDP
						struct sockaddr_in udpaddr_in;

						memset((char *)&udpaddr_in, 0, sizeof(struct sockaddr_in));
						udpaddr_in.sin_family = AF_INET;
						udpaddr_in.sin_addr.s_addr = INADDR_ANY;
						udpaddr_in.sin_port = htons(0); // 0 --> Puerto efímero

						// bind
						if (bind(nuevoSocketUDP, (struct sockaddr *)&udpaddr_in, sizeof(struct sockaddr_in)) == -1)
						{
							perror(argv[0]);
							printf("%s: unable to bind address UDP\n", argv[0]);
							close(nuevoSocketUDP);
							exit(1);
						}

						// sendto
						if (sendto(nuevoSocketUDP, " ", 1, 0, (struct sockaddr *)&clientaddr_in, addrlen) == -1)
						{
							perror("serverUDP");
							printf("%s: sendto error\n", "serverUDP");
							exit(1);
						}

						serverUDP(nuevoSocketUDP, clientaddr_in);
						exit(0);
					}
				}
			}
		} /* Fin del bucle infinito de atenci�n a clientes */
		/* Cerramos los sockets UDP y TCP */
		close(ls_TCP);
		close(s_UDP);
		sem_destroy(&sem);

		printf("\nFin de programa servidor!\n");

	default: /* Parent process comes here. */
		exit(0);
	}
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
void serverUDP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;			/* keeps count of number of requests */
	char hostname[MAXHOST]; /* remote host's name string */

	int len, len1, status;
	long timevar; /* contains time returned by time() */

	struct linger linger; /* allow a lingering, graceful close; */
						  /* used when setting SO_LINGER */

	// Variables
	char *mensaje_r;
	FILE *log;
	char errorString[512];
	int nivel = 1;
	int case4 = 0;
	int addrlen;
	int smtp_number = 500;
	char *respuesta = 0;
	int n_intentos = 0;

	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */

	status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in),
						 hostname, MAXHOST, NULL, 0, 0);
	if (status)
	{
		/* The information is unavailable for the remote
		 * host.  Just format its internet address to be
		 * printed out in the logging information.  The
		 * address will be shown in "internet dot format".
		 */
		/* inet_ntop para interoperatividad con IPv6 */
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}

	addrlen = sizeof(struct sockaddr_in);

	escribirLogServer("", hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port), &sem, "UDP", 0);

	/* Set the socket for a lingering, graceful close.
	 * This will cause a final close of this socket to wait until all of the
	 * data sent on it has been received by the remote host.
	 */
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1)
	{
		errout(hostname);
	}

	mensaje_r = (char *)malloc(516);
	respuesta = (char *)malloc(516);

	// Respuesta cuando el cliente realiza la conexión
	// printf("Respuesta: 220 Servicio de transferencia simple de correo preparado\n");
	snprintf(respuesta, 516, "%s", resp220);

	if (sendto(s, respuesta, 516, 0, (struct sockaddr *)&clientaddr_in, addrlen) == -1)
	{
		perror("serverUDP");
		printf("%s: sendto error\n", "serverUDP");
		exit(1);
	}

	escribirLogServer(respuesta, hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port), &sem, "UDP", 2);

	for (;;)
	{
		n_intentos = 0;
		while (n_intentos < MAX_INTENTOS)
		{
			alarm(TIMEOUT);
			if (recvfrom(s, mensaje_r, 516, 0, (struct sockaddr *)&clientaddr_in, &addrlen) == -1)
			{
				if (errno == EINTR)
				{
					fprintf(stderr, "Se encontro una señal mientras se esperaba un mensaje. Aumentando número de intentos a %d", ++n_intentos);
					if (n_intentos == 5)
					{
						snprintf(errorString, 512, "Número máximo de intentos de recepción de mensaje en el servidor UDP para %d.\nCerrando ordenadamente el servidor\n", ntohs(clientaddr_in.sin_port));
						cierreOrdenado(errorString, &sem, s, mensaje_r, respuesta);
					}
				}
				else
				{
					snprintf(errorString, 512, "No se ha podido recibir una respuesta en el servidor UDP para %d.\nCerrando ordenadamente el servidor\n", ntohs(clientaddr_in.sin_port));
					cierreOrdenado(errorString, &sem, s, mensaje_r, respuesta);
				}
			}
			else
			{
				alarm(0);
				break;
			}
		}

		escribirLogServer(mensaje_r, hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port), &sem, "UDP", 1);

		// printf("Recibido: \"%s\"\tLength: %d\tNivel: %d\tSMTP_NUM%d\n", mensaje_r, (int) strlen(mensaje_r), nivel,smtp_number);

		// bucle
		/*	Refenciando el diagrama de las diapositivas
			Nivel	Client												Server
			0		Conexion											send(220)
			1		HELO												send(250)
			2		MAIL FROM											send(250)
			3		RCPT TO:	De 3 puede pasar a 3 otra vez o a 4		send(250)
			4		DATA												send(354)
			5		leer datos hasta punto								no enviar nada
			6 		.\r\n		De 6 puede volver a 2					send(250)
			7		QUIT												send(221)
		*/
		switch (nivel)
		{
		case 1: // REGEX HELO <dominio-emisor>
			if (reg(mensaje_r, regHELO))
			{
				smtp_number = 250;
				nivel++;
			}
			else
				smtp_number = 500;
			break;
		case 2: // REGEX MAIL FROM <reverse-path>
			if (reg(mensaje_r, regMAIL))
			{
				smtp_number = 250;
				nivel++;
			}
			else
				smtp_number = 500;
			break;
		case 3: // REGEX RCPT <fordward-path>
			if (reg(mensaje_r, regRCPT))
				smtp_number = 250;
			else if (case4)
			{
				// REGEX DATA
				if (reg(mensaje_r, regDATA))
				{
					smtp_number = 354;
					nivel += 2;
				}
				else
					smtp_number = 500;
			}
			else
				smtp_number = 500;
			case4 = 1; // bool case4 = true;
			break;
		case 5: // REGEX .\r\n
			if ((strstr(mensaje_r, ".\r\n") != NULL) && (strlen(mensaje_r) == 3))
			{
				smtp_number = 500;
			}
			else
			{
				smtp_number = 0; // 0
				nivel++;
			}
			break;
		case 6: // REGEX .\r\n
			if ((strstr(mensaje_r, ".\r\n") != NULL) && (strlen(mensaje_r) == 3))
			{
				nivel++;
				smtp_number = 250;
			}
			else
				smtp_number = 0; // 0
			break;
		case 7: // REGEX QUIT
			if (reg(mensaje_r, regQUIT))
				smtp_number = 221;
			else if (reg(mensaje_r, regMAIL))
			{
				smtp_number = 250;
				nivel = 3;
			}
			else
				smtp_number = 500;
			break;
		}

		if (smtp_number != 0)
		{
			// Aquí ya se debe tener la respuesta que queremos enviar al cliente en smtp_number
			// Reservamos memoria para los mensajes
			respuesta = (char *)malloc(516);
			//	TODO: Tenemos la ip del cliente en accept para TCP y en recvfrom para UDP, el nombre se obtiene con una llamada a addrinfo (?)
			//	TODO: No hay que usar dos cadenas ya que si que se envia la cadena del mensaje, por lo tante es el mismo mensaje para el log y la cadena a enviar.
			// Una vez decidido el mensaje que se va a enviar se (smtp_number) se crea la cadena
			switch (smtp_number)
			{
			case 221: // Respuesta a la orden QUIT
				printf("Respuesta: 221 Cerrando el servicio\n");
				snprintf(respuesta, 516, "%s", resp221);
				break;
			case 250: // Respuesta correcta a las ordenes MAIL, RCPT, DATA
				printf("Respuesta: 250 OK\n");
				snprintf(respuesta, 516, "%s", resp250);
				break;
			case 354: // Respuesta al envío de la orden DATA
				printf("Respuesta: 354 Comenzando con el texto del correo, finalice con .\n");
				snprintf(respuesta, 516, "%s", resp354);
				break;
			case 500: // Respuesta a errores de sintaxis en cualquier orden
				printf("Respuesta: 500 Error de sintaxis\n");
				snprintf(respuesta, 516, "%s", resp500);
				break;
			}

			if (sendto(s, respuesta, 516, 0, (struct sockaddr *)&clientaddr_in, addrlen) == -1)
			{
				cierreOrdenado("serverUDP: sendto error\n", &sem, s, mensaje_r, respuesta);
			}

			escribirLogServer(respuesta, hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port), &sem, "UDP", 2);
		}
		else
		{
			printf("DATA. No se envía respuesta\n"); // No se envía respuesta
		}

		free(mensaje_r);
		mensaje_r = (char *)malloc(1024 * sizeof(char));
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

	escribirLogServer("", hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port), &sem, "UDP", 3);
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
	int reqcnt = 0;			/* keeps count of number of requests */
	char hostname[MAXHOST]; /* remote host's name string */

	int status;
	struct hostent *hp; /* pointer to host info for remote host */
	long timevar;		/* contains time returned by time() */

	struct linger linger; /* allow a lingering, graceful close; */
						  /* used when setting SO_LINGER */

	// Variables
	char *mensaje_r;
	FILE *log;
	char errorString[512];
	int nivel = 1;
	int case4 = 0;
	int smtp_number = 500;
	char *respuesta = 0;
	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */

	status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in),
						 hostname, MAXHOST, NULL, 0, 0);
	if (status)
	{
		/* The information is unavailable for the remote
		 * host.  Just format its internet address to be
		 * printed out in the logging information.  The
		 * address will be shown in "internet dot format".
		 */
		/* inet_ntop para interoperatividad con IPv6 */
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}

	escribirLogServer("", hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port), &sem, "TCP", 0);

	/* Set the socket for a lingering, graceful close.
	 * This will cause a final close of this socket to wait until all of the
	 * data sent on it has been received by the remote host.
	 */
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1)
	{
		errout(hostname);
	}

	mensaje_r = (char *)malloc(516);
	respuesta = (char *)malloc(516);

	// Respuesta cuando el cliente realiza la conexión
	// printf("Respuesta: 220 Servicio de transferencia simple de correo preparado\n");
	snprintf(respuesta, 516, "%s", resp220);

	if (send(s, respuesta, 516, 0) < 0)
	{
		snprintf(errorString, 512, "ServerTCP: Conexion abortada en error en send para %d\n", ntohs(clientaddr_in.sin_port));
		cierreOrdenado(errorString, &sem, s, mensaje_r, respuesta);
	}

	escribirLogServer(respuesta, hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port), &sem, "TCP", 2);

	while (recv(s, mensaje_r, 516, 0) == 516)
	{

		escribirLogServer(mensaje_r, hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port), &sem, "TCP", 1);

		printf("Recibido: \"%s\"\tLength: %d\tNivel: %d\n", mensaje_r, (int)strlen(mensaje_r), nivel);

		// bucle
		/*	Refenciando el diagrama de las diapositivas
			Nivel	Client												Server
			0		Conexion											send(220)
			1		HELO												send(250)
			2		MAIL FROM											send(250)
			3		RCPT TO:	De 3 puede pasar a 3 otra vez o a 4		send(250)
			4		DATA												send(354)
			5		leer datos hasta punto								no enviar nada
			6 		.\r\n		De 6 puede volver a 2					send(250)
			7		QUIT												send(221)
		*/
		switch (nivel)
		{
		case 1: // REGEX HELO <dominio-emisor>
			if (reg(mensaje_r, regHELO))
			{
				smtp_number = 250;
				nivel++;
			}
			else
				smtp_number = 500;
			break;
		case 2: // REGEX MAIL FROM <reverse-path>
			if (reg(mensaje_r, regMAIL))
			{
				smtp_number = 250;
				nivel++;
			}
			else
				smtp_number = 500;
			break;
		case 3: // REGEX RCPT <fordward-path>
			if (reg(mensaje_r, regRCPT))
				smtp_number = 250;
			else if (case4)
			{
				// REGEX DATA
				if (reg(mensaje_r, regDATA))
				{
					smtp_number = 354;
					nivel += 2;
				}
				else
					smtp_number = 500;
			}
			else
				smtp_number = 500;
			case4 = 1; // bool case4 = true;
			break;
		case 5: // REGEX .\r\n
			if ((strstr(mensaje_r, ".\r\n") != NULL) && (strlen(mensaje_r) == 3))
			{
				smtp_number = 500;
			}
			else
			{
				smtp_number = 0; // 0
				nivel++;
			}
			break;
		case 6: // REGEX .\r\n
			if ((strstr(mensaje_r, ".\r\n") != NULL) && (strlen(mensaje_r) == 3))
			{
				nivel++;
				smtp_number = 250;
			}
			else
				smtp_number = 0; // 0
			break;
		case 7: // REGEX QUIT
			if (reg(mensaje_r, regQUIT))
				smtp_number = 221;
			else if (reg(mensaje_r, regMAIL))
			{
				smtp_number = 250;
				nivel = 3;
			}
			else
				smtp_number = 500;
			break;
		}

		if (smtp_number != 0)
		{
			// Aquí ya se debe tener la respuesta que queremos enviar al cliente en smtp_number
			// Reservamos memoria para los mensajes
			respuesta = (char *)malloc(516);
			// Una vez decidido el mensaje que se va a enviar se (smtp_number) se crea la cadena
			switch (smtp_number)
			{
			case 221:											 // Respuesta a la orden QUIT
				printf("Respuesta: 221 Cerrando el servicio\n"); // Cambiar Respuesta
				snprintf(respuesta, 516, "%s", resp221);
				break;
			case 250:						   // Respuesta correcta a las ordenes MAIL, RCPT, DATA
				printf("Respuesta: 250 OK\n"); // Cambiar Respuesta
				snprintf(respuesta, 516, "%s", resp250);
				break;
			case 354:																		   // Respuesta al envío de la orden DATA
				printf("Respuesta: 354 Comenzando con el texto del correo, finalice con .\n"); // Cambiar Respuesta
				snprintf(respuesta, 516, "%s", resp354);
				break;
			case 500:										  // Respuesta a errores de sintaxis en cualquier orden
				printf("Respuesta: 500 Error de sintaxis\n"); // Cambiar Respuesta
				snprintf(respuesta, 516, "%s", resp500);
				break;
			}

			if (send(s, respuesta, 516, 0) <= 0)
			{
				cierreOrdenado("serverTCP: sendto error\n", &sem, s, mensaje_r, respuesta);
			}

			escribirLogServer(respuesta, hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port), &sem, "TCP", 2);
		}
		else
		{
			printf("DATA. No se envía respuesta\n"); // No se envía respuesta
		}

		free(mensaje_r);
		mensaje_r = (char *)malloc(516);
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

	escribirLogServer("", hostname, clientaddr_in.sin_addr.s_addr, ntohs(clientaddr_in.sin_port), &sem, "TCP", 3);

	close(s);
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);
}
