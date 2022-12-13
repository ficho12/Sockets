/*
 *	Autor:
 *	Fiz Rey Armesto 		34292873B
 */

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define LOGFILENAME "logs/peticiones.log"


/* Formato: 0 --> Comunicación Realizada, 1 --> Mensaje Recibido, 2 --> Mensaje Enviado, 3 --> Comunicación Finalizada 
 * Devuelve 0 si va todo bien o -1 en caso de fallo al abrir o crear el archivo de log
*/
int escribirLogServer(char * mensaje ,char * hostname, int ip, int puerto, sem_t *sem, char * protocolo, int formato);
int escribirRespuestaLog (char * mensaje, sem_t *sem);
int escribirRespuestaLogCliente (char * mensaje, char * logName);
