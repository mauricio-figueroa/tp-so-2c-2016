/*
 * pokeServerConection.h
 *
 *  Created on: 8/9/2016
 *      Author: utnso
 */

#ifndef HEADERS_POKECLIENTECONECTIONS_H_
#define HEADERS_POKECLIENTECONECTIONS_H_

#include <stddef.h>
#include <stdlib.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stddef.h>
#include <stdbool.h>
#include <protocolos/p_pokeDex.h>

#include <arpa/inet.h>

#define PORT "3490" // the port client will be connecting to

#define MAXDATASIZE 100 // max number of bytes we can get at once


void *get_in_addr(struct sockaddr *sa);
void* funcionHiloConexion(void* argumentos);
int leerArchivo(size_t size, off_t offset, char * path, char * buf);
bool getAttr(const char *path,struct stat *stbuf);


typedef struct argumentos_t{
	char* PTO_MONTAJE;
	char* IP;
	char* PUERTO;

}argumentos_t;

#endif /* HEADERS_POKECLIENTECONECTIONS_H_ */
