#include "log.h"

int escribirLogServer(char * mensaje, char * hostname, int ip, int puerto, sem_t *sem, char * protocolo, int formato){
	long timevar;
    time (&timevar);

    char logString[512];
    FILE * log;

    mkdir("logs", S_IRWXU | S_IRWXG | S_IRWXO); // Creamos la carpeta de logs
	
    //Creamos la String para el nombre del archivo de log dinámicamente con el numero de puerto que se recibe
	//snprintf(logFileName, sizeof(logFileName), "logs/peticiones.log");
	    
	sem_wait(sem);
	//Abrimos el archvio de log, sino existe se crea
	log = fopen(LOGFILENAME, "a");
	if(log == NULL)
	{
		fprintf(stdout,"Error al abrir o crear el archivo peticiones.log.\n");
		return(-1);
	}

    /* Log a startup message. */
    char * time_str = ctime(&timevar);
	time_str[strlen(time_str)-1] = '\0';

    //Guardamos la string del mensaje de log
    switch(formato){
        case 0:	snprintf(logString,sizeof(logString), "Comunicación Realizada\t Fecha: %s Nombre del host: %s IP: %d Protocolo: %s Puerto: %d\n", time_str, hostname, ip, protocolo, puerto);
            break;
        case 1:	snprintf(logString,sizeof(logString), "Mensaje Recibido\t\t Fecha: %s Nombre del host: %s IP: %d Protocolo: %s Puerto: %d Mensaje: %s", time_str, hostname, ip, protocolo, puerto, mensaje);
            break;
        case 2:	snprintf(logString,sizeof(logString), "Mensaje Enviado\t\t\t Fecha: %s Nombre del host: %s IP: %d Protocolo: %s Puerto: %d Mensaje: %s", time_str, hostname, ip, protocolo, puerto, mensaje);
            break;
        case 3:	snprintf(logString,sizeof(logString), "Comunicación Finalizada\t Fecha: %s Nombre del host: %s IP: %d Protocolo: %s Puerto: %d\n", time_str, hostname, ip, protocolo, puerto);
            break;
    }
	//Escribimos la String en el archivo de log
	fputs(logString, log);
	
	//Cerramos archivo de log
    fclose(log);
	sem_post(sem);

    return 0;
}

int escribirRespuestaLog (char * mensaje, sem_t *sem)
{
    FILE * log;
    long timevar;
    //Comienzo LOG
    sem_wait(sem);
    log = fopen(LOGFILENAME, "a");
    if(log == NULL)
    {
        fprintf(stdout,"Error al abrir el archivo log %s.\n", LOGFILENAME);
        return (-1);
    }

    fputs(mensaje, log);
    fclose(log);
    sem_post(sem);
    free(mensaje);
    return 0;
}

int escribirRespuestaLogCliente (char * mensaje, char * logName)
{
    FILE * log;
    //Abrimos el archvio de log, sino existe se crea
    log = fopen(logName, "a");
    if(log == NULL)
    {
        fprintf(stdout,"Error al crear archivo log.\n");
        return(-1);
    }

    fputs(mensaje, log);
    fclose(log);
	free(mensaje);
    return 0;
}